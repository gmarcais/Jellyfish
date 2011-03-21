TEST_EXTENSIONS = .sh
SH_LOG_COMPILER = /bin/sh
AM_SH_LOG_FLAGS =

TESTS = generate_sequence.sh serial_hashing.sh parallel_hashing.sh	\
serial_direct_indexing.sh parallel_direct_indexing.sh raw_hash.sh	\
generate_fastq_sequence.sh parallel_fastq_hashing.sh multi_file.sh	\
multi_file_fastq.sh from_stream.sh parallel_fastq_sequence_hashing.sh

EXTRA_DIST = $(TESTS)
clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	-rm -f seq10m* seq1m*

serial_hashing.log: generate_sequence.log
parallel_hashing.log: generate_sequence.log
serial_direct_indexing.log: generate_sequence.log
parallel_direct_indexing.log: generate_sequence.log
multi_file.log: generate_sequence.log
raw_hash.log: generate_sequence.log
from_stream.log: generate_sequence.log
parallel_fastq_hashing.log: generate_fastq_sequence.log
parallel_fastq_sequence_hashing.log: generate_fastq_sequence.log
multi_file_fastq.log: generate_fastq_sequence.log
