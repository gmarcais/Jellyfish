#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>

#include <jellyfish/mer_overlap_sequence_parser.hpp>
#include <jellyfish/thread_exec.hpp>
#include <jellyfish/intersection_array.hpp>
#include <jellyfish/mer_dna.hpp>
#include <jellyfish/locks_pthread.hpp>
#include <jellyfish/mer_iterator.hpp>
#include <jflib/multiplexed_io.hpp>
#include <sub_commands/intersection_main_cmdline.hpp>

typedef jellyfish::intersection_array<jellyfish::mer_dna> inter_array;
typedef std::vector<const char*> file_vector;
typedef jellyfish::mer_overlap_sequence_parser<std::ifstream*> sequence_parser;
typedef jellyfish::mer_iterator<sequence_parser, jellyfish::mer_dna> mer_iterator;
typedef std::map<std::string, int> name_map;

intersection_main_cmdline args;


class compute_intersection : public jellyfish::thread_exec {
  int                     nb_threads_;
  locks::pthread::barrier barrier_;
  inter_array&            ary_;
  file_vector&            files_;
  name_map                map_;

  sequence_parser* volatile      parser_;
  jflib::o_multiplexer* volatile multiplexer_;

public:
  compute_intersection(int nb_threads, inter_array& ary, file_vector& files) :
    nb_threads_(nb_threads),
    barrier_(nb_threads),
    ary_(ary),
    files_(files)
  { }

  virtual void start(int thid) {
    load_in_files(thid);
    output_intersection(thid);
    output_uniq(thid);
  }

  void load_in_files(int thid) {
    unsigned int   file_index = 0;
    std::ifstream* file       = 0;

    while(true) {
      if(thid == 0) {
        if(file_index != files_.size()) {
          unsigned int cindex = file_index++;
          file                = new std::ifstream(files_[cindex]);
          parser_             = new sequence_parser(jellyfish::mer_dna::k(), 3 * nb_threads_, 4096, file, file + 1);
          if(args.verbose_flag)
            std::cout << "Load in file '" << files_[cindex] << "'\n";
        }
      }
      barrier_.wait();
      if(!parser_)
        break;

      add_mers_to_ary(thid);
      barrier_.wait();
      if(thid == 0) {
        delete parser_;
        parser_ = 0;
        delete file;
        file    = 0;
      }
    }
  }

  void add_mers_to_ary(int thid) {
    for(mer_iterator mers(*parser_, args.canonical_flag) ; mers; ++mers)
      ary_.add(*mers);
    ary_.done();
    ary_.postprocess(thid);
  }


  void output_intersection(int thid) {
    std::ofstream* file_out = 0;
    if(thid == 0) {
      file_out     = new std::ofstream(args.intersection_arg);
      multiplexer_ = new jflib::o_multiplexer(file_out, 3 * nb_threads_, 4096);
      if(args.verbose_flag)
        std::cout << "Writing intersection mers in '" << args.intersection_arg << "'\n";
    }

    barrier_.wait();
    output_intersection_mers(thid);
    barrier_.wait();
    if(thid == 0) {
      delete multiplexer_;
      multiplexer_ = 0;
      delete file_out;
    }
  }

  void output_intersection_mers(int thid) {
    jflib::omstream out(*multiplexer_);
    inter_array::array::lazy_iterator it = ary_.ary()->lazy_slice(thid, nb_threads_);
    while(out && it.next()) {
      inter_array::mer_info info = ary_.info_at(it.id());
      if(!info.info.nall) {
        out << it.key() << "\n";
        out << jflib::endr;
      }
    }
  }

  void output_uniq(int thid) {
    unsigned int          file_index  = 0;
    std::ifstream*        file        = 0;
    std::ofstream*        out         = 0;

    while(true) {
      if(thid == 0) {
        if(file_index != files_.size()) {
          unsigned int cindex    = file_index++;
          file                   = new std::ifstream(files_[cindex]);
          parser_                = new sequence_parser(jellyfish::mer_dna::k(), 3 * nb_threads_, 4096, file, file + 1);
          std::ostringstream  out_name;
          out_name << args.prefix_arg;
          const char* basename   = strrchr(files_[cindex], '/');
          if(basename == 0)
            basename = files_[cindex];
          else
            ++basename;
          int count = map_[basename]++;
          if(count > 0)
            out_name << count << "_";
          out_name << basename;
          out          = new std::ofstream(out_name.str().c_str());
          multiplexer_ = new jflib::o_multiplexer(out, 3 * nb_threads_, 4096);
          if(args.verbose_flag)
            std::cout << "Writing unique mers of '" << files_[cindex] << "' to '" << out_name.str() << "'\n";
        }
      }
      barrier_.wait();
      if(!parser_)
        break;

      output_uniq_mers_file(thid);
      barrier_.wait();
      if(thid == 0) {
        delete multiplexer_;
        multiplexer_ = 0;
        delete out;
        out          = 0;
        delete parser_;
        parser_      = 0;
        delete file;
        file         = 0;
      }
    }
  }

  void output_uniq_mers_file(int thid) {
    jflib::omstream out(*multiplexer_);

    for(mer_iterator mers(*parser_, args.canonical_flag); mers; ++mers) {
      inter_array::mer_info info = ary_[*mers];
      if(!info.info.mult) {
        out << *mers << "\n";
        out << jflib::endr;
      }
    }
  }
};

int intersection_main(int argc, char* argv[]) {
  args.parse(argc, argv);
  jellyfish::mer_dna::k(args.mer_arg);

  if(!args.size_given && !args.mem_given)
    args.error("One of [-s, --size] or --mem is required");

  if(args.mem_info_flag) {
    inter_array::usage_info info(args.mer_arg);
    if(args.size_given)
      std::cout << info.mem(args.size_arg) << "\n";
    else
      std::cout << info.size(args.mem_arg) << "\n";
    exit(EXIT_SUCCESS);
  }

  if(args.genome_arg.size() < 2)
    args.error("Requires at least 2 arguments.");

  if(args.mem_given) {
    inter_array::usage_info info(args.mer_arg);
    args.size_arg = info.size(args.mem_arg);
  }

  inter_array ary(args.size_arg, jellyfish::mer_dna::k() * 2, args.thread_arg);
  compute_intersection workers(args.thread_arg, ary, args.genome_arg);
  workers.exec_join(args.thread_arg);

  return EXIT_SUCCESS;
}
