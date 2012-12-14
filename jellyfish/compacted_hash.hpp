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
#include <jellyfish/mapped_file.hpp>
#include <jellyfish/square_binary_matrix.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/parse_dna.hpp>
#include <jellyfish/misc.hpp>

namespace jellyfish {
  namespace compacted_hash {
    define_error_class(ErrorReading);

    static const char *file_type = "JFLISTDN";
    struct header {
      char     type[8];         // type of file. Expect file_type
      uint64_t key_len;
      uint64_t val_len;         // In bytes
      uint64_t size;            // In bytes
      uint64_t max_reprobe;
      uint64_t unique;
      uint64_t distinct;
      uint64_t total;
      uint64_t max_count;

      header() { }
      explicit header(char *ptr) {
        if(memcmp(ptr, file_type, sizeof(type)))
          eraise(ErrorReading) << "Bad file type '" << err::substr(ptr, sizeof(type))
                              << "', expected '" << err::substr(file_type, sizeof(type)) << "'";
        memcpy((void *)this, ptr, sizeof(struct header));
      }
    };

    template<typename storage_t>
    class writer {
      uint64_t   unique, distinct, total, max_count;
      size_t     nb_records;
      uint_t     klen, vlen;
      uint_t     key_len, val_len;
      storage_t *ary;
      char      *buffer, *end, *ptr;

    public:
      writer() : unique(0), distinct(0), total(0), max_count(0)
      { buffer = ptr = end = NULL; }

      writer(size_t _nb_records, uint_t _klen, uint_t _vlen, storage_t *_ary)
      { 
        initialize(_nb_records, _klen, _vlen, _ary);
      }

      void initialize(size_t _nb_records, uint_t _klen, uint_t _vlen, storage_t *_ary) {
        unique     = distinct = total = max_count = 0;
        nb_records = _nb_records;
        klen       = _klen;
        vlen       = _vlen;
        key_len    = bits_to_bytes(klen);
        val_len    = bits_to_bytes(vlen);
        ary        = _ary;
        buffer     = new char[nb_records * (key_len + val_len)];
        end        = buffer + (nb_records * (key_len + val_len));
        ptr        = buffer;
      }

      ~writer() {
        if(buffer)
          delete buffer;
      }

      bool append(uint64_t key, uint64_t val) {
        if(ptr >= end)
          return false;
        memcpy(ptr, &key, key_len);
        ptr += key_len;
        memcpy(ptr, &val, val_len);
        ptr += val_len;
        unique += val == 1;
        distinct++;
        total += val;
        if(val > max_count)
          max_count = val;
        return true;
      }

      void dump(std::ostream *out) {
        out->write(buffer, ptr - buffer);
        ptr = buffer;
      }

      void write_header(std::ostream *out) const {
        struct header head;
        memset(&head, '\0', sizeof(head));
        memcpy(&head.type, file_type, sizeof(head.type));
        head.key_len = klen;
        head.val_len = val_len;
        head.size = ary->get_size();
        head.max_reprobe = ary->get_max_reprobe_offset();
        out->write((char *)&head, sizeof(head));
        ary->write_ary_header(out);
      }

      void update_stats(std::ostream *out) const {
        update_stats_with(out, unique, distinct, total, max_count);
      }

      void update_stats_with(std::ostream *out, uint64_t _unique, uint64_t _distinct,
                             uint64_t _total, uint64_t _max_count) const {
        if(!out->good())
          return;
        out->seekp(0);
        if(!out->good()) {
          out->clear();
          return;
        }

        struct header head;
        memcpy(&head.type, file_type, sizeof(head.type));
        head.key_len     = klen;
        head.val_len     = val_len;
        head.size        = ary->get_size();
        head.max_reprobe = ary->get_max_reprobe_offset();
        head.unique      = _unique;
        head.distinct    = _distinct;
        head.total       = _total;
        head.max_count   = _max_count;
        out->write((char *)&head, sizeof(head));
      }

      uint64_t get_unique() const { return unique; }
      uint64_t get_distinct() const { return distinct; }
      uint64_t get_total() const { return total; }
      uint64_t get_max_count() const { return max_count; }
      uint_t   get_key_len_bytes() const { return key_len; }
      uint_t   get_val_len_bytes() const { return val_len; }

      void reset_counters() {
        unique = distinct = total = max_count = 0;
      }
    };
    
