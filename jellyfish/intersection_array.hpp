#ifndef __JELLYFISH_INTERSECTION_ARRAY_HPP__
#define __JELLYFISH_INTERSECTION_ARRAY_HPP__

#include <jflib/compare_and_swap.hpp>
#include <jellyfish/large_hash_array.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/locks_pthread.hpp>


namespace jellyfish {

template<typename Key>
class intersection_array {
public:
  typedef large_hash::array<Key> array;

protected:
  array*                  ary_;
  array*                  new_ary_;
  unsigned char*          mer_status_;
  unsigned char*          new_mer_status_;
  uint16_t                nb_threads_;
  locks::pthread::barrier size_barrier_;
  volatile uint16_t       size_thid_, done_threads_;

  union mer_info2 {
    unsigned char raw;
    struct info {
      unsigned char set1:1;    // Set in current file
      unsigned char uniq1:1; // Unique
      unsigned char mult1:1;   // Non-unique
      unsigned char all1:1;    // In all files
      unsigned char set2:1;
      unsigned char uniq2:1;
      unsigned char mult2:1;
      unsigned char all2:1;
    } info;
  };

public:
  union mer_info {
    unsigned char raw;
    struct info {
      unsigned char set:1;    // Set in current file
      unsigned char uniq:1; // Unique
      unsigned char mult:1;   // Non-unique
      unsigned char all:1;    // In all files
    } info;
  };

  intersection_array(size_t   size, // Initial size of hash
                     uint16_t key_len, // Size of mer
                     uint16_t nb_threads) : // Number of threads accessing hash (cooperative)
    ary_(new array(size, key_len, 0, 126, jellyfish::quadratic_reprobes)),
    new_ary_(0),
    mer_status_(new unsigned char[ary_->size() / 2]), // TODO: change this to a mmap allocated area?
    new_mer_status_(0),
    nb_threads_(nb_threads),
    size_barrier_(nb_threads),
    size_thid_(0),
    done_threads_(0)
  {
    memset(mer_status_, '\0', ary_->size() / 2);
  }

  ~intersection_array() {
    delete [] mer_status_;
    delete ary_;
  }

  size_t size() const { return ary_->size(); }
  uint16_t key_len() const { return ary_->key_len(); }
  uint16_t nb_threads() const { return nb_threads_; }

  array* ary() { return ary_; }

  /// Signify that current thread is done and wait for all threads to
  /// be done as well.
  void done() {
    atomic::gcc::fetch_add(&done_threads_, (uint16_t)1);
    while(!double_size()) ;
  }

  void add(const Key& k) {
    bool   is_new;
    size_t id;
    while(!ary_->set(k, &is_new, &id))
      double_size();

    unsigned char oinfo;
    mer_info2 ninfo;
    ninfo.raw = mer_status_[id / 2];
    do {
      oinfo = ninfo.raw;
      if(id & 0x1)
        ninfo.info.set2 = 1;
      else
        ninfo.info.set1 = 1;
      ninfo.raw = atomic::gcc::cas(&mer_status_[id / 2], oinfo, ninfo.raw);
    } while(oinfo != ninfo.raw);
  }

  /// Post-process mer status after a file.
  void postprocess(uint16_t thid, bool first) {
    const std::pair<size_t,size_t> part = slice((size_t)thid, (size_t)nb_threads_, size() / 2);
    if(first) {
      for(size_t i = part.first; i < part.second; ++i) {
        unsigned char& info = mer_status_[i];
        switch(info) {
        case 0x1 : info = 0xa;  break;
        case 0x10: info = 0xa0; break;
        case 0x11: info = 0xaa; break;
        }
      }
    } else {
      for(size_t i = part.first; i < part.second; ++i) {
        mer_info2& info = *reinterpret_cast<mer_info2*>(&mer_status_[i]);
        // Update all
        info.info.all1 &= info.info.set1;
        info.info.all2 &= info.info.set2;

        // Update mult and uniq
        // info.raw |= ((info.raw & 0x11) << 2) & ((info.raw & 0x22) << 1);
        // info.raw |= (info.raw & 0x11) << 1;
        info.info.mult1 |= info.info.set1 & info.info.uniq1;
        info.info.uniq1 |= info.info.set1;
        info.info.mult2 |= info.info.set2 & info.info.uniq2;
        info.info.uniq2 |= info.info.set2;

        // Reset raw
        info.raw &= 0xee;
      }
    }
  }

  mer_info operator[](const Key& k) const {
    size_t   id;

    if(!ary_->get_key_id(k, &id)) {
      mer_info res;
      res.raw = 0;
      return res;
    }
    return info_at(id);
  }

  mer_info info_at(size_t id) const {
    mer_info res;
    res.raw = mer_status_[id / 2];
    if(id & 0x1)
      res.raw >>= 4;

    return res;
  }

protected:
  void add_new(const Key& k, mer_info i) {
    bool   is_new;
    size_t id;
    new_ary_->set(k, &is_new, &id);

    unsigned char oinfo;
    unsigned char ninfo = new_mer_status_[id / 2];
    do {
      oinfo = ninfo;
      if(id & 0x1)
        ninfo = (ninfo & 0xf) | (i.raw << 4);
      else
        ninfo = (ninfo & 0xf0) | (i.raw & 0xf);
      ninfo = atomic::gcc::cas(&new_mer_status_[id / 2], oinfo, ninfo);
    } while(oinfo != ninfo);

  }

  bool double_size() {
    if(size_barrier_.wait()) {
      // Serial thread -> allocate new array for size doubling
      if(done_threads_ < nb_threads_) {
        new_ary_        = new array(ary_->size() * 2, ary_->key_len(), 0, 126, jellyfish::quadratic_reprobes);
        new_mer_status_ = new unsigned char[ary_->size()];
        memset(new_mer_status_, '\0', ary_->size());
        size_thid_      = 0;
      } else {
        new_ary_        = 0;
        new_mer_status_ = 0;
        done_threads_   = 0;
      }
    }

    // Return if all done
    size_barrier_.wait();
    array* my_ary = *(array* volatile*)&new_ary_;
    if(my_ary == 0)
      return true;

    // Copy data from old to new
    uint16_t                 id = atomic::gcc::fetch_add(&size_thid_, (uint16_t)1);
    typename array::iterator it = ary_->iterator_slice(id, nb_threads_);
    while(it.next()) {
      assert(it.id() >= 0 && it.id() < ary_->size());
      add_new(it.key(), info_at(it.id()));
    }

    if(size_barrier_.wait()) {
      // Serial thread -> set new ary to be current
      delete ary_;
      delete [] mer_status_;
      ary_        = new_ary_;
      mer_status_ = new_mer_status_;
    }

    // Done. Last sync point
    size_barrier_.wait();
    return false;
  }
};
}

#endif /* __JELLYFISH_INTERSECTION_ARRAY_HPP__ */
