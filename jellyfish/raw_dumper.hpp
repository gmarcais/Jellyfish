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

#ifndef __RAW_DUMPER_HPP__
#define __RAW_DUMPER_HPP__

#include <jellyfish/dumper.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/token_ring.hpp>
#include <jellyfish/locks_pthread.hpp>
#include <jellyfish/square_binary_matrix.hpp>

namespace jellyfish {
  namespace raw_hash {
    static const char *file_type  __attribute__((used)) = "JFRHSHDN";
    struct header {
      char     type[8];
      uint64_t key_len;
      uint64_t val_len;
      uint64_t size;
      uint64_t max_reprobe;
    };
    define_error_class(ErrorReading);
    template<typename storage_t>
    class dumper : public dumper_t, public thread_exec {
      typedef token_ring<locks::pthread::cond> token_ring_t;
      struct thread_info_t {
        token_ring_t::token *token;
      };
      const uint_t         threads;
      const std::string    file_prefix;
      storage_t     *const ary;
      int                  file_index;
      token_ring_t         tr;
    
      struct thread_info_t *thread_info;
      size_t                nb_records, nb_blocks;
      std::ofstream        *out;

    public:
      dumper(uint_t _threads, const char *_file_prefix, size_t chunk_size, storage_t *_ary) :
        threads(_threads), file_prefix(_file_prefix),
        ary(_ary), file_index(0), tr()
      {
        nb_records = ary->floor_block(chunk_size / ary->get_block_len(), nb_blocks);
        while(nb_records < ary->get_max_reprobe_offset()) {
          nb_records = ary->floor_block(2 * nb_records, nb_blocks);
        }
        thread_info = new struct thread_info_t[threads];
        for(uint_t i = 0; i < threads; i++) {
          thread_info[i].token = tr.new_token();
        }
      }
    
      ~dumper() {
        if(thread_info) {
          delete[] thread_info;
        }
      }
    
      virtual void start(int i) { dump_to_file(i); }
      void dump_to_file(int i);
      void write_header();

      virtual void _dump();
    };

    template<typename _storage_t>
    class query {
    public:
      typedef _storage_t storage_t;
      typedef typename storage_t::iterator iterator;

    private:
      mapped_file         _file;
      storage_t          *_ary;
      bool                _canonical;
      bool                _cary_bit;
      SquareBinaryMatrix  hash_matrix;
      SquareBinaryMatrix  hash_inverse_matrix;

    public:
      explicit query(mapped_file &map) : 
        _file(map), _ary(0), _canonical(false), _cary_bit(false) { 
        _ary = init(_file, hash_matrix, hash_inverse_matrix); 
      }
      explicit query(std::string filename) : 
        _file(filename.c_str()), _ary(0), _canonical(false), _cary_bit(false)
      { 
        _ary = init(_file, hash_matrix, hash_inverse_matrix); 
      }
      explicit query(const char* filename) : 
        _file(filename), _ary(0), _canonical(false), _cary_bit(false)
      { 
        _ary = init(_file, hash_matrix, hash_inverse_matrix); 
      }

      ~query() {
        if(_ary)
          delete _ary;
      }

      size_t get_size() const { return _ary->get_size(); }
      size_t get_key_len() const { return _ary->get_key_len(); }
      uint_t get_mer_len() const { return _ary->get_key_len() / 2; }
      uint_t get_val_len() const { return _ary->get_val_len(); }
      uint_t get_max_reprobe() const { return _ary->get_max_reprobe(); }
      size_t get_max_reprobe_offset() const { return _ary->get_max_reprobe_offset(); }
      bool   get_canonical() const { return _canonical; }
      void   set_canonical(bool v) { _canonical = v; }
      bool   get_cary_bit() const { return _cary_bit; }
      void   set_cary_bit(bool v) { _cary_bit = v; }
      SquareBinaryMatrix get_hash_matrix() { return hash_matrix; }
      SquareBinaryMatrix get_hash_inverse_matrix() { return hash_inverse_matrix; }
      storage_t *get_ary() const { return _ary; }

      iterator get_iterator() const { return iterator_all(); }
      iterator iterator_all() const { return _ary->iterator_all(); }
      iterator iterator_slice(size_t slice_number, size_t number_of_slice) const {
        return _ary->iterator_slice(slice_number, number_of_slice);
      }

