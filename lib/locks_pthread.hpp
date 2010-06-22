#ifndef __LOCKS_PTHREAD_HPP__
#define __LOCKS_PTHREAD_HPP__

#include <pthread.h>

namespace locks {
  namespace pthread {
    class cond
    {
      pthread_mutex_t   _mutex;
      pthread_cond_t	_cond;
      
    public:
      cond() { 
	pthread_mutex_init(&_mutex, NULL);
	pthread_cond_init(&_cond, NULL);
      }
      
      ~cond() {
	pthread_cond_destroy(&_cond);
	pthread_mutex_destroy(&_mutex);
      }
      
      inline void lock() { pthread_mutex_lock(&_mutex); }
      inline void unlock() { pthread_mutex_unlock(&_mutex); }
      inline void wait() { pthread_cond_wait(&_cond, &_mutex); }
      inline void signal() { pthread_cond_signal(&_cond); }
    };
    
    class mutex
    {
      pthread_mutex_t     _mutex;
    
    public:
      mutex() {
	pthread_mutex_init(&_mutex, NULL);
      }
    
      ~mutex() {
	pthread_mutex_destroy(&_mutex);
      }
    
      inline void lock() { pthread_mutex_lock(&_mutex); }
      inline void unlock() { pthread_mutex_unlock(&_mutex); }
    };
  }
}
#endif
