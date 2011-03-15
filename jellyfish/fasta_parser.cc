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

#include <jellyfish/fasta_parser.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

namespace jellyfish {
  bool fasta_parser::parse(char *start, char **end) {
    int base = '\n';
    int pbase = '\n';

    while(start < *end && base != EOF) {
      pbase = base;
      base  = sbumpc();
      switch(base) {
      case EOF:
        break;

      case '>':
        if(pbase == '\n') {
          while(base != EOF && base != '\n') { base = sbumpc(); }
          *start++ = 'N';
        } else
          *start++ = base;
        break;

      case '\n':
        break;

      default:
        *start++ = base;
      }
    }

    *end = start;
    return base != EOF;
  }
}
