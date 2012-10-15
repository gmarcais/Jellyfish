/*  This file is part of Jellyfish.

    Jellyfish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jellyfish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jellyfish.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __HASH_COUNTER_HPP__
#define __HASH_COUNTER_HPP__

#include <jellyfish/large_hash_array.hpp>
#include <jellyfish/locks_pthread.hpp>

/// Cooperative version of the hash_counter. In this implementation,
/// it is expected that the given number of threads will call the
/// `add` method regularly. In case the hash table is full, it gets
/// enlarged using all the threads. After the work is done, every
/// thread must call promptly the `done` method.

namespace jellyfish{ namespace cooperative {

template<typename Key, typename word = uint64_t, typename atomic_t = ::atomic::gcc, typename mem_block_t = ::allocators::mmap>
class hash_counter {
public:
  typedef large_hash::array<Key, word, atomic_t, mem_block_t> array;
  typedef typename array::key_type        key_type;
  typedef typename array::mapped_type     mapped_type;
  typedef typename array::value_type      value_type;
  typedef typename array::reference       reference;
  typedef typename array::const_reference const_reference;
  typedef typename array::pointer         pointer;
  typedef typename array::const_pointer   const_pointer;

protected:
  array*                  ary_;
  array*                  new_ary_;
  uint16_t                nb_threads_;
  locks::pthread::barrier size_barrier_;
  volatile uint16_t       size_thid_, done_threads_;

public:
  hash_counter(size_t size, // Size of hash. To be rounded up to a power of 2
               uint16_t key_len, // Size of key in bits
               uint16_t val_len, // Size of val in bits
               uint16_t nb_threads, // Number of threads accessing this hash
               uint16_t reprobe_limit = 126, // Maximum reprobe
               const size_t* reprobes = jellyfish::quadratic_reprobes) :
    ary_(new array(size, key_len, val_len, reprobe_limit, reprobes)),
    new_ary_(0),
    nb_threads_(nb_threads),
    size_barrier_(nb_threads),
    size_thid_(0),
    done_threads_(0)
  { }

  ~hash_counter() {
    delete ary_;
  }

  const array* ary() { return ary_; }
  size_t size() const { return ary_->size(); }
  uint16_t key_len() const { return ary_->key_len(); }
  uint16_t val_len() const { return ary_->val_len(); }
  uint16_t nb_threads() const { return nb_threads; }
  uint16_t reprobe_limit() const { return ary_->max_reprobe(); }

  /// Add `v` to the entry `k`. This method is multi-thread safe. If
  /// the entry for `k` does not exists, it is inserted.
  ///
  /// @param k Key to add to
  /// @param v Value to add
  void add(const Key& k, uint64_t v) {
    while(!ary_->add(k, v))
      double_size();
  }

  /// Signify that thread is done and wait for all threads to be done.
  void done() {
    atomic_t::fetch_add(&done_threads_, (uint16_t)1);
    while(!double_size()) ;
  }

protected:
  // Double the size of the hash and return false. Unless all the
  // thread have reported they are done, in which case do nothing and
  // return true.
  bool double_size() {
    if(size_barrier_.wait()) {
      // Serial thread -> allocate new array for size doubling
      if(done_threads_ < nb_threads_) {
        new_ary_   = new array(ary_->size() * 2, ary_->key_len(), ary_->val_len(),
                               ary_->max_reprobe(), ary_->reprobes());
        size_thid_ = 0;
      } else {
        new_ary_      = 0;
        done_threads_ = 0;
      }
    }

    // Return if all done
    size_barrier_.wait();
    array* my_ary = *(array* volatile*)&new_ary_;
    if(my_ary == 0)
      return true;

    // Copy data from old to new
    uint16_t                 id       = atomic_t::fetch_add(&size_thid_, (uint16_t)1);
    typename array::iterator it       = ary_->iterator_slice(id, nb_threads_);
    while(it.next())
      my_ary->add(it.key(), it.val());

    if(size_barrier_.wait()) {
      // Serial thread -> set new ary to be current
      delete ary_;
      ary_ = new_ary_;
    }

    // Done. Last sync point
    size_barrier_.wait();
    return false;
  }
};

} } // namespace jellyfish { namespace cooperative {
#endif /* __HASH_COUNTER_HPP__ */
