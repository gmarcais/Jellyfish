#ifndef __JELLYFISH_INTERSECTION_ARRAY_HPP__
#define __JELLYFISH_INTERSECTION_ARRAY_HPP__

#include <jellyfish/compare_and_swap.hpp>
#include <jellyfish/large_hash_array.hpp>
#include <jellyfish/atomic_gcc.hpp>
#include <jellyfish/locks_pthread.hpp>


namespace jellyfish {

template<typename Key>
class intersection_array {
public:
  typedef large_hash::array<Key> array;

protected:
  typedef uint64_t status_w; // Type to store the mer status
  static const uint16_t status_size = 4; // Nb of bits in a status
  static const size_t status_pw = (8 * sizeof(status_w)) / status_size; // Nb of status per word

  // sbits<b>::v has the b-th bits (starting at 0) set in every 4 bit group
  template<unsigned int b, int l = sizeof(status_w) - 1>
  struct sbits {
    static const status_w v = sbits<b, 0>::v | (sbits<b, l - 1>::v << 8);
  };
  template<unsigned int b>
  struct sbits<b, 0> {
    static const status_w v = (status_w)0x11 << b;
  };

  array*                  ary_;
  array*                  new_ary_;
  status_w*               mer_status_;
  status_w*               new_mer_status_;
  uint16_t                nb_threads_;
  locks::pthread::barrier size_barrier_;
  volatile uint16_t       size_thid_, done_threads_;


public:
  union mer_info {
    unsigned char raw;
    struct info {
      unsigned char set:1;      // Set in current file
      unsigned char uniq:1;     // Unique (if mult is 0)
      unsigned char mult:1;     // Non-unique
      unsigned char nall:1;     // Not in all files
    } info;
  };

  struct usage_info {
    uint16_t key_len_;
    usage_info(uint16_t key_len) : key_len_(key_len) { }

    size_t mem(size_t size) {
      typename array::usage_info array_info(key_len_, 0, 126);
      return array_info.mem(size) + sizeof(status_w) * (array_info.asize(size) / status_pw);
    }

    /// Maximum size for a given maximum memory.
    size_t size(size_t mem) { return (size_t)1 << size_bits(mem); }

    struct fit_in {
      usage_info* i_;
      size_t      mem_;
      fit_in(usage_info* i, size_t mem) : i_(i), mem_(mem) { }
      bool operator()(uint16_t size_bits) const { return i_->mem((size_t)1 << size_bits) < mem_; }
    };

    uint16_t size_bits(size_t mem) {
      uint16_t res = *binary_search_first_false(pointer_integer<uint16_t>(0), pointer_integer<uint16_t>(64), fit_in(this, mem));
      return res > 0 ? res - 1 : 0;
    }
  };

  intersection_array(size_t   size, // Initial size of hash
                     uint16_t key_len, // Size of mer
                     uint16_t nb_threads) : // Number of threads accessing hash (cooperative)
    ary_(new array(size, key_len, 0, 126, jellyfish::quadratic_reprobes)),
    new_ary_(0),
    mer_status_(new status_w[ary_->size() / status_pw]), // TODO: change this to a mmap allocated area?
    new_mer_status_(0),
    nb_threads_(nb_threads),
    size_barrier_(nb_threads),
    size_thid_(0),
    done_threads_(0)
  {
    memset(mer_status_, '\0', sizeof(status_w) * ary_->size() / status_pw);
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
    bool   is_new = false;
    size_t id     = 0;
    while(!ary_->set(k, &is_new, &id))
      double_size();

    // Set lower bit of the status (the set bit)
    status_w oinfo = 0;
    status_w ninfo = mer_status_[id / status_pw];
    status_w mask  = (status_w)1 << (status_size * (id % status_pw));
    do {
      oinfo  = ninfo;
      ninfo |= mask;
      ninfo  = atomic::gcc::cas(&mer_status_[id / status_pw], oinfo, ninfo);
    } while(oinfo != ninfo);
  }

  /// Post-process mer status after a file.
  void postprocess(uint16_t thid) {
    const std::pair<size_t,size_t> part = slice((size_t)thid, (size_t)nb_threads_, size() / status_pw);
    for(size_t i = part.first; i < part.second; ++i) {
      status_w info = mer_status_[i];
      // Update nall: nall (3rd bit) is 1 if the set bit (0th bit) is 0.
      info |= (~info & sbits<0>::v) << 3;
      // Update mult: mult (2nd bit) is 1 if the set bit (0th bit) and the uniq bit (1th bit) are 1
      info |= ((info & sbits<0>::v) << 2) & ((info & sbits<1>::v) << 1);
      // Update uniq: uniq (1st bit) is 1 if the set bit (0th bit) is 1
      info |= (info & sbits<0>::v) << 1;

      // Store result and zero set bits (0th bit)
      mer_status_[i] = info & ~sbits<0>::v;
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
    status_w raw = mer_status_[id / status_pw];
    res.raw = raw >> (status_size * (id % status_pw));

    return res;
  }

protected:
  void add_new(const Key& k, mer_info i) {
    bool   is_new;
    size_t id;
    new_ary_->set(k, &is_new, &id);

    status_w oinfo;
    status_w ninfo = new_mer_status_[id / status_pw];
    size_t   shift = status_size * (id % status_pw);
    status_w mask  = ~((status_w)0xf << shift);
    status_w val   = (status_w)i.raw << shift;
    do {
      oinfo = ninfo;
      ninfo = (ninfo & mask) | val;
      ninfo = atomic::gcc::cas(&new_mer_status_[id / status_pw], oinfo, ninfo);
    } while(oinfo != ninfo);
  }

  bool double_size() {
    if(size_barrier_.wait()) {
      // Serial thread -> allocate new array for size doubling
      if(done_threads_ < nb_threads_) {
        new_ary_        = new array(ary_->size() * 2, ary_->key_len(), 0, 126, jellyfish::quadratic_reprobes);
        new_mer_status_ = new status_w[new_ary_->size() / status_pw];
        memset(new_mer_status_, '\0', sizeof(status_w) * new_ary_->size() / status_pw);
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
    uint16_t                       id = atomic::gcc::fetch_add(&size_thid_, (uint16_t)1);
    typename array::eager_iterator it = ary_->eager_slice(id, nb_threads_);
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