      typename storage_t::val_t operator[](const char *key_s) const {
        typename storage_t::key_t key = parse_dna::mer_string_to_binary(key_s, get_mer_len());
        return (*this)[key];
      }
      typename storage_t::val_t operator[](const typename storage_t::key_t &key) const { 
        typename storage_t::val_t res = 0;
        bool success;
        if(_canonical) {
          typename storage_t::key_t key2 = parse_dna::reverse_complement(key, get_mer_len());
          success = _ary->get_val(key2 < key ? key2 : key, res, true, _cary_bit);
        } else
          success = _ary->get_val(key, res, true, _cary_bit);
        return success ? res : 0;
      }

      bool has_key(const char *key_s) const {
        return has_key(parse_dna::mer_string_to_binary(key_s, get_mer_len()));
      }
      bool has_key(const typename storage_t::key_t &key) const {
        typename storage_t::val_t res = 0;
        if(_canonical) {
          typename storage_t::key_t key2 = parse_dna::reverse_complement(key, get_mer_len());
          return _ary->get_val(key2 < key ? key2 : key, res, false);
        } else {
          return _ary->get_val(key, res, false);
        }
      }

      
      static storage_t* init(mapped_file& _file, 
                             SquareBinaryMatrix& hash_matrix,
                             SquareBinaryMatrix& hash_inverse_matrix) {
        if(_file.length() < sizeof(struct header))
          eraise(ErrorReading) << "'" << _file.path() << "': "
                               << "File truncated";
        char *map = _file.base();
        struct header *header = (struct header *)map;
        map += sizeof(struct header);
        if(strncmp(header->type, file_type, sizeof(header->type)))
           eraise(ErrorReading) << "'" << _file.path() << "': "
                                << "Invalid file format '" 
                                << err::substr(header->type, sizeof(header->type))
                                << "'. Expected '" << file_type << "'.";
        if(header->size != (1UL << floorLog2(header->size)))
          eraise(ErrorReading) << "'" << _file.path() << "': "
                               << "Size '" << header->size << "' is not a power of 2";
        if(header->key_len > 64 || header->key_len == 0)
          eraise(ErrorReading) << "'" << _file.path() << "': "
                               << "Invalid key length '" << header->key_len << "'";
        // TODO: Should that be in the file instead?
        // reprobes = jellyfish::quadratic_reprobes;
        map += hash_matrix.read(map);
        if((uint_t)hash_matrix.get_size() != header->key_len)
          eraise(ErrorReading) << "'" << _file.path() << "': "
                               << "Size of hash matrix '" << hash_matrix.get_size() 
                              << "' not equal to key length '" << header->key_len << "'";
        map += hash_inverse_matrix.read(map);
        if((uint_t)hash_inverse_matrix.get_size() != header->key_len)
          eraise(ErrorReading) << "'" << _file.path() << "': "
                               << "Size of inverse hash matrix '" << hash_inverse_matrix.get_size()
                               << "' not equal to key length '" << header->key_len << "'";
        if((size_t)map & 0x7)
          map += 0x8 - ((size_t)map & 0x7); // Make sure aligned for 64bits word. TODO: use alignof?
        return new storage_t(map, header->size, header->key_len, header->val_len,
                             header->max_reprobe, jellyfish::quadratic_reprobes,
                             hash_matrix, hash_inverse_matrix);
      }
    };

    template<typename storage_t>
    void dumper<storage_t>::_dump() {
      std::ofstream _out;
      open_next_file(file_prefix.c_str(), &file_index, _out);
      out = &_out;
      tr.reset();
      write_header();
      exec_join(threads);
      _out.close();
    }

    template<typename storage_t>
    void dumper<storage_t>::dump_to_file(int id) {
      size_t i;
      struct thread_info_t *my_info = &thread_info[id];
    
      for(i = id; i * nb_records < ary->get_size(); i += threads) {
        my_info->token->wait();
        ary->write_blocks(out, i * nb_blocks, nb_blocks);
        my_info->token->pass();
        ary->zero_blocks(i * nb_blocks, nb_blocks);
      }
    }

    template<typename storage_t>
    void dumper<storage_t>::write_header() {
      struct header header;
      memcpy(&header.type, file_type, sizeof(header.type));
      header.key_len = ary->get_key_len();
      header.val_len = ary->get_val_len();
      header.size = ary->get_size();
      header.max_reprobe = ary->get_max_reprobe();
      out->write((char *)&header, sizeof(header));
      ary->write_ary_header(out);
    }

  }
}

#endif
