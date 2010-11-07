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

#ifndef __JELLYFISH_MAPPED_FILE_HPP__
#define __JELLYFISH_MAPPED_FILE_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <vector>
#include <errno.h>

#include <jellyfish/misc.hpp>

class mapped_file {
  char   *_base, *_end;
  size_t  _length;
  bool    unmap;

public:
  define_error_class(ErrorMMap);
  mapped_file(char *__base, size_t __length) :
    _base(__base), _end(__base + __length), _length(__length), unmap(false) {}

  mapped_file(const char *filename) {
    int fd = open(filename, O_RDONLY);
    struct stat stat;

    if(fd < 0)
      throw_perror<ErrorMMap>("Can't open file '%s'", filename);
    
    if(fstat(fd, &stat) < 0)
      throw_perror<ErrorMMap>("Can't stat file '%s'", filename);

    _length = stat.st_size;
    _base = (char *)mmap(NULL, _length, PROT_READ, MAP_SHARED, fd, 0);
    if(_base == MAP_FAILED)
      throw_perror<ErrorMMap>("Can't mmap file '%s'", filename);
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

class mapped_files_t : public std::vector<mapped_file> {
public:
  mapped_files_t(int nb_files, char *argv[]) {
    for(int j = 0; j < nb_files; j++)
      push_back(mapped_file(argv[j]));
  }

  mapped_files_t(int nb_files, char *argv[], bool sequential) {
    for(int j = 0; j < nb_files; j++) {
      push_back(mapped_file(argv[j]));
      if(sequential)
        end()->sequential();
    }
  }
};

#endif
