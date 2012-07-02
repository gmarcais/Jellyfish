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
#include <jellyfish/double_fifo_input.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/seq_qual_parser.hpp>
#include <jellyfish/allocators_mmap.hpp>
#include <jellyfish/dna_codes.hpp>

namespace jellyfish {
  class parse_qual_dna : public double_fifo_input<seq_qual_parser::sequence_t> {
    typedef std::vector<const char *> fary_t;

    uint_t                  mer_len;
    size_t                  buffer_size;
    const fary_t            files;
    fary_t::const_iterator  current_file;
    bool                    have_seam;
    allocators::mmap        buffer_data;
    char                   *seam;
    const char              quality_start;
    const char              min_q;
    bool                    canonical;
    seq_qual_parser        *fparser;

  public:
    /* Action to take for a given letter in fasta file:
     * A, C, G, T: map to 0, 1, 2, 3. Append to kmer
     * Other nucleic acid code: map to -1. reset kmer
     * '\n': map to -2. ignore
     * Other ASCII: map to -3. Skip to next line
     */
    parse_qual_dna(const fary_t &_files, uint_t _mer_len, 
                   unsigned int nb_buffers, size_t _buffer_size,
                   const char _qs, const char _min_q); 

    ~parse_qual_dna() { }

    void set_canonical(bool v = true) { canonical = v; }
    virtual void fill();

    class thread {
      parse_qual_dna *parser;
      bucket_t       *sequence;
      const uint_t    mer_len, lshift;
      uint64_t        kmer, rkmer;
      const uint64_t  masq;
      uint_t          cmlen;
      const bool      canonical;
      const char      q_thresh;
      uint64_t        distinct, total;
      typedef         void (*error_reporter)(std::string& err);
      error_reporter  error_report;

    public:
      thread(parse_qual_dna *_parser, const char _qs, const char _min_q) :
        parser(_parser), sequence(0),
        mer_len(_parser->mer_len), lshift(2 * (mer_len - 1)),
        kmer(0), rkmer(0), masq((1UL << (2 * mer_len)) - 1),
        cmlen(0), canonical(parser->canonical),
        q_thresh(_qs + _min_q),
        distinct(0), total(0), error_report(0) { }

      uint64_t get_distinct() const { return distinct; }
      uint64_t get_total() const { return total; }

      template<typename T>
      void parse(T &counter) {
        cmlen = kmer = rkmer = 0;
        while((sequence = parser->next())) {
          const char         *start = sequence->start;
          const char * const  end   = sequence->end;
          while(start < end) {
            uint_t     c = dna_codes[(uint_t)*start++];
            const char q = *start++;
            if(q < q_thresh)
              c = CODE_RESET;

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
                cmlen = mer_len;
                typename T::val_type oval;
                if(canonical)
                  counter->add(kmer < rkmer ? kmer : rkmer, 1, &oval);
                else
                  counter->add(kmer, 1, &oval);
                distinct += !oval;
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
        std::string error("Bad input: ");
        error += c;
        error_report(error);
      }
    };
    friend class thread;
    thread new_thread() { return thread(this, quality_start, min_q); }
  };
}

#endif
