## Process this file with automake to produce Makefile.in

AM_LDFLAGS = -lpthread
AM_CPPFLAGS = -Wall -Werror -g -O2 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -I$(top_srcdir)

if MACOSX
AM_CPPFLAGS += -arch x86_64 -I../gnulib
endif

lib_LTLIBRARIES = libjellyfish-1.0.la
libjellyfish_1_0_la_SOURCES = square_binary_matrix.cc fasta_parser.cc

bin_PROGRAMS = jellyfish
if MACOSX
jellyfish_LDADD = ../gnulib/libgnu.a libjellyfish-1.0.la
else
jellyfish_LDADD = libjellyfish-1.0.la
endif


jellyfish_SOURCES = jellyfish.cc dump_stats_compacted.cc hash_merge.cc storage.cc misc.cc mer_counter.cc histogram.cc time.cc thread_exec.cc query_main.cc

library_includedir=$(includedir)/jellyfish-1.0/jellyfish
library_include_HEADERS = compacted_hash.hpp mapped_file.hpp misc.hpp square_binary_matrix.hpp mer_counting.hpp
