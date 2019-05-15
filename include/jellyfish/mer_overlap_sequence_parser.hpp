/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifndef __JELLYFISH_MER_OVELAP_SEQUENCE_PARSER_H_
#define __JELLYFISH_MER_OVELAP_SEQUENCE_PARSER_H_

#include <stdint.h>

#include <memory>

#include <jellyfish/err.hpp>
#include <jellyfish/cooperative_pool2.hpp>
#include <jellyfish/cpp_array.hpp>
#include <jellyfish/parser_common.hpp>

namespace jellyfish {

struct sequence_ptr {
  char* start;
  char* end;
};

template<typename StreamIterator>
class mer_overlap_sequence_parser : public jellyfish::cooperative_pool2<mer_overlap_sequence_parser<StreamIterator>, sequence_ptr> {
  typedef jellyfish::cooperative_pool2<mer_overlap_sequence_parser<StreamIterator>, sequence_ptr> super;

  struct stream_status {
    char*       seam;
    size_t      seq_len;
    bool        have_seam;
    file_type   type;
    stream_type stream;

    stream_status() : seam(0), seq_len(0), have_seam(false), type(DONE_TYPE) { }
  };

  uint16_t                 mer_len_;
  size_t                   buf_size_;
  char*                    buffer;
  char*                    seam_buffer;
  locks::pthread::mutex    streams_mutex;
  char*                    data;
  cpp_array<stream_status> streams_;
  StreamIterator&          streams_iterator_;
  size_t                   files_read_; // nb of files read
  size_t                   reads_read_; // nb of reads read

public:
  /// Max_producers is the maximum number of concurrent threads than
  /// can produce data simultaneously. Size is the number of buffer to
  /// keep around. It should be larger than the number of thread
  /// expected to read from this class. buf_size is the size of each
  /// buffer. A StreamIterator is expected to have a next() method,
  /// which is thread safe, and which returns (move) a
  /// std::unique<std::istream> object.
  mer_overlap_sequence_parser(uint16_t mer_len, uint32_t max_producers, uint32_t size, size_t buf_size,
                              StreamIterator& streams) :
    super(max_producers, size),
    mer_len_(mer_len),
    buf_size_(buf_size),
    buffer(new char[size * buf_size]),
    seam_buffer(new char[max_producers * (mer_len - 1)]),
    streams_(max_producers),
    streams_iterator_(streams),
    files_read_(0), reads_read_(0)
  {
    for(sequence_ptr* it = super::element_begin(); it != super::element_end(); ++it)
      it->start = it->end = buffer + (it - super::element_begin()) * buf_size;
    for(uint32_t i = 0; i < max_producers; ++i) {
      streams_.init(i);
      streams_[i].seam = seam_buffer + i * (mer_len - 1);
      open_next_file(streams_[i]);
    }
  }

  ~mer_overlap_sequence_parser() {
    delete [] buffer;
    delete [] seam_buffer;
  }

  //  file_type get_type() const { return type; }

  inline bool produce(uint32_t i, sequence_ptr& buff) {
    stream_status& st = streams_[i];

    switch(st.type) {
    case FASTA_TYPE:
      read_fasta(st, buff);
      break;
    case FASTQ_TYPE:
      read_fastq(st, buff);
      break;
#ifdef HAVE_HTSLIB
    case SAM_TYPE:
      read_sam(st, buff);
      break;
#endif
    case DONE_TYPE:
      return true;
    }

    if(st.stream.good())
      return false;

    // Reach the end of file, close current and try to open the next one
    st.have_seam = false;
    open_next_file(st);
    return false;
  }

  size_t nb_files() const { return files_read_; }
  size_t nb_reads() const { return reads_read_; }

protected:
  bool open_next_file(stream_status& st) {
    // The stream must be released, with .reset(), before calling
    // .next() on the streams_iterator_, to ensure that the
    // streams_iterator_ noticed that we closed that stream before
    // requesting a new one.
    st.stream.reset();
    st.stream = std::move(streams_iterator_.next());
    if(!st.stream.good()) {
      st.type = DONE_TYPE;
      return false;
    }

    ++files_read_;
    if(st.stream.standard) {
      switch(st.stream.standard->peek()) {
      case EOF: return open_next_file(st);
      case '>':
        st.type = FASTA_TYPE;
        ignore_line(*st.stream.standard); // Pass header
        ++reads_read_;
        break;
      case '@':
        st.type = FASTQ_TYPE;
        ignore_line(*st.stream.standard); // Pass header
        ++reads_read_;
        break;
      default:
        throw std::runtime_error("Unsupported format"); // Better error management
      }
    }
#ifdef HAVE_HTSLIB
    else if(st.stream.sam) {
      st.type = SAM_TYPE;
    }
#endif
    else {
      st.type = DONE_TYPE;
    }
    return true;
  }

