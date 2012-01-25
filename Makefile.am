ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = m4/gnulib-cache.m4 doc/jellyfish.pdf doc/jellyfish.man README LICENSE HalfLICENSE
man1_MANS = doc/jellyfish.man

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = jellyfish-1.1.pc

AM_LDFLAGS = -lpthread
AM_CPPFLAGS = -Wall -Werror -Wnon-virtual-dtor -I$(top_srcdir)
AM_CXXFLAGS = -g -O3

# What to build
bin_PROGRAMS = bin/jellyfish
lib_LTLIBRARIES = libjellyfish.la
check_PROGRAMS = bin/generate_sequence bin/test_double_fifo_input	\
                 bin/test_read_parser

########################################
# Build Jellyfish the exec
########################################
bin_jellyfish_SOURCES = jellyfish/jellyfish.cc jellyfish/stats_main.cc			\
                    jellyfish/hash_merge.cc jellyfish/storage.cc jellyfish/misc.cc	\
                    jellyfish/err.cc jellyfish/mer_counter.cc jellyfish/histo_main.cc	\
                    jellyfish/dump_main.cc jellyfish/time.cc jellyfish/thread_exec.cc	\
                    jellyfish/query_main.cc jellyfish/square_binary_matrix.cc	\
                    jellyfish/dump_fastq_main.cc jellyfish/histo_fastq_main.cc	\
                    jellyfish/cite.cc jellyfish/parse_dna.cc jellyfish/file_parser.cc	\
                    jellyfish/parse_quake.cc jellyfish/parse_qual_dna.cc		\
                    jellyfish/sequence_parser.cc jellyfish/seq_qual_parser.cc	\
                    jellyfish/half.cpp jellyfish/hash_fastq_merge.cc jellyfish/dbg.cc	\
                    jellyfish/mapped_file.cc jellyfish/backtrace.cc jellyfish/floats.cc	\
                    jellyfish/allocators_mmap.cc jellyfish/yaggo.cpp

EXTRA_DIST += jellyfish/cite_cmdline.hpp jellyfish/query_cmdline.hpp		\
              jellyfish/hash_merge_cmdline.hpp jellyfish/histo_main_cmdline.hpp	\
              jellyfish/stats_main_cmdline.hpp				\
              jellyfish/histo_fastq_main_cmdline.hpp				\
              jellyfish/dump_fastq_main_cmdline.hpp				\
              jellyfish/count_main_cmdline.hpp				\
              jellyfish/hash_fastq_merge_cmdline.hpp				\
              jellyfish/dump_main_cmdline.hpp jellyfish/double_fifo_input.hpp	\
              jellyfish/simple_growing_array.hpp jellyfish/backtrace.hpp		\
              jellyfish/noop_dumper.hpp jellyfish/yaggo.hpp				\
              jellyfish/fstream_default.hpp

########################################
# Build Jellyfish the shared library
########################################
libjellyfish_la_LDFLAGS = -version-info 1:0:0
libjellyfish_la_SOURCES = jellyfish/square_binary_matrix.cc		\
                          jellyfish/err.cc jellyfish/misc.cc		\
                          jellyfish/storage.cc				\
                          jellyfish/thread_exec.cc jellyfish/time.cc	\
                          jellyfish/file_parser.cc			\
                          jellyfish/read_parser.cc			\
                          jellyfish/parse_read.cc jellyfish/half.cpp	\
                          jellyfish/mapped_file.cc			\
                          jellyfish/parse_dna.cc			\
                          jellyfish/parse_quake.cc			\
                          jellyfish/parse_qual_dna.cc			\
                          jellyfish/sequence_parser.cc			\
                          jellyfish/seq_qual_parser.cc			\
                          jellyfish/backtrace.cc jellyfish/floats.cc	\
                          jellyfish/dbg.cc				\
                          jellyfish/allocators_mmap.cc
