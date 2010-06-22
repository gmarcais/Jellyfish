#ifndef __MER_COUNT_THREAD__
#define __MER_COUNT_THREAD__

#include "mer_counting.hpp"

struct thread_stats {
  uint32_t      empty_rq;
  uint32_t      new_reader;
};

class thread_worker {
  unsigned int		 id;
  unsigned int		 klen;
  seq_queue             *rq;
  seq_queue             *wq;
  mer_counters::thread  *counter;
  struct io             *io;
  unsigned int		 rshift;
  struct thread_stats   *stats;
  uint64_t		 masq;

  void count_kmers(seq *sequence);
  void read_sequence();

public:
  thread_worker(unsigned int _id, unsigned int _klen,
                struct qc *qc, struct io *_io, struct thread_stats *_stats) :
    id(_id), klen(_klen), rq(qc->rq), wq(qc->wq), 
    counter(qc->counters->new_hash_counter()), io(_io), rshift(2 * (_klen - 1)),
    stats(_stats), masq((((uint64_t)1) << (2 * klen)) - 1)
  { }
  
  ~thread_worker() {
    delete counter;
  }
  
  void start();
};

#endif /* __MER_COUNT_THREAD__ */