  void read_fasta(stream_status& st, sequence_ptr& buff) {
    auto& stream = *st.stream.standard;
    size_t read = 0;
    if(st.have_seam) {
      memcpy(buff.start, st.seam, mer_len_ - 1);
      read = mer_len_ - 1;
    }

    // Here, the current stream is assumed to always point to some
    // sequence (or EOF). Never at header.
    while(stream.good() && read < buf_size_ - mer_len_ - 1) {
      read += read_sequence(stream, read, buff.start, '>');
      if(stream.peek() == '>') {
        if(read > 0)
          *(buff.start + read++) = 'N'; // Add N between reads
        ignore_line(stream); // Skip to next sequence (skip headers, quals, ...)
        ++reads_read_;
      }
    }
    buff.end = buff.start + read;

    st.have_seam = read >= (size_t)(mer_len_ - 1);
    if(st.have_seam)
      memcpy(st.seam, buff.end - mer_len_ + 1, mer_len_ - 1);
  }

  void read_fastq(stream_status& st, sequence_ptr& buff) {
    auto& stream = *st.stream.standard;
    size_t read = 0;
    if(st.have_seam) {
      memcpy(buff.start, st.seam, mer_len_ - 1);
      read = mer_len_ - 1;
    }

    // Here, the st.stream is assumed to always point to some
    // sequence (or EOF). Never at header.
    while(stream.good() && read < buf_size_ - mer_len_ - 1) {
      char le;
      size_t nread  = read_sequence(stream, read, buff.start, '+', le);
      read         += nread;
      st.seq_len   += nread;
      if(stream.peek() == '+') {
        skip_quals(stream, st.seq_len, le);
        if(stream.good()) {
          *(buff.start + read++) = 'N'; // Add N between reads
          ignore_line(stream); // Skip sequence header
          ++reads_read_;
        }
        st.seq_len = 0;
      }
    }
    buff.end = buff.start + read;

    st.have_seam = read >= (size_t)(mer_len_ - 1);
    if(st.have_seam)
      memcpy(st.seam, buff.end - mer_len_ + 1, mer_len_ - 1);
  }

#ifdef HAVE_HTSLIB
  void read_sam(stream_status& st, sequence_ptr& buff) {
    auto&        stream    = *st.stream.sam;
    size_t read = 0;
    if(st.have_seam) {
      memcpy(buff.start, st.seam, mer_len_ - 1);
      read = mer_len_ - 1;
    }

    // std.seq_len is the amount of sequence left in the stream buffer
    // to read. When st.seq_len==0, we need to get the next sequence
    // from the stream.
    auto seq = buff.start;
    while(read < buf_size_ - mer_len_ - 1) {
      if(st.seq_len == 0) {
        if(stream.next() < 0)
          break;
        st.seq_len = stream.seq_len();
        if(read > 0)
          seq[read++] = 'N';
        ++reads_read_;
      }
      const size_t start = stream.seq_len() - st.seq_len;
      const size_t limit = std::min(st.seq_len, buf_size_ - 1 - read) + start;
      for(size_t i = start; i < limit; ++i, ++read)
        seq[read] = stream.base(i);
      st.seq_len -= (limit - start);
    }
    buff.end = buff.start + read;

    st.have_seam = read >= (size_t)(mer_len_ - 1);
    if(st.have_seam)
      memcpy(st.seam, buff.end - mer_len_ + 1, mer_len_ - 1);
  }
#endif

  inline size_t read_sequence(std::istream& is, const size_t read, char* const start, const char stop) {
    char le;
    return read_sequence(is, read, start, stop, le);
  }

  size_t read_sequence(std::istream& is, const size_t read, char* const start, const char stop, char& le) {
    size_t nread = read;
    le = '\n';

    skip_newlines(is); // Skip new lines -> get below doesn't like them
    while(is && nread < buf_size_ - 1 && is.peek() != stop) {
      is.get(start + nread, buf_size_ - nread);
      nread += is.gcount();
      while(nread > 0 && *(start + nread - 1) == '\r') {
        le = '\r';
        nread--;
      }
      skip_newlines(is);
    }
    return nread - read;
  }

  inline void ignore_line(std::istream& is) {
    is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  inline void skip_newlines(std::istream& is) {
    while(is.peek() == '\n' || is.peek() == '\r')
      is.get();
  }

  // Skip quals header and qual values (read_len) of them.
  void skip_quals(std::istream& is, size_t read_len, char le) {
    ignore_line(is);
    size_t quals = 0;

    skip_newlines(is);
    while(is.good() && quals < read_len) {
      is.ignore(read_len - quals + 1, le);
      quals += is.gcount();
      if(is)
        ++read_len;
      skip_newlines(is);
    }
    skip_newlines(is);
    if(quals == read_len && (is.peek() == '@' || is.peek() == EOF))
      return;

    throw std::runtime_error("Invalid fastq sequence");
  }

  char peek(std::istream& is) { return is.peek(); }
};
}

#endif /* __JELLYFISH_MER_OVELAP_SEQUENCE_PARSER_H_ */
