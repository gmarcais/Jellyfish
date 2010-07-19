#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>
#include <iostream>
#include <fstream>
#include <vector>
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

#define MAX_KMER_SIZE 32

/**
 * Structure holds the items in the heap as we are processing
 * the hashes
 **/
struct heap_item {
  hash_reader_t *it;
  uint64_t	 key;
  uint64_t	 val;
  uint64_t	 pos;
  char		 kmer[MAX_KMER_SIZE + 1];

  heap_item() : it(0), key(0), val(0), pos(0) {
    memset(kmer, 0, MAX_KMER_SIZE + 1);
  }

  heap_item(hash_reader_t *iter) :
    it(iter), key(iter->key), val(iter->val), pos(iter->get_pos()) {
    //    iter->get_string(kmer);
  }

  // bool operator<(const heap_item & other) {
  //   if(hash == other.hash)
  //     return key < other.key;
  //   return hash < other.hash;
  // }
  // Want a min heap: smaller element first
  bool operator<(const heap_item & other) {
    if(pos == other.pos)
      return key > other.key;
    return pos > other.pos;
  }
};

class heap_item_compare {
public:
  inline bool operator() (struct heap_item *item1, struct heap_item *item2) {
    return *item1 < *item2;
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
  bool         fasta;
  const char * output;
  unsigned int out_counter_len;
};


/**
 * Parse the command line arguments
 **/
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = (struct arguments *)state->input;

#define ULONGP(field) errno = 0;					\
  arguments->field = (typeof(arguments->field))strtoul(arg,NULL,0);     \
  if(errno) return errno;						\
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
  uint64_t key_len_bits;
  uint64_t val_len_bits;
  size_t   size;
  uint64_t max_reprobes;
  uint64_t key_len; // in BYTES
  uint64_t val_len; // in BYTES
  uint64_t max_count;
  std::ofstream out;
  char * buffer;
  char * buffer_end;
  char * current;
   
public:
  // klen is in BASES; vlen is in BITs
  serial_writer(std::string filename, uint_t klen, uint_t vlen, size_t _size,
		uint64_t _max_reprobes, size_t _buff_len = 20000000UL) :
    key_len_bits(klen),
    val_len_bits(vlen),
    size(_size),
    max_reprobes(_max_reprobes),
    key_len((klen / 8) + (klen % 8 != 0)),
    val_len((vlen / 8) + (vlen % 8 != 0)),
    max_count((((uint64_t)1) << (8*val_len)) - 1),
    out(filename.c_str())
  {
    if(!out.good())
      throw new ErrorWriting("Can't open file");
    size_t record_len = key_len + val_len;
    size_t buffer_size = record_len * (_buff_len / record_len);
    buffer = new char[buffer_size];
    buffer_end = buffer + buffer_size;
    current = buffer;
  }

  ~serial_writer() {
    close();
    delete [] buffer;
  }

  uint64_t get_key_len_bytes() const { return key_len; }
  uint64_t get_val_len_bytes() const { return val_len; }
  uint64_t get_max_reprobes() const { return max_reprobes; }

  /**
   * Write the top of the hash table
   **/
  void write_header() {
    jellyfish::compacted_hash::header head;
    memset(&head, '\0', sizeof(head));
    head.key_len = key_len_bits;
    head.val_len = val_len_bits / 8;
    head.size = size;
    head.max_reprobes = max_reprobes;
    head.size = 0; // field is unused
    out.write((char *)&head, sizeof(head));
  }

  void write_matrices(SquareBinaryMatrix &m1, SquareBinaryMatrix &m2) {
    m1.dump(out);
    m2.dump(out);
  }

  // /**
  //  * Write a single key, value entry
  //  **/
  // void write_entry(uint64_t key, uint64_t val) {
  //   char *ptr = buffer;
  //   memcpy(ptr, &key, key_len);
  //   ptr += key_len;
  //   uint64_t count = (val > max_count) ? max_count : val;
  //   memcpy(ptr, &count, val_len);
  //   out.write(buffer, key_len + val_len);
  // }
  void write_entry(uint64_t key, uint64_t val) {
    if(current >= buffer_end)
      flush_buffer();

    memcpy(current, &key, key_len);
    current += key_len;
    uint64_t count = (val > max_count) ? max_count : val;
    memcpy(current, &count, val_len);
    current += val_len;
  }
  
