#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF
8ebb01305cbb36754ef060c1e37d6e4d ${pref}.histo
EOF
echo "Counting 22-mers on ${nCPUs} CPU" &&      \
    $JF count -q --matrix seq10m_matrix_22 -m 22 -t $nCPUs \
    -o $pref -s 5000000 --timing ${pref}.timing seq1m_*.fq && \
    $JF qhisto -f -l 0.0 -h 2.0 -i 0.1 ${pref}_0 > ${pref}.histo && \
    check ${pref}.md5sum
RET=$?

cat ${pref}.timing
# [ -z "$NODEL" ] && \
#     rm -f ${pref}_* ${pref}.md5sum ${pref}.histo ${pref}.timing
exit $RET
