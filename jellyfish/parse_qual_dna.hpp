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

#ifndef __JELLYFISH_PARSE_QUAL_DNA_HPP__
#define __JELLYFISH_PARSE_QUAL_DNA_HPP__

#include <iostream>
#include <vector>
#include <jellyfish/concurrent_queues.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/file_parser.hpp>

namespace jellyfish {
  class parse_qual_dna {
    struct seq {
      char *start;
      char *end;
    };
    typedef concurrent_queue<struct seq> seq_queue;
    typedef std::vector<const char *> fary_t;

    seq_queue               rq, wq;
    uint_t                  mer_len;
    uint64_t volatile       reader;
    size_t                  buffer_size;
    fary_t                  files;
    fary_t::const_iterator  current_file;
    bool                    have_seam;
    const char              quality_start;
    const char              min_q;
    char                   *buffer_data;
    struct seq             *buffers;
    char                   *seam;
    bool                    canonical;
    std::ifstream           fd;
    jellyfish::file_parser *fparser;

  public:
    /* Action to take for a given letter in fasta file:
     * A, C, G, T: map to 0, 1, 2, 3. Append to kmer
     * Other nucleic acid code: map to -1. reset kmer
     * '\n': map to -2. ignore
     * Other ASCII: map to -3. Skip to next line
     */
    static const uint_t codes[256];
    static const uint_t CODE_RESET = -1;

    parse_qual_dna(int nb_files, char *argv[], uint_t _mer_len, 
                   unsigned int nb_buffers, size_t _buffer_size,
                   const char _qs, const char _min_q); 

    ~parse_qual_dna() {
      delete [] buffers;
    }

    void set_canonical(bool v = true) { canonical = v; }

    class thread {
      parse_qual_dna         *parser;
      struct seq             *sequence;
      const uint_t            mer_len, lshift;
      uint64_t                kmer, rkmer;
      const uint64_t          masq;
      uint_t                  cmlen;
      seq_queue              *rq;
      seq_queue              *wq;
      const bool              canonical;
      const char              quality_start;
      const char              min_q;

    public:
      thread(parse_qual_dna *_parser, const char _qs, const char _min_q) :
        parser(_parser), sequence(0),
        mer_len(_parser->mer_len), lshift(2 * (mer_len - 1)),
        kmer(0), rkmer(0), masq((1UL << (2 * mer_len)) - 1),
        cmlen(0), rq(&parser->rq), wq(&parser->wq),
        canonical(parser->canonical),
        quality_start(_qs), min_q(_min_q) { }

      template<typename T>
      void parse(T &counter) {
        cmlen = kmer = rkmer = 0;
        while(true) {
          while(!sequence) {
            if(!next_sequence())
              return;
          }
          const char         *start = sequence->start;
          const char * const  end   = sequence->end;
          while(start < end) {
            // std::cerr << *start << " " << *(start + 1) << " "
            //           << (void*)start << " " << (void*)end << std::endl;
                  uint_t c = codes[(uint_t)*start++];
            const char   q = *start++ - quality_start;
            if(q < min_q)
              c = CODE_RESET;

            switch(c) {
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
    thread new_thread() { return thread(this, quality_start, min_q); }

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
