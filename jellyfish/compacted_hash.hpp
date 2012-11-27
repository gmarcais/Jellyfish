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

#ifndef __JELLYFISH_COMPACTED_HASH__
#define __JELLYFISH_COMPACTED_HASH__

#include <iostream>
#include <fstream>
#include <string.h>
#include <pthread.h>
#include <jellyfish/err.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/rectangular_binary_matrix.hpp>
#include <jellyfish/bstream.hpp>

namespace jellyfish {
namespace compacted_hash {
/// The writer and reader are only responsible to write and read the
/// binary part of the file. The header parsing is not done.

/// On disk format is bit packed. Every block starts aligned on a word
/// boundary. It contains an id corresponding to the position in the
/// original hash table, written as a word (default to uint64_t). It
/// is followed by a list of (short key, offset, value). The short key
/// is only the higher bits of the key stored in the hash. The offset
/// is the change in position since the previous key. Value is the
/// value. These triplets are bit packed.
///
/// Each block contains the same number of triplets and hence every
/// block has the same length. 
///
/// An offset of 0 is meaningful (the two mers hashed to the same
/// position) and is treated normally. An offset equal to the max
/// value (field set to all 1s) has a special meaning. In that case,
/// there is no key and the key field is used to encode a larger
/// offset. The actual offset is the value of the offset field plus
/// what is stored in the first word of the key field (or part of the
/// word if the key field is smaller than a word).
///
/// The offset stored in the key field of all 1s is special: it is
/// used as a end of stream marker. The last word in the stream is
/// padded with 1s. The end of the stream is detected either by an EOR
/// of a maximum offset stored in the key field.
///
/// Key is assumed to have the method 'word get_bits(uint start, uint
/// len)' (see mer_dna.hpp).

template<typename word>
struct base {
  const uint16_t key_len_, short_key_len_, off_len_, val_len_;
  const uint16_t block_word_len_, block_len_;
  const word     max_offset_;     // special treatment if offset equal to max
  const word     max_key_offset_; // maximum offset that can be encoded in key field

  static const size_t bword = sizeof(word) * 8;

  base(uint16_t key_len, // Length of key
       uint16_t short_key_len, // Length of short key (i.e. minus implied by position)
       uint16_t off_len, // offset length. off_len <= (key_len - short_key_len)
       uint16_t val_len, // Length of value in bits
       uint16_t approx_block_len = 1024) : // Approximative number of triplets per block
    key_len_(key_len),
    short_key_len_(short_key_len),
    off_len_(std::min(key_len_ - short_key_len_, (int)off_len)),
    val_len_(val_len),
    block_word_len_(approx_block_len * (short_key_len_ + off_len_ + val_len_) / sizeof(word)),
    block_len_(block_word_len_ * sizeof(word) / (short_key_len_ + off_len_ + val_len_)),
    max_offset_(bitmask<word>(off_len_)),
    max_key_offset_(bitmask<word>(short_key_len_))
  {}

  uint16_t key_len() const { return key_len_; }
  uint16_t short_key_len() const { return short_key_len_; }
  uint16_t off_len() const { return off_len_; }
  uint16_t val_len() const { return val_len_; }
  uint16_t block_word_len() const { return block_word_len_; }
  uint16_t block_len() const { return block_len_; }
  word max_offset() const { return max_offset_; }
  word max_key_offset() const { return max_key_offset_; }
};
template<typename word>
const size_t base<word>::bword;

template<typename Key, typename Val = uint64_t, typename word = uint64_t>
class writer : public base<word> {
  typedef base<word> super;

  obstream<word> os_;
  uint16_t       index_;          // triplet index in block
  word           id_;             // current position in hash

public:
  writer(std::ostream& os, // Stream to write to
         uint16_t key_len, uint16_t short_key_len, uint16_t off_len, uint16_t val_len,
         uint16_t approx_block_len = 1024) :
    super(key_len, short_key_len, off_len, val_len, approx_block_len),
    os_(os),
    index_(super::block_len_),
    id_(0)
  { }

  ~writer() {
    os_.one_pad();
  }

