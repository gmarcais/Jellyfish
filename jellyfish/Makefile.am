## Process this file with automake to produce Makefile.in

LDFLAGS = -lpthread
CPPFLAGS = -Wall -Werror -g -O2 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -I.

if MACOSX
CPPFLAGS += -arch x86_64 -I../gnulib
endif

bin_PROGRAMS = jellyfish
if MACOSX
jellyfish_LDADD = ../gnulib/libgnu.a
endif
jellyfish_SOURCES = jellyfish.cc dump_stats_compacted.cc square_binary_matrix.cc hash_merge.cc storage.cc misc.cc mer_counter.cc histogram.cc time.cc thread_exec.cc fasta_parser.cc query_main.cc
