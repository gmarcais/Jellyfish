## Process this file with automake to produce Makefile.in

AM_LDFLAGS = -lpthread
AM_CPPFLAGS = -Wall -Werror -g -O2 -I$(top_srcdir)

bin_PROGRAMS = jellyfish

jellyfish_SOURCES = jellyfish.cc dump_stats_compacted.cc hash_merge.cc	\
storage.cc misc.cc err.cc mer_counter.cc histogram.cc time.cc		\
thread_exec.cc query_main.cc square_binary_matrix.cc fasta_parser.cc	\
dump_stats_raw.cc rquery_main.cc dump_fastq_main.cc			\
histo_fastq_main.cc cite.cc parse_dna.cc file_parser.cc			\
fastq_sequence_parser.cc parse_quake.cc fastq_seq_qual_parser.cc	\
hash_fastq_merge.cc cite_cmdline.cc query_cmdline.cc			\
hash_merge_cmdline.cc histogram_cmdline.cc histo_fastq_main_cmdline.cc	\
dump_fastq_main_cmdline.cc rquery_main_cmdline.cc			\
dump_stats_raw_cmdline.cc dump_stats_compacted_cmdline.cc		\
mer_counter_cmdline.cc hash_fastq_merge_cmdline.cc half.cpp
# jellyfish_CPPFLAGS = $(AM_CPPFLAGS)

EXTRA_DIST = cite_cmdline.hpp query_cmdline.hpp hash_merge_cmdline.hpp	\
histogram_cmdline.hpp histo_fastq_main_cmdline.hpp			\
dump_fastq_main_cmdline.hpp rquery_main_cmdline.hpp			\
dump_stats_raw_cmdline.hpp dump_stats_compacted_cmdline.hpp		\
mer_counter_cmdline.hpp hash_fastq_merge_cmdline.hpp

lib_LTLIBRARIES = libjellyfish.la
libjellyfish_la_LDFLAGS = -version-info 1:0:0
libjellyfish_la_SOURCES = square_binary_matrix.cc fasta_parser.cc	\
err.cc misc.cc storage.cc thread_exec.cc time.cc file_parser.cc		\
fastq_seq_qual_parser.cc fastq_sequence_parser.cc half.cpp
libjellyfish_la_CPPFLAGS = $(AM_CPPFLAGS)
library_includedir=$(includedir)/jellyfish-@PACKAGE_VERSION@/jellyfish
library_include_HEADERS = allocators_malloc.hpp allocators_mmap.hpp	\
allocators_shm.hpp atomic_gcc.hpp compacted_dumper.hpp			\
compacted_hash.hpp concurrent_queues.hpp direct_indexing_array.hpp	\
direct_sorted_dumper.hpp divisor.hpp dumper.hpp fasta_parser.hpp	\
hash_function.hpp hash.hpp heap.hpp invertible_hash_array.hpp		\
locking_hash_counters.hpp locks_pthread.hpp mapped_file.hpp		\
mer_counting.hpp err.hpp misc.hpp offsets_key_value.hpp			\
reversible_hash_function.hpp sorted_dumper.hpp				\
square_binary_matrix.hpp storage.hpp thread_exec.hpp time.hpp		\
token_ring.hpp raw_dumper.hpp capped_integer.hpp			\
aligned_values_array.hpp fastq_dumper.hpp floats.hpp			\
circular_buffer.hpp counter.hpp parse_quake.hpp parse_dna.hpp		\
fastq_sequence_parser.hpp fastq_seq_qual_parser.hpp file_parser.hpp	\
fasta_parser.hpp fastq_sequence_parser.hpp half.h toFloat.h eLut.h

check_PROGRAMS = generate_sequence
generate_sequence_SOURCES = generate_sequence.cc	\
generate_sequence_cmdline.cc misc.cc mersenne.cpp	\
square_binary_matrix.cc
EXTRA_DIST += randomc.h generate_sequence_cmdline.hpp