  /// Write a (key, off, val) triplet. It is assumed that the id
  /// is not less than the current id_.
  void write(const Key& key, Val val, word id) {
    while(true) {
      if(index_ < super::block_len_) {
        ++index_;
      } else {
        id_    = id;
        index_ = 0;
        os_.align();
        os_.write(id_, super::bword);
        break;
      }
      if(id < id_ + super::max_offset_)
        break;

      // Offset to large to encode in the offset field. Special treatment: use key field.
      word aoffset = std::min(super::max_key_offset_ - 1, id - id_ - super::max_offset_);
      os_.write(aoffset, std::min(super::bword, (size_t)super::short_key_len_));
      for(uint16_t i = super::bword; i < super::short_key_len_; i += super::bword) {
        size_t len = std::min((size_t)(super::short_key_len_ - i), super::bword);
        os_.write((word)0, len);
      }
      os_.write(super::max_offset_, super::off_len_);
      os_.write((word)0, super::val_len_);

      id_ += aoffset + super::max_offset_;
    }

    for(uint16_t i = super::key_len_ - super::short_key_len_; i < super::key_len_; i += super::bword) {
      size_t len = std::min((size_t)(super::key_len_ - i), super::bword);
      os_.write(key.get_bits(i, len), len);
    }
    os_.write(id - id_, super::off_len_);
    os_.write(val, super::val_len_);
    id_ = id;
  }
};

template<typename Key, typename Val = uint64_t, typename word = uint64_t>
class stream_iterator : public base<word> {
  typedef base<word> super;

  ibstream<word>          is_;
  uint16_t                index_; // triplet index in block
  word                    id_;  // current position
  RectangularBinaryMatrix inv_;
  Key                     key_;
  Val                     val_;
  bool                    reversed_key_;

public:
  stream_iterator(std::istream& is, // Stream to write to
                  uint16_t key_len, uint16_t short_key_len, uint16_t off_len, uint16_t val_len,
                  RectangularBinaryMatrix inv, uint16_t approx_block_len = 1024) :
    super(key_len, short_key_len, off_len, val_len, approx_block_len),
    is_(is),
    index_(super::block_len_),
    id_(0),
    inv_(inv),
    reversed_key_(false)
  { }

  const Key& key() {
    if(!reversed_key_) {
      key_.set_bits(0, super::key_len_ - super::short_key_len_, inv_.times(key_));
      reversed_key_ = true;
    }
    return key_;
  }
  Val val() const { return val_; }
  size_t id() const { return id_; }

  bool next() {
    while(true) {
      if(index_ < super::block_len_) {
        ++index_;
      } else {
        is_.align();
        id_ = is_.read(super::bword);
        index_ = 0;
      }

      reversed_key_ = false;
      for(uint16_t i = super::key_len_ - super::short_key_len_; i < super::key_len_; i += super::bword) {
        size_t len = std::min((size_t)(super::key_len_ - i), super::bword);
        word w = is_.read(len);
        key_.set_bits(i, len, w);
      }
      word offset = is_.read(super::off_len_);
      val_        = is_.read(super::val_len_);

      if(!is_.stream().good())
        return false;

      if(offset < super::max_offset_) {
        id_ += offset;
        key_.set_bits(0, super::key_len_ - super::short_key_len_, id_);
        return true;
      }

      // offset is == to max_offset_. Special treatment
      word aoffset = key_.get_bits(super::key_len_ - super::short_key_len_, std::min((size_t)super::short_key_len_, super::bword));
      if(aoffset == super::max_key_offset_)
        return false;
      id_ += super::max_offset_ + aoffset;
    }

    // Never reached
    return true;
  }
};

    // template<typename key_t, typename val_t>
    // class reader {
    //   struct header       header;
    //   std::ifstream      *io;
    //   uint_t              key_len;
    //   SquareBinaryMatrix  hash_matrix, hash_inverse_matrix;
    //   size_t              record_len, buffer_len;
    //   size_t              size_mask;
    //   char               *buffer, *end_buffer, *ptr;
    //   char                dna_str[33];

    // public:
    //   key_t key;
    //   val_t val;

    //   reader() { io = 0; buffer = 0; memset(dna_str, '\0', sizeof(dna_str)); }
    //   reader(std::string filename, size_t _buff_len = 10000000UL) { 
    //     initialize(filename, _buff_len);
    //   }

    //   void initialize(std::string filename, size_t _buff_len) {
    //     memset(dna_str, '\0', sizeof(dna_str)); 
    //     io = new std::ifstream(filename.c_str());
    //     io->read((char *)&header, sizeof(header));
    //     if(!io->good())
    //       eraise(ErrorReading) << "'" << filename << "': " 
    //                            << "File truncated";
    //     if(memcmp(header.type, file_type, sizeof(header.type)))
    //       eraise(ErrorReading) << "'" << filename << "': " 
    //                            << "Bad file type '" 
    //                           << err::substr(header.type, sizeof(header.type)) << "', expected '"
    //                           << err::substr(file_type, sizeof(header.type)) << "'";

