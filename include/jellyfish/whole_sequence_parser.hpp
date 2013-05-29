#ifndef __JELLYFISH_WHOLE_SEQUENCE_PARSER_HPP__
#define __JELLYFISH_WHOLE_SEQUENCE_PARSER_HPP__

#include <string>

#include <jellyfish/err.hpp>
#include <jellyfish/cooperative_pool.hpp>

namespace jellyfish {
struct sequence_strings {
  std::string header;
  std::string seq;
  std::string qual;
};

template<typename StreamIterator>
class whole_sequence_parser : public jellyfish::cooperative_pool<whole_sequence_parser<StreamIterator>, sequence_strings> {
  typedef jellyfish::cooperative_pool<whole_sequence_parser<StreamIterator>, sequence_strings> super;
  enum file_type { DONE_TYPE, FASTA_TYPE, FASTQ_TYPE };

  file_type      type;
  StreamIterator current_file;
  StreamIterator stream_end;
  std::string    buffer_;


public:
  /// Size is the number of buffers to keep around. It should be
  /// larger than the number of thread expected to read from this
  /// class. 'begin' and 'end' are iterators to a range of istream.
  whole_sequence_parser(uint32_t size,
                  StreamIterator begin, StreamIterator end) :
    super(size),
    type(DONE_TYPE),
    current_file(begin),
    stream_end(end)
  {
    if(current_file != stream_end)
      update_type();
  }

  file_type get_type() const { return type; }

  inline bool produce(sequence_strings& buff) {
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
      break;
    case '@':
      type = FASTQ_TYPE;
      break;
    default:
      eraise(std::runtime_error) << "Unsupported format"; // Better error management
    }
  }

  void read_fasta(sequence_strings& buff) {
    current_file->get(); // Skip '>'
    std::getline(*current_file, buff.header);
    buff.seq.clear();
    while(current_file->peek() != '>' && current_file->peek() != EOF) {
      std::getline(*current_file, buffer_); // Wish there was an easy way to combine the
      buff.seq.append(buffer_);             // two lines avoiding copying
    }

    if(!*current_file)
      open_next_file();
  }

  void read_fastq(sequence_strings& buff) {
    current_file->get(); // Skip '@'
    std::getline(*current_file, buff.header);
    buff.seq.clear();
    while(current_file->peek() != '+' && current_file->peek() != EOF) {
      std::getline(*current_file, buffer_); // Wish there was an easy way to combine the
      buff.seq.append(buffer_);             // two lines avoiding copying
    }
    if(!*current_file)
      eraise(std::runtime_error) << "Truncated fastq file";
    current_file->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    buff.qual.clear();
    while(buff.qual.size() < buff.seq.size() && current_file->good()) {
      std::getline(*current_file, buffer_);
      buff.qual.append(buffer_);
    }
    if(buff.qual.size() != buff.seq.size())
      eraise(std::runtime_error) << "Invalid fastq file: wrong number of quals";
    if(!*current_file || current_file->peek() == EOF) {
      open_next_file();
    } else if(current_file->peek() != '@') {
      eraise(std::runtime_error) << "Invalid fastq file: header missing";
    }
  }
};
} // namespace jellyfish

#endif /* __JELLYFISH_WHOLE_SEQUENCE_PARSER_HPP__ */
