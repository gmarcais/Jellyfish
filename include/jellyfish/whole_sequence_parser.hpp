#ifndef __JELLYFISH_WHOLE_SEQUENCE_PARSER_HPP__
#define __JELLYFISH_WHOLE_SEQUENCE_PARSER_HPP__

#include <string>
#include <memory>

#include <jellyfish/err.hpp>
#include <jellyfish/cooperative_pool2.hpp>

namespace jellyfish {
struct sequence_strings {
  std::string header;
  std::string seq;
  std::string qual;
};

template<typename StreamIterator>
class whole_sequence_parser : public jellyfish::cooperative_pool2<whole_sequence_parser<StreamIterator>, sequence_strings> {
  typedef jellyfish::cooperative_pool2<whole_sequence_parser<StreamIterator>, sequence_strings> super;
  typedef std::unique_ptr<std::istream> stream_type;
  enum file_type { DONE_TYPE, FASTA_TYPE, FASTQ_TYPE };

  struct stream_status {
    file_type   type;
    std::string buffer;
    stream_type stream;
    stream_status() : type(DONE_TYPE) { }
  };
  std::vector<stream_status> streams_;
  StreamIterator&            streams_iterator_;

public:
  /// Size is the number of buffers to keep around. It should be
  /// larger than the number of thread expected to read from this
  /// class. 'begin' and 'end' are iterators to a range of istream.
  whole_sequence_parser(uint32_t size, uint32_t max_producers, StreamIterator& streams) :
    super(max_producers, size),
    streams_(max_producers),
    streams_iterator_(streams)
  {
    for(uint32_t i = 0; i < max_producers; ++i)
      open_next_file(streams_[i]);
  }

  inline bool produce(uint32_t i, sequence_strings& buff) {
    stream_status& st = streams_[i];

    switch(st.type) {
    case FASTA_TYPE:
      read_fasta(st, buff);
      break;
    case FASTQ_TYPE:
      read_fastq(st, buff);
      break;
    case DONE_TYPE:
      return true;
    }

    if(*st.stream)
      return false;

    // Reach the end of file, close current and try to open the next one
    open_next_file(st);
    return false;
  }

protected:
  void open_next_file(stream_status& st) {
    st.stream.reset();
    st.stream = streams_iterator_.next();
    if(!st.stream) {
      st.type = DONE_TYPE;
      return;
    }

    // Update the type of the current file and move past first header
    // to beginning of sequence.
    switch(st.stream->peek()) {
    case EOF: return open_next_file(st);
    case '>':
      st.type = FASTA_TYPE;
      break;
    case '@':
      st.type = FASTQ_TYPE;
      break;
    default:
      eraise(std::runtime_error) << "Unsupported format"; // Better error management
    }
  }

  void read_fasta(stream_status& st, sequence_strings& buff) {
    st.stream->get(); // Skip '>'
    std::getline(*st.stream, buff.header);
    buff.seq.clear();
    while(st.stream->peek() != '>' && st.stream->peek() != EOF) {
      std::getline(*st.stream, st.buffer); // Wish there was an easy way to combine the
      buff.seq.append(st.buffer);             // two lines avoiding copying
    }
  }

  void read_fastq(stream_status& st, sequence_strings& buff) {
    st.stream->get(); // Skip '@'
    std::getline(*st.stream, buff.header);
    buff.seq.clear();
    while(st.stream->peek() != '+' && st.stream->peek() != EOF) {
      std::getline(*st.stream, st.buffer); // Wish there was an easy way to combine the
      buff.seq.append(st.buffer);             // two lines avoiding copying
    }
    if(!*st.stream)
      eraise(std::runtime_error) << "Truncated fastq file";
    st.stream->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    buff.qual.clear();
    while(buff.qual.size() < buff.seq.size() && st.stream->good()) {
      std::getline(*st.stream, st.buffer);
      buff.qual.append(st.buffer);
    }
    if(buff.qual.size() != buff.seq.size())
      eraise(std::runtime_error) << "Invalid fastq file: wrong number of quals";
    if(st.stream->peek() != EOF && st.stream->peek() != '@')
      eraise(std::runtime_error) << "Invalid fastq file: header missing";
  }
};
} // namespace jellyfish

#endif /* __JELLYFISH_WHOLE_SEQUENCE_PARSER_HPP__ */
