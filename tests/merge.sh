#! /bin/sh

cd tests
. ./compat.sh

sort > ${pref}.md5sum <<EOF
c448e173e5e18264ed00fad0008d5340 ${pref}_quake.histo
EOF
echo "Counting mers and merging" && \
    $JF count --quake --matrix seq10m_matrix_19 -m 19 -t $nCPUs \
    -o ${pref}_quake -s 3000000 --timing ${pref}_quake.timing seq10m.fq && \
    $JF qmerge -s 10000000 -m 19 -o ${pref}_quake_merged ${pref}_quake_[012] && \
    $JF qhisto -f -h 3 -i 0.01 -l 0 ${pref}_quake_merged_0 > ${pref}_quake.histo && \
    check ${pref}.md5sum
RET=$?

echo "Qmer timing"; cat ${pref}_quake.timing 2>/dev/null
exit $RET
