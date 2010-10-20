#ifndef __JELLYFISH_THREAD_EXEC_HPP__
#define __JELLYFISH_THREAD_EXEC_HPP__

#include <pthread.h>
#include <vector>
#include <exception>
#include <stdexcept>
#include <string>
#include <string.h>

class thread_exec {
  struct thread_info {
    int          id;
    pthread_t    thid;
    thread_exec *self;
  };
  static void *start_routine(void *);
  std::vector<struct thread_info> infos;

public:
  thread_exec() {}
  virtual ~thread_exec() {}
  virtual void start(int id) = 0;
  void exec(int nb_threads);
  void join();
  void exec_join(int nb_threads) {
    exec(nb_threads);
    join();
  }
};

#endif // __THREAD_EXEC_HPP__
