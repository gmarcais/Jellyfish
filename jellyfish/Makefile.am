## Process this file with automake to produce Makefile.in

CPPFLAGS = -Wall -Werror -g -O2 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -I. -I../gnulib -arch x86_64
LDFLAGS = -lpthread

bin_PROGRAMS = jellyfish
jellyfish_LDADD = ../gnulib/libgnu.a
jellyfish_SOURCES = jellyfish.cc dump_stats_compacted.cc square_binary_matrix.cc hash_merge.cc storage.cc misc.cc mer_count_thread.cc mer_counter.cc

