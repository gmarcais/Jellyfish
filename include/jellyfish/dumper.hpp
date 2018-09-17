/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifndef __JELLYFISH_DUMPER_HPP__
#define __JELLYFISH_DUMPER_HPP__

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <jellyfish/err.hpp>
#include <jellyfish/time.hpp>

/**
 * A dumper is responsible to dump the hash array to permanent storage
 * and zero out the array.
 **/
namespace jellyfish {
template<typename storage_t>
class dumper_t {
  Time                     writing_time_;
  int                      index_;
  bool                     one_file_;
  std::vector<std::string> file_names_;

protected:
  uint64_t                 min_;
  uint64_t                 max_;

public:
  define_error_class(ErrorWriting);

protected:
  /// Open the next file with given prefix. If one_file is false,
  /// append _0, _1, etc. to the prefix for actual file name. If
  /// one_file is true, the prefix is the file name. The first time
  /// the file is open in trunc mode, the subsequent times in append
  /// mode.
  void open_next_file(const char *prefix, std::ofstream &out) {
    std::ostringstream name;
    name << prefix;
    std::ios::openmode mode = std::ios::out;
    if(one_file_) {
      mode |= (index_++ ? std::ios::ate : std::ios::trunc);
    } else {
      name << index_++;
      mode |= std::ios::trunc;
    }
    file_names_.push_back(name.str());

    out.open(name.str().c_str());
    if(out.fail())
      throw ErrorWriting(err::msg() << "'" << name.str() << "': "
                         << "Can't open file for writing" << err::no);
  }

public:
  dumper_t() : writing_time_(::Time::zero), index_(0), one_file_(false),
               min_(0), max_(std::numeric_limits<uint64_t>::max())
  {}

  void dump(storage_t* ary) {
    Time start;
    _dump(ary);
    Time end;
    writing_time_ += end - start;
  }

  bool one_file() const { return one_file_; }
  void one_file(bool v) { one_file_ = v; }

  virtual void _dump(storage_t* ary) = 0;
  uint64_t min() const { return min_; }
  void min(uint64_t m) { min_ = m; }
  uint64_t max() const { return max_; }
  void max(uint64_t m) { max_ = m; }
  Time get_writing_time() const { return writing_time_; }
  int nb_files() const { return index_; }
  std::vector<std::string> file_names() { return file_names_; }
  std::vector<const char*> file_names_cstr() {
    std::vector<const char*> res;
    for(size_t i = 0; i < file_names_.size(); ++i)
      res.push_back(file_names_[i].c_str());
    return res;
  }
  virtual ~dumper_t() {};
};
}
#endif // __DUMPER_HPP__
