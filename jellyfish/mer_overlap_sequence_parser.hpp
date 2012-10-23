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

#include <jellyfish/err.hpp>

namespace jellyfish {
#include <jflib/cooperative_pool.hpp>

struct sequence_ptr {
  char* start;
  char* end;
};

template<typename StreamIterator>
class mer_overlap_sequence_parser : public jflib::cooperative_pool<mer_overlap_sequence_parser<StreamIterator>, sequence_ptr> {
  typedef jflib::cooperative_pool<mer_overlap_sequence_parser<StreamIterator>, sequence_ptr> super;
  enum file_type { DONE_TYPE, FASTA_TYPE, FASTQ_TYPE };

  uint16_t                       mer_len_;
  size_t                         buf_size_;
  char*                          buffer;
  char*                          seam_buffer;
  bool                           have_seam;
  file_type                      type;
  StreamIterator                 current_file;
  StreamIterator                 stream_end;

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
    have_seam(false),
    seam_buffer(new char[mer_len - 1]),
    buffer(new char[size * buf_size]),
    type(DONE_TYPE),
    current_file(begin),
    stream_end(end)
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
      //    case '@': type = FASTQ_TYPE; break;
    default:
      eraise(std::runtime_error) << "Unsupported format"; // Better error management
    }
  }

  inline void ignore_line() {
    current_file->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  inline bool produce(sequence_ptr& buff) {
    switch(type) {
    case FASTA_TYPE:
      read_fasta(buff);
      return false;
    case FASTQ_TYPE:
      eraise(std::runtime_error) << "Type not yet supported";
      return false;
    case DONE_TYPE:
      return true;
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
    while(*current_file && read < buf_size_ - mer_len_ - 1) {
      while(*current_file && read < buf_size_ - 1 && current_file->peek() != '>') {
        // Skip new lines -> get below does like them
        while(current_file->peek() == '\n')
          current_file->get();
        current_file->get(buff.start + read, buf_size_ - read);
        read += current_file->gcount();
        while(current_file->peek() == '\n')
          current_file->get(); // Skip new line
      }
      if(current_file->peek() == '>') {
        *(buff.start + read++) = 'N'; // Add N between reads
        ignore_line(); // Skip header
      }
    }
    buff.end = buff.start + read;

    if(!*current_file) {
      have_seam = false;
      open_next_file();
      return;
    }
    have_seam = read >= mer_len_ - 1;
    if(have_seam)
      memcpy(seam_buffer, buff.end - mer_len_ + 1, mer_len_ - 1);
  }
};
}

#endif /* __JELLYFISH_MER_OVELAP_SEQUENCE_PARSER_H_ */
