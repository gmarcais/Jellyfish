#ifndef __MER_COUNTING__
#define __MER_COUNTING__

#include <sys/types.h>
#include <stdint.h>
#include <stdarg.h>
#include <vector>
#include <utility>

#include <invertible_hash_config.hpp>

#include "misc.hpp"
#include "storage.hpp"
#include "hash.hpp"
#include "concurrent_queues.hpp"
#include "atomic_gcc.hpp"
#include "allocators_mmap.hpp"
#include "hash.hpp"
#include "compacted_hash.hpp"
#include "compacted_dumper.hpp"
#include "sorted_dumper.hpp"

// Invertible hash types
#include "invertible_hash_array.hpp"
typedef jellyfish::invertible_hash::array<uint64_t,atomic::gcc<uint64_t>,allocators::mmap> inv_hash_storage_t;
typedef jellyfish::sorted_dumper< inv_hash_storage_t,atomic::gcc<uint64_t> > inv_hash_dumper_t;
typedef jellyfish::hash< uint64_t,uint64_t,inv_hash_storage_t,atomic::gcc<uint64_t> > inv_hash_t;

// Direct indexing types
// #include "direct_indexing_array.hpp"
// typedef jellyfish::direct_indexing::array<uint64_t,uint64_t,atomic::gcc<uint64_t>,allocators::mmap> direct_index_storage_t;

typedef jellyfish::compacted_hash::reader<uint64_t,uint64_t> hash_reader_t;
typedef jellyfish::compacted_hash::query<uint64_t,uint64_t> hash_query_t;
typedef jellyfish::compacted_hash::writer<hash_reader_t> hash_writer_t;


struct seq {
  char  *buffer;
  char  *end;
  char	*map_end;
  bool   nl;			// Char before buffer start is a new line
  bool   ns;			// Beginning of buffer is not sequence data
};
typedef struct seq seq;

typedef concurrent_queue<seq> seq_queue;

#endif /* __MER_COUNTING__ */
