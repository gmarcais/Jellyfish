#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF
c448e173e5e18264ed00fad0008d5340 ${pref}.histo
554c9c76fb1f54f2c7526dd1af5aa252 ${pref}_lines.dump
174c7b3873c8aaa5ffee36d5d405bbce ${pref}.stats
EOF

echo "Counting 19-mers qmers, on ${nCPUs} CPU"
$JF count --quake --matrix seq10m_matrix_19 -m 19 -t $nCPUs \
    -o $pref -s 10000000 --timing ${pref}.timing --stats ${pref}.stats seq10m.fq
$JF qhisto -f -h 3 -i 0.01 -l 0 ${pref}_0 > ${pref}.histo
$JF qdump -c -L 0.035 -U 0.905 ${pref}_0 | wc -l | awk '{ print $1 }' > ${pref}_lines.dump
check ${pref}.md5sum

cat ${pref}.timing