    //     if(header.key_len > 64 || header.key_len == 0)
    //       eraise(ErrorReading) << "'" << filename << "': " 
    //                            << "Invalid key length '"
    //                            << header.key_len << "'";
    //     if(header.size != (1UL << floorLog2(header.size)))
    //       eraise(ErrorReading) << "'" << filename << "': " 
    //                            << "Size '" << header.size 
    //                            << "' is not a power of 2";
    //     key_len  = (header.key_len / 8) + (header.key_len % 8 != 0);
    //     record_len = key_len + header.val_len;
    //     buffer_len = record_len * (_buff_len / record_len);
    //     buffer = new char[buffer_len];
    //     ptr = buffer;
    //     end_buffer = NULL;

    //     hash_matrix.load(io);
    //     hash_inverse_matrix.load(io);
        
    //     if(header.distinct != 0) {
    //       std::streamoff list_size = get_file_size(*io);
    //       if(list_size != (std::streamoff)-1 &&
    //          list_size - (header.distinct * record_len) != 0) {
    //         eraise(ErrorReading) << "'" << filename << "': " 
    //                              << "Bad hash size '" << list_size 
    //                              << "', expected '"
    //                              << (header.distinct * record_len) << "' bytes";
    //       }
    //     }
    //     key = val = 0;
    //     size_mask = header.size - 1;
    //   }

    //   ~reader() {
    //     if(io)
    //       delete io;
    //     if(buffer)
    //       delete[] buffer;
    //   }

    //   uint_t get_key_len() const { return header.key_len; }
    //   uint_t get_mer_len() const { return header.key_len / 2; }
    //   uint_t get_val_len() const { return header.val_len; }
    //   size_t get_size() const { return header.size; }
    //   uint64_t get_max_reprobe() const { return header.max_reprobe; }
    //   uint64_t get_max_reprobe_offset() const { return header.max_reprobe; }
    //   uint64_t get_unique() const { return header.unique; }
    //   uint64_t get_distinct() const { return header.distinct; }
    //   uint64_t get_total() const { return header.total; }
    //   uint64_t get_max_count() const { return header.max_count; }
    //   SquareBinaryMatrix get_hash_matrix() const { return hash_matrix; }
    //   SquareBinaryMatrix get_hash_inverse_matrix() const { return hash_inverse_matrix; }
    //   void write_ary_header(std::ostream *out) const {
    //     hash_matrix.dump(out);
    //     hash_inverse_matrix.dump(out);
    //   }

    //   key_t get_key() const { return key; }
    //   val_t get_val() const { return val; }
      

    //   void get_string(char *out) const {
    //     parse_dna::mer_binary_to_string(key, get_mer_len(), out);
    //   }
    //   char* get_dna_str() {
    //     parse_dna::mer_binary_to_string(key, get_mer_len(), dna_str);
    //     return dna_str;
    //   }
    //   uint64_t get_hash() const { return hash_matrix.times(key); }
    //   uint64_t get_pos() const { return hash_matrix.times(key) & size_mask; }

    //   bool next() {
    //     while(true) {
    //       if(ptr <= end_buffer) {
    //         memcpy(&key, ptr, key_len);
    //         ptr += key_len;
    //         memcpy(&val, ptr, header.val_len);
    //         ptr += header.val_len;
    //         return true;
    //       }

    //       if(io->fail())
    //         return false;
    //       io->read(buffer, buffer_len);
    //       //      if(record_len * (io->gcount() / record_len) != io->gcount())
    //       //        return false;
    //       ptr = buffer;
    //       end_buffer = NULL;
    //       if((size_t)io->gcount() >= record_len)
    //         end_buffer = ptr + (io->gcount() - record_len);
    //     }
    //   }
    // };

    // template<typename key_t, typename val_t>
    // class query {
    //   mapped_file         file;
    //   struct header       header;
    //   uint_t              key_len;
    //   uint_t              val_len;
    //   uint_t              record_len;
    //   SquareBinaryMatrix  hash_matrix;
    //   SquareBinaryMatrix  hash_inverse_matrix;
    //   char               *base;
    //   uint64_t            size;
    //   uint64_t            size_mask;
    //   uint64_t            last_id;
    //   key_t               first_key, last_key;
    //   uint64_t            first_pos, last_pos;
    //   bool                canonical;

