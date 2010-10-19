#ifndef __JELLYFISH_ATOMIC_GCC_HPP__
#define __JELLYFISH_ATOMIC_GCC_HPP__

namespace atomic
{
  template<typename T>
  class gcc
  {
  public:
    typedef T type;
    inline T cas(volatile T *ptr, T oval, T nval) {
      return __sync_val_compare_and_swap(ptr, oval, nval);
    }

    inline T set(T *ptr, T nval) {
      return __sync_lock_test_and_set(ptr, nval);
    }

    inline T add_fetch(volatile T *ptr, T x) {
      T ncount = *ptr, count;
      do {
	count = ncount;
	ncount = cas((T *)ptr, count, count + x);
      } while(ncount != count);
      return count + x;
    }

    inline T set_to_max(volatile T *ptr, T x) {
      T count = *ptr;
      while(x > count) {
        T ncount = cas(ptr, count, x);
        if(ncount == count)
          return x;
        count = ncount;
      }
      return count;
    }
  };
}
#endif
