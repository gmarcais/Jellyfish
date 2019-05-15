#! /bin/sh

cd tests
. ../compat.sh

for i in $(seq 2 10); do
    $JF count -t $nCPUs -o ${pref}_m${i}_10M_25.jf -s 10M -c 25 -m $i -C seq10m.fa
    $JF count -t $nCPUs -o ${pref}_m${i}_10M_5.jf -s 1k -c 5 -m $i -C seq10m.fa
    diff -q <($JF histo ${pref}_m${i}_10M_25.jf) <($JF histo ${pref}_m${i}_10M_5.jf)
done
