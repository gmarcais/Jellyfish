#! /bin/sh

nCPUs=$(grep -c '^processor' /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
pref=$(basename $0 .sh)
JF=../jellyfish/jellyfish
cat > ${pref}.md5sum <<EOF
641f95f9e8b11523379c832f44ed037e  ${pref}.stats
EOF
echo "Counting 22-mers on ${nCPUs} CPU" &&      \
    $JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs \
    -o $pref -s 5000000 --timing ${pref}.timing seq1m_*.fa && \
    $JF stats ${pref}_0 > ${pref}.stats && \
    md5sum -c ${pref}.md5sum
RET=$?

[ -z "$NODEL" ] && \
    rm -f ${pref}_* ${pref}.md5sum ${pref}.timing ${pref}.stats
exit $RET
