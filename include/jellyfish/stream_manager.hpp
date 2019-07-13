/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <memory>
#include <vector>
#include <list>
#include <set>

#include <jellyfish/locks_pthread.hpp>
#include <jellyfish/err.hpp>
#include <jellyfish/parser_common.hpp>

namespace jellyfish {
template<typename PathIterator>
class stream_manager {
  /// A wrapper around an ifstream for a standard file. Standard in
  /// opposition to a pipe_stream below, but the file may be a regular
  /// file or a pipe. The file is opened once and notifies the manager
  /// that it is closed upon destruction.
  class file_stream : public std::ifstream {
    stream_manager& manager_;
  public:
    file_stream(const char* path, stream_manager& manager) :
      std::ifstream(path),
      manager_(manager)
    {
      manager_.take_file();
    }
    virtual ~file_stream() { manager_.release_file(); }
  };
  friend class file_stream;

  /// A wrapper around an ifstream for a "multi pipe". The multi pipe
  /// are connected to generators (external commands generating
  /// sequence). They are opened repeatedly, until they are unlinked
  /// from the file system.
  class pipe_stream : public std::ifstream {
    const char*     path_;
    stream_manager& manager_;
  public:
    pipe_stream(const char* path, stream_manager& manager) :
      std::ifstream(path),
      path_(path),
      manager_(manager)
    { }
    virtual ~pipe_stream() { manager_.release_pipe(path_); }
  };
  friend class pipe_stream;

#ifdef HAVE_HTSLIB
  /// A wrapper around a SAM file wrapper.
  class sam_stream : public sam_wrapper {
    const char*     path_;
    stream_manager& manager_;
  public:
    sam_stream(const char* path, stream_manager& manager)
      : sam_wrapper(path)
      , path_(path)
      , manager_(manager)
    {
      manager_.take_file();
    }
    virtual ~sam_stream() { manager_.release_file(); }
  };
  friend class sam_stream;
#endif

  PathIterator           paths_cur_, paths_end_;
#ifdef HAVE_HTSLIB
  PathIterator           sams_cur_, sams_end_;
#endif
  int                    files_open_;
  const int              concurrent_files_;
  std::list<const char*> free_pipes_;
  std::set<const char*>  busy_pipes_;
  locks::pthread::mutex_recursive  mutex_;

public:
  define_error_class(Error);

  explicit stream_manager(int concurrent_files = 1)
    : paths_cur_(PathIterator()), paths_end_(PathIterator())
#ifdef HAVE_HTSLIB
    , sams_cur_(PathIterator()), sams_end_(PathIterator())
#endif
    , files_open_(0)
    , concurrent_files_(concurrent_files)
  { }
  stream_manager(PathIterator paths_begin, PathIterator paths_end, int concurrent_files = 1)
    : paths_cur_(paths_begin), paths_end_(paths_end)
#ifdef HAVE_HTSLIB
    , sams_cur_(PathIterator()), sams_end_(PathIterator())
#endif
    , files_open_(0)
    , concurrent_files_(concurrent_files)
  { }

  stream_manager(PathIterator paths_begin, PathIterator paths_end,
                 PathIterator pipe_begin, PathIterator pipe_end,
                 int concurrent_files = 1)
    : paths_cur_(paths_begin), paths_end_(paths_end)
#ifdef HAVE_HTSLIB
    , sams_cur_(PathIterator()), sams_end_(PathIterator())
#endif
    , files_open_(0)
    , concurrent_files_(concurrent_files)
    , free_pipes_(pipe_begin, pipe_end)
  { }

  void paths(PathIterator paths_begin, PathIterator paths_end) {
    paths_cur_ = paths_begin;
    paths_end_ = paths_end;
  }

  void pipes(PathIterator pipe_begin, PathIterator pipe_end) {
    free_pipes_.assign(pipe_begin, pipe_end);
  }

#ifdef HAVE_HTSLIB
  void sams(PathIterator sam_begin, PathIterator sam_end) {
    sams_cur_ = sam_begin;
    sams_end_ = sam_end;
  }
#endif

  stream_type next() {
    locks::pthread::mutex_lock lock(mutex_);
    stream_type res;
    open_next_file(res);
    if(res.standard) return res;
    open_next_pipe(res);
    if(res.standard) return res;
#ifdef HAVE_HTSLIB
    open_next_sam(res);
#endif
    return res;
  }

  int concurrent_files() const { return concurrent_files_; }
  // Number of pipes available. Not thread safe
  int concurrent_pipes() const { return free_pipes_.size() + busy_pipes_.size(); }
  // Number of streams available. Not thread safe
  int nb_streams() const { return concurrent_files() + concurrent_pipes(); }

protected:
  void open_next_file(stream_type& res) {
    if(files_open_ >= concurrent_files_)
      return;
    while(paths_cur_ != paths_end_) {
      std::string path = *paths_cur_;
      ++paths_cur_;
      res.standard.reset(new file_stream(path.c_str(), *this));
      if(res.standard->good())
        return;
      res.standard.reset();
      throw std::runtime_error(err::msg() << "Can't open file '" << path << "'");
    }
  }

#ifdef HAVE_HTSLIB
  void open_next_sam(stream_type& res) {
    if(files_open_ >= concurrent_files_)
      return;
    while(sams_cur_ != sams_end_) {
      std::string path = *sams_cur_;
      ++sams_cur_;
      res.sam.reset(new sam_stream(path.c_str(), *this));
      if(res.sam->good())
        return;
      res.sam.reset();
      throw std::runtime_error(err::msg() << "Can't open SAM file '" << path << '\'');
    }
  }
#endif

  void open_next_pipe(stream_type& res) {
    while(!free_pipes_.empty()) {
      const char* path = free_pipes_.front();
      free_pipes_.pop_front();
      res.standard.reset(new pipe_stream(path, *this));
      if(res.standard->good()) {
        busy_pipes_.insert(path);
        return;
      }
      // The pipe failed to open, so it is not marked as busy. This
      // reset will make us forget about this path.
      res.standard.reset();
    }
  }

  void take_file() {
    locks::pthread::mutex_lock lock(mutex_);
    ++files_open_;
  }

  void release_file() {
    locks::pthread::mutex_lock lock(mutex_);
    --files_open_;
  }

  // void take_pipe(const char* path) {
  //   locks::pthread::mutex_lock lock(mutex_);
  // }
  void release_pipe(const char* path) {
    locks::pthread::mutex_lock lock(mutex_);
    if(busy_pipes_.erase(path) == 0)
      return; // Nothing erased. We forget about that path
    free_pipes_.push_back(path);
  }
};
} // namespace jellyfish
