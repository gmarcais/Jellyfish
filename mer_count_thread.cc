#include "mer_count_thread.hpp"
#include "mer_counting.hpp"
#include <stdint.h>
#include <stdio.h>

void thread_worker::start()
{
  seq *sequence = NULL;
  struct timespec time_sleep = { 0, 10000000 };
  
  while(1) {
    if(sequence) {
      wq->enqueue(sequence);
      sequence = NULL;
    }

    if(rq->is_low() && !rq->is_closed()) {
      if(__sync_bool_compare_and_swap(&io->thread_id, 0, id)) {
        read_sequence();
        io->thread_id = 0;
      }
    }
    while(!(sequence = rq->dequeue())) {
      if(rq->is_closed()) {
        return ;
      } else if(__sync_bool_compare_and_swap(&io->thread_id, 0, id)) {
	read_sequence();
	io->thread_id = 0;
      } else {
        __sync_add_and_fetch(&stats->empty_rq, 1);
        nanosleep(&time_sleep, NULL);
        continue;
      }
    }
    if(sequence->end - sequence->buffer >= klen)
      count_kmers(sequence);
  }
}

void thread_worker::count_kmers(seq *sequence) {
  uint64_t kmer, rkmer, letter;
  unsigned int i, space = 0;
  char *buf_ptr = sequence->buffer;

  if(sequence->ns) {
    while(buf_ptr < sequence->end) {
      if(*buf_ptr++ == '\n')
        break;
    }
  }

  while(buf_ptr < sequence->map_end) {
    kmer = rkmer = 0;
    i = klen - 1;
    for( ; buf_ptr < sequence->map_end; buf_ptr++) {
      switch(*buf_ptr) {
      case 'a': case 'A': letter = 0x0; break;
      case 'c': case 'C': letter = 0x1; break;
      case 'g': case 'G': letter = 0x2; break;
      case 't': case 'T': letter = 0x3; break;
      case '\n': letter = 0x4; break;
      case '>': letter = 0x5; break;
      default:
        i = klen - 1;
        kmer = rkmer = 0;
        letter = 0x4;
      }
      
      /* Seam between 2 buffers. Allow to go beyond sequence->end by
       * no more than kmer-1 (plus adjustment for new lines).
       */
      if(buf_ptr >= sequence->end) {
        if(letter == 0x5)
          return;
        if(letter == 0x4)
          space++;
        if(!(buf_ptr < sequence->end + klen + space - 1))
          return;
      }

      if(letter == 0x5)
        break;
      if(letter == 0x4)
        continue;

      kmer = ((kmer << 2) | letter) & masq;
      //    rkmer = (rkmer >> 2) | ((0x3 - letter) << rshift);
      if(i > 0) {
        i--;
        continue;
      }
      counter->inc(kmer);
      //      counters->inc(rkmer);
    }

    while(buf_ptr < sequence->end) {
      if(*buf_ptr++ == '\n')
        break;
    }
  }
}

void thread_worker::read_sequence() {
  char *str;
  seq *sequence;
  bool input_done = false;
  //  __sync_add_and_fetch(&stats->new_reader, 1);
  
  while(1) {
    sequence = wq->dequeue();
    if(!sequence)
      break;

    sequence->buffer  = io->current;
    sequence->end     = io->current + io->buffer_size;
    sequence->map_end = io->map_end;
    sequence->nl      = io->nl;
    sequence->ns      = io->ns;
    if(sequence->end >= io->map_end) {
      sequence->end = io->map_end;
      input_done = true;
    } 
    io->current = sequence->end;
    rq->enqueue(sequence);

    if(input_done) {
      if(++io->current_file == io->mapped_files.end()) {
	rq->close();
	break;
      }
      madvise(io->current_file->base, io->current_file->length, MADV_WILLNEED);
      io->map_base = io->current_file->base;
      io->current  = io->current_file->base;
      io->map_end  = io->current_file->end;
      io->nl	   = true;
      io->ns	   = false;
      input_done   = false;
    } else {
      str = io->current - 1;
      if(*str == '\n') {
	io->nl = true;
	io->ns = false;
      } else {
	io->nl = false;
	while(str-- >= sequence->buffer) {
	  if(*str == '>') {
	    io->ns = true;
	    break;
	  } else if(*str == '\n') {
	    io->ns = false;
	    break;
	  }
	}
      }
    }
  }
}
