#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF 
EOF

echo "Test double fifo" &&
$DIR/test_double_fifo_input 1 seq1m_0.fa > ${pref}_double_fifo.stats &&
echo "Test read parser" &&
$DIR/test_read_parser seq10m.fa > ${pref}_read_parser_fa.dump
RET=$?

exit $RET
