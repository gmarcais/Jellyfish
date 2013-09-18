#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF
5c5d07dfb4e3de89b7fe4c72c714b921 ${pref}.stats
dbe881e4649406321d0e481da08eab5c ${pref}_L.dump
c3233e107bb6b42d0c979707f156264c ${pref}.query
7059a4e90b6670b2d814e44e2bc7d429 ${pref}.histo
EOF
echo "Counting 22-mers on ${nCPUs} CPU" && \
    $JF count --matrix seq10m_matrix_22 -m 22 -c 2 -t $nCPUs \
    -o $pref -s 10000000 --raw --timing ${pref}.timing seq10m.fa && \
    $JF stats ${pref}_0 > ${pref}.stats && \
    $JF dump -c -L 2 ${pref}_0 > ${pref}_L.dump && \
    $JF histo -f ${pref}_0 > ${pref}.histo &&
    echo "GCCATTTCGATTAAAGAATGAT TAGGCATGCAACGCTTCCCTTT" | $JF query ${pref}_0 > ${pref}.query && \
    check ${pref}.md5sum
RET=$?

cat ${pref}.timing
# [ -z "$NODEL" ] && \
#     rm -f ${pref}_* ${pref}.md5sum ${pref}.timing ${pref}.stats
exit $RET
