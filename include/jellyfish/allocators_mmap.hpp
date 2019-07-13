/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifndef __JELLYFISH_ALLOCATORS_MMAP_HPP__
#define __JELLYFISH_ALLOCATORS_MMAP_HPP__

#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>

namespace allocators {
class mmap {
  void   *ptr_;
  size_t  size_;

public:
  mmap() : ptr_(MAP_FAILED), size_(0) {}
  explicit mmap(size_t _size) : ptr_(MAP_FAILED), size_(0) {
    realloc(_size);
  }
  mmap(mmap&& rhs) : ptr_(rhs.ptr_), size_(rhs.size_) {
    rhs.ptr_  = MAP_FAILED;
    rhs.size_ = 0;
  }
  ~mmap() { free(); }

  mmap& operator=(mmap&& rhs) {
    swap(rhs);
    return *this;
  }

  void swap(mmap& rhs) {
    std::swap(ptr_, rhs.ptr_);
    std::swap(size_, rhs.size_);
  }

  void *get_ptr() const { return ptr_ != MAP_FAILED ? ptr_ : NULL; }
  size_t get_size() const { return size_; }
  void free();
  void *realloc(size_t new_size);
  int lock() { return mlock(ptr_, size_); }
  int unlock() { return munlock(ptr_, size_); }

  // Return a a number of bytes which is a number of whole pages at
  // least as large as size.
  static size_t round_to_page(size_t _size);

private:
  static const int nb_threads = 4;
  struct tinfo {
    pthread_t  thid;
    char      *start, *end;
    size_t     pgsize;
  };
  void fast_zero();
  static void * _fast_zero(void *_info);
};
inline void swap(mmap& a, mmap& b) { a.swap(b); }
}

#endif