    template<typename key_t, typename val_t>
    class reader {
      struct header       header;
      std::ifstream      *io;
      uint_t              key_len;
      SquareBinaryMatrix  hash_matrix, hash_inverse_matrix;
      size_t              record_len, buffer_len;
      size_t              size_mask;
      char               *buffer, *end_buffer, *ptr;
      char                dna_str[33];

    public:
      key_t key;
      val_t val;

      reader() { io = 0; buffer = 0; memset(dna_str, '\0', sizeof(dna_str)); }
      explicit reader(std::string filename, size_t _buff_len = 10000000UL) { 
        initialize(filename, _buff_len);
      }

      void initialize(std::string filename, size_t _buff_len) {
        memset(dna_str, '\0', sizeof(dna_str)); 
        io = new std::ifstream(filename.c_str());
        io->read((char *)&header, sizeof(header));
        if(!io->good())
          eraise(ErrorReading) << "'" << filename << "': " 
                               << "File truncated";
        if(memcmp(header.type, file_type, sizeof(header.type)))
          eraise(ErrorReading) << "'" << filename << "': " 
                               << "Bad file type '" 
                              << err::substr(header.type, sizeof(header.type)) << "', expected '"
                              << err::substr(file_type, sizeof(header.type)) << "'";

        if(header.key_len > 64 || header.key_len == 0)
          eraise(ErrorReading) << "'" << filename << "': " 
                               << "Invalid key length '"
                               << header.key_len << "'";
        if(header.size != (1UL << floorLog2(header.size)))
          eraise(ErrorReading) << "'" << filename << "': " 
                               << "Size '" << header.size 
                               << "' is not a power of 2";
        key_len  = (header.key_len / 8) + (header.key_len % 8 != 0);
        record_len = key_len + header.val_len;
        buffer_len = record_len * (_buff_len / record_len);
        buffer = new char[buffer_len];
        ptr = buffer;
        end_buffer = NULL;

        hash_matrix.load(io);
        hash_inverse_matrix.load(io);
        
        if(header.distinct != 0) {
          std::streamoff list_size = get_file_size(*io);
          if(list_size != (std::streamoff)-1 &&
             list_size - (header.distinct * record_len) != 0) {
            eraise(ErrorReading) << "'" << filename << "': " 
                                 << "Bad hash size '" << list_size 
                                 << "', expected '"
                                 << (header.distinct * record_len) << "' bytes";
          }
        }
        key = val = 0;
        size_mask = header.size - 1;
      }

      ~reader() {
        if(io)
          delete io;
        if(buffer)
          delete[] buffer;
      }

      uint_t get_key_len() const { return header.key_len; }
      uint_t get_mer_len() const { return header.key_len / 2; }
      uint_t get_val_len() const { return header.val_len; }
      size_t get_size() const { return header.size; }
      uint64_t get_max_reprobe() const { return header.max_reprobe; }
      uint64_t get_max_reprobe_offset() const { return header.max_reprobe; }
      uint64_t get_unique() const { return header.unique; }
      uint64_t get_distinct() const { return header.distinct; }
      uint64_t get_total() const { return header.total; }
      uint64_t get_max_count() const { return header.max_count; }
      SquareBinaryMatrix get_hash_matrix() const { return hash_matrix; }
      SquareBinaryMatrix get_hash_inverse_matrix() const { return hash_inverse_matrix; }
      void write_ary_header(std::ostream *out) const {
        hash_matrix.dump(out);
        hash_inverse_matrix.dump(out);
      }

      key_t get_key() const { return key; }
      val_t get_val() const { return val; }
      

      void get_string(char *out) const {
        parse_dna::mer_binary_to_string(key, get_mer_len(), out);
      }
      char* get_dna_str() {
        parse_dna::mer_binary_to_string(key, get_mer_len(), dna_str);
        return dna_str;
      }
      uint64_t get_hash() const { return hash_matrix.times(key); }
      uint64_t get_pos() const { return hash_matrix.times(key) & size_mask; }

      bool next() {
        while(true) {
          if(ptr <= end_buffer) {
            memcpy(&key, ptr, key_len);
            ptr += key_len;
            memcpy(&val, ptr, header.val_len);
            ptr += header.val_len;
            return true;
          }

          if(io->fail())
            return false;
          io->read(buffer, buffer_len);
          //      if(record_len * (io->gcount() / record_len) != io->gcount())
          //        return false;
          ptr = buffer;
          end_buffer = NULL;
          if((size_t)io->gcount() >= record_len)
            end_buffer = ptr + (io->gcount() - record_len);
        }
      }
    };

