#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF
dcbb23c4a74a923c37a3b059f6a6d89a ${pref}_0
7b7419dea9e3917c2e27b7a7a35f64ca ${pref}.histo
7b7419dea9e3917c2e27b7a7a35f64ca ${pref}_S.histo
8ac9533ccb34203fdac1f80b58774898 ${pref}.stats
8c9400cd7064ea24374fe68817da9926 ${pref}_LU.histo
EOF

echo "Counting 10-mers on ${nCPUs} CPU"
$JF count --matrix seq10m_matrix_10 -m 10 -t $nCPUs \
    -o $pref -s 10000000 --timing ${pref}.timing \
    --stats ${pref}.stats seq10m.fa
$JF count --matrix seq10m_matrix_10 -m 10 -t $nCPUs \
    -o /dev/fd/1 -O -s 10M seq10m.fa | cat > ${pref}_S
$JF count --matrix seq10m_matrix_10 -m 10 -t $nCPUs \
    -o ${pref}_LU -s 10M -C -L 2 -U 4 seq1m_0.fa

$JF histo ${pref}_0 > ${pref}.histo
$JF histo ${pref}_S > ${pref}_S.histo
$JF histo ${pref}_LU_0 > ${pref}_LU.histo

check ${pref}.md5sum

cat ${pref}.timing

