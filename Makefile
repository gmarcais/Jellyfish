# This Makefile is meant to be run in the traget directories. There
# name start with a '_' by convention and contain a config.h.

CC = g++
CPPFLAGS = -Wall -Werror -g -O2 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -I.
# Uncomment following to use SSE
# CPPFLAGS += -msse -msse2 -DSSE
LDFLAGS = -lpthread
TARGETS = jellyfish 
# old TARGETS = mer_counter dump_stats dump_stats_compacted hash_merge
SOURCES = $(patsubst %,%.cc,$(TARGETS)) mer_count_thread.cc

.SECONDARY:

#VPATH=..:../lib

%.d: %.cc
	$(CC) -M -MM -MG -MP $(CPPFLAGS) $< > $@.tmp
	sed -e 's/$*.o/& $@/g' $@.tmp > $@ && rm $@.tmp

all: $(TARGETS)

jellyfish: dump_stats_compacted.o square_binary_matrix.o hash_merge.o storage.o misc.o mer_count_thread.o mer_counter.o

# compare_dump3: compare_dump3.o
# dump_stats_compacted: dump_stats_compacted.o square_binary_matrix.o
total_mers_single_fasta: total_mers_single_fasta.o
mer_counter_C: mer_counter_C.o

# hash_merge: hash_merge.o storage.o misc.o
# mer_counter: mer_counter.o mer_count_thread.o storage.o misc.o
# dump_mers: dump_mers.o storage.o misc.o
dump_stats: dump_stats.o storage.o misc.o

clean:
	rm -f $(TARGETS) *.o *.d

# Is the following OK? Shouldn't we include more than that?
include $(SOURCES:.cc=.d)
-include local.mk
