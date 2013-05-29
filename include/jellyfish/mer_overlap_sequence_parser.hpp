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

#ifndef __JELLYFISH_MER_OVELAP_SEQUENCE_PARSER_H_
#define __JELLYFISH_MER_OVELAP_SEQUENCE_PARSER_H_

#include <stdint.h>
#include <jellyfish/err.hpp>
#include <jellyfish/cooperative_pool.hpp>

namespace jellyfish {

struct sequence_ptr {
  char* start;
  char* end;
};

template<typename StreamIterator>
class mer_overlap_sequence_parser : public jellyfish::cooperative_pool<mer_overlap_sequence_parser<StreamIterator>, sequence_ptr> {
  typedef jellyfish::cooperative_pool<mer_overlap_sequence_parser<StreamIterator>, sequence_ptr> super;
  enum file_type { DONE_TYPE, FASTA_TYPE, FASTQ_TYPE };

  uint16_t                       mer_len_;
  size_t                         buf_size_;
  char*                          buffer;
  char*                          seam_buffer;
  bool                           have_seam;
  file_type                      type;
  StreamIterator                 current_file;
  StreamIterator                 stream_end;
  size_t                         current_seq_len;

public:
  /// Size is the number of buffer to keep around. It should be larger
  /// than the number of thread expected to read from this
  /// class. buf_size is the size of each buffer. 'begin' and 'end'
  /// are iterators to a range of istream.
  mer_overlap_sequence_parser(uint16_t mer_len, uint32_t size, size_t buf_size,
                              StreamIterator begin, StreamIterator end) :
    super(size),
    mer_len_(mer_len),
    buf_size_(buf_size),
    buffer(new char[size * buf_size]),
    seam_buffer(new char[mer_len - 1]),
    have_seam(false),
    type(DONE_TYPE),
    current_file(begin),
    stream_end(end),
    current_seq_len(0)
  {
    for(sequence_ptr* it = super::element_begin(); it != super::element_end(); ++it)
      it->start = it->end = buffer + (it - super::element_begin()) * buf_size;
    if(current_file != stream_end)
      update_type();
  }

  ~mer_overlap_sequence_parser() {
    delete [] buffer;
    delete [] seam_buffer;
  }

  file_type get_type() const { return type; }

  inline bool produce(sequence_ptr& buff) {
    switch(type) {
    case FASTA_TYPE:
      read_fasta(buff);
      return false;
    case FASTQ_TYPE:
      read_fastq(buff);
      return false;
    case DONE_TYPE:
      return true;
    }
    return true;
  }

protected:
  void open_next_file() {
    if(++current_file == stream_end) {
      type = DONE_TYPE;
      return;
    }
    update_type();
  }

  /// Update the type of the current file and move past first header
  /// to beginning of sequence.
  void update_type() {
    switch(current_file->peek()) {
    case EOF: return open_next_file();
    case '>':
      type = FASTA_TYPE;
      ignore_line(); // Pass header
      break;
    case '@':
      type = FASTQ_TYPE;
      ignore_line(); // Pass header
      break;
    default:
      eraise(std::runtime_error) << "Unsupported format"; // Better error management
    }
  }

  void read_fasta(sequence_ptr& buff) {
    size_t read = 0;
    if(have_seam) {
      memcpy(buff.start, seam_buffer, mer_len_ - 1);
      read = mer_len_ - 1;
    }

    // Here, the current_file is assumed to always point to some
    // sequence (or EOF). Never at header.
    while(current_file->good() && read < buf_size_ - mer_len_ - 1) {
      read += read_sequence(read, buff.start, '>');
      if(current_file->peek() == '>') {
        *(buff.start + read++) = 'N'; // Add N between reads
        ignore_line(); // Skip to next sequence (skip headers, quals, ...)
      }
    }
    buff.end = buff.start + read;

    if(!*current_file) {
      have_seam = false;
      open_next_file();
      return;
    }
    have_seam = read >= (size_t)(mer_len_ - 1);
    if(have_seam)
      memcpy(seam_buffer, buff.end - mer_len_ + 1, mer_len_ - 1);
  }

  void read_fastq(sequence_ptr& buff) {
    size_t read = 0;
    if(have_seam) {
      memcpy(buff.start, seam_buffer, mer_len_ - 1);
      read = mer_len_ - 1;
    }

    // Here, the current_file is assumed to always point to some
    // sequence (or EOF). Never at header.
    while(*current_file && read < buf_size_ - mer_len_ - 1) {
      size_t nread     = read_sequence(read, buff.start, '+');
      read            += nread;
      current_seq_len += nread;
      if(current_file->peek() == '+') {
        skip_quals(current_seq_len);
        if(*current_file) {
          *(buff.start + read++) = 'N'; // Add N between reads
          ignore_line(); // Skip sequence header
        }
        current_seq_len = 0;
      }
    }
    buff.end = buff.start + read;

    if(!*current_file) {
      have_seam       = false;
      current_seq_len = 0;
      open_next_file();
      return;
    }
    have_seam = read >= (size_t)(mer_len_ - 1);
    if(have_seam)
      memcpy(seam_buffer, buff.end - mer_len_ + 1, mer_len_ - 1);
  }

  size_t read_sequence(const size_t read, char* const start, const char stop) {
    size_t nread = read;
    while(*current_file && nread < buf_size_ - 1 && current_file->peek() != stop) {
      // Skip new lines -> get below does like them
      skip_newlines();
      current_file->get(start + nread, buf_size_ - nread);
      nread += current_file->gcount();
      skip_newlines();
    }
    return nread - read;
  }

  inline void ignore_line() {
    current_file->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  inline void skip_newlines() {
    while(current_file->peek() == '\n')
      current_file->get();
  }

  // Skip quals header and qual values (read_len) of them.
  void skip_quals(size_t read_len) {
    ignore_line();
    size_t quals = 0;
    while(current_file->good() && quals < read_len) {
      skip_newlines();
      current_file->ignore(read_len - quals + 1, '\n');
      quals += current_file->gcount();
      if(*current_file)
        ++read_len;
    }
    skip_newlines();
    if(quals == read_len && (peek() == '@' || peek() == EOF))
      return;

    eraise(std::runtime_error) << "Invalid fastq sequence";
  }

  char peek() { return current_file->peek(); }
};
}

#endif /* __JELLYFISH_MER_OVELAP_SEQUENCE_PARSER_H_ */
