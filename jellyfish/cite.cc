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

const char *cite = 
  "A fast, lock-free approach for efficient parallel counting ofoccurrences of k-mers\n"
  "Guillaume Marcais; Carl Kingsford\n"
  "Bioinformatics 2011; doi: 10.1093/bioinformatics/btr011";

const char *url =
  "http://bioinformatics.oxfordjournals.org/content/early/2011/01/07/bioinformatics.btr011";

const char *bibtex = 
  "@article{Jellyfish2010,\n"
  "  author = {Mar\\c{c}ais, Guillaume and Kingsford, Carl},\n"
  "  title = {{A fast, lock-free approach for efficient parallel counting of occurrences of k-mers}},\n"
  "  doi = {10.1093/bioinformatics/btr011},\n"
  "  URL = {http://bioinformatics.oxfordjournals.org/content/early/2011/01/07/bioinformatics.btr011.abstract},\n"
  "  eprint = {http://bioinformatics.oxfordjournals.org/content/early/2011/01/07/bioinformatics.btr011.full.pdf+html},\n"
  "  journal = {Bioinformatics}}";

#include <argp.h>
#include <iostream>

/*
 * Option parsing
 */
static char doc[] = "Paper citation";
static char args_doc[] = "";

static struct argp_option options[] = {
  {"bibtex",    'b',       0,      0,      "BibTeX format"},
  { 0 }
};

struct arguments {
  bool bibtex;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *)state->input;

#define FLAG(field) arguments->field = true; break;

  switch(key) {
  case 'b': FLAG(bibtex);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };

int cite_main(int argc, char *argv[])
{
  struct arguments arguments;
  int arg_st;

  arguments.bibtex = false;
  argp_parse(&argp, argc, argv, 0, &arg_st, &arguments);

  if(arguments.bibtex) {
    std::cout << bibtex << std::endl;
  } else {
    std::cout << "This software has been published. If you use it for your research, cite:\n\n"
              << cite << "\n\n" << url << std::endl;
  }

  return 0;
}
