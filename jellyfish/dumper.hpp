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

#ifndef __JELLYFISH_DUMPER_HPP__
#define __JELLYFISH_DUMPER_HPP__

#include <iostream>
#include <fstream>
#include <sstream>
#include <jellyfish/err.hpp>
#include <jellyfish/time.hpp>

/**
 * A dumper is responsible to dump the hash array to permanent storage
 * and zero out the array.
 **/
namespace jellyfish {
  class dumper_t {
    Time writing_time;
  public:
    define_error_class(ErrorWriting);

  protected:
    void open_next_file(const char *prefix, int *index, std::ofstream &out) {
      int eindex = atomic::gcc::fetch_add(index, (int)1);
      std::ostringstream file_path;
      file_path << prefix << "_" << eindex;

      out.open(file_path.str().c_str());
      if(out.fail())
        eraise(ErrorWriting) << "'" << file_path.str() << "': "
                             << "Can't open file for writing" << err::no;
    }

  public:
    dumper_t() : writing_time(::Time::zero) {}
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