    // public:
    //   /* Can't wait for C++0x to be finalized and call constructor
    //      from constructor!
    //    */
    //   explicit query(mapped_file &map) :
    //     file(map),
    //     header(file.base()), 
    //     key_len((header.key_len / 8) + (header.key_len % 8 != 0)),
    //     val_len(header.val_len),
    //     record_len(key_len + header.val_len),
    //     hash_matrix(file.base() + sizeof(header)),
    //     hash_inverse_matrix(file.base() + sizeof(header) + hash_matrix.dump_size()),
    //     base(file.base() + sizeof(header) + hash_matrix.dump_size() + hash_inverse_matrix.dump_size()),
    //     size(header.size),
    //     size_mask(header.size - 1),
    //     last_id((file.end() - base) / record_len),
    //     canonical(false)
    //   {
    //     if(header.distinct != 0 && file.end() - base - header.distinct * record_len != 0)
    //       eraise(ErrorReading) << "'" << file.path() << "': " 
    //                            << "Bad hash size '" << (file.end() - base)
    //                            << "', expected '" << header.distinct * record_len << "' bytes";
          
    //     get_key(0, &first_key);
    //     first_pos = get_pos(first_key);
    //     get_key(last_id - 1, &last_key);
    //     last_pos = get_pos(last_key);
    //   }
    //   explicit query(std::string filename) : 
    //     file(filename.c_str()), 
    //     header(file.base()), 
    //     key_len((header.key_len / 8) + (header.key_len % 8 != 0)),
    //     val_len(header.val_len),
    //     record_len(key_len + header.val_len),
    //     hash_matrix(file.base() + sizeof(header)),
    //     hash_inverse_matrix(file.base() + sizeof(header) + hash_matrix.dump_size()),
    //     base(file.base() + sizeof(header) + hash_matrix.dump_size() + hash_inverse_matrix.dump_size()),
    //     size(header.size),
    //     size_mask(header.size - 1),
    //     last_id((file.end() - base) / record_len),
    //     canonical(false)
    //   { 
    //     if(header.distinct != 0 && file.end() - base - header.distinct * record_len != 0)
    //       eraise(ErrorReading) << "'" << file.path() << "': " 
    //                            << "Bad hash size '" << (file.end() - base)
    //                            << "', expected '" << header.distinct * record_len << "' bytes";
          
    //     get_key(0, &first_key);
    //     first_pos = get_pos(first_key);
    //     get_key(last_id - 1, &last_key);
    //     last_pos = get_pos(last_key);
    //   }

    //   uint_t get_key_len() const { return header.key_len; }
    //   uint_t get_mer_len() const { return header.key_len / 2; }
    //   uint_t get_val_len() const { return header.val_len; }
    //   size_t get_size() const { return header.size; }
    //   size_t get_nb_mers() const { return last_id; }
    //   uint64_t get_max_reprobe() const { return header.max_reprobe; }
    //   uint64_t get_max_reprobe_offset() const { return header.max_reprobe; }
    //   uint64_t get_unique() const { return header.unique; }
    //   uint64_t get_distinct() const { return header.distinct; }
    //   uint64_t get_total() const { return header.total; }
    //   uint64_t get_max_count() const { return header.max_count; }
    //   SquareBinaryMatrix get_hash_matrix() const { return hash_matrix; }
    //   SquareBinaryMatrix get_hash_inverse_matrix() const { return hash_inverse_matrix; }
    //   bool get_canonical() const { return canonical; }
    //   void set_canonical(bool v) { canonical = v; }

    //   /* No check is made on the validity of the string passed. Should only contained [acgtACGT] to get a valid answer.
    //    */
    //   val_t operator[] (const char *key_s) const {
    //     return get_key_val(parse_dna::mer_string_to_binary(key_s, get_mer_len()));
    //   }
    //   val_t operator[] (const key_t key) const { return get_key_val(key); }

    //   void get_key(size_t id, key_t *k) const {
    //     *k = 0;
    //     memcpy(k, base + id * record_len, key_len);
    //   }
    //   void get_val(size_t id, val_t *v) const {
    //     *v = 0;
    //     memcpy(v, base + id * record_len + key_len, val_len);
    //   }
    //   uint64_t get_pos(key_t k) const { 
    //     return hash_matrix.times(k) & size_mask;
    //   }
        
    //   val_t get_key_val(const key_t key) const {
    //     uint64_t id;
    //     val_t res;
    //     if(get_key_val_id(key, &res, &id))
    //       return res;
    //     else
    //       return 0;
    //   }

