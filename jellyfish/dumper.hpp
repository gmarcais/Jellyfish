#ifndef __DUMPER_HPP__
#define __DUMPER_HPP__

#include "mer_counting.hpp"
#include "time.hpp"

/**
 * A dumper is responsible to dump the hash array to permanent storage
 * and zero out the array.
 **/
namespace jellyfish {
  class dumper_t {
  public:
    virtual void dump() = 0;
    virtual Time get_writing_time() const = 0;
    virtual ~dumper_t() {};
  };
}
#endif // __DUMPER_HPP__
