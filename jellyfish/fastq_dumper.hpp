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

#ifndef __FASTQ_DUMPER_HPP__
#define __FASTQ_DUMPER_HPP__

#include <jellyfish/err.hpp>
#include <jellyfish/dumper.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/mapped_file.hpp>

namespace jellyfish {
  namespace fastq_hash {
    define_error_class(ErrorReading);
    static const char *file_type = "JFFSTQDN";
    struct header {
      char     type[8];
      uint64_t key_len;
      uint64_t size;
      uint64_t max_reprobes;
      uint64_t values_pos;

      header(uint64_t _key_len, uint64_t _size, uint64_t _max_reprobes, 
             uint64_t _values_pos) : 
        key_len(_key_len), size(_size), max_reprobes(_max_reprobes), 
        values_pos(_values_pos)
      {
        memcpy(&type, file_type, sizeof(type));
      }

      explicit header(const char *ptr) {
        if(memcmp(ptr, file_type, sizeof(type)))
          eraise(ErrorReading) << "Bad file type '" << err::substr(ptr, sizeof(type))
                               << "', expected '" << err::substr(file_type, sizeof(type)) << "'";
        memcpy((void *)this, ptr, sizeof(struct header));
      }
    };

    template<typename storage_t>
    class raw_dumper : public dumper_t {
      const uint_t      threads;
      const std::string file_prefix;
      storage_t *const  ary;
      int               file_index;

    public:
      raw_dumper(uint_t _threads, const char *_file_prefix, size_t chunk_size,
                 storage_t *_ary) :
        threads(_threads), file_prefix(_file_prefix),
        ary(_ary), file_index(0) {}

      virtual void _dump();

      static storage_t * read(const mapped_file &file);
      static storage_t * read(const std::string &file);
      static storage_t * read(const char *file);
    };

    template<typename storage_t>
    void raw_dumper<storage_t>::_dump() {
      std::ofstream _out;
      open_next_file(file_prefix.c_str(), &file_index, _out);

      // TODO: the zeroing out of the hash is not parallelized.

      // Skip header
      _out.seekp(sizeof(struct header));
      // Write matrices
      ary->write_matrices(&_out);
      // Write key set
      ary->write_keys_blocks(&_out, 0, ary->get_size());
      std::streampos pos = _out.tellp();
      ary->zero_keys(0, ary->get_size());
      // Write values array
      ary->write_values(&_out, 0, ary->get_size());
      ary->zero_values(0, ary->get_size());
      // Update header      
      _out.seekp(0);
      struct header header(ary->get_key_len(), ary->get_size(),
                           ary->get_max_reprobe(), pos);
      _out.write((char *)&header, sizeof(header));
      _out.close();
    }
    
    template<typename storage_t>
    storage_t * raw_dumper<storage_t>::read(const std::string &file) {
      mapped_file mf(file.c_str());
      return read(mf);
    }

    template<typename storage_t>
    storage_t * raw_dumper<storage_t>::read(const char *file) {
      mapped_file mf(file);
      return read(mf);
    }

    template<typename storage_t>
    storage_t * raw_dumper<storage_t>::read(const mapped_file &mf) {
      if(mf.length() < sizeof(struct header))
        eraise(ErrorReading) << "File '" << mf.path() 
                             << "' too short. Should be at least '" 
                            << sizeof(struct header) << "' bytes";

      struct header header(mf.base());
      size_t off = sizeof(header);
      SquareBinaryMatrix hash_matrix, hash_inv_matrix;
      off += hash_matrix.read(mf.base() + off);
      off += hash_inv_matrix.read(mf.base() + off);
      return new storage_t(mf.base() + off,
                           mf.base() + header.values_pos,
                           header.size, header.key_len, header.max_reprobes,
                           jellyfish::quadratic_reprobes, hash_matrix,
                           hash_inv_matrix);
    }
  }
}

#endif
