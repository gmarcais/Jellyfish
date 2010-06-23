#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <algorithm>

#include "mer_counting.hpp"
#include "compacted_hash.hpp"
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

enum {
   OPT_VAL_LEN = 1024
};

static struct argp_option options[] = {
  {"fasta",     'f',    0,  0,  "Print k-mers in fasta format (false)"},
  {"output",    'o',  "FILE", 0, "Output file"},
  {"out-counter-len", OPT_VAL_LEN, "LEN", 0, "Length (in bytes) of counting field in output"},
  {0}
};

struct arguments {
  bool fasta;
  char * output;
  unsigned int out_counter_len;
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
#define STRING(field) arguments->field = arg; break;

  switch(key) {
  case 'f': FLAG(fasta);
  case 'o': STRING(output);
  case OPT_VAL_LEN: ULONGP(out_counter_len);

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

class ErrorWriting : public std::exception {
   std::string msg;
  public:
   ErrorWriting(const std::string _msg) : msg(_msg) {}
   virtual ~ErrorWriting() throw() {}
   virtual const char* what() const throw() {
     return msg.c_str();
   }
};


class serial_writer {
   uint64_t mer_len_bases;
   uint64_t val_len_bits;
   uint64_t key_len; // in BYTES
   uint64_t val_len; // in BYTES
   uint64_t max_count;
   std::ofstream out;
   char * buffer;
   
   public:
     // klen is in BASES; vlen is in BITs
     serial_writer(std::string filename, uint_t klen, uint_t vlen) :
        mer_len_bases(klen),
        val_len_bits(vlen),
        key_len((klen / 4) + (klen % 4 != 0)),
        val_len((vlen / 8) + (vlen % 8 != 0)),
        max_count((((uint64_t)1) << (8*val_len)) - 1)
    {
        out.open(filename.c_str(), std::ios::binary);
        if(!out.good())
           throw new ErrorWriting("Can't open file");
        buffer = new char[key_len + val_len];
    }

    ~serial_writer() {
        delete [] buffer;
    }

    uint64_t get_key_len_bytes() { return key_len; }
    uint64_t get_val_len_bytes() { return val_len; }

    /**
     * Write the top of the hash table
     **/
    void write_header(uint64_t size) {
        jellyfish::compacted_hash::header head;
        memset(&head, '\0', sizeof(head));
        head.mer_len = mer_len_bases;
        head.val_len = val_len_bits;
        head.size = size;
        out.write((char *)&head, sizeof(head));
    }

    /**
     * Write a single key, value entry
     **/
    void write_entry(uint64_t key, uint64_t val) {
      char *ptr = buffer;
      memcpy(ptr, &key, key_len);
      ptr += key_len;
      uint64_t count = (val > max_count) ? max_count : val;
      memcpy(ptr, &count, val_len);
      out.write(buffer, key_len + val_len);
    }

    void close() {
        // XXX: nop 
    }
};


int main(int argc, char *argv[]) {

  // Process the command line
  struct arguments cmdargs;
  int arg_st = 1;
  cmdargs.fasta = false;
  cmdargs.out_counter_len = 4;
  cmdargs.output = "mer_counts_merged.hash";
  argp_parse(&argp, argc, argv, 0, &arg_st, &cmdargs);

  int i;
  unsigned int rklen = 0;
  size_t max_reprobe = 0;
  uint64_t size = 0;

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

    uint64_t si = tables[i-arg_st]->get_size();
    fprintf(stderr, "size = %ld\n", si);
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

  // open the output file
  serial_writer writer(cmdargs.output, rklen, cmdargs.out_counter_len);
  writer.write_header(size);

  fprintf(stderr, "out kmer len = %ld bytes\n", writer.get_key_len_bytes());
  fprintf(stderr, "out val len = %ld bytes\n", writer.get_val_len_bytes());

  // order the heap
  make_heap(heap, heap + h);

  // h is the current heap size
  while(h > 0) {
    //char out[rklen+1];
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
    //tostring(key, rklen, out);
    //std::cout << out << " " << sum << endl; 
    writer.write_entry(key, sum);
  }

  // free the hashes and iterators
  for(i = 0; i < num_hashes; i++) {
     delete iters[i];
     delete tables[i];
  }

  writer.close();
}