  void close() {
    // XXX: nop 
    flush_buffer();
    if(out.is_open())
      out.close();
  }

private:
  void flush_buffer() {
    if(current > buffer)
      out.write(buffer, current - buffer);
    current = buffer;
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
  size_t max_reprobes = 0;
  size_t hash_size = 0;
  SquareBinaryMatrix hash_matrix;
  SquareBinaryMatrix hash_inverse_matrix;

  // compute the number of hashes we're going to read
  int num_hashes = argc - arg_st;
  if (num_hashes <= 0) {
    fprintf(stderr, "No hash files given\n");
    argp_help(&argp, stderr, ARGP_HELP_SEE, argv[0]);
    exit(1);
  }

  // this is our row of iterators
  vector<hash_reader_t *> iters;
 
  // create an iterator for each hash file
  for(i = arg_st; i < argc; i++) {
    char *db_file;

    // open the hash database
    db_file = argv[i];
    try {
      iters.push_back(new hash_reader_t(db_file));
    } catch(exception *e) {
      die("Can't open k-mer database: %s\n", e->what());
    }

    unsigned int len = iters.back()->get_key_len();
    if(rklen != 0 && len != rklen)
      die("Can't merge hashes of different key lengths\n");
    rklen = len;
    fprintf(stderr, "reading: %s\n", db_file);

    uint64_t rep = iters.back()->get_max_reprobes();
    if(max_reprobes != 0 && rep != max_reprobes)
      die("Can't merge hashes with different reprobing stratgies\n");
    max_reprobes = rep;

    size_t size = iters.back()->get_size();
    if(hash_size != 0 && size != hash_size)
      die("Can't merge hash with different size\n");
    hash_size = size;

    if(hash_matrix.get_size() < 0) {
      hash_matrix = iters.back()->get_hash_matrix();
      hash_inverse_matrix = iters.back()->get_hash_inverse_matrix();
    } else {
      SquareBinaryMatrix new_hash_matrix = iters.back()->get_hash_matrix();
      SquareBinaryMatrix new_hash_inverse_matrix = 
	iters.back()->get_hash_inverse_matrix();
      if(new_hash_matrix != hash_matrix || 
	 new_hash_inverse_matrix != hash_inverse_matrix)
	die("Can't merge hash with different hash function\n");
    }
  }

  if(rklen == 0)
    die("No valid hash tables found.\n");

  num_hashes = iters.size();
  fprintf(stderr, "mer length  = %d\n", rklen / 2);
  fprintf(stderr, "hash size   = %ld\n", hash_size);
  fprintf(stderr, "num hashes  = %d\n", num_hashes);
  fprintf(stderr, "max reprobe = %ld\n", max_reprobes);

  // create the heap storage
  int heap_size = num_hashes * max_reprobes;
  heap_item heap_storage[heap_size];
  heap_item *heap[heap_size];
  heap_item_compare compare;

  fprintf(stderr, "heap size = %d\n", heap_size);

  // populate the initial heap
  int h = 0;
  for (i = 0; i < num_hashes; i++) {
    for(size_t r = 0; r < max_reprobes && iters[i]->next(); r++) {
      heap_storage[h] = heap_item(iters[i]);
      heap[h] = &heap_storage[h];
      h++;
      assert(h <= heap_size);
    }
  }

  if(h == 0) 
    die("Hashes contain no items.");

  // open the output file
  serial_writer writer(cmdargs.output, rklen, 8 * cmdargs.out_counter_len,
		       hash_size, max_reprobes);
  writer.write_header();
  writer.write_matrices(hash_matrix, hash_inverse_matrix);

  fprintf(stderr, "out kmer len = %ld bytes\n", writer.get_key_len_bytes());
  fprintf(stderr, "out val len = %ld bytes\n", writer.get_val_len_bytes());

  // order the heap
  make_heap(heap, heap + h, compare);

  // h is the current heap size
  while(h > 0) {
    //char out[rklen+1];
    int starth = h;

    // pop one element off
    pop_heap(heap, heap + h--, compare);

    uint64_t key = heap[starth-1]->key;

    // pop elements off that have the same key
    // all the popped elements will be at the end of the heap array
    while(h > 0 && heap[0]->key == key)
      pop_heap(heap, heap + h--, compare);

    // process the entries with the same keys
    uint64_t sum = 0;
    for(i = h; i < starth;) {

      // sum their values
      sum += heap[i]->val;

      // if there is more in this iterator, read the next entry, replace
      // this entry with the new values, and insert it back into the heap
      if(heap[i]->it->next()) {
	heap[i]->key = heap[i]->it->key;
	heap[i]->val = heap[i]->it->val;
	heap[i]->pos = heap[i]->it->get_pos();
	//	head[i].it->get_string(head[i].kmer);
	push_heap(heap, heap + i, compare);
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
  for(i = 0; i < num_hashes; i++)
    delete iters[i];
  //   delete tables[i];
  // }

  writer.close();
}
