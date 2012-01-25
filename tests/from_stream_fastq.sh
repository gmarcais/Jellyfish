#! /bin/sh

cd tests
. ./compat.sh

sort > ${pref}.md5sum <<EOF 
c448e173e5e18264ed00fad0008d5340 ${pref}.histo
EOF

echo "Counting 22-mers on ${nCPUs} CPU" &&      \
    cat seq10m.fq | $JF count -q --matrix seq10m_matrix_19 -m 19 -t $nCPUs \
    -o $pref -s 10000000 --timing ${pref}.timing /dev/fd/0 && \
    $JF qhisto -f -h 3 -i 0.01 -l 0.0 ${pref}_0 > ${pref}.histo &&      \
    check ${pref}.md5sum
RET=$?

cat ${pref}.timing
# [ -z "$NODEL" ] && \
#     rm -f ${pref}_* ${pref}.histo ${pref}.md5sum ${pref}.timing

exit $RET
