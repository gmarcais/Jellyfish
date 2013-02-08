#include <iostream>
#include <fstream>

#include <jellyfish/err.hpp>
#include <jellyfish/file_header.hpp>
#include <sub_commands/info_main_cmdline.hpp>

static info_main_cmdline args;

int info_main(int argc, char *argv[]) {
  args.parse(argc, argv);

  std::ifstream file(args.file_arg);
  if(!file.good())
    die << "Can't open '" << args.file_arg << "'" << jellyfish::err::no;

  jellyfish::generic_file_header header;
  header.read(file);

  if(args.skip_flag)
    std::cout << file.rdbuf();
  else
    std::cout << header;

  return 0;
}
