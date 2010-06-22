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
  };
}
#endif
