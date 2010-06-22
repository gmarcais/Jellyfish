# This Makefile is meant to be run in the traget directories. There
# name start with a '_' by convention and contain a config.h.

CC = g++
CPPFLAGS = -Wall -Werror -O2 -g -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -I.
LDFLAGS = -lpthread
TARGETS = mer_counter dump_stats dump_stats_compacted hash_merge
SOURCES = $(patsubst %,%.cc,$(TARGETS)) mer_count_thread.cc

.SECONDARY:

VPATH=..:../lib

%.d: %.cc
	$(CC) -M -MM -MG -MP $(CPPFLAGS) $< > $@.tmp
	sed -e 's/$*.o/& $@/g' $@.tmp > $@ && rm $@.tmp

all: $(TARGETS)

compare_dump3: compare_dump3.o
dump_stats_compacted: dump_stats_compacted.o
total_mers_single_fasta: total_mers_single_fasta.o
mer_counter_C: mer_counter_C.o

hash_merge: hash_merge.o misc.o
mer_counter: mer_counter.o mer_count_thread.o storage.o hash_function.o misc.o
dump_mers: dump_mers.o storage.o hash_function.o misc.o
dump_stats: dump_stats.o storage.o hash_function.o misc.o

clean:
	rm -f $(TARGETS) *.o *.d

include $(SOURCES:.cc=.d)
