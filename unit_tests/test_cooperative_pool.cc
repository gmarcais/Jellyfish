#include <gtest/gtest.h>
#include <jflib/cooperative_pool.hpp>
#include <jellyfish/thread_exec.hpp>
#include <math.h>

namespace {
class sequence : public jellyfish::jflib::cooperative_pool<sequence, double> {
  typedef cooperative_pool<sequence, double> super;

  double              current_;
public:
  static const double max = 1000000.0;
  typedef double element_type;

  explicit sequence(uint32_t size) : super(size), current_(0.0) { }

  bool produce(double& e) {
    if(current_ < max) {
      e = current_;
      current_ += 1.0;
      return false;
    }
    return true;
  }
};

class computation : public thread_exec {
  sequence& producer_;

public:
  jellyfish::jflib::atomic_field<double> sum;
  computation(sequence& producer) : producer_(producer), sum(0.0) { }
  virtual void start(int tid) {
    double res = 0.0;

    while(true) {
      sequence::job j(producer_);
      if(j.is_empty())
        break;
      res += sin(log(*j + 1));
    }

    sum += res;
  }
};

TEST(CooperativePool, Computation) {
  uint32_t nb_threads = 10;
  sequence producer(nb_threads * 3);

  computation comp(producer);
  comp.exec_join(nb_threads);

  double sum = 0.0;
  for(double i = 0; i < sequence::max; i += 1.0)
    sum += sin(log(i + 1));
  EXPECT_NEAR(sum, (double)comp.sum, 0.01);
}
}
