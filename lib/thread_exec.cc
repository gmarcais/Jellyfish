/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#include <jellyfish/thread_exec.hpp>

void jellyfish::thread_exec::exec(int nb_threads) {
  struct thread_info empty = {0, 0, 0};
  infos.resize(nb_threads, empty);

  for(int i = 0; i < nb_threads; i++) {
    infos[i].id   = i;
    infos[i].self = this;
    int err = pthread_create(&infos[i].thid, NULL, start_routine, &infos[i]);
    if(err)
      throw Error(err::msg() << "Can't create thread: " << err::no);
  }
}

void jellyfish::thread_exec::join() {
  for(unsigned int i = 0; i < infos.size(); i++) {
    int err = pthread_join(infos[i].thid, NULL);
    if(err)
      throw Error(err::msg() << "Can't join thread '" << infos[i].thid << "': " << err::no);
  }
}

void *jellyfish::thread_exec::start_routine(void *_info) {
  struct thread_info *info = (struct thread_info *)_info;
  info->self->start(info->id);
  return 0;
}
