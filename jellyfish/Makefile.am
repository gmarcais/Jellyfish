## Process this file with automake to produce Makefile.in

AM_LDFLAGS = -lpthread
AM_CPPFLAGS = -Wall -Werror -g -O2 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -I$(top_srcdir)

#if MACOSX
#AM_CPPFLAGS += -arch x86_64 -I../gnulib
#endif

lib_LTLIBRARIES = libjellyfish-1.0.la
libjellyfish_1_0_la_SOURCES = square_binary_matrix.cc fasta_parser.cc

bin_PROGRAMS = jellyfish
#if MACOSX
#jellyfish_LDADD = ../gnulib/libgnu.a libjellyfish-1.0.la
#else
jellyfish_LDADD = libjellyfish-1.0.la
#endif


jellyfish_SOURCES = jellyfish.cc dump_stats_compacted.cc hash_merge.cc storage.cc misc.cc mer_counter.cc histogram.cc time.cc thread_exec.cc query_main.cc

library_includedir=$(includedir)/jellyfish-1.0/jellyfish
library_include_HEADERS = allocators_malloc.hpp allocators_mmap.hpp allocators_shm.hpp atomic_gcc.hpp compacted_dumper.hpp compacted_hash.hpp concurrent_hash_counters.hpp concurrent_queues.hpp direct_indexing_array.hpp direct_sorted_dumper.hpp divisor.hpp dumper.hpp fasta_parser.hpp hash_function.hpp hash.hpp heap.hpp invertible_hash_array.hpp locking_hash_counters.hpp locks_pthread.hpp mapped_file.hpp mer_counting.hpp misc.hpp offsets_key_value.hpp packed_concurrent_hash_counters.hpp packed_key_value_array.hpp reversible_hash_function.hpp small_packed_array.hpp sorted_dumper.hpp square_binary_matrix.hpp storage.hpp thread_exec.hpp time.hpp token_ring.hpp
