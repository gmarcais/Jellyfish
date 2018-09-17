/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifndef __JELLYFISH_THREAD_EXEC_HPP__
#define __JELLYFISH_THREAD_EXEC_HPP__

#include <pthread.h>
#include <vector>
#include <exception>
#include <stdexcept>
#include <string>
#include <string.h>
#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>

namespace jellyfish {
class thread_exec {
  struct thread_info {
    int          id;
    pthread_t    thid;
    thread_exec *self;
  };
  static void *start_routine(void *);
  std::vector<struct thread_info> infos;

public:
  define_error_class(Error);
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
} // namespace jellyfish {

#endif // __THREAD_EXEC_HPP__
