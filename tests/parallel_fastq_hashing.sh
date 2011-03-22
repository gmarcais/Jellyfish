#! /bin/sh

. ./compat.sh

cat > ${pref}.md5sum <<EOF
c448e173e5e18264ed00fad0008d5340  ${pref}.histo
EOF
echo "Counting 19-mers qmers, on ${nCPUs} CPU" && \
    $JF count --quake --matrix seq10m_matrix_19 -m 19 -t $nCPUs \
    -o $pref -s 10000000 --timing ${pref}.timing seq10m.fq && \
    $JF qhisto -h 3 -i 0.01 -l 0 ${pref}_0 > ${pref}.histo && \
    ${MD5} -c ${pref}.md5sum
RET=$?

cat ${pref}.timing
[ -z "$NODEL" ] && \
    rm ${pref}.md5sum ${pref}.histo ${pref}_* ${pref}.timing

exit $RET
