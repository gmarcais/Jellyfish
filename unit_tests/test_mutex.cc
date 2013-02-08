#include <gtest/gtest.h>
#include <jellyfish/locks_pthread.hpp>

using testing::Types;

#define NB_THREADS 3
#define COUNT	1000000

template <class mutex>
class MutexTest : public ::testing::Test
{
  int				volatile count;
  mutex				count_mutex;
  pthread_t			threads[NB_THREADS];
  pthread_attr_t		attr;

public:
  MutexTest() : count(0), count_mutex() {
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for(int i = 0; i < NB_THREADS; i++)
      pthread_create(&threads[i], &attr, thread_fun, (void *)this);
    
    for(int i = 0; i < NB_THREADS; i++)
      pthread_join(threads[i], NULL);
  }

  static void *thread_fun(void *t) {
    MutexTest<mutex> *self = (MutexTest<mutex> *)t;

    self->inc();
    return NULL;
  }

  void inc() {
    for(int i = 0; i < COUNT; i++) {
      count_mutex.lock();
      count++;
      count_mutex.unlock();
    }
  }


  int get_count() { return count; }
};

typedef Types<locks::pthread::mutex> Implementations;

TYPED_TEST_CASE(MutexTest, Implementations);

TYPED_TEST(MutexTest, SuccessfullLocking) {
  EXPECT_EQ(NB_THREADS * COUNT, this->get_count());
}
