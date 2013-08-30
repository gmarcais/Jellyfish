ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = doc/jellyfish.pdf doc/jellyfish.man README LICENSE HalfLICENSE
man1_MANS = doc/jellyfish.man

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = jellyfish-2.0.pc

AM_LDFLAGS = -lpthread # $(VALGRIND_LIBS)
AM_CPPFLAGS = -Wall -Wnon-virtual-dtor -I$(top_srcdir) -I$(top_srcdir)/include -g -O3 $(VALGRIND_CFLAGS)
AM_CXXFLAGS = $(ALL_CXXFLAGS) -g -O3

noinst_HEADERS = $(YAGGO_SOURCES)
bin_PROGRAMS =
dist_bin_SCRIPTS =
data_DATA =
BUILT_SOURCES = $(YAGGO_SOURCES)
CLEANFILES =
DISTCLEANFILES = $(BUILT_SOURCES)

# Yaggo automatic rules with silencing
V_YAGGO = $(V_YAGGO_$(V))
V_YAGGO_ = $(V_YAGGO_$(AM_DEFAULT_VERBOSITY))
V_YAGGO_0 = @echo "  YAGGO " $@;
.yaggo.hpp:
	$(V_YAGGO)$(YAGGO) --license $(srcdir)/header-license -o $@ $<

YAGGO_SOURCES = # Append all file to be built by yaggo

# What to build
bin_PROGRAMS += bin/jellyfish
lib_LTLIBRARIES = libjellyfish-2.0.la
LDADD = libjellyfish-2.0.la # $(VALGRIND_LIBS)
# check_PROGRAMS =  bin/test_double_fifo_input	\
#                  bin/test_read_parser
check_PROGRAMS = bin/generate_sequence

############################
# Build Jellyfish the exec #
############################
bin_jellyfish_SOURCES = sub_commands/jellyfish.cc	\
                        sub_commands/count_main.cc	\
                        sub_commands/info_main.cc	\
                        sub_commands/dump_main.cc	\
                        sub_commands/histo_main.cc	\
                        sub_commands/merge_main.cc	\
                        sub_commands/bf_main.cc		\
                        sub_commands/query_main.cc	\
                        jellyfish/merge_files.cc
bin_jellyfish_LDFLAGS = $(AM_LDFLAGS) $(STATIC_FLAGS)


YAGGO_SOURCES += sub_commands/count_main_cmdline.hpp	\
                 sub_commands/info_main_cmdline.hpp	\
                 sub_commands/dump_main_cmdline.hpp	\
                 sub_commands/histo_main_cmdline.hpp	\
                 sub_commands/merge_main_cmdline.hpp	\
                 sub_commands/bf_main_cmdline.hpp	\
                 sub_commands/query_main_cmdline.hpp

######################################
# Build Jellyfish the shared library #
######################################
libjellyfish_2_0_la_LDFLAGS = -version-info 2:0:0
libjellyfish_2_0_la_SOURCES = lib/rectangular_binary_matrix.cc		\
                              lib/mer_dna.cc lib/storage.cc		\
                              lib/allocators_mmap.cc lib/misc.cc	\
                              lib/int128.cc lib/thread_exec.cc		\
                              lib/err.cc lib/jsoncpp.cpp		\
                              lib/time.cc

library_includedir=$(includedir)/jellyfish-@PACKAGE_VERSION@/jellyfish
JFI = include/jellyfish
library_include_HEADERS = $(JFI)/allocators_mmap.hpp			\
                          $(JFI)/backtrace.hpp $(JFI)/atomic_gcc.hpp	\
                          $(JFI)/large_hash_array.hpp $(JFI)/err.hpp	\
                          $(JFI)/misc.hpp				\
                          $(JFI)/offsets_key_value.hpp			\
                          $(JFI)/int128.hpp				\
                          $(JFI)/rectangular_binary_matrix.hpp		\
                          $(JFI)/mer_dna.hpp $(JFI)/storage.hpp		\
                          $(JFI)/simple_circular_buffer.hpp		\
                          $(JFI)/circular_buffer.hpp			\
                          $(JFI)/atomic_field.hpp			\
                          $(JFI)/compare_and_swap.hpp			\
                          $(JFI)/divisor.hpp				\
                          $(JFI)/large_hash_iterator.hpp		\
                          $(JFI)/jellyfish.hpp $(JFI)/thread_exec.hpp	\
                          $(JFI)/stream_iterator.hpp			\
                          $(JFI)/mer_overlap_sequence_parser.hpp	\
                          $(JFI)/whole_sequence_parser.hpp		\
                          $(JFI)/binary_dumper.hpp			\
                          $(JFI)/sorted_dumper.hpp			\
                          $(JFI)/text_dumper.hpp $(JFI)/dumper.hpp	\
                          $(JFI)/time.hpp $(JFI)/mer_heap.hpp		\
                          $(JFI)/token_ring.hpp				\
                          $(JFI)/locks_pthread.hpp			\
                          $(JFI)/file_header.hpp			\
                          $(JFI)/generic_file_header.hpp		\
                          $(JFI)/json.h $(JFI)/hash_counter.hpp		\
                          $(JFI)/mapped_file.hpp			\
                          $(JFI)/mer_dna_bloom_counter.hpp		\
                          $(JFI)/bloom_counter2.hpp			\
                          $(JFI)/cooperative_pool.hpp			\
                          $(JFI)/cooperative_pool2.hpp

noinst_HEADERS += jellyfish/fstream_default.hpp			\
                  include/jflib/tmpstream.hpp			\
                  jellyfish/mer_iterator.hpp jellyfish/dbg.hpp	\
                  jellyfish/randomc.h jellyfish/merge_files.hpp	\
                  jellyfish/cpp_array.hpp


