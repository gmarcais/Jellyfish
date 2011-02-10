#! /bin/sh

nCPUs=$(grep -c '^processor' /proc/cpuinfo)
pref=$(basename $0 .sh)
JF=../jellyfish/jellyfish
echo -e "c52c09131223cad72d4417e3227f84fe  parallel_hashing_0\n7059a4e90b6670b2d814e44e2bc7d429  parallel_hashing.histo\nc3233e107bb6b42d0c979707f156264c  parallel_hashing.query" > ${pref}.md5sum
echo "Counting 22-mers on ${nCPUs} CPU" &&      \
    $JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs -o $pref -s 10000000 --timing ${pref}.timing seq10m.fa && \
    $JF histo ${pref}_0 > ${pref}.histo &&      \
    echo "GCCATTTCGATTAAAGAATGAT TAGGCATGCAACGCTTCCCTTT" | $JF query ${pref}_0 > ${pref}.query && \
    md5sum -c ${pref}.md5sum
RET=$?

if [ -f ${pref}.timing ]; then
    cat ${pref}.timing
    rm ${pref}.timing
fi

rm -f ${pref}_* ${pref}.histo ${pref}.query ${pref}.md5sum
exit $RET
