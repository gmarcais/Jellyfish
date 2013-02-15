#include <gtest/gtest.h>
#include <jellyfish/locks_pthread.hpp>

using testing::Types;

#define NB_THREADS 3
#define COUNT	1000000

template <class cond>
class CondTest : public ::testing::Test
{
  int				volatile count;
  int				count_at_signal;
  cond				count_cond;
  pthread_t			threads[NB_THREADS+1];
  pthread_attr_t		attr;

public:
  CondTest() : count(0), count_at_signal(0), count_cond() {
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&threads[0], &attr, master_fun, (void *)this);

    for(int i = 1; i < NB_THREADS+1; i++)
      pthread_create(&threads[i], &attr, thread_fun, (void *)this);
    
    for(int i = 0; i <= NB_THREADS; i++)
      pthread_join(threads[i], NULL);
  }

  static void *master_fun(void *t) {
    CondTest<cond> *self = (CondTest<cond> *)t;
    
    self->master_wait();
    return NULL;
  }
  void master_wait() {
    count_cond.lock();
    count_cond.wait();
    count_at_signal = count;
    sleep(1);
    count *= 10;
    count_cond.unlock();
  }

  static void *thread_fun(void *t) {
    CondTest<cond> *self = (CondTest<cond> *)t;

    self->inc();
    return NULL;
  }

  void inc() {
    for(int i = 0; i < COUNT; i++) {
      count_cond.lock();
      if(count < 1000000) {
	count++;
	if(count > 100000)
	  count_cond.signal();
	count_cond.unlock();
      } else {
	count_cond.unlock();
	return;
      }
    }
  }


  int get_count() { return count; }
  int get_count_at_signal() { return count_at_signal; }
};

typedef Types<locks::pthread::cond> Implementations;

TYPED_TEST_CASE(CondTest, Implementations);

TYPED_TEST(CondTest, SuccessfullLocking) {
  EXPECT_EQ(10 *this->get_count_at_signal(), this->get_count());
}