###############
# Build tests #
###############
bin_generate_sequence_SOURCES = jellyfish/generate_sequence.cc		\
                                jellyfish/mersenne.cpp			\
                                jellyfish/backtrace.cc			\
                                jellyfish/dbg.cc
YAGGO_SOURCES += jellyfish/generate_sequence_cmdline.hpp

# bin_test_double_fifo_input_SOURCES =						\
#                                      jellyfish/test_double_fifo_input.cc	\
#                                      jellyfish/parse_dna.cc			\
#                                      jellyfish/file_parser.cc			\
#                                      jellyfish/sequence_parser.cc		\
#                                      jellyfish/backtrace.cc			\
#                                      jellyfish/thread_exec.cc			\
#                                      jellyfish/dbg.cc				\
#                                      jellyfish/time.cc				\
#                                      jellyfish/allocators_mmap.cc		\
#                                      jellyfish/dna_codes.cc
# bin_test_read_parser_SOURCES = jellyfish/test_read_parser.cc		\
#                                jellyfish/file_parser.cc			\
#                                jellyfish/read_parser.cc			\
#                                jellyfish/parse_read.cc			\
#                                jellyfish/dbg.cc jellyfish/backtrace.cc	\
#                                jellyfish/time.cc
# EXTRA_DIST += jellyfish/randomc.h jellyfish/generate_sequence_cmdline.hpp

#########
# Tests #
#########
TEST_EXTENSIONS = .sh
SH_LOG_COMPILER = /bin/sh
AM_SH_LOG_FLAGS =

TESTS = tests/generate_sequence.sh tests/parallel_hashing.sh	\
        tests/merge.sh tests/bloom_filter.sh tests/big.sh	\
        tests/subset_hashing.sh tests/multifile.sh
# TESTS = tests/generate_sequence.sh tests/serial_hashing.sh		\
#         tests/parallel_hashing.sh tests/serial_direct_indexing.sh	\
#         tests/parallel_direct_indexing.sh tests/raw_hash.sh		\
#         tests/generate_fastq_sequence.sh				\
#         tests/parallel_fastq_hashing.sh tests/multi_file.sh		\
#         tests/multi_file_fastq.sh tests/from_stream.sh			\
#         tests/parallel_fastq_sequence_hashing.sh			\
#         tests/from_stream_fastq.sh tests/merge.sh tests/min_qual.sh	\
#         tests/big.sh tests/parsers.sh tests/small.sh			\
#         tests/parallel_fastq_direct_indexing.sh

EXTRA_DIST += $(TESTS)
clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	-cd tests; rm -f *

#tests/serial_hashing.log: tests/generate_sequence.log
tests/parallel_hashing.log: tests/generate_sequence.log
tests/subset_hashing.log: tests/generate_sequence.log
tests/bloom_filter.log: tests/generate_sequence.log
#tests/serial_direct_indexing.log: tests/generate_sequence.log
#tests/parallel_direct_indexing.log: tests/generate_sequence.log
#tests/parallel_fastq_direct_indexing.log: tests/generate_fastq_sequence.log
tests/multi_file.log: tests/generate_sequence.log
#tests/raw_hash.log: tests/generate_sequence.log
#tests/from_stream.log: tests/generate_sequence.log
#tests/parallel_fastq_hashing.log: tests/generate_fastq_sequence.log
##tests/parallel_fastq_sequence_hashing.log: tests/generate_fastq_sequence.log
#tests/multi_file_fastq.log: tests/generate_fastq_sequence.log
##tests/from_stream_fastq.log: tests/generate_fastq_sequence.log
tests/merge.log: tests/generate_sequence.log
tests/min_qual.log: tests/generate_fastq_sequence.log

##############
# Unit tests #
##############
TESTS += unit_tests/unit_tests.sh
check_PROGRAMS += bin/test_all

bin_test_all_SOURCES = unit_tests/test_main.cc				\
	               unit_tests/test_misc.cc				\
	               unit_tests/test_offsets_key_value.cc		\
	               unit_tests/test_simple_circular_buffer.cc	\
	               unit_tests/test_rectangular_binary_matrix.cc	\
	               unit_tests/test_mer_dna.cc			\
	               unit_tests/test_large_hash_array.cc		\
	               unit_tests/test_mer_overlap_sequence_parser.cc	\
	               unit_tests/test_file_header.cc			\
	               unit_tests/test_mer_iterator.cc			\
	               unit_tests/test_hash_counter.cc			\
	               unit_tests/test_mer_heap.cc			\
	               unit_tests/test_stream_iterator.cc		\
	               unit_tests/test_token_ring.cc			\
	               unit_tests/test_text_dumper.cc			\
	               unit_tests/test_dumpers.cc			\
	               unit_tests/test_mapped_file.cc			\
	               unit_tests/test_int128.cc			\
	               unit_tests/test_mer_dna_bloom_counter.cc		\
	               unit_tests/test_whole_sequence_parser.cc		\
	               unit_tests/test_allocators_mmap.cc		\
	               unit_tests/test_cooperative_pool2.cc
bin_test_all_SOURCES += jellyfish/backtrace.cc

bin_test_all_CPPFLAGS = -Dprotected=public -Dprivate=public -DJSON_IS_AMALGAMATION=1
bin_test_all_CXXFLAGS = $(AM_CXXFLAGS) -I$(top_srcdir)/unit_tests/gtest/include -I$(top_srcdir)/unit_tests -I$(top_srcdir)/include
bin_test_all_LDADD = libgtest.la $(LDADD)
YAGGO_SOURCES += unit_tests/test_main_cmdline.hpp
noinst_HEADERS += unit_tests/test_main.hpp

include gtest.mk
-include $(top_srcdir)/development.mk

########
# info #
########
print-%:
	@echo -n $($*)
