#include "fasta_parser.hpp"

namespace jellyfish {
  fasta_parser::fasta_parser(int nb_files, char *argv[], uint_t _mer_len, unsigned int nb_buffers, size_t _buffer_size) :
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

  bool fasta_parser::thread::next_sequence() {
    if(rq->is_low() && !rq->is_closed()) {
      parser->read_sequence();
    }
    while(!(sequence = rq->dequeue())) {
      if(rq->is_closed())
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
