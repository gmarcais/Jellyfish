#! /bin/sh

cd tests
. ./compat.sh

sort > ${pref}.md5sum <<EOF
8bfdd1b137b73ebfed05d0496bbddd0c ${pref}.histo
9418b1a2e05a2526a81ae9b1200ed5df ${pref}_Q.histo
EOF

echo "Counting 10-mers on ${nCPUs} CPU"
$JF count --matrix seq10m_matrix_10 -m 10 -t $nCPUs \
    -o $pref -s 10M -C --timing ${pref}.timing seq1m_0.fq
$JF histo ${pref}_0 > ${pref}.histo
$JF count --matrix seq10m_matrix_10 -m 10 -t $nCPUs \
    --quality-start 64 --min-quality 5 \
    -o ${pref}_Q -s 10M -C seq1m_0.fq
$JF histo ${pref}_Q_0 > ${pref}_Q.histo

check ${pref}.md5sum

cat ${pref}.timing

