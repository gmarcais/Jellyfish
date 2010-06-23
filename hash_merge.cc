#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <algorithm>

#include "mer_counting.hpp"
#include "lib/misc.hpp"


/**
 * Structure holds the items in the heap as we are processing
 * the hashes
 **/
struct heap_item {
   mer_counters::iterator *it;
   uint64_t key;
   uint64_t val;
   uint64_t hash;

   heap_item() : it(0), key(0), val(0), hash(0) {}

   heap_item(mer_counters::iterator * iter) :
     it(iter), key(iter->key), val(iter->val), hash(iter->get_hash()) {}

   bool operator<(const heap_item & other) {
      if(hash == other.hash)
        return key < other.key;
      return hash < other.hash;
   }
};

/**
 * Command line processing
 **/
const char *argp_program_version = "hash_merge 1.0";
const char *argp_program_bug_address = "<guillaume@marcais.net>";
static char doc[] = "Merge dumped hash tables";
static char args_doc[] = "";

static struct argp_option options[] = {
  {"fasta",     'f',    0,  0,  "Print k-mers in fasta format (false)"},
  {0}
};

struct arguments {
  bool fasta;
};


/**
 * Parse the command line arguments
 **/
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *)state->input;

#define ULONGP(field) errno = 0; \
arguments->field = (typeof(arguments->field))strtoul(arg,NULL,0);     \
if(errno) return errno; \
break;

#define FLAG(field) arguments->field = true; break;

  switch(key) {
  case 'f': FLAG(fasta);

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };


/**
 * Convert a bit-packed key to a char* string
 **/
inline void tostring(uint64_t key, unsigned int rklen, char * out) {
  char table[4] = { 'A', 'C', 'G', 'T' };

  for(unsigned int i = 0 ; i < rklen; i++) {
    out[rklen-1-i] = table[key & UINT64_C(0x3)];
    key >>= 2;
  }
  out[rklen] = '\0';
}


int main(int argc, char *argv[]) {

  // Process the command line
  struct arguments cmdargs;
  int arg_st = 1;
  cmdargs.fasta = false;
  argp_parse(&argp, argc, argv, 0, &arg_st, &cmdargs);

  int i;
  unsigned int rklen = 0;
  size_t max_reprobe = 0;

  // compute the number of hashes we're going to read
  int num_hashes = argc - arg_st;
  if (num_hashes <= 0) {
    fprintf(stderr, "No hash files given\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }

  // this is our row of iterators
  mer_counters *tables[num_hashes];
  mer_counters::iterator *iters[num_hashes];
 
  // create an iterator for each hash file
  for(i = arg_st; i < argc; i++) {
    char *db_file;

    // open the hash database
    db_file = argv[i];
    tables[i-arg_st] = new mer_counters();
    try {
      tables[i-arg_st]->open(db_file, true);
    } catch(exception *e) {
      die("Can't open k-mer database: %s\n", e->what());
    }
    iters[i-arg_st] = new mer_counters::iterator(tables[i-arg_st]->iterator_all());

    unsigned int len = tables[i-arg_st]->get_key_len();
    if(rklen != 0 && len != rklen)
       die("Can't merge hashes of different key lengths\n");
    rklen = len;
    fprintf(stderr, "reading: %s\n", db_file);

    size_t rep = tables[i-arg_st]->get_max_reprobe_offset();
    if(max_reprobe != 0 && rep != max_reprobe)
       die("Can't merge hashes with different reprobing stratgies\n");
    max_reprobe = rep;
  }

  if(max_reprobe == 0 || rklen == 0)
    die("No valid hash tables found.\n");

  rklen /= 2;
  fprintf(stderr, "key length = %d\n", rklen);
  fprintf(stderr, "num hashes = %d\n", num_hashes);
  fprintf(stderr, "max reprobe = %ld\n", max_reprobe);

  // create the heap storage
  int heap_size = num_hashes * max_reprobe;
  heap_item heap[heap_size];  

  fprintf(stderr, "heap size = %d\n", heap_size);

  // populate the initial heap
  int h = 0;
  for (i = 0; i < num_hashes; i++) {
    for(size_t r = 0; r < max_reprobe && iters[i]->next(); r++) {
      heap[h++] = heap_item(iters[i]);
      assert(h <= heap_size);
    }
  }

  if(h == 0) 
    die("Hashes contain no items.");

  // order the heap
  make_heap(heap, heap + h);

  // h is the current heap size
  while(h > 0) {
    char out[rklen+1];
    int starth = h;

    // pop one element off
    pop_heap(heap, heap + h--);

    uint64_t key = heap[starth-1].key;

    // pop elements off that have the same key
    // all the popped elements will be at the end of the heap array
    while(h > 0 && heap[0].key == key)
      pop_heap(heap, heap + h--);

    // process the entries with the same keys
    uint64_t sum = 0;
    for(i = h; i < starth;) {

      // sum their values
      sum += heap[i].val;

      // if there is more in this iterator, read the next entry, replace
      // this entry with the new values, and insert it back into the heap
      if(heap[i].it->next()) {
          heap[i].key = heap[i].it->key;
          heap[i].val = heap[i].it->val;
          push_heap(heap, heap + i);
          i++;
          h++;

      // if there is no more in this iterator, move the last popped element
      // to this place, and don't advance i.
      } else {
          heap[i] = heap[starth-1];
          starth--;
      }
    }

    // for debugging, just write out the counts
    tostring(key, rklen, out);
    std::cout << out << " " << sum << endl; 
  }

  // free the hashes and iterators
  for(i = 0; i < num_hashes; i++) {
     delete iters[i];
     delete tables[i];
  }
}
/*
    fd = open(db_file, O_RDONLY);
    if(fd < 0)
      die("Can't open '%s'\n", db_file);

    if(fstat(fd, &finfo) < 0)
      die("Can't stat '%s'\n", db_file);

    map = (char *)mmap(NULL, finfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if(map == MAP_FAILED)
      die("Can't mmap '%s'\n", db_file);
    int adv = madvise(map, finfo.st_size, MADV_SEQUENTIAL);
    if (adv != 0)
      die("Can't set memory parameters correctly\n");
    close(fd);

    tables[i-arg_st] = new mer_counters(map, finfo.st_size);
*/
