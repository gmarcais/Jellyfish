#ifndef __FASTA_PARSER_HPP__
#define __FASTA_PARSER_HPP__

#include <iostream>
#include "concurrent_queues.hpp"
#include "atomic_gcc.hpp"
#include "mapped_file.hpp"

namespace jellyfish {
  class fasta_parser {
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
