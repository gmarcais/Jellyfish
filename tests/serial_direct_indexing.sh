#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF
dcbb23c4a74a923c37a3b059f6a6d89a ${pref}_0
EOF
echo "Counting 10-mers on 1 CPU" && \
    $JF count --matrix seq10m_matrix_10 -m 10 -t 1 \
    -o $pref -s 10000000 --timing ${pref}.timing seq10m.fa && \
    check ${pref}.md5sum
RET=$?
cat ${pref}.timing
# [ -z "$NODEL" ] && rm -f ${pref}_* ${pref}.timing ${pref}.md5sum
exit $RET
