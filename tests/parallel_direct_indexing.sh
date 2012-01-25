#! /bin/sh

cd tests
. ./compat.sh

sort > ${pref}.md5sum <<EOF
dcbb23c4a74a923c37a3b059f6a6d89a ${pref}_0
8ac9533ccb34203fdac1f80b58774898 ${pref}.stats
EOF
echo "Counting 10-mers on ${nCPUs} CPU" && \
    $JF count --matrix seq10m_matrix_10 -m 10 -t $nCPUs \
    -o $pref -s 10000000 --timing ${pref}.timing \
    --stats ${pref}.stats seq10m.fa && \
    check ${pref}.md5sum
RET=$?

cat ${pref}.timing

exit $RET
