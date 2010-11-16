TEST_EXTENSIONS = .sh
SH_LOG_COMPILER = /bin/sh
AM_SH_LOG_FLAGS =

TESTS = generate_sequence.sh serial_hashing.sh parallel_hashing.sh	\
serial_direct_indexing.sh parallel_direct_indexing.sh

EXTRA_DIST = $(TESTS)
clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	-rm -f seq10m*


serial_hashing.log: generate_sequence.log
parallel_hashing.log: generate_sequence.log
serial_direct_indexing.log: generate_sequence.log
parallel_direct_indexing.log: generate_sequence.log
