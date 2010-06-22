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


struct heap_item {
   mer_counters::iterator *it;
   uint64_t key;
   uint64_t val;

   heap_item() : it(0), key(0), val(0) {}

   heap_item(mer_counters::iterator * iter) :
     it(iter), key(iter->key), val(iter->val) {}

   bool operator<(const heap_item & other) {
      if(key == other.key)
        return val < other.val;
      return key < other.key;
   }
};

//const char *argp_program_version = "hash_merge 1.0";
//const char *argp_program_bug_address = "<guillaume@marcais.net>";
//static char doc[] = "Merge dumped hash tables";

// Convert a bit-packed key to a char* string
inline void tostring(uint64_t key, unsigned int rklen, char * out) {
  char table[4] = { 'A', 'C', 'G', 'T' };

  for(unsigned int i = 0 ; i < rklen; i++) {
    out[rklen-1-i] = table[key & UINT64_C(0x3)];
    key >>= 2;
  }
  out[rklen] = '\0';
}


int main(int argc, char *argv[]) {

  // XXX: include the argument parsing here
  int arg_st = 1;
  int i, j;
  unsigned int rklen = 0;
  int max_reprob = 10;

  // compute the number of hashes we're going to read
  int num_hashes = argc - arg_st;
  if (num_hashes <= 0) {
    fprintf(stderr, "No hash files given\n");
    exit(1);
  }

  // this is our row of iterators
  mer_counters *tables[num_hashes];
  mer_counters::iterator *iters[num_hashes];
 
  for(i = arg_st; i < argc; i++) {
    char *db_file, *map;
    int fd;
    struct stat finfo;
    //struct header *fheader;

    db_file = argv[i];
    fd = open(db_file, O_RDONLY);
    if(fd < 0)
      die("Can't open '%s'\n", db_file);

    if(fstat(fd, &finfo) < 0)
      die("Can't stat '%s'\n", db_file);

    map = (char *)mmap(NULL, finfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if(map == MAP_FAILED)
      die("Can't mmap '%s'\n", db_file);
    close(fd);

    tables[i-arg_st] = new mer_counters(map, finfo.st_size);
    unsigned int len = tables[i-arg_st]->get_key_len();
    if(rklen != 0 && len != rklen)
       die("Can't merge hashes of different key lengths\n");
    rklen = len;
    iters[i-arg_st] = new mer_counters::iterator(tables[i-arg_st]->iterator_all());
  }

  rklen /= 2;
  fprintf(stderr, "key length = %d\n", rklen);
  fprintf(stderr, "num hashes = %d\n", num_hashes);

  // create the heap storage
  int heap_size = num_hashes * max_reprob;
  heap_item heap[heap_size];  

  fprintf(stderr, "heap size = %d\n", heap_size);

  // populate the initial heap
  int h = 0;
  for (i = 0; i < num_hashes; i++) {
    for(j = 0; j < max_reprob && iters[i]->next(); j++) {
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
    // all the popped elements will be at the end of the heap
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
}

