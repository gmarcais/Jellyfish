#include "thread_exec.hpp"

void thread_exec::exec(int nb_threads) {
  struct thread_info empty = {0, 0, 0};
  infos.resize(nb_threads, empty);

  for(int i = 0; i < nb_threads; i++) {
    infos[i].id   = i;
    infos[i].self = this;
    int err = pthread_create(&infos[i].thid, NULL, start_routine, &infos[i]);
    if(err)
      throw std::runtime_error("Can't create thread: " + std::string(strerror(err)));
  }
}

void thread_exec::join() {
  for(unsigned int i = 0; i < infos.size(); i++) {
    int err = pthread_join(infos[i].thid, NULL);
    if(err)
      throw std::runtime_error("Can't join thread: " + std::string(strerror(err)));
  }
}

void *thread_exec::start_routine(void *_info) {
  struct thread_info *info = (struct thread_info *)_info;
  info->self->start(info->id);
  return 0;
}
