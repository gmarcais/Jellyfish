#! /bin/sh

nCPUs=$(grep -c '^processor' /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
pref=$(basename $0 .sh)
JF=../jellyfish/jellyfish
cat > ${pref}.md5sum <<EOF
8a0fe8ee1293f341fbde69e670beb74c  ${pref}.histo
EOF
echo "Counting 22-mers, fastq format, no quality, on ${nCPUs} CPU" && \
    $JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs \
    -o ${pref} -s 10000000 --timing ${pref}.timing seq10m.fq && \
    $JF histo ${pref}_0 > ${pref}.histo && \
    md5sum -c ${pref}.md5sum
RET=$?

cat ${pref}.timing
[ -z "$NODEL" ] && \
    rm ${pref}.md5sum ${pref}.histo ${pref}_* ${pref}.timing

exit $RET
