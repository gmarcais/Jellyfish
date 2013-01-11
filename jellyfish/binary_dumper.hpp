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

#ifndef __JELLYFISH_BINARY_DUMPER_HPP__
#define __JELLYFISH_BINARY_DUMPER_HPP__

#include <iostream>

#include <jellyfish/sorted_dumper.hpp>

namespace jellyfish {
/// Dump a hash array in sorted binary format. The key/value pairs are
/// written in a sorted list according to the hash function order. The
/// k-mer and count are written in binary, byte aligned.
template<typename storage_t>
class binary_dumper : public sorted_dumper<binary_dumper<storage_t>, storage_t> {
  typedef sorted_dumper<binary_dumper<storage_t>, storage_t> super;

  int      val_len_;
  uint64_t max_val;
  int      key_len_;

public:
  static const char* format;

  binary_dumper(int val_len, // length of value field in bytes
                int nb_threads, const char* file_prefix,
                file_header* header = 0) :
    super(nb_threads, file_prefix, header),
    val_len_(val_len), max_val(((uint64_t)1 << (8 * val_len)) - 1)
  { }

  virtual void _dump(storage_t* ary) {
    key_len_ = ary->key_len() / 8 + (ary->key_len() % 8 != 0);
    if(super::header_) {
      super::header_->update_from_ary(*ary);
      super::header_->format(format);
      super::header_->counter_len(val_len_);
    }
    super::_dump(ary);
  }

  void write_key_value_pair(std::ostream& out, typename super::heap_item item) {
    out.write((const char*)item->key_.data(), key_len_);
    uint64_t v = std::min(max_val, item->val_);
    out.write((const char*)&v, val_len_);
  }
};
template<typename storage_t>
const char* jellyfish::binary_dumper<storage_t>::format = "binary/sorted";

/// Reader of the format written by binary_dumper. Behaves like an
/// iterator (has next() method which behaves similarly to the next()
/// method of the hash array).
template<typename Key, typename Val>
class binary_reader {
  std::istream& is_;
  int           val_len_;
  Key           key_;
  Val           val_;

public:
  binary_reader(std::istream& is, // stream containing data (past any header)
                int val_len) :  // val length in bytes
    is_(is), val_len_(val_len)
  { }

  const Key& key() { return key_; }
  const Val& val() { return val_; }

  bool next() {
    key_.template read<1>(is_);
    val_ = 0;
    is_.read((char*)&val_, val_len_);
    return is_.good();
  }
};
}

#endif /* __JELLYFISH_BINARY_DUMPER_HPP__ */
