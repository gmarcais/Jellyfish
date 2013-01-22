#! /bin/sh

cd tests
. ./compat.sh

sort > ${pref}.md5sum <<EOF
72f1913b3503114c7df7a4dcc68ce867 ${pref}_m40_s16m.histo
72f1913b3503114c7df7a4dcc68ce867 ${pref}_automerge_m40_s1m.histo
72f1913b3503114c7df7a4dcc68ce867 ${pref}_m40_s1m_merged.histo
72f1913b3503114c7df7a4dcc68ce867 ${pref}_m40_s1m_text.histo
EOF

FILES="seq1m_0.fa seq1m_1.fa seq1m_0.fa seq1m_2.fa seq1m_2.fa"
echo $FILES | xargs $JF count -t $nCPUs -o ${pref}_m40_s16m -s 4M -C -m 40
$JF histo ${pref}_m40_s16m > ${pref}_m40_s16m.histo

ls | grep "^${pref}_m40_s1m" | xargs rm
echo $FILES | xargs $JF count -t $nCPUs -o ${pref}_m40_s1m -s 1M --disk --no-merge -C -m 40
$JF merge -o ${pref}_m40_s1m_merged ${pref}_m40_s1m[0-9]*

echo $FILES | xargs $JF count -t $nCPUs -o ${pref}_automerge_m40_s1m -s 1M --disk -C -m 40

echo $FILES | xargs $JF count -t $nCPUs -o ${pref}_m40_s1m_text -s 1M --text --disk -C -m 40

$JF histo ${pref}_automerge_m40_s1m > ${pref}_automerge_m40_s1m.histo
$JF histo ${pref}_m40_s1m_merged > ${pref}_m40_s1m_merged.histo
$JF histo ${pref}_m40_s1m_text > ${pref}_m40_s1m_text.histo

check ${pref}.md5sum
