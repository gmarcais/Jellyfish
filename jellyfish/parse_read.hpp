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

#ifndef __JELLYFISH_FASTQ_READ_PARSER_HPP__
#define __JELLYFISH_FASTQ_READ_PARSER_HPP__

#include <vector>
#include <limits>

#include <jellyfish/double_fifo_input.hpp>
#include <jellyfish/read_parser.hpp>
#include <jellyfish/misc.hpp>
#include <jellyfish/dbg.hpp>

namespace jellyfish {
  class parse_read : public double_fifo_input<read_parser::reads_t> {
  private:
    typedef std::vector<const char *> fary_t;

    fary_t                  files;
    fary_t::const_iterator  current_file;
    bool                    canonical;
    read_parser            *fparser;

  public:
    typedef read_parser::read_t read_t;

    static const uint_t codes[256];
    static const uint_t CODE_RESET = (uint_t)-1;

    //    parse_read(int nb_files, char *argv[], unsigned int nb_buffers);
    template<typename T>
    parse_read(T argv_start, T argv_end, unsigned int nb_buffers);
    template<typename T>
    parse_read(int argc, T argv, unsigned int nb_buffers);
    ~parse_read() { }

    void set_canonical(bool v = true) { canonical = v; }
    virtual void fill();

    class thread {
      parse_read *parser;
      bucket_t   *sequence;
      int         current_read;

    public:
      explicit thread(parse_read *_parser) :
        parser(_parser), sequence(parser->next()),
        current_read(0) {}

      read_parser::read_t * next_read() {
        while(sequence) {
          if(current_read < sequence->nb_reads)
            return &sequence->reads[current_read++];

          sequence->unlink(); // unmap file if not used anymore
          parser->release(sequence);
          sequence     = parser->next();
          current_read = 0;
        }
        return 0;
      }

    private:
      bool next_sequence();
    };
    friend class thread;
    thread new_thread() { return thread(this); }
  };
}

template<typename T>
jellyfish::parse_read::parse_read(T argv_start, T argv_end, unsigned int nb_buffers) :
  double_fifo_input<read_parser::reads_t>(nb_buffers), 
  files(argv_start, argv_end),
  current_file(files.begin()),
  fparser(read_parser::new_parser(*current_file))
{ 
  fparser->link();
}

template<typename T>
jellyfish::parse_read::parse_read(int argc, T argv, unsigned int nb_buffers) :
  double_fifo_input<read_parser::reads_t>(nb_buffers), 
  files(argv, argv + argc),
  current_file(files.begin()),
  fparser(read_parser::new_parser(*current_file))
{ 
  fparser->link();
}

#endif
