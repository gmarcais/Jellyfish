#ifndef __MER_COUNT_THREAD__
#define __MER_COUNT_THREAD__

#include "mer_counting.hpp"

struct thread_stats {
  uint32_t      empty_rq;
  uint32_t      new_reader;
};

class thread_worker {
  unsigned int                id;
  unsigned int                klen;
  struct qc                  *qc;
  seq_queue                  *rq;
  seq_queue                  *wq;
  mer_counters::thread_ptr_t  counter;
  struct io                  *io;
  unsigned int                rshift;
  struct thread_stats        *stats;
  uint64_t                    masq;
  bool                        both_strands;

  void count_kmers(seq *sequence);
  void read_sequence();

public:
  thread_worker(unsigned int _id, unsigned int _klen,
                struct qc *_qc, struct io *_io, struct thread_stats *_stats) :
    id(_id), klen(_klen), qc(_qc), rq(qc->rq), wq(qc->wq), 
    counter(qc->counters->new_hash_counter()), io(_io), rshift(2 * (_klen - 1)),
    stats(_stats), masq((((uint64_t)1) << (2 * klen)) - 1)
  { }
  
  ~thread_worker() { 
    qc->counters->relase_hash_counter(counter);
  }
  
  void start();
  bool get_both_strands() const { return both_strands; }
  void set_both_strands(bool bs) { both_strands = bs; }
};

#endif /* __MER_COUNT_THREAD__ */
