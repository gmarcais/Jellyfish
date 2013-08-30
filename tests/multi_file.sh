#! /bin/sh

cd tests
. ./compat.sh

sort > ${pref}.md5sum <<EOF
d93b7678037814c256d1d9120a0e6422 ${pref}_m15_s2M.histo
EOF

# Count multiple files with many readers
$JF count -t $nCPUs -R 4 -o ${pref}_m15_s2M.jf -s 2M -C -m 15 seq1m_0.fa seq1m_1.fa seq1m_2.fa seq10m.fa seq1m_3.fa seq1m_4.fa
$JF histo ${pref}_m15_s2M.jf > ${pref}_m15_s2M.histo

check ${pref}.md5sum
