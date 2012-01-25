#! /bin/sh

cd tests
. ./compat.sh

${DIR}/generate_sequence -v -o seq10m -m 10 -m 22 -s 3141592653 10000000
${DIR}/generate_sequence -v -o seq1m -s 1040104553 1000000 1000000 1000000 1000000 1000000
