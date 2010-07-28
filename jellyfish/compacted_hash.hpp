#ifndef __COMPACTED_HASH__
#define __COMPACTED_HASH__

#include <iostream>
#include <fstream>
#include <string.h>
#include <pthread.h>
#include "square_binary_matrix.hpp"

namespace jellyfish {
  namespace compacted_hash {
    struct header {
      uint64_t  key_len;
      uint64_t  val_len; // In bytes
      uint64_t  size; // In bytes
      uint64_t  max_reprobe;
      uint64_t  unique;
      uint64_t  distinct;
      uint64_t  total;
    };
    class ErrorReading : public std::exception {
      std::string msg;
    public:
      ErrorReading(const std::string _msg) : msg(_msg) { }
      virtual ~ErrorReading() throw() {}
      virtual const char* what() const throw() {
        return msg.c_str();
      }
    };

    template<typename storage_t>
    class writer {
      uint64_t   unique, distinct, total;
      size_t     nb_records;
      uint_t     klen, vlen;
      uint_t     key_len, val_len;
      storage_t *ary;
      char      *buffer, *end, *ptr;

    public:
      writer() : unique(0), distinct(0), total(0) { buffer = ptr = end = NULL; }

      writer(size_t _nb_records, uint_t _klen, uint_t _vlen, storage_t *_ary)
      { 
        initialize(_nb_records, _klen, _vlen, _ary);
      }

      void initialize(size_t _nb_records, uint_t _klen, uint_t _vlen, storage_t *_ary) {
        unique     = distinct = total = 0;
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
        return true;
      }

      void dump(std::ostream *out) {
        out->write(buffer, ptr - buffer);
        ptr = buffer;
      }

      void write_header(std::ostream *out) const {
        struct header head;
        memset(&head, '\0', sizeof(head));
        head.key_len = klen;
        head.val_len = val_len;
        head.size = ary->get_size();
        head.max_reprobe = ary->get_max_reprobe_offset();
        out->write((char *)&head, sizeof(head));
        ary->write_ary_header(out);
      }

      void update_stats(std::ostream *out) const {
        update_stats_with(out, unique, distinct, total);
      }

      void update_stats_with(std::ostream *out, uint64_t _unique, uint64_t _distinct,
                             uint64_t _total) const {
        struct header head;
        head.key_len = klen;
        head.val_len = val_len;
        head.size = ary->get_size();
        head.max_reprobe = ary->get_max_reprobe_offset();
        head.unique = _unique;
        head.distinct = _distinct;
        head.total = _total;
        out->seekp(0);
        out->write((char *)&head, sizeof(head));
      }

      uint64_t get_unique() const { return unique; }
      uint64_t get_distinct() const { return distinct; }
      uint64_t get_total() const { return total; }
      uint_t   get_key_len_bytes() const { return key_len; }
      uint_t   get_val_len_bytes() const { return val_len; }
    };
    
    template<typename key_t, typename val_t>
    class reader {
      struct header              header;
      std::ifstream             *io;
      uint_t                     key_len;
      SquareBinaryMatrix         hash_matrix, hash_inverse_matrix;
      size_t                     record_len, buffer_len;
      size_t                     size_mask;
      char                      *buffer, *end_buffer, *ptr;

    public:
      key_t key;
      val_t val;

      reader() { io = 0; buffer = 0; }
      reader(std::string filename, size_t _buff_len = 10000000UL) { 
        initialize(filename, _buff_len);
      }

      void initialize(std::string filename, size_t _buff_len) {
        io = new ifstream(filename.c_str());
        io->read((char *)&header, sizeof(header));
        if(!io->good())
          throw new ErrorReading("Error reading header");
        key_len  = (header.key_len / 8) + (header.key_len % 8 != 0);
        record_len = key_len + header.val_len;
        buffer_len = record_len * (_buff_len / record_len);
        buffer = new char[buffer_len];
        ptr = buffer;
        end_buffer = NULL;

        hash_matrix.load(io);
        hash_inverse_matrix.load(io);
        key = val = 0;
        size_mask = header.size - 1; //TODO: check that header.size is a power of 2
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
      SquareBinaryMatrix get_hash_matrix() const { return hash_matrix; }
      SquareBinaryMatrix get_hash_inverse_matrix() const { return hash_inverse_matrix; }
      void write_ary_header(std::ostream *out) const {
        hash_matrix.dump(out);
        hash_inverse_matrix.dump(out);
      }

      void get_string(char *out) const {
        tostring(key, get_mer_len(), out);
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
          if((typeof record_len)io->gcount() >= record_len)
            end_buffer = ptr + (io->gcount() - record_len);
        }
      }
    };
  }
}
#endif /* __COMPACTED_HASH__ */
