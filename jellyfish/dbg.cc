/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#include <jellyfish/dbg.hpp>
#include <jellyfish/time.hpp>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif

namespace dbg {
  pthread_mutex_t print_t::_lock      = PTHREAD_MUTEX_INITIALIZER;
  volatile pid_t  print_t::_print_tid = 0;

#ifdef DEBUG
  Time _tic_time;
#endif

  void tic() {
#ifdef DEBUG
    _tic_time.now();
#endif
 }
  Time toc() {
#ifdef DEBUG
    Time t;
    return t - _tic_time;
#else
    return Time::zero;
#endif
  }

#ifdef SYS_gettid
  pid_t gettid() { return (pid_t)syscall(SYS_gettid); }
#else
  pid_t gettid() { return getpid(); }
#endif

  int print_t::set_signal(int signum) {
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_sigaction = signal_handler;
    act.sa_flags     = SA_SIGINFO;
    return sigaction(signum, &act, 0);
  }

  void print_t::signal_handler(int signum, siginfo_t *info, void *context) {
#ifdef  HAVE_SI_INT
    if(info->si_code != SI_QUEUE)
      return;
    _print_tid = info->si_int;
#endif
  }
}
