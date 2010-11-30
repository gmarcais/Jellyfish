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

#ifndef __JELLYFISH_FASTQ_PARSER_HPP__
#define __JELLYFISH_FASTQ_PARSER_HPP__

#include <jellyfish/mapped_file.hpp>
#include <jellyfish/circular_buffer.hpp>
#include <jellyfish/concurrent_queues.hpp>

namespace jellyfish {
  class fastq_parser {
    static const int max_nb_reads = 100;
    struct read {
      char *seq_s, *seq_e, *qual_s, *qual_e;
    };
    struct seq {
      int         nb_reads;
      struct read reads[max_nb_reads];
    };
    typedef concurrent_queue<struct seq>  seq_queue;
    uint_t                                mer_len;
    mapped_files_t                        mapped_files;
    mapped_files_t::const_iterator        current_file;
    char                                 *map_base, *current, *map_end;
    uint64_t volatile                     reader;
    seq_queue                             rq, wq;
    struct seq                           *buffers;
    bool                                  canonical;

  public:
    static const uint_t codes[256];
    static const uint_t CODE_RESET = -1;
    static const float proba_codes[256];
    static const float one_minus_proba_codes[256];

    fastq_parser(int nb_files, char *argv[], uint_t _mer_len, 
                 unsigned int nb_buffers);

    ~fastq_parser() {
      delete [] buffers;
    }

    void set_canonical(bool v = true) { canonical = v; }

    class thread {
      fastq_parser   *parser;
      struct seq     *sequence;
      const uint_t    mer_len, lshift;
      uint64_t        kmer, rkmer;
      const uint64_t  masq;
      uint_t          cmlen;
      seq_queue      *rq, *wq;
      const bool      canonical;

    public:
      thread(fastq_parser *_parser) :
        parser(_parser), sequence(0),
        mer_len(parser->mer_len), lshift(2 * (mer_len - 1)),
        kmer(0), rkmer(0), masq((1UL << (2 * mer_len)) - 1),
        cmlen(0), rq(&parser->rq), wq(&parser->wq),
        canonical(parser->canonical) {}

      template<typename counter_t>
      void parse(counter_t &counter) {
        circular_buffer<float> quals(mer_len);

        while(true) {
          while(!sequence)
            if(!next_sequence())
              return;
          for(int i = 0; i < sequence->nb_reads; ++i) {
            const char *seq = sequence->reads[i].seq_s;
            const char * const seq_e = sequence->reads[i].seq_e;
            const char *qual = sequence->reads[i].qual_s;
            cmlen = 0;
            //            kmer = rkmer = 0UL;
            while(seq < seq_e) {
              const uint_t c = codes[(uint_t)*seq++];
              const float  q = one_minus_proba_codes[(uint_t)*qual++];
              if(c == CODE_RESET) {
                cmlen = 0;
                // kmer = rkmer = 0;
                continue;
              }

              kmer = ((kmer << 2) & masq) | c;
              rkmer = (rkmer >> 2) | ((0x3 - c) << lshift);
              quals.append(q);
              if(++cmlen >= mer_len) {
                cmlen = mer_len;
                if(canonical)
                  counter->add(kmer < rkmer ? kmer : rkmer, quals.prod());
                else
                  counter->add(kmer, quals.prod());
              }
            }
          }
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
