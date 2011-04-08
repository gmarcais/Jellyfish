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
#include <jellyfish/concurrent_queues.hpp>
#include <jellyfish/misc.hpp>

class fastq_read_parser {
public:
  static const int max_nb_reads = 100;
  struct read {
    const char *seq_s, *seq_e, *header, *qual_s, *qual_e;
    size_t      hlen;
  };

private:
  struct seq {
    int                 nb_reads;
    struct read         reads[max_nb_reads];
    lazy_mapped_file_t *file;
  };
  typedef concurrent_queue<struct seq>  seq_queue;
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
    thread(fastq_read_parser *_parser) :
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

#endif
