#! /bin/sh

nCPUs=$(grep -c '^processor' /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
pref=$(basename $0 .sh)
cat > ${pref}.md5sum <<EOF
5c5d07dfb4e3de89b7fe4c72c714b921  -
EOF
JF=../jellyfish/jellyfish
echo "Counting 22-mers on ${nCPUs} CPU" && \
    $JF count --matrix seq10m_matrix_22 -m 22 -c 2 -t $nCPUs \
    -o $pref -s 10000000 --raw \
    --timing ${pref}.timing seq10m.fa && \
    $JF rstats ${pref}_0 | md5sum -c ${pref}.md5sum
RET=$?

cat ${pref}.timing
[ -z "$NODEL" ] && \
    rm -f ${pref}_* ${pref}.md5sum ${pref}.timing
exit $RET
