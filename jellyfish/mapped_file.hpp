#ifndef __MAPPED_FILE_HPP__
#define __MAPPED_FILE_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <vector>
#include "misc.hpp"

class mapped_file {
  char   *_base, *_end;
  size_t  _length;
  bool    unmap;

public:
  mapped_file(char *__base, size_t __length) :
    _base(__base), _end(__base + __length), _length(__length), unmap(false) {}

  mapped_file(const char *filename) {
    int fd = open(filename, O_RDONLY);
    struct stat stat;

    if(fd < 0)
      throw StandardError(errno, "Can't open file '%s'", filename);
    
    if(fstat(fd, &stat) < 0)
      throw StandardError(errno, "Can't stat file '%s'", filename);

    _length = stat.st_size;
    _base = (char *)mmap(NULL, _length, PROT_READ, MAP_SHARED, fd, 0);
    if(_base == MAP_FAILED)
      throw StandardError(errno, "Can't mmap file '%s'", filename);
    close(fd);
    _end = _base + _length;

    unmap = true;
  }

  ~mapped_file() { }

  char *base() const { return _base; }
  char *end() const { return _end; }
  size_t length() const { return _length; }

  void will_need() const {
    madvise(_base, _length, MADV_WILLNEED);
  }
  void sequential() const {
    madvise(_base, _length, MADV_SEQUENTIAL);
  }
};

typedef std::vector<mapped_file> mapped_files_t;

#endif
