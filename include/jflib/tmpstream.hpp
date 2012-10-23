/*  This file is part of Jflib.

    Jflib is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jflib is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jflib.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __TMP_FILE_H_
#define __TMP_FILE_H_

#include <ext/stdio_filebuf.h>
#include <stdio.h>
#include <stdexcept>

namespace jflib {

/** Temporary file stream. Wrapper around FILE* tmpfile().
 */

template<typename _CharT, typename _Traits = std::char_traits<_CharT> >
class basic_tmpstream : public std::iostream {
  typedef __gnu_cxx::stdio_filebuf<_CharT> stdbuf;
public:
  basic_tmpstream() : std::iostream(open_tmp()) { }
  virtual ~basic_tmpstream() { close(); delete rdbuf(); }
  bool is_open() { return ((stdbuf*)rdbuf())->is_open(); }
  void close() {
    if(is_open()) {
      stdbuf* buf = (stdbuf*)rdbuf();
      ::fclose(buf->file());
      buf->close();
    }
  }
private:
  static stdbuf * open_tmp() {
    FILE *f = tmpfile();
    if(!f)
      throw std::runtime_error("Failed to create a temporary file.");
    return new stdbuf(f, std::ios::in|std::ios::out);
  }
};
typedef basic_tmpstream<char> tmpstream;

} // namespace jflib
#endif /* __TMP_FILE_H_ */