    //   bool get_key_val_id(const key_t _key, val_t *res, 
    //                       uint64_t *id) const {
    //     key_t key;
    //     if(canonical) {
    //       key = parse_dna::reverse_complement(_key, get_mer_len());
    //       if(key > _key)
    //         key = _key;
    //     } else {
    //       key = _key;
    //     }
    //     if(key == first_key) {
    //       get_val(0, res);
    //       *id = 0;
    //       return true;
    //     }
    //     if(key == last_key) {
    //       get_val(last_id - 1, res);
    //       *id = last_id;
    //       return true;
    //     }
    //     uint64_t pos = get_pos(key);
    //     if(pos < first_pos || pos > last_pos)
    //       return false;
    //     uint64_t first = 0, last = last_id;
    //     while(first < last - 1) {
    //       uint64_t middle = (first + last) / 2;
    //       key_t mid_key;
    //       get_key(middle, &mid_key);
    //       //          printf("%ld %ld %ld %ld %ld %ld %ld\n", key, pos, first, middle, last, mid_key, get_pos(mid_key));
    //       if(key == mid_key) {
    //         get_val(middle, res);
    //         *id = middle;
    //         return true;
    //       }
    //       uint64_t mid_pos = get_pos(mid_key);
    //       if(mid_pos > pos || (mid_pos == pos && mid_key > key))
    //         last = middle;
    //       else
    //         first = middle;
    //     }
    //     return false;
    //   }
      
    //   class iterator {
    //     char     *base, *ptr;
    //     uint64_t  last_id;
    //     uint_t    key_len;
    //     uint_t    val_len;
    //     uint_t    record_len;
    //     uint_t    mer_len;
    //     uint64_t  id;
    //     key_t     key;
    //     val_t     val;
    //     char      dna_str[33];

    //   public:
    //     iterator(char *_base, uint64_t _last_id, uint_t _key_len, uint_t _val_len, uint_t _mer_len) :
    //       base(_base), ptr(_base), last_id(_last_id), key_len(_key_len), val_len(_val_len),
    //       record_len(key_len + val_len), mer_len(_mer_len), id(0), key(0), val(0)
    //     {
    //       memset(dna_str, '\0', sizeof(dna_str));
    //     }

    //     key_t get_key() const { return key; }
    //     val_t get_val() const { return val; }
    //     uint64_t get_id() const { return id; }

    //     bool next() {
    //       if(id >= last_id)
    //         return false;
    //       ++id;
    //       memcpy(&key, ptr, key_len);
    //       ptr += key_len;
    //       memcpy(&val, ptr, val_len);
    //       ptr += val_len;
    //       return true;
    //     }

    //     bool next(uint64_t *_id, key_t *_key, val_t *_val) {
    //       if(id >= last_id)
    //         return false;
    //       *_id = atomic::gcc::add_fetch(&id, (uint64_t)1) - 1;
    //       if(*_id >= last_id)
    //         return false;
    //       char *ptr = base + (*_id) * record_len;
    //       *_key = 0;
    //       memcpy(_key, ptr, key_len);
    //       ptr += key_len;
    //       *_val = 0;
    //       memcpy(_val, ptr, val_len);
    //       return true;
    //     }

    //     inline bool next(key_t *_key, val_t *_val) {
    //       uint64_t _id;
    //       return next(&_id, _key, _val);
    //     }

    //     char *get_dna_str() {
    //       parse_dna::mer_binary_to_string(key, mer_len, dna_str);
    //       return dna_str;
    //     }

    //     void get_dna_str(char *out) {
    //       parse_dna::mer_binary_to_string(key, mer_len, out);
    //     }
    //   };

    //   iterator get_iterator() const { return iterator_all(); }
    //   iterator iterator_all() const { return iterator(base, last_id, key_len, val_len, get_mer_len()); }
    //   iterator iterator_slice(uint64_t slice_number, uint64_t number_of_slice) const {
    //     std::pair<uint64_t, uint64_t> res =
    //       slice(slice_number, number_of_slice, last_id);
    //     char   *it_base    = base + res.first * record_len;
    //     uint64_t  it_last_id = res.second - res.first;

    //     if(it_base >= file.end()) {
    //       it_base    = base;
    //       it_last_id = 0;
    //     } else if(it_base + it_last_id * record_len > file.end())
    //       it_last_id = (file.end() - it_base) / record_len;

    //     return iterator(it_base, it_last_id, key_len, val_len, get_mer_len()); 
    //   }
    // };
} } // namespace jellyfish { namespace compacted_hash {
#endif /* __COMPACTED_HASH__ */
