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

#include <jellyfish/seq_qual_parser.hpp>
#include <stddef.h>

jellyfish::seq_qual_parser *
jellyfish::seq_qual_parser::new_parser(const char *path) {
  int fd;
  char peek;

  fd = file_peek(path, &peek);

  switch(peek) {
  case '@': return new fastq_seq_qual_parser(fd, path, &peek, 1);
      
  default:
    eraise(FileParserError) << "'" << path << "': "
                            << "Invalid input file. Expected fastq";
  }

  return 0;
}


bool jellyfish::fastq_seq_qual_parser::parse(char *start, char **end) {
  char *qual_start = start + 1;
  // Make sure even length
  *end -= (*end - start) & 0x1;
  if(!_read_buf.empty()) {
    // Copy the saved sequence, then the quality score for this
    // saved sequence. The stream is left at the first qual value.
    if((*end - start) / 2 < (ptrdiff_t)_read_buf.size())
      eraise(FastqSeqQualParserError) << "Buffer is too small to "
        "contain an entire read and its qual values";
    qual_start = start + 1;
    for(const char *b = _read_buf.begin(); b < _read_buf.end(); b++, start += 2)
      *start = *b;
    _read_buf.reset();
    copy_qual_values(qual_start, start);
  }
      
  while(start < *end) {
    // Read sequence header
    bool found_seq_header = false;
    while(!found_seq_header) {
      switch(sbumpc()) {
      case EOF:
        goto done;
      case '\n':
        break;
      case '@':
        found_seq_header = pbase() == '\n';
        if(found_seq_header)
          break;
        // fall through
      default:
        eraise(FastqSeqQualParserError) << "Unexpected character '" << base()
                                        << "'. Expected '@'";
      }
    }
    // skip seq header
    while(base() != EOF && base() != '\n') { sbumpc(); }
    *start = 'N';
    start += 2;
    qual_start = start + 1;

    // copy sequence
    bool found_qual_header = false;
    //      char *save_start = start;
    //      std::cerr << "save_start " << save_start << std::endl;
    while(start < *end && !found_qual_header) {
      switch(sbumpc()) {
      case EOF:
        eraise(FastqSeqQualParserError) << "Truncated input file";
      case '\n':
        break;
      case '+':
        if((found_qual_header = pbase() == '\n'))
          break;
        // fall through regular character
      default:
        *start = base();
        start += 2;
      }
    }
      
    if(!found_qual_header) { // copy extra sequence to read buffer
      while(!found_qual_header) {
        switch(sbumpc()) {
        case EOF:
          eraise(FastqSeqQualParserError) << "Truncated input file";
        case '\n':
          break;
        case '+':
          if((found_qual_header = pbase() == '\n'))
            break;
        default:
          _read_buf.push_back(base());
        }
      }
    }
    // skip qual header && copy qual values
    while(base() != EOF && base() != '\n') { sbumpc(); }
    //      std::cerr << "Copy qual values" << std::endl;
    copy_qual_values(qual_start, start);
  }

 done:
  *end = start;
  return base() != EOF;
}

void jellyfish::fastq_seq_qual_parser::copy_qual_values(char *&qual_start, const char *start) {
  while(qual_start < start + 1) {
    switch(sbumpc()) {
    case EOF:
      eraise(FastqSeqQualParserError) << "Truncated input file";
    case '\n':
      break;
    default:
      *qual_start = base();
      qual_start += 2;
    }
  }
}
