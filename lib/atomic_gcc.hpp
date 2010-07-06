#ifndef __ATOMIC_GCC_HPP__
#define __ATOMIC_GCC_HPP__

namespace atomic
{
  template<typename T>
  class gcc
  {
  public:
    inline T cas(T *ptr, T oval, T nval) {
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
  };
}
#endif
