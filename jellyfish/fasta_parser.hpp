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

#ifndef __JELLYFISH_FASTA_PARSER_HPP__
#define __JELLYFISH_FASTA_PARSER_HPP__

#include <iostream>
#include <jellyfish/concurrent_queues.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/mapped_file.hpp>
#include <jellyfish/misc.hpp>

namespace jellyfish {
  class fasta_parser {
    struct seq {
      char *buffer;
      char *end;
    };
    typedef concurrent_queue<struct seq> seq_queue;

    seq_queue                       rq, wq;
    uint_t                          mer_len;
    mapped_files_t                  mapped_files;
    uint64_t volatile               reader;
    char                           *current;
    char                           *map_base;
    char                           *map_end;
    size_t                          buffer_size;
    struct seq                     *buffers;
    mapped_files_t::const_iterator  current_file;
    bool                            canonical;

  public:
    /* Action to take for a given letter in fasta file:
     * A, C, G, T: map to 0, 1, 2, 3. Append to kmer
     * Other nucleic acid code: map to -1. reset kmer
     * '\n': map to -2. ignore
     * Other ASCII: map to -3. Skip to next line
     */
    static const uint_t codes[256];
    static const uint_t CODE_RESET = -1;
    static const uint_t CODE_IGNORE = -2;
    static const uint_t CODE_COMMENT = -3;
    static const uint_t CODE_NOT_DNA = ((uint_t)1) << (sizeof(uint_t) - 1);

    static uint64_t mer_string_to_binary(const char *in, uint_t klen) {
      uint64_t res = 0;
      for(uint_t i = 0; i < klen; i++) {
        const uint_t c = fasta_parser::codes[(uint_t)*in++];
        if(c & CODE_NOT_DNA)
          return 0;
        res = (res << 2) | c;
      }
      return res;
    }
    static void mer_binary_to_string(uint64_t mer, uint_t klen, char *out) {
      static const char table[4] = { 'A', 'C', 'G', 'T' };
      
      for(unsigned int i = 0 ; i < klen; i++) {
        out[klen-1-i] = table[mer & UINT64_C(0x3)];
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


    fasta_parser(int nb_files, char *argv[], uint_t _mer_len, unsigned int nb_buffers, size_t _buffer_size); 

    ~fasta_parser() {
      delete [] buffers;
    }

    void set_canonical(bool v = true) { canonical = v; }

    class thread {
      fasta_parser   *parser;
      struct seq     *sequence;
      const uint_t    mer_len, lshift;
      uint64_t        kmer, rkmer;
      const uint64_t  masq;
      uint_t          cmlen;
      bool            done;
      seq_queue      *rq;
      seq_queue      *wq;
      const bool      canonical;

    public:
      thread(fasta_parser *_parser) :
        parser(_parser), sequence(0),
        mer_len(_parser->mer_len), lshift(2 * (mer_len - 1)),
        kmer(0), rkmer(0), masq((1UL << (2 * mer_len)) - 1),
        cmlen(0), done(false), rq(&parser->rq), wq(&parser->wq),
        canonical(parser->canonical) { }

      template<typename T>
      void parse(T &counter) {
        cmlen = kmer = rkmer = 0;
        while(true) {
          while(!sequence) {
            if(!next_sequence())
              return;
          }
          char *buffer = sequence->buffer;
          const char *end = sequence->end;
          while(buffer < end) {
            const uint_t c = codes[(uint_t)*buffer++];
            switch(c) {
            case CODE_IGNORE: break;
            case CODE_COMMENT:
              while(buffer < end)
                if(codes[(uint_t)*buffer++] == CODE_IGNORE)
                  break;
              // Fall through CODE_RESET
            case CODE_RESET:
              cmlen = kmer = rkmer = 0;
              break;

            default:
              kmer = ((kmer << 2) & masq) | c;
              rkmer = (rkmer >> 2) | ((0x3 - c) << lshift);
              if(++cmlen >= mer_len) {
                cmlen  = mer_len;
                if(canonical)
                  counter->inc(kmer < rkmer ? kmer : rkmer);
                else
                  counter->inc(kmer);
              }
            }
          }

          // Buffer exhausted. Get a new one
          cmlen = kmer = rkmer = 0;
          wq->enqueue(sequence);
          sequence = 0;
        }
      }


    private:
      bool next_sequence();
    };
    friend class thread;
    thread new_thread() { return thread(this); }

  private:
    void _read_sequence();
    void read_sequence() {
      static struct timespec time_sleep = { 0, 10000000 };
      if(!__sync_bool_compare_and_swap(&reader, 0, 1)) {
        nanosleep(&time_sleep, NULL);
        return;
      }
      _read_sequence();
      reader = 0;
    }
  };
}

#endif
