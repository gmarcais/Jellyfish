#! /bin/sh

mkdir -p tests-data; cd tests-data
. ../compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF
72f1913b3503114c7df7a4dcc68ce867 ${pref}_m40_s16m.histo
72f1913b3503114c7df7a4dcc68ce867 ${pref}_automerge_m40_s1m.histo
72f1913b3503114c7df7a4dcc68ce867 ${pref}_m40_s1m_merged.histo
72f1913b3503114c7df7a4dcc68ce867 ${pref}_m40_s1m_text.histo
4199aa97e646281b9c36a03564f082ee ${pref}_m9_min.histo
10b5ca10bcf85183f82837ea10638a8d ${pref}_m9_max.histo
0ff4b7a2f3f67fd26011a41f820bdf36 ${pref}_m9_jaccard
EOF

# Compare counts without merging, with explicit merging and implicit merging
FILES="seq1m_0.fa seq1m_1.fa seq1m_0.fa seq1m_2.fa seq1m_2.fa"
echo $FILES | xargs $JF count -t $nCPUs -o ${pref}_m40_s16m.jf -s 4M -C -m 40
$JF histo ${pref}_m40_s16m.jf > ${pref}_m40_s16m.histo

ls | grep "^${pref}_m40_s1m[0-9].*" | xargs rm -f
echo $FILES | xargs $JF count -t $nCPUs -o ${pref}_m40_s1m -s 1M --disk --no-merge -C -m 40
$JF merge -o ${pref}_m40_s1m_merged.jf ${pref}_m40_s1m[0-9]*
ls | grep "^${pref}_m40_s1m[0-9].*" | xargs rm -f

echo $FILES | xargs $JF count -t $nCPUs -o ${pref}_automerge_m40_s1m.jf -s 1M --disk -C -m 40

echo $FILES | xargs $JF count -t $nCPUs -o ${pref}_m40_s1m_text.jf -s 1M --text --disk -C -m 40

$JF histo ${pref}_automerge_m40_s1m.jf > ${pref}_automerge_m40_s1m.histo
$JF histo ${pref}_m40_s1m_merged.jf > ${pref}_m40_s1m_merged.histo
$JF histo ${pref}_m40_s1m_text.jf > ${pref}_m40_s1m_text.histo

# Compute weighted intersection (min), weighted uniont (max) and Jaccard
$JF count -t $nCPUs -o ${pref}_m9_2.jf -C -m 9 -s 4M seq1m_2.fa
$JF count -t $nCPUs -o ${pref}_m9_3.jf -C -m 9 -s 4M seq1m_3.fa
$JF merge -m -o ${pref}_m9_min.jf ${pref}_m9_2.jf ${pref}_m9_3.jf
$JF merge -M -o ${pref}_m9_max.jf ${pref}_m9_2.jf ${pref}_m9_3.jf
$JF merge -j -o ${pref}_m9_jaccard ${pref}_m9_2.jf ${pref}_m9_3.jf
$JF histo ${pref}_m9_min.jf > ${pref}_m9_min.histo
$JF histo ${pref}_m9_max.jf > ${pref}_m9_max.histo


check ${pref}.md5sum
