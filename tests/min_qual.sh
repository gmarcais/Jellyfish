#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF
7f78bc16dedc972fc253bf1378e01561 ${pref}.histo
EOF

echo "Counting 22-mers on ${nCPUS} CPU" && \
    $JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs -o $pref \
    -s 10000000 --timing ${pref}.timing --min-quality 3 seq10m.fq && \
    $JF histo -f ${pref}_0 > ${pref}.histo && \
    check ${pref}.md5sum
RET=$?

cat ${pref}.timing 2>/dev/null
exit $RET
