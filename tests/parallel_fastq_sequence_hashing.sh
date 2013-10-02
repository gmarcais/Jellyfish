#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF
8a0fe8ee1293f341fbde69e670beb74c ${pref}.histo
EOF
echo "Counting 22-mers, fastq format, no quality, on ${nCPUs} CPU" && \
    $JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs \
    -o ${pref} -s 10000000 --timing ${pref}.timing seq10m.fq && \
    $JF histo -f ${pref}_0 > ${pref}.histo && \
    check ${pref}.md5sum
RET=$?

cat ${pref}.timing
# [ -z "$NODEL" ] && \
#     rm ${pref}.md5sum ${pref}.histo ${pref}_* ${pref}.timing

exit $RET
