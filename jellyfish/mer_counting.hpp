#ifndef __MER_COUNTING__
#define __MER_COUNTING__

#include <sys/types.h>
#include <stdint.h>
#include <stdarg.h>
#include <vector>
#include <utility>

#include <invertible_hash_config.hpp>

#include "misc.hpp"
#include "concurrent_queues.hpp"
#include "atomic_gcc.hpp"
#include "allocators_mmap.hpp"
#include "hash.hpp"
#include "compacted_hash.hpp"
#include "compacted_dumper.hpp"
#include "sorted_dumper.hpp"

#if defined(PACKED_KEY_VALUE)
#include "packed_key_value_array.hpp"
typedef jellyfish::packed_key_value::array<uint64_t,atomic::gcc<uint64_t>,allocators::mmap> storage_t;
#elif defined(INVERTIBLE_HASH)
#include "invertible_hash_array.hpp"
typedef jellyfish::invertible_hash::array<uint64_t,atomic::gcc<uint64_t>,allocators::mmap> storage_t;
#else
#error No constant specifying the type of storage class
#endif

typedef jellyfish::hash< uint64_t,uint64_t,storage_t,atomic::gcc<uint64_t> > mer_counters;
typedef mer_counters::iterator mer_iterator_t;
typedef jellyfish::compacted_hash::reader<uint64_t,uint64_t> hash_reader_t;
typedef jellyfish::compacted_hash::query<uint64_t,uint64_t> hash_query_t;
typedef jellyfish::compacted_hash::writer<hash_reader_t> hash_writer_t;
typedef jellyfish::sorted_dumper< storage_t,atomic::gcc<uint64_t> > hash_dumper_t;

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
