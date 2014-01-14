#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF
46c5d23c3560cf908fea997fff8abc1c ${pref}.histo
46c5d23c3560cf908fea997fff8abc1c ${pref}_q@.histo
d41d8cd98f00b204e9800998ecf8427e ${pref}_qi.histo
4faa0517f41d7dc808ec0b930fe0d88e ${pref}_qC.histo
EOF

count_histo () {
    PREFIX=$1
    shift
    $JF count -t $nCPUs -o $PREFIX.jf -s 10M -C -m 15 seq10m.fq "$@"
    $JF histo $PREFIX.jf > $PREFIX.histo
}

count_histo ${pref}
count_histo ${pref}_q@ -Q '@'
count_histo ${pref}_qi -Q 'i'
count_histo ${pref}_qC -Q 'C'

check ${pref}.md5sum
