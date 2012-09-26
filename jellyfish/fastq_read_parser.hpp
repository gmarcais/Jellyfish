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

#ifndef __JELLYFISH_FASTQ_READ_PARSER_HPP__
#define __JELLYFISH_FASTQ_READ_PARSER_HPP__

#include <jellyfish/mapped_file.hpp>
#include <jellyfish/double_fifo_input.hpp>
#include <jellyfish/misc.hpp>

namespace jellyfish {
  struct read_t {
    const char *seq_s, *seq_e, *header, *qual_s, *qual_e;
    size_t      hlen;
  };
  struct seq_qual_reads_t {
    static const int max_nb_reads = 100;
    int              nb_reads;
    read_t           reads[max_nb_reads];
  };

  class fastq_read_parser : public double_fifo_input {
  private:
    uint_t                                mer_len;
    lazy_mapped_files_t                   mapped_files;
    lazy_mapped_files_t::iterator         current_file;
    char                                 *map_base, *current, *map_end;
    uint64_t volatile                     reader;
    seq_queue                             rq, wq;
    struct seq                           *buffers;
    bool                                  canonical;

  public:
    static const uint_t codes[256];
    static const uint_t CODE_RESET = -1;
    static const float proba_codes[41];
    static const float one_minus_proba_codes[41];
    typedef struct read read_t;

    fastq_read_parser(int nb_files, char *argv[], unsigned int nb_buffers);

    ~fastq_read_parser() {
      delete [] buffers;
    }

    void set_canonical(bool v = true) { canonical = v; }

    class thread {
      fastq_read_parser *parser;
      struct seq        *sequence;
      seq_queue         *rq, *wq;
      int                current_read;

    public:
      explicit thread(fastq_read_parser *_parser) :
        parser(_parser),
        sequence(0), rq(&parser->rq), wq(&parser->wq), current_read(max_nb_reads) {}
      read_t * next_read() {
        while(true) {
          if(sequence) {
            if(current_read < sequence->nb_reads)
              return &sequence->reads[current_read++];
            sequence->file->dec();
            wq->enqueue(sequence);
            sequence = 0;
          }
          current_read = 0;
          while(!sequence)
            if(!next_sequence())
              return 0;
        }
      }

    private:
      bool next_sequence();
    };
    friend class thread;
    thread new_thread() { return thread(this); }
  };
}

#endif
