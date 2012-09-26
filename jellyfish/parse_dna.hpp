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

#ifndef __JELLYFISH_PARSE_DNA_HPP__
#define __JELLYFISH_PARSE_DNA_HPP__

#include <iostream>
#include <vector>
#include <jellyfish/double_fifo_input.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/sequence_parser.hpp>
#include <jellyfish/allocators_mmap.hpp>
#include <jellyfish/dna_codes.hpp>

namespace jellyfish {
  class parse_dna : public double_fifo_input<sequence_parser::sequence_t> {
    typedef std::vector<const char *> fary_t;

    uint_t                      mer_len;
    size_t                      buffer_size;
    const fary_t                files;
    fary_t::const_iterator      current_file;
    bool                        have_seam;
    char                       *seam;
    allocators::mmap            buffer_data;
    bool                        canonical;
    sequence_parser            *fparser;

  public:
    /* Action to take for a given letter in fasta file:
     * A, C, G, T: map to 0, 1, 2, 3. Append to kmer
     * Other nucleic acid code: map to -1. reset kmer
     * '\n': map to -2. ignore
     * Other ASCII: map to -3. Report error.
     */
    static uint64_t mer_string_to_binary(const char *in, uint_t klen) {
      uint64_t res = 0;
      for(uint_t i = 0; i < klen; i++) {
        const uint_t c = dna_codes[(uint_t)*in++];
        if(c & CODE_NOT_DNA)
          return 0;
        res = (res << 2) | c;
      }
      return res;
    }
    static void mer_binary_to_string(uint64_t mer, uint_t klen, char *out) {
      static const char table[4] = { 'A', 'C', 'G', 'T' };
      
      for(unsigned int i = 0 ; i < klen; i++) {
        out[klen-1-i] = table[mer & (uint64_t)0x3];
        mer >>= 2;
      }
      out[klen] = '\0';
    }

    static uint64_t reverse_complement(uint64_t v, uint_t length) {
      v = ((v >> 2)  & 0x3333333333333333UL) | ((v & 0x3333333333333333UL) << 2);
      v = ((v >> 4)  & 0x0F0F0F0F0F0F0F0FUL) | ((v & 0x0F0F0F0F0F0F0F0FUL) << 4);
      v = ((v >> 8)  & 0x00FF00FF00FF00FFUL) | ((v & 0x00FF00FF00FF00FFUL) << 8);
      v = ((v >> 16) & 0x0000FFFF0000FFFFUL) | ((v & 0x0000FFFF0000FFFFUL) << 16);
      v = ( v >> 32                        ) | ( v                         << 32);
      return (((uint64_t)-1) - v) >> (bsizeof(v) - (length << 1));
    }

    template<typename T>
    parse_dna(T _files_start, T _files_end, uint_t _mer_len, 
              unsigned int nb_buffers, size_t _buffer_size); 

    ~parse_dna() {
      delete [] seam;
    }

    void set_canonical(bool v = true) { canonical = v; }
    virtual void fill();

    class thread {
      parse_dna      *parser;
      bucket_t       *sequence;
      const uint_t    mer_len, lshift;
      uint64_t        kmer, rkmer;
      const uint64_t  masq;
      uint_t          cmlen;
      const bool      canonical;
      uint64_t        distinct, total;
      typedef         void (*error_reporter)(std::string& err);
      error_reporter  error_report;

    public:
      explicit thread(parse_dna *_parser) :
        parser(_parser), sequence(0),
        mer_len(_parser->mer_len), lshift(2 * (mer_len - 1)),
        kmer(0), rkmer(0), masq((1UL << (2 * mer_len)) - 1),
        cmlen(0), canonical(parser->canonical),
        distinct(0), total(0), error_report(0) {}

      uint64_t get_uniq() const { return 0; }
      uint64_t get_distinct() const { return distinct; }
      uint64_t get_total() const { return total; }

      template<typename T>
      void parse(T &counter) {
        cmlen = kmer = rkmer = 0;
        while((sequence = parser->next())) {
          const char         *start = sequence->start;
          const char * const  end   = sequence->end;
          while(start < end) {
            const uint_t c = dna_codes[(uint_t)*start++];
            switch(c) {
            case CODE_IGNORE: break;
            case CODE_COMMENT:
              report_bad_input(*(start-1));
              // Fall through
            case CODE_RESET:
              cmlen = kmer = rkmer = 0;
              break;

            default:
              kmer = ((kmer << 2) & masq) | c;
              rkmer = (rkmer >> 2) | ((0x3 - c) << lshift);
              if(++cmlen >= mer_len) {
                cmlen  = mer_len;
                typename T::val_type oval;
                if(canonical)
                  counter->add(kmer < rkmer ? kmer : rkmer, 1, &oval);
                else
                  counter->add(kmer, 1, &oval);
                distinct += oval == (typename T::val_type)0;
                ++total;
              }
            }
          }

          // Buffer exhausted. Get a new one
          cmlen = kmer = rkmer = 0;
          parser->release(sequence);
        }
      }

      void set_error_reporter(error_reporter e) {
        error_report = e;
      }

    private:
      void report_bad_input(char c) {
        if(!error_report)
          return;
        std::string err("Bad character in sequence: ");
        err += c;
        error_report(err);
      }
    };
    friend class thread;
    thread new_thread() { return thread(this); }
  };
}

template<typename T>
jellyfish::parse_dna::parse_dna(T _files_start, T _files_end,
                     uint_t _mer_len,
                     unsigned int nb_buffers, size_t _buffer_size) :
  double_fifo_input<sequence_parser::sequence_t>(nb_buffers), mer_len(_mer_len), 
  buffer_size(allocators::mmap::round_to_page(_buffer_size)),
  files(_files_start, _files_end), current_file(files.begin()),
  have_seam(false), buffer_data(buffer_size * nb_buffers), canonical(false)
{
  seam        = new char[mer_len];
  memset(seam, 'A', mer_len);

  unsigned long i = 0;
  for(bucket_iterator it = bucket_begin();
      it != bucket_end(); ++it, ++i) {
    it->end = it->start = (char *)buffer_data.get_ptr() + i * buffer_size;
  }
  assert(i == nb_buffers);

  fparser = sequence_parser::new_parser(*current_file);
}


#endif
