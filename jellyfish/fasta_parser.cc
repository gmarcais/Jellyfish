#include "fasta_parser.hpp"

namespace jellyfish {
  bool fasta_parser::thread::next(uint64_t &_kmer, uint64_t &_rkmer) {
    while(true) {
      while(!sequence) {
        if(!next_sequence())
          return false;
      }

      while(sequence->buffer < sequence->end) {
        uint_t c = codes[(uint_t)*sequence->buffer++];
        switch(c) {
        case CODE_IGNORE: break;
        case CODE_COMMENT:
          while(sequence->buffer < sequence->end)
            if(codes[(uint_t)*sequence->buffer++] == CODE_IGNORE)
              break;
          // Fall through CODE_RESET
        case CODE_RESET:
          cmlen = 0;
          kmer  = rkmer = 0;
          break;

        default:
          kmer = ((kmer << 2) & masq) | c;
          rkmer = (rkmer >> 2) | (c << lshift);
          if(++cmlen >= mer_len) {
            cmlen  = mer_len;
            _kmer  = kmer;
            _rkmer = rkmer;
            return true;
          }
        }
      }

      // Buffer exhausted. Get a new one
      cmlen = 0;
      kmer = rkmer = 0;
      parser->wq.enqueue(sequence);
      sequence = 0;
    }
  }

  bool fasta_parser::thread::next_sequence() {
    if(parser->rq.is_low() && !parser->rq.is_closed()) {
      parser->read_sequence();
    }
    while(!(sequence = parser->rq.dequeue())) {
      if(parser->rq.is_closed())
        return false;
      parser->read_sequence();
    }
    return true;
  }

  void fasta_parser::_read_sequence() {
    seq *new_seq;
  
    while((new_seq = wq.dequeue()) != 0) {
      new_seq->buffer  = current;
      current         += buffer_size;
      if(current >= map_end)
        current = map_end;
      new_seq->end = current;

      /* Check where we are at the begininning of the next
         buffer. Look back into current buffer until: 1) find an
         ignored code (i.e. a new line), then we are in sequence
         proper. Extend previous buffer upto mer_len - 1 into new
         buffer to go over the seam; or 2) find a comment code
         (e.g. >), then we are in a header line. Shorten previous
         buffer. Find end of header line (upto ignore code) for the
         beginning of next buffer. No seam.
      */
      bool do_extend = true;
      for(char *str = current; str >= new_seq->buffer; str--) {
        uint_t c = codes[(uint_t)*str];
        if(c == CODE_COMMENT) {
          new_seq->end = str;
          while(current < map_end)
            if(codes[(uint_t)*current++] == CODE_IGNORE)
              break;
          do_extend = false;
          break;
        } else if(c == CODE_IGNORE) {
          break;
        }
      }
      if(do_extend) {
        uint_t seam = mer_len - 1;
        for(uint_t i = 0; i < seam && new_seq->end < map_end; i++) {
          uint_t cc = codes[(uint_t)*new_seq->end++];
          if(cc == CODE_IGNORE) {
            seam++;
          } else if(cc == CODE_RESET || cc == CODE_COMMENT) {
            break;
          }
        }
      }
      rq.enqueue(new_seq);

      if(current >= map_end) {
        if(++current_file == mapped_files.end()) {
          rq.close();
          break;
        }
        current_file->will_need();
        map_base   = current_file->base();
        current    = current_file->base();
        map_end    = current_file->end();
      }
    }
  }

  const uint_t fasta_parser::codes[256] = {
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -2, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -1, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3,  0, -1,  1, -1, -3, -3,  2, -1, -3, -3, -1, -3, -1, -1, -3, 
    -3, -3, -1, -1,  3, -3, -1, -1, -1, -1, -3, -3, -3, -3, -3, -3, 
    -3,  0, -1,  1, -1, -3, -3,  2, -1, -3, -3, -1, -3, -1, -1, -3, 
    -3, -3, -1, -1,  3, -3, -1, -1, -1, -1, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, 
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3
  };
}
