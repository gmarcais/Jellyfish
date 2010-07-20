#ifndef __DUMPER_HPP__
#define __DUMPER_HPP__

#include "mer_counting.hpp"

/**
 * A dumper is responsible to dump the hash array to permanent storage
 * and zero out the array.
 **/
namespace jellyfish {
  class dumper_t {
  public:
    virtual void dump() = 0;
    virtual ~dumper_t() {};
  };
  //dumper_t::~dumper_t() {}

  template <typename T>
  size_t bits_to_bytes(T bits) {
    return (size_t)((bits / 8) + (bits % 8 != 0));
  }
}
#endif // __DUMPER_HPP__
