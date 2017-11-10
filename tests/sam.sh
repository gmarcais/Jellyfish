#! /bin/sh

cd tests
. ./compat.sh

if ! grep -q '#define HAVE_HTSLIB' ../config.h; then
    echo "Skip SAM/BAM/CRAM file format test"
    exit 77
fi

sort -k2,2 > ${pref}.md5sum <<EOF
8f8a71e04c27cd88918f11d44d9b3852 ${pref}_sam.histo
8f8a71e04c27cd88918f11d44d9b3852 ${pref}_bam.histo
8f8a71e04c27cd88918f11d44d9b3852 ${pref}_cram.histo
f0faf797cc55add8b6e88ab67bbcf19b ${pref}_sam_qual.histo
f0faf797cc55add8b6e88ab67bbcf19b ${pref}_bam_qual.histo
f0faf797cc55add8b6e88ab67bbcf19b ${pref}_cram_qual.histo
EOF

comp_histo() {
    set -x
    suffix=$1
    $JF count -t $nCPUs -m 20 -s 10M -o ${pref}_${suffix}.jf -C --sam seq10m.${suffix}
    $JF histo ${pref}_${suffix}.jf > ${pref}_${suffix}.histo
    $JF count -t $nCPUs -m 20 -s 10M -o ${pref}_${suffix}_qual.jf -C -Q D --sam seq10m.${suffix}
    $JF histo ${pref}_${suffix}_qual.jf > ${pref}_${suffix}_qual.histo
}

comp_histo sam

if [ -f seq10m.bam ]; then
    comp_histo bam
else
    cp ${pref}_sam.histo ${pref}_bam.histo
    cp ${pref}_sam_qual.histo ${pref}_bam_qual.histo
fi

if [ -f seq10m.cram ]; then
    comp_histo cram
else
    cp ${pref}_sam.histo ${pref}_cram.histo
    cp ${pref}_sam_qual.histo ${pref}_cram_qual.histo
fi

check ${pref}.md5sum
