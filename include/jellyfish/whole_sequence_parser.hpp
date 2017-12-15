#ifndef __JELLYFISH_WHOLE_SEQUENCE_PARSER_HPP__
#define __JELLYFISH_WHOLE_SEQUENCE_PARSER_HPP__

#include <string>
#include <memory>

#include <jellyfish/err.hpp>
#include <jellyfish/cooperative_pool2.hpp>
#include <jellyfish/cpp_array.hpp>
#include <jellyfish/parser_common.hpp>

namespace jellyfish {
struct header_sequence_qual {
  std::string header;
  std::string seq;
  std::string qual;
};
struct sequence_list {
  size_t nb_filled;
  std::vector<header_sequence_qual> data;
};

template<typename StreamIterator>
class whole_sequence_parser : public jellyfish::cooperative_pool2<whole_sequence_parser<StreamIterator>, sequence_list> {
  typedef jellyfish::cooperative_pool2<whole_sequence_parser<StreamIterator>, sequence_list> super;

  struct stream_status {
    file_type   type;
    std::string buffer;
    stream_type stream;
    stream_status() : type(DONE_TYPE) { }
  };
  cpp_array<stream_status> streams_;
  StreamIterator&          streams_iterator_;
  size_t                   files_read_; // nb of files read
  size_t                   reads_read_; // nb of reads read


public:
  /// Size is the number of buffers to keep around. It should be
  /// larger than the number of thread expected to read from this
  /// class. nb_sequences is the number of sequences to read into a
  /// buffer. 'begin' and 'end' are iterators to a range of istream.
  whole_sequence_parser(uint32_t size, uint32_t nb_sequences,
                        uint32_t max_producers, StreamIterator& streams) :
    super(max_producers, size),
    streams_(max_producers),
    streams_iterator_(streams),
    files_read_(0),
    reads_read_(0)
  {
    for(auto it = super::element_begin(); it != super::element_end(); ++it) {
      it->nb_filled = 0;
      it->data.resize(nb_sequences);
    }
    for(uint32_t i = 0; i < max_producers; ++i) {
      streams_.init(i);
      open_next_file(streams_[i]);
    }
  }

  inline bool produce(uint32_t i, sequence_list& buff) {
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
    open_next_file(st);
    return false;
  }

  size_t nb_files() const { return files_read_; }
  size_t nb_reads() const { return reads_read_; }

protected:
  void open_next_file(stream_status& st) {
    st.stream.reset();
    st.stream = std::move(streams_iterator_.next());
    if(!st.stream.good()) {
      st.type = DONE_TYPE;
      return;
    }

    ++files_read_;
    // Update the type of the current file and move past first header
    // to beginning of sequence.
    if(st.stream.standard) {
      switch(st.stream.standard->peek()) {
      case EOF: return open_next_file(st);
      case '>':
        st.type = FASTA_TYPE;
        break;
      case '@':
        st.type = FASTQ_TYPE;
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
  }

  void read_fasta(stream_status& st, sequence_list& buff) {
    size_t&      nb_filled = buff.nb_filled;
    const size_t data_size = buff.data.size();
    auto&        stream    = *st.stream.standard;

    for(nb_filled = 0; nb_filled < data_size && stream.peek() != EOF; ++nb_filled) {
      ++reads_read_;
      header_sequence_qual& fill_buff = buff.data[nb_filled];
      stream.get(); // Skip '>'
      std::getline(stream, fill_buff.header);
      fill_buff.seq.clear();
      for(int c = stream.peek(); c != '>' && c != EOF; c = stream.peek()) {
        std::getline(stream, st.buffer); // Wish there was an easy way to combine the
        fill_buff.seq.append(st.buffer); // two lines avoiding copying
      }
    }
  }

  void read_fastq(stream_status& st, sequence_list& buff) {
    size_t&      nb_filled = buff.nb_filled;
    const size_t data_size = buff.data.size();
    auto&        stream    = *st.stream.standard;

    for(nb_filled = 0; nb_filled < data_size && stream.peek() != EOF; ++nb_filled) {
      ++reads_read_;
      header_sequence_qual& fill_buff = buff.data[nb_filled];
      stream.get(); // Skip '@'
      std::getline(stream, fill_buff.header);

      if(stream.peek() != '+')
        std::getline(stream, fill_buff.seq);
      else
        fill_buff.seq.clear();
      while(stream.peek() != '+' && stream.peek() != EOF) {
        std::getline(stream, st.buffer); // Wish there was an easy way to combine the
        fill_buff.seq.append(st.buffer);             // two lines avoiding copying
      }
      if(!stream.good())
        throw std::runtime_error("Truncated fastq file");
      stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      if(stream.peek() != '+')
        std::getline(stream, fill_buff.qual);
      else
        fill_buff.qual.clear();
      while(fill_buff.qual.size() < fill_buff.seq.size() && stream.good()) {
        std::getline(stream, st.buffer);
        fill_buff.qual.append(st.buffer);
      }
      if(fill_buff.qual.size() != fill_buff.seq.size())
        throw std::runtime_error("Invalid fastq file: wrong number of quals");
      if(stream.peek() != EOF && stream.peek() != '@')
        throw std::runtime_error("Invalid fastq file: header missing");
      // std::cerr << nb_filled << ": " << fill_buff.seq << '\n'
      //           << nb_filled << ": " << fill_buff.qual << '\n';
    }
  }

#ifdef HAVE_HTSLIB
  void read_sam(stream_status& st, sequence_list& buff) {
    size_t&      nb_filled = buff.nb_filled;
    const size_t data_size = buff.data.size();
    auto&        stream    = *st.stream.sam;

    for(nb_filled = 0; nb_filled < data_size && stream.next() >= 0; ++nb_filled) {
      ++reads_read_;
      header_sequence_qual& fill_buff = buff.data[nb_filled];
      fill_buff.header = stream.qname();
      auto& seq  = fill_buff.seq;
      auto& qual = fill_buff.qual;
      seq.resize(stream.seq_len());
      qual.resize(stream.seq_len());
      for(ssize_t i = 0; i < stream.seq_len(); ++i) {
        seq[i] = stream.base(i);
        qual[i] = stream.qual(i) + '!';
      }
    }
  }
#endif
};
} // namespace jellyfish

#endif /* __JELLYFISH_WHOLE_SEQUENCE_PARSER_HPP__ */