    template<typename key_t, typename val_t>
    class query {
      mapped_file         file;
      struct header       header;
      uint_t              key_len;
      uint_t              val_len;
      uint_t              record_len;
      SquareBinaryMatrix  hash_matrix;
      SquareBinaryMatrix  hash_inverse_matrix;
      char               *base;
      uint64_t            size;
      uint64_t            size_mask;
      uint64_t            last_id;
      key_t               first_key, last_key;
      uint64_t            first_pos, last_pos;
      bool                canonical;

    public:
      /* Can't wait for C++0x to be finalized and call constructor
         from constructor!
       */
      explicit query(mapped_file &map) :
        file(map),
        header(file.base()), 
        key_len((header.key_len / 8) + (header.key_len % 8 != 0)),
        val_len(header.val_len),
        record_len(key_len + header.val_len),
        hash_matrix(file.base() + sizeof(header)),
        hash_inverse_matrix(file.base() + sizeof(header) + hash_matrix.dump_size()),
        base(file.base() + sizeof(header) + hash_matrix.dump_size() + hash_inverse_matrix.dump_size()),
        size(header.size),
        size_mask(header.size - 1),
        last_id((file.end() - base) / record_len),
        canonical(false)
      {
        if(header.distinct != 0 && file.end() - base - header.distinct * record_len != 0)
          eraise(ErrorReading) << "'" << file.path() << "': " 
                               << "Bad hash size '" << (file.end() - base)
                               << "', expected '" << header.distinct * record_len << "' bytes";
          
        get_key(0, &first_key);
        first_pos = get_pos(first_key);
        get_key(last_id - 1, &last_key);
        last_pos = get_pos(last_key);
      }
      explicit query(std::string filename) : 
        file(filename.c_str()), 
        header(file.base()), 
        key_len((header.key_len / 8) + (header.key_len % 8 != 0)),
        val_len(header.val_len),
        record_len(key_len + header.val_len),
        hash_matrix(file.base() + sizeof(header)),
        hash_inverse_matrix(file.base() + sizeof(header) + hash_matrix.dump_size()),
        base(file.base() + sizeof(header) + hash_matrix.dump_size() + hash_inverse_matrix.dump_size()),
        size(header.size),
        size_mask(header.size - 1),
        last_id((file.end() - base) / record_len),
        canonical(false)
      { 
        if(header.distinct != 0 && file.end() - base - header.distinct * record_len != 0)
          eraise(ErrorReading) << "'" << file.path() << "': " 
                               << "Bad hash size '" << (file.end() - base)
                               << "', expected '" << header.distinct * record_len << "' bytes";
          
        get_key(0, &first_key);
        first_pos = get_pos(first_key);
        get_key(last_id - 1, &last_key);
        last_pos = get_pos(last_key);
      }

      uint_t get_key_len() const { return header.key_len; }
      uint_t get_mer_len() const { return header.key_len / 2; }
      uint_t get_val_len() const { return header.val_len; }
      size_t get_size() const { return header.size; }
      size_t get_nb_mers() const { return last_id; }
      uint64_t get_max_reprobe() const { return header.max_reprobe; }
      uint64_t get_max_reprobe_offset() const { return header.max_reprobe; }
      uint64_t get_unique() const { return header.unique; }
      uint64_t get_distinct() const { return header.distinct; }
      uint64_t get_total() const { return header.total; }
      uint64_t get_max_count() const { return header.max_count; }
      SquareBinaryMatrix get_hash_matrix() const { return hash_matrix; }
      SquareBinaryMatrix get_hash_inverse_matrix() const { return hash_inverse_matrix; }
      bool get_canonical() const { return canonical; }
      void set_canonical(bool v) { canonical = v; }

      /* No check is made on the validity of the string passed. Should only contained [acgtACGT] to get a valid answer.
       */
      val_t operator[] (const char *key_s) const {
        return get_key_val(parse_dna::mer_string_to_binary(key_s, get_mer_len()));
      }
      val_t operator[] (const key_t key) const { return get_key_val(key); }

