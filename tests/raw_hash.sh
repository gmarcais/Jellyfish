#! /bin/sh

. ./compat.sh

cat > ${pref}.md5sum <<EOF
5c5d07dfb4e3de89b7fe4c72c714b921  ${pref}.stats
EOF
echo "Counting 22-mers on ${nCPUs} CPU" && \
    $JF count --matrix seq10m_matrix_22 -m 22 -c 2 -t $nCPUs \
    -o $pref -s 10000000 --raw \
    --timing ${pref}.timing seq10m.fa && \
    $JF rstats ${pref}_0 > ${pref}.stats && \
    ${MD5} -c ${pref}.md5sum
RET=$?

cat ${pref}.timing
[ -z "$NODEL" ] && \
    rm -f ${pref}_* ${pref}.md5sum ${pref}.timing ${pref}.stats
exit $RET