libjellyfish_la_CPPFLAGS = $(AM_CPPFLAGS)
library_includedir=$(includedir)/jellyfish-@PACKAGE_VERSION@/jellyfish
library_include_HEADERS = jellyfish/allocators_malloc.hpp			\
                          jellyfish/allocators_mmap.hpp			\
                          jellyfish/allocators_shm.hpp jellyfish/atomic_gcc.hpp	\
                          jellyfish/compacted_dumper.hpp			\
                          jellyfish/compacted_hash.hpp			\
                          jellyfish/concurrent_queues.hpp			\
                          jellyfish/direct_indexing_array.hpp			\
                          jellyfish/direct_sorted_dumper.hpp jellyfish/divisor.hpp	\
                          jellyfish/dumper.hpp jellyfish/hash_function.hpp		\
                          jellyfish/hash.hpp jellyfish/heap.hpp			\
                          jellyfish/invertible_hash_array.hpp			\
                          jellyfish/locking_hash_counters.hpp			\
                          jellyfish/locks_pthread.hpp jellyfish/mapped_file.hpp	\
                          jellyfish/mer_counting.hpp jellyfish/err.hpp		\
                          jellyfish/misc.hpp jellyfish/offsets_key_value.hpp	\
                          jellyfish/reversible_hash_function.hpp		\
                          jellyfish/sorted_dumper.hpp				\
                          jellyfish/square_binary_matrix.hpp jellyfish/storage.hpp	\
                          jellyfish/thread_exec.hpp jellyfish/time.hpp		\
                          jellyfish/token_ring.hpp jellyfish/raw_dumper.hpp		\
                          jellyfish/capped_integer.hpp			\
                          jellyfish/aligned_values_array.hpp			\
                          jellyfish/fastq_dumper.hpp jellyfish/floats.hpp		\
                          jellyfish/circular_buffer.hpp jellyfish/counter.hpp	\
                          jellyfish/parse_quake.hpp jellyfish/parse_dna.hpp		\
                          jellyfish/parse_qual_dna.hpp jellyfish/file_parser.hpp	\
                          jellyfish/sequence_parser.hpp			\
                          jellyfish/seq_qual_parser.hpp			\
                          jellyfish/double_fifo_input.hpp			\
                          jellyfish/read_parser.hpp jellyfish/parse_read.hpp	\
                          jellyfish/simple_growing_array.hpp jellyfish/toFloat.h	\
                          jellyfish/eLut.h jellyfish/dbg.hpp jellyfish/half.h		\
                          jellyfish/backtrace.hpp

########################################
# Build tests
########################################
bin_generate_sequence_SOURCES = jellyfish/generate_sequence.cc jellyfish/misc.cc	\
                            jellyfish/mersenne.cpp				\
                            jellyfish/square_binary_matrix.cc			\
                            jellyfish/backtrace.cc jellyfish/dbg.cc jellyfish/time.cc
bin_test_double_fifo_input_SOURCES = jellyfish/test_double_fifo_input.cc		\
                                 jellyfish/parse_dna.cc jellyfish/file_parser.cc	\
                                 jellyfish/sequence_parser.cc			\
                                 jellyfish/backtrace.cc jellyfish/thread_exec.cc	\
                                 jellyfish/dbg.cc jellyfish/time.cc			\
                                 jellyfish/allocators_mmap.cc
bin_test_read_parser_SOURCES = jellyfish/test_read_parser.cc jellyfish/file_parser.cc	\
                           jellyfish/read_parser.cc jellyfish/parse_read.cc		\
                           jellyfish/dbg.cc jellyfish/backtrace.cc jellyfish/time.cc
EXTRA_DIST += jellyfish/randomc.h jellyfish/generate_sequence_cmdline.hpp

########################################
# Tests
########################################
TEST_EXTENSIONS = .sh
SH_LOG_COMPILER = /bin/sh
AM_SH_LOG_FLAGS =

TESTS = tests/generate_sequence.sh tests/serial_hashing.sh		\
        tests/parallel_hashing.sh tests/serial_direct_indexing.sh	\
        tests/parallel_direct_indexing.sh tests/raw_hash.sh		\
        tests/generate_fastq_sequence.sh				\
        tests/parallel_fastq_hashing.sh tests/multi_file.sh		\
        tests/multi_file_fastq.sh tests/from_stream.sh			\
        tests/parallel_fastq_sequence_hashing.sh			\
        tests/from_stream_fastq.sh tests/merge.sh tests/min_qual.sh	\
        tests/big.sh tests/parsers.sh

EXTRA_DIST += $(TESTS)
clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	-cd tests; rm -f seq10m* seq1m* *_0 *_1 *_2 *.md5sum *.histo *.stats *.timing *.query *.dump *.fa

tests/serial_hashing.log: tests/generate_sequence.log
tests/parallel_hashing.log: tests/generate_sequence.log
tests/serial_direct_indexing.log: tests/generate_sequence.log
tests/parallel_direct_indexing.log: tests/generate_sequence.log
tests/multi_file.log: tests/generate_sequence.log
tests/raw_hash.log: tests/generate_sequence.log
tests/from_stream.log: tests/generate_sequence.log
tests/parallel_fastq_hashing.log: tests/generate_fastq_sequence.log
tests/parallel_fastq_sequence_hashing.log: tests/generate_fastq_sequence.log
tests/multi_file_fastq.log: tests/generate_fastq_sequence.log
tests/from_stream_fastq.log: tests/generate_fastq_sequence.log
tests/merge.log: tests/generate_fastq_sequence.log
tests/min_qual.log: tests/generate_fastq_sequence.log
tests/parsers.log: tests/generate_sequence.log

########################################
# Unit tests
########################################
TESTS += unit_tests/unit_tests.sh
check_PROGRAMS += bin/test_offsets_key_value

bin_test_offsets_key_value_SOURCES = unit_tests/test_offsets_key_value.cc
bin_test_offsets_key_value_CXXFLAGS = -I$(top_srcdir)/unit_tests/gtest/include -I$(top_srcdir)/unit_tests/gtest
bin_test_offsets_key_value_LDADD = libgtest_main.la

include gtest.mk
