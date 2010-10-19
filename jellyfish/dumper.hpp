#ifndef __JELLYFISH_DUMPER_HPP__
#define __JELLYFISH_DUMPER_HPP__

#include <jellyfish/mer_counting.hpp>
#include <jellyfish/time.hpp>

/**
 * A dumper is responsible to dump the hash array to permanent storage
 * and zero out the array.
 **/
namespace jellyfish {
  class dumper_t {
    Time writing_time;

  protected:
    void open_next_file(const char *prefix, int &index, std::ofstream &out) {
      static const long file_len = pathconf("/", _PC_PATH_MAX);

      char file[file_len + 1];
      file[file_len] = '\0';
      int off = snprintf(file, file_len, "%s", prefix);
      if(off > 0 && off < file_len)
        off += snprintf(file + off, file_len - off, "_%d", index++);
      if(off < 0 || off >= file_len)
        return; // TODO: Should throw an error
      
      out.open(file);
      if(out.fail())
        return; // TODO: Should throw an error
    }

  public:
    dumper_t() : writing_time(Time::zero) {}
    void dump() {
      Time start;
      _dump();
      Time end;
      writing_time += end - start;
    }
    virtual void _dump() = 0;
    Time get_writing_time() const { return writing_time; }
    virtual ~dumper_t() {};
  };
}
#endif // __DUMPER_HPP__
