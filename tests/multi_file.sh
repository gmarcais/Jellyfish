#! /bin/sh

cd tests
. ./compat.sh

sort > ${pref}.md5sum <<EOF
0e2705617b44144daedc5f9e35ba7b93 ${pref}.stats
EOF
echo "Counting 22-mers on ${nCPUs} CPU" &&      \
    $JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs \
    -o $pref -s 5000000 --timing ${pref}.timing seq1m_*.fa && \
    $JF stats ${pref}_0 > ${pref}.stats && \
    check ${pref}.md5sum
RET=$?

# [ -z "$NODEL" ] && \
#     rm -f ${pref}_* ${pref}.md5sum ${pref}.timing ${pref}.stats
exit $RET