      void get_key(size_t id, key_t *k) const {
        *k = 0;
        memcpy(k, base + id * record_len, key_len);
      }
      void get_val(size_t id, val_t *v) const {
        *v = 0;
        memcpy(v, base + id * record_len + key_len, val_len);
      }
      uint64_t get_pos(key_t k) const { 
        return hash_matrix.times(k) & size_mask;
      }
        
      val_t get_key_val(const key_t key) const {
        uint64_t id;
        val_t res;
        if(get_key_val_id(key, &res, &id))
          return res;
        else
          return 0;
      }

      bool get_key_val_id(const key_t _key, val_t *res, 
                          uint64_t *id) const {
        key_t key;
        if(canonical) {
          key = parse_dna::reverse_complement(_key, get_mer_len());
          if(key > _key)
            key = _key;
        } else {
          key = _key;
        }
        if(key == first_key) {
          get_val(0, res);
          *id = 0;
          return true;
        }
        if(key == last_key) {
          get_val(last_id - 1, res);
          *id = last_id;
          return true;
        }
        uint64_t pos = get_pos(key);
        if(pos < first_pos || pos > last_pos)
          return false;
        uint64_t first = 0, last = last_id;
        while(first < last - 1) {
          uint64_t middle = (first + last) / 2;
          key_t mid_key;
          get_key(middle, &mid_key);
          //          printf("%ld %ld %ld %ld %ld %ld %ld\n", key, pos, first, middle, last, mid_key, get_pos(mid_key));
          if(key == mid_key) {
            get_val(middle, res);
            *id = middle;
            return true;
          }
          uint64_t mid_pos = get_pos(mid_key);
          if(mid_pos > pos || (mid_pos == pos && mid_key > key))
            last = middle;
          else
            first = middle;
        }
        return false;
      }
      
      class iterator {
        char     *base, *ptr;
        uint64_t  last_id;
        uint_t    key_len;
        uint_t    val_len;
        uint_t    record_len;
        uint_t    mer_len;
        uint64_t  id;
        key_t     key;
        val_t     val;
        char      dna_str[33];

      public:
        iterator(char *_base, uint64_t _last_id, uint_t _key_len, uint_t _val_len, uint_t _mer_len) :
          base(_base), ptr(_base), last_id(_last_id), key_len(_key_len), val_len(_val_len),
          record_len(key_len + val_len), mer_len(_mer_len), id(0), key(0), val(0)
        {
          memset(dna_str, '\0', sizeof(dna_str));
        }

        key_t get_key() const { return key; }
        val_t get_val() const { return val; }
        uint64_t get_id() const { return id; }

        bool next() {
          if(id >= last_id)
            return false;
          ++id;
          memcpy(&key, ptr, key_len);
          ptr += key_len;
          memcpy(&val, ptr, val_len);
          ptr += val_len;
          return true;
        }

        bool next(uint64_t *_id, key_t *_key, val_t *_val) {
          if(id >= last_id)
            return false;
          *_id = atomic::gcc::add_fetch(&id, (uint64_t)1) - 1;
          if(*_id >= last_id)
            return false;
          char *ptr = base + (*_id) * record_len;
          *_key = 0;
          memcpy(_key, ptr, key_len);
          ptr += key_len;
          *_val = 0;
          memcpy(_val, ptr, val_len);
          return true;
        }

        inline bool next(key_t *_key, val_t *_val) {
          uint64_t _id;
          return next(&_id, _key, _val);
        }

        char *get_dna_str() {
          parse_dna::mer_binary_to_string(key, mer_len, dna_str);
          return dna_str;
        }

        void get_dna_str(char *out) {
          parse_dna::mer_binary_to_string(key, mer_len, out);
        }
      };

      iterator get_iterator() const { return iterator_all(); }
      iterator iterator_all() const { return iterator(base, last_id, key_len, val_len, get_mer_len()); }
      iterator iterator_slice(uint64_t slice_number, uint64_t number_of_slice) const {
        std::pair<uint64_t, uint64_t> res =
          slice(slice_number, number_of_slice, last_id);
        char   *it_base    = base + res.first * record_len;
        uint64_t  it_last_id = res.second - res.first;

        if(it_base >= file.end()) {
          it_base    = base;
          it_last_id = 0;
        } else if(it_base + it_last_id * record_len > file.end())
          it_last_id = (file.end() - it_base) / record_len;

        return iterator(it_base, it_last_id, key_len, val_len, get_mer_len()); 
      }
    };
  }
}
#endif /* __COMPACTED_HASH__ */
