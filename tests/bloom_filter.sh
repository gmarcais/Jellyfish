#! /bin/sh

cd tests
. ./compat.sh

sort > ${pref}.md5sum <<EOF
9251799dd5dbd3f617124aa2ff72112a ${pref}.histo
9251799dd5dbd3f617124aa2ff72112a ${pref}_filtered.histo
EOF

$JF bf -t $nCPUs -o ${pref}.bf -s 1M -C -m 40 seq1m_0.fa seq1m_0.fa
# Counting without filtering
$JF count -t $nCPUs -o ${pref}.jf -s 2M -C -m 40 seq1m_0.fa
# Filtereing should not do anything here: all mers are loaded twice in the bf
$JF count -t $nCPUs -o ${pref}_filtered.jf --bf ${pref}.bf -s 2M -C -m 40 seq1m_0.fa

$JF histo ${pref}.jf > ${pref}.histo
$JF histo ${pref}_filtered.jf > ${pref}_filtered.histo

check ${pref}.md5sum
