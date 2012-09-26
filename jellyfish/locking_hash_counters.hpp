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

#ifndef __JELLYFISH_LOCKING_HASH_COUNTERS__
#define __JELLYFISH_LOCKING_HASH_COUNTERS__

#include <new>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <stdint.h>
using namespace std;

struct stats {
  unsigned int  key_conflicts;
  unsigned int  val_conflicts;
  unsigned int  destroyed_key;
  unsigned int  destroyed_val;
  unsigned int  maxed_out_val;
  unsigned int  maxed_reprobe;
  unsigned int  resized_arys;
};
#ifdef LOCKING_HASH_STATS
#define LOCKING_HASH_STAT_INC(var) __sync_fetch_and_add(&stats->var, 1);
#else
#define LOCKING_HASH_STAT_INC(var)
#endif

template <class Key, class Val, class A>
class arys_iterator {
  A                     *ary;
  unsigned long         pos;

public:
  Key key;
  Val val;

  explicit arys_iterator(A *_ary) : ary(_ary), pos(0) {
  }

  ~arys_iterator() { }

  void rewind() { pos = 0; }

  bool next() {
    Key *ckey;
    Val *cval;
    
    while(pos < ary->size) {
      ary->get(pos++, &ckey, &cval);
      key = *ckey;
      if(key & 0x1) {
        val = *cval;
        key >>= 1;
        return true;
      }
    }
    return false;
  }
};

class bad_size_exception: public exception
{
  virtual const char* what() const throw() {
    return "Size must be a power of 2";
  }
};


template <class Key, class Val, unsigned int nb, unsigned int shift, unsigned int mask>
class arys_t {
public:
  typedef Key key_t;
  typedef Val val_t;
  /* Keys. Lower order bits are used internally.
   * 0: is key set?
   * 1: is key destroyed?
   */
  /* Values. Lower order bit is used internally.
   * 0: is value destroyed?
   */
  typedef struct elt {
    Key keys[nb];
    Val vals[nb];
  } elt;
  elt *key_vals;

  unsigned long size;
  unsigned long esize;
  unsigned long mod_mask;
  bool          allocated;

  /* arys are in a linked list. They are freed in reverse order of
   * creation. The link from previous arys counts as 1 reference, to
   * ensure proper order of deletion.
   * The constructor is not thread safe (protect by mutex)
   */
  explicit arys_t(unsigned long _size) : 
    allocated(true) {
    size = 1;
    while(_size > size) { size <<= 1; }
    esize = size >> shift;
    mod_mask = size - 1;
    key_vals = new elt[esize];
    memset(key_vals, '\0', sizeof(elt) * esize);
  }

  arys_t(unsigned long _size, char *map) :
    allocated(false) {
    size = 1;
    while(_size > size) { size <<= 1; }
    if(_size != size)
      throw new bad_size_exception;
    mod_mask = size - 1;
    key_vals = (elt *)map;
  }

  ~arys_t() {
    if(allocated)
      delete[] key_vals;
  }

  inline void get(unsigned long idx, Key **k, Val **v) {
    elt *e = &key_vals[idx >> shift];
    *k = &e->keys[idx & mask];
    *v = &e->vals[idx & mask];
  }

  typedef arys_iterator<Key,Val,arys_t> iterator;
  arys_iterator<Key,Val,arys_t> * new_iterator() {
    return new arys_iterator<Key,Val,arys_t>(this);
  }
};

template <class Key, class Val, class arys_t>
class thread_hash_counter {
  arys_t                        * volatile *key_val_arys_ptr;
  arys_t                        *key_val_arys;
  unsigned int                  max_reprobe;
  pthread_mutex_t               *hash_lock;
  struct stats                  *stats;

  void resize(arys_t *cary);
  void no_lock_add(Key k, Val v);

public:
  thread_hash_counter(arys_t * volatile * _arys,
                      unsigned int _max_reprobe,
                      pthread_mutex_t *_hash_lock, struct stats *_stats) : 
    key_val_arys_ptr(_arys), max_reprobe(_max_reprobe), 
    hash_lock(_hash_lock), stats(_stats) {
    key_val_arys = *key_val_arys_ptr;
  }
  
  ~thread_hash_counter() { }
  
  inline void add(Key k, Val v);
  inline void inc(Key k) { add(k, 1); }
};

template <class Key, class Val, class arys_t>
class locking_hash_counter {
  arys_t                        * volatile key_val_arys;
  struct stats                  stats;
  unsigned int                  max_reprobe;
  pthread_mutex_t               hash_lock;

public:
  locking_hash_counter(unsigned long _size, unsigned int _max_reprobe);
  ~locking_hash_counter();

  typedef thread_hash_counter<Key, Val, arys_t> thread;

  thread *new_hash_counter() {
    return new thread_hash_counter<Key,Val,arys_t>(&key_val_arys, 
                                                   max_reprobe, &hash_lock,
                                                   &stats);
  }

  unsigned long size() { return key_val_arys->size; }

  // Not thread safe
  void print(ostream &out);
  void print_debug(ostream &out);
  void write_keys_vals(ostream &out);

  int has_stats() { 
#ifdef LOCKING_HASH_STATS
    return 1;
#else
    return 0;
#endif
  };
  void print_stats(ostream &out);
};

/*
 * Usefull sizes
 */
