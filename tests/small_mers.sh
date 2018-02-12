#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF

EOF

$JF count -t $nCPUs -o ${pref}_m10_10M.jf -s 1M -m 7 -C seq10m.fa
$JF histo ${pref}_m10_10M.jf > ${pref}_m10_10M.histo
