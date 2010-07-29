#include <config.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#include "mer_counting.hpp"
#include "lib/misc.hpp"

struct header {
  uint64_t      klen;
  uint64_t      clen;
  uint64_t      size;
};

int main(int argc, char *argv[]) {
  int fd;
  struct stat stat;
  struct header *header;
  char *db_file, *map;
  int count = 0;
  //  off_t expect_size;

  if(argc != 2) {
    exit(1);    // TODO: better error management
  }

  db_file = argv[1];
  fd = open(db_file, O_RDONLY);
  if(fd < 0)
    die("Can't open '%s'\n", db_file);

  if(fstat(fd, &stat) < 0)
    die("Can't stat '%s'\n", db_file);

  if(stat.st_size < (off_t)sizeof(struct header))
    die("'%s' too small\n", db_file);
  
  map = (char *)mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if(map == MAP_FAILED)
    die("Can't mmap '%s'\n", db_file);
  close(fd);
  header = (struct header *)map;
  std::cerr << "klen " << header->klen << " clen " << header->clen << " size " << header->size << "\n";
  
  //  fprintf(stderr, "%ld %ld\n", header->klen, header->size);
//   expect_size = sizeof(struct header) + 
//     (sizeof(uint64_t) + sizeof(uint32_t)) * header->size;
//   if(stat.st_size != expect_size)
//     die("Size of '%s' is wrong. Expect %ld, got %ld\n", db_file, expect_size,
//         stat.st_size);

#ifdef USE_PACKED_HASH
  arys_64_32_t arys(2*header->klen, header->size, map + sizeof(struct header));
  arys_64_32_t::iterator *it = arys.new_iterator();
#elif USE_PACKED_KV_HASH
  chc_64_32_t hash_table(map + sizeof(struct header), header->size, 2*header->klen,
                         header->clen, 50);
  chc_64_32_t::iterator *it = hash_table.new_iterator();
#else
  arys_64_32_t arys(header->size, map + sizeof(struct header));
  arys_64_32_t::iterator *it = arys.new_iterator();
#endif

  char kmer[header->klen+1];
  char table[4] = { 'A', 'C', 'G', 'T' };
  kmer[header->klen] = '\0';
  while(it->next()) {
    uint64_t kmeri = it->key;
    uint32_t vali = it->val;

    for(unsigned int i = 0; i < header->klen; i++) {
      kmer[header->klen - 1 - i] = table[kmeri & UINT64_C(0x3)];
      kmeri >>= 2;
    }
    std::cout << ">" << vali << "\n" << kmer << "\n";
    count++;
  }
  delete it;
  std::cerr << "count " << count << "\n";
  std::cout.flush();

  return 0;
}