typedef arys_t<uint64_t,uint32_t,2,1,0x1> arys_64_32_t;
typedef locking_hash_counter<uint64_t,uint32_t,arys_64_32_t> chc_64_32_t;

/*
 * locking_hash_counter
 */
template <class Key, class Val, class arys_t>
locking_hash_counter<Key,Val,arys_t>::locking_hash_counter(unsigned long _size, 
                                                          unsigned int _max_reprobe) :
  max_reprobe(_max_reprobe) {
  key_val_arys = new arys_t(_size);
  pthread_mutex_init(&hash_lock, NULL);
  memset(&stats, 0, sizeof(stats));
}

template<class Key, class Val, class arys_t>
locking_hash_counter<Key,Val,arys_t>::~locking_hash_counter() {
  pthread_mutex_destroy(&hash_lock);
  delete key_val_arys;
}

template<class Key, class Val, class arys_t>
void locking_hash_counter<Key,Val,arys_t>::print_debug(ostream &out) {
//   unsigned long i;
//   arys_t *carys = key_val_arys;
//   Key key;

//   for(i = 0; i < carys->size; i++) {
//     key = carys->keys[i];
//     if(key & 0x1)
//       out << i << " " << (key >> 2) << " " << (key & 0x3) << " " << 
//         (carys->vals[i] >> 1) << endl;
//     else
//       out << i << " Empty" << endl;
//   }
}

template<class Key, class Val, class arys_t>
void locking_hash_counter<Key,Val,arys_t>::print(ostream &out) {
  unsigned long i;
  arys_t *carys = key_val_arys;
  Key *ckey, key;
  Val *cval;

  for(i = 0; i < carys->size; i++) {
    carys->get(i, &ckey, &cval);
    key = *ckey;
    if(key & 0x1)
      out << (key >> 2) << " " << (*cval >> 1) << endl;
  }
}

template<class Key, class Val, class arys_t>
void locking_hash_counter<Key,Val,arys_t>::write_keys_vals(ostream &out) {
  arys_t *carys = key_val_arys;

  out.write((char *)carys->key_vals, sizeof(typename arys_t::elt) * carys->esize);
}

template<class Key, class Val, class arys_t>
void locking_hash_counter<Key,Val,arys_t>::print_stats(ostream &out) {
#ifdef STATS
#define PRINT_STAT(var) { out << #var ": " << stats.var << endl; }
#else
#define PRINT_STAT(var) { out << #var ": -" << endl; }
#endif

  PRINT_STAT(key_conflicts);
  PRINT_STAT(val_conflicts);
  PRINT_STAT(destroyed_key);
  PRINT_STAT(destroyed_val);
  PRINT_STAT(maxed_out_val);
  PRINT_STAT(maxed_reprobe);
  PRINT_STAT(resized_arys);
}

/*
 * thread_hash_counter
 */
template <class Key, class Val, class arys_t>
inline void thread_hash_counter<Key,Val,arys_t>::add(Key k, Val v) {
  pthread_mutex_lock(hash_lock);
  no_lock_add(k, v);
  pthread_mutex_unlock(hash_lock);
}

template <class Key, class Val, class arys_t>
void thread_hash_counter<Key,Val,arys_t>::no_lock_add(Key k, Val v) {
  unsigned long idx;
  Key key, inkey;
  Val val, sval, cval;
  arys_t *carys;
  Key *ckeys;
  Val *cvals;
  unsigned int reprobe = 0;

  inkey = (k << 1) | 0x1;
  carys = *key_val_arys_ptr;
  idx = k & carys->mod_mask;
  while(1) {
    carys->get(idx, &ckeys, &cvals);
    key = *ckeys;
    if(!(key & 0x1)) {    // Key is free
      *ckeys = inkey;
      break;
    }
    if(inkey == key) {    // Key already in
      break;
    }

    // Reprobe, or resize and try again.
    if(++reprobe > max_reprobe) {
      LOCKING_HASH_STAT_INC(maxed_reprobe);
      resize(carys);
      carys = *key_val_arys_ptr;
      idx = k & carys->mod_mask;
    } else {
      idx = (idx + 1) & carys->mod_mask; // reprobe
    }
  }

  val = *cvals;
  cval = ~val;
  if(cval == (Val)0x0) { // If key at its max, do not increment
    LOCKING_HASH_STAT_INC(maxed_out_val);
  } else {
    if(cval < v) { // Key reaches its max, truncate
      sval = (Val)~0x0;
      LOCKING_HASH_STAT_INC(maxed_out_val);
    } else {
      sval = val + v;
    }
    *cvals = sval;
  }
}

template<class Key, class Val, class arys_t>
void thread_hash_counter<Key,Val,arys_t>::resize(arys_t *carys) {
  unsigned long i;
  Key *ckey;
  Val *cval;
  arys_t *narys;

  narys = new arys_t(carys->size << 1);
  *key_val_arys_ptr = narys;  

  // Copy over and free old data
  Key key;
  for(i = 0; i < carys->size; i++) {
    carys->get(i, &ckey, &cval);
    key = *ckey;
    if(key & 0x1) { // Copy value
      no_lock_add(key >> 1, *cval);
    }
  }
  delete carys;
  LOCKING_HASH_STAT_INC(resized_arys);
}

#endif /* __LOCKING_HASH_COUNTERS__ */
