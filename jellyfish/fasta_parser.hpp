#ifndef __FASTA_PARSER_HPP__
#define __FASTA_PARSER_HPP__

#include <iostream>
#include "concurrent_queues.hpp"
#include "atomic_gcc.hpp"
#include "mapped_file.hpp"

namespace jellyfish {
  class fasta_parser {
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

    struct seq {
      char *buffer;
      char *end;
    };
    typedef concurrent_queue<struct seq> seq_queue;

    seq_queue                       rq, wq;
    uint_t                          mer_len;
    uint64_t volatile               reader;
    char                           *current;
    char                           *map_base;
    char                           *map_end;
    size_t                          buffer_size;
    mapped_files_t                  mapped_files;
    struct seq                     *buffers;
    mapped_files_t::const_iterator  current_file;

  public:
    fasta_parser(int nb_files, char *argv[], uint_t _mer_len, unsigned int nb_buffers, 
                 size_t _buffer_size) : 
      rq(nb_buffers), wq(nb_buffers), mer_len(_mer_len)
    {
      unsigned int i;
      int j;
      buffers = new seq[nb_buffers];
      memset(buffers, '\0', sizeof(struct seq) * nb_buffers);

      for(i = 0; i < nb_buffers; i++)
        wq.enqueue(&buffers[i]);
      
      for(j = 0; j < nb_files; j++) {
        mapped_files.push_back(mapped_file(argv[j]));
        mapped_files.end()->sequential();
      }

      current_file = mapped_files.begin();
      current_file->will_need();
      map_base     = current_file->base();
      current      = current_file->base();
      map_end      = current_file->end();
      reader       = 0;
      buffer_size  = _buffer_size;
    }

    ~fasta_parser() {
      delete [] buffers;
    }

    class thread {
      fasta_parser *parser;
      struct seq   *sequence;
      char         *buf_ptr, *seam_end;
      uint_t        mer_len, lshift;
      uint64_t      kmer, rkmer, masq;
      uint_t        cmlen;
      bool          done;
    public:
      thread(fasta_parser *_parser) :
        parser(_parser), sequence(0), buf_ptr(0), seam_end(0),
        mer_len(_parser->mer_len), lshift(2 * (mer_len - 1)),
        kmer(0), rkmer(0), masq((1UL << (2 * mer_len)) - 1),
        cmlen(0), done(false) { }
      bool next(uint64_t &kmer, uint64_t &rkmer);

    private:
      bool next_sequence();
    };
    friend class thread;
    thread new_thread() { return thread(this); }

  private:
    void _read_sequence();
    void read_sequence() {
      if(!__sync_bool_compare_and_swap(&reader, 0, 1))
        return;
      _read_sequence();
      reader = 0;
    }
  };
}

#endif
