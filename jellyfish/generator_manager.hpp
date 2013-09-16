/*  This file is part of Jellyfish.

    Jellyfish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jellyfish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jellyfish.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef __JELLYFISH_SPAWN_EXTERNAL_HPP_
#define __JELLYFISH_SPAWN_EXTERNAL_HPP_

#include <iostream>
#include <vector>
#include <map>
#include <stdexcept>

#include <ext/stdio_filebuf.h>

#include <jellyfish/err.hpp>

namespace jellyfish {
// Input stream (inherit from std::istream, behaves mostly like an
// ifstream), with flag O_CLOEXEC (close-on-exec) turned on.
class cloexec_istream : public std::istream
{
  static std::streambuf* open_file(const char* path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    return new __gnu_cxx::stdio_filebuf<std::istream::char_type>(fd, std::ios::in);
  }

public:
  cloexec_istream(const cloexec_istream&) = delete;
  cloexec_istream(const char* path) :
  std::istream((open_file(path)))
  { }
  cloexec_istream(const std::string& path) :
    std::istream(open_file(path.c_str()))
  { }
  virtual ~cloexec_istream() { close(); }
  void close() { delete std::istream::rdbuf(0); }
};

// // Transform an iterator on vector<string> to an iterator of
// // vector<const char*>, where the values returned are the c_str of the
// // elements.
// class char_ptr_iterator : public std::iterator<std::input_iterator_tag, const char*> {
//   std::vector<std::string>::const_iterator it_;
// public:
//   char_ptr_iterator(const std::vector<std::string>::const_iterator& it) : it_(it) { }
//   char_ptr_iterator(const char_ptr_iterator& rhs) : it_(rhs.it_) { }
//   char_ptr_iterator& operator=(const char_ptr_iterator& rhs) {
//     it_ = rhs.it_;
//     return *this;
//   }
//   const char* operator*() { return it_->c_str(); }
//   char_ptr_iterator& operator++() { ++it_; return *this; }
//   char_ptr_iterator operator++(int) { char_ptr_iterator tmp(*this); operator++(); return tmp; }
//   bool operator==(const char_ptr_iterator& rhs) const { return it_ == rhs.it_; }
//   bool operator!=(const char_ptr_iterator& rhs) const { return it_ != rhs.it_; }
//   void swap(char_ptr_iterator& rhs) { std::swap(it_, rhs.it_); }
// };
// inline void swap(char_ptr_iterator a, char_ptr_iterator b) { a.swap(b); }


// This class is responsible for creating a tmp directory and
// populating it with fifos.
class tmp_pipes {
  static std::string create_tmp_dir();
  std::vector<std::string> create_pipes(const std::string& tmpdir, int nb_pipes);

  std::string              tmpdir_;
  std::vector<std::string> pipes_;
  std::vector<const char*> pipes_paths_;

public:
  tmp_pipes(int nb_pipes):
  tmpdir_(create_tmp_dir()),
  pipes_(create_pipes(tmpdir_, nb_pipes))
  {
    for(auto it = pipes_.cbegin(); it != pipes_.cend(); ++it)
      pipes_paths_.push_back(it->c_str());
  }
  ~tmp_pipes() {
    for(auto it = pipes_.cbegin(); it != pipes_.cend(); ++it) {
      if(!it->empty())
        unlink(it->c_str());
    }
    rmdir(tmpdir_.c_str());
  }

  size_t size() const { return pipes_.size(); }
  const char* operator[](int i) const { return pipes_[i].c_str(); }
  std::vector<const char*>::const_iterator begin() const { return pipes_paths_.cbegin(); }
  std::vector<const char*>::const_iterator end() const { return pipes_paths_.cend(); }

  // Discard a pipe: unlink it while it is open for writing. The
  // reading process will get no data and won't be able to reopen the
  // file, marking the end of this pipe.
  void discard(int i);
};

// This class creates a new process which manages a bunch of
// "generators", sub-processes that writes into a fifo (named pipe)
// and generate sequence.
class generator_manager {
  cloexec_istream cmds_;
  tmp_pipes       pipes_;
  pid_t           manager_pid_;

  struct cmd_info_type {
    std::string command;
    int         pipe;
  };
  typedef std::map<pid_t, cmd_info_type> pid2pipe_type;
  pid2pipe_type pid2pipe_;

public:
  generator_manager(const char* cmds, int nb_pipes) :
    cmds_(cmds),
    pipes_(nb_pipes),
    manager_pid_(-1)
  {
    if(!cmds_.good())
      eraise(std::runtime_error) << "Failed to open cmds file '" << cmds << "'";
  }
  ~generator_manager() { wait(); }

  const tmp_pipes& pipes() const { return pipes_; }

  // Start the manager process
  void start();
  // Wait for manager process to finish. Return true if it finishes
  // with no error, false otherwise.
  bool wait();

private:
  void start_commands();
  void start_one_command(const std::string& command, int pipe);
  void display_status(int status, const std::string& command);
};
}

#endif /* __JELLYFISH_SPAWN_EXTERNAL_HPP_ */
