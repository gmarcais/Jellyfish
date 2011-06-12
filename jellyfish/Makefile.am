## Process this file with automake to produce Makefile.in

AM_LDFLAGS = -lpthread
AM_CPPFLAGS = -Wall -Werror -Wnon-virtual-dtor -g -O2 -I$(top_srcdir)

bin_PROGRAMS = jellyfish

jellyfish_SOURCES = jellyfish.cc stats_main.cc hash_merge.cc		\
storage.cc misc.cc err.cc mer_counter.cc histo_main.cc dump_main.cc	\
time.cc thread_exec.cc query_main.cc square_binary_matrix.cc		\
dump_fastq_main.cc histo_fastq_main.cc cite.cc parse_dna.cc		\
file_parser.cc parse_quake.cc parse_qual_dna.cc sequence_parser.cc	\
seq_qual_parser.cpp half.cpp hash_fastq_merge.cc cite_cmdline.cc	\
query_cmdline.cc hash_merge_cmdline.cc histo_cmdline.cc			\
stats_cmdline.cc histo_fastq_main_cmdline.cc				\
dump_fastq_main_cmdline.cc mer_counter_cmdline.cc			\
hash_fastq_merge_cmdline.cc dump_cmdline.cc dbg.cc mapped_file.cc	\
backtrace.cc
# jellyfish_CPPFLAGS = $(AM_CPPFLAGS)

EXTRA_DIST = cite_cmdline.hpp query_cmdline.hpp hash_merge_cmdline.hpp	\
histo_cmdline.hpp stats_cmdline.hpp histo_fastq_main_cmdline.hpp	\
dump_fastq_main_cmdline.hpp mer_counter_cmdline.hpp			\
hash_fastq_merge_cmdline.hpp dump_cmdline.hpp double_fifo_input.hpp	\
simple_growing_array.hpp

lib_LTLIBRARIES = libjellyfish.la
libjellyfish_la_LDFLAGS = -version-info 1:0:0
libjellyfish_la_SOURCES = square_binary_matrix.cc err.cc misc.cc	\
storage.cc thread_exec.cc time.cc file_parser.cc read_parser.cc		\
parse_read.cc half.cpp mapped_file.cc
libjellyfish_la_CPPFLAGS = $(AM_CPPFLAGS)
library_includedir=$(includedir)/jellyfish-@PACKAGE_VERSION@/jellyfish
library_include_HEADERS = allocators_malloc.hpp allocators_mmap.hpp	\
allocators_shm.hpp atomic_gcc.hpp compacted_dumper.hpp			\
compacted_hash.hpp concurrent_queues.hpp direct_indexing_array.hpp	\
direct_sorted_dumper.hpp divisor.hpp dumper.hpp hash_function.hpp	\
hash.hpp heap.hpp invertible_hash_array.hpp locking_hash_counters.hpp	\
locks_pthread.hpp mapped_file.hpp mer_counting.hpp err.hpp misc.hpp	\
offsets_key_value.hpp reversible_hash_function.hpp sorted_dumper.hpp	\
square_binary_matrix.hpp storage.hpp thread_exec.hpp time.hpp		\
token_ring.hpp raw_dumper.hpp capped_integer.hpp			\
aligned_values_array.hpp fastq_dumper.hpp floats.hpp			\
circular_buffer.hpp counter.hpp parse_quake.hpp parse_dna.hpp		\
parse_qual_dna.hpp file_parser.hpp sequence_parser.hpp			\
seq_qual_parser.hpp double_fifo_input.hpp read_parser.hpp		\
parse_read.hpp simple_growing_array.hpp toFloat.h eLut.h dbg.hpp

check_PROGRAMS = generate_sequence test_double_fifo_input	\
test_read_parser
generate_sequence_SOURCES = generate_sequence.cc	\
generate_sequence_cmdline.cc misc.cc mersenne.cpp	\
square_binary_matrix.cc
test_double_fifo_input_SOURCES = test_double_fifo_input.cc	\
parse_dna.cc file_parser.cc sequence_parser.cc backtrace.cc	\
thread_exec.cc dbg.cc
test_read_parser_SOURCES = test_read_parser.cc file_parser.cc	\
read_parser.cc parse_read.cc dbg.cc backtrace.cc
EXTRA_DIST += randomc.h generate_sequence_cmdline.hpp
