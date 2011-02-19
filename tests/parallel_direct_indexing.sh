#! /bin/sh

nCPUs=$(grep -c '^processor' /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
pref=$(basename $0 .sh)
JF=../jellyfish/jellyfish
echo "Counting 10-mers on ${nCPUs} CPU" && \
    $JF count --matrix seq10m_matrix_10 -m 10 -t $nCPUs \
    -o $pref -s 10000000 --timing ${pref}.timing seq10m.fa && \
    echo "dcbb23c4a74a923c37a3b059f6a6d89a  ${pref}_0" | md5sum -c
RET=$?

cat ${pref}.timing
[ -z "$NODEL" ] && rm -f ${pref}_* ${pref}.timing
exit $RET
