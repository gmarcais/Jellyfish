#! /bin/sh

mkdir -p tests-data; cd tests-data
. ../compat.sh

${DIR}/generate_sequence -v -q -o seq10m -m 19 -s 2718281828 10000000
${DIR}/generate_sequence -v -q -o seq1m -s 3246465313 1000000 1000000 1000000 1000000 1000000
