#include <iostream>
#include <string>
#include <string.h>
#include "lib/misc.hpp"

const char *argp_program_version = "jellyfish 1.0rc1";
const char *argp_program_bug_address = 
  "<guillaume@marcais.net> <carlk@umiacs.umd.edu>";

typedef int (main_func_t)(int argc, char *argv[]);

main_func_t count_main;
main_func_t stats_main;
main_func_t merge_main;
main_func_t sos;

struct cmd_func {
  std::string  cmd;
  main_func_t *func;
};
cmd_func cmd_list[] = {
  {"count",             &count_main},
  {"stats",             &stats_main},
  {"merge",             &merge_main},

  /* help in all its form. Must be first non-command */
  {"help",              &sos},
  {"-h",                &sos},
  {"-help",             &sos},
  {"--help",            &sos},
  {"-?",                &sos},

  {"",                  0}
};

int sos(int argc, char *argv[])
{
  std::cerr << "Usage: jellyfish <cmd> [options] arg..."  << std::endl <<
    "Where <cmd> is one of: ";
  bool comma = false;
  for(cmd_func *ccmd = cmd_list; ccmd->func != sos; ccmd++) {
    std::cerr << (comma ? ", " : "") << ccmd->cmd;
    comma = true;
  }
  std::cerr << "." << std::endl;
  return 0;
}

int main(int argc, char *argv[])
{
  std::string error;

  if(argc < 2) {
    error = "Too few arguments";
  } else {
    for(cmd_func *ccmd = cmd_list; ccmd->func != 0; ccmd++) {
      if(!ccmd->cmd.compare(argv[1])) {
        std::string name(argv[0]);
        name += " ";
        name += argv[1];
        argv[1] = strdup(name.c_str());
        return ccmd->func(argc - 1, argv + 1);
      }
    }
    error = "Unknown command '";
    error += argv[1];
    error += "'\n";
  }

  std::cerr << error << std::endl;
  sos(argc - 1, argv + 1);
  return 1;
}
