#! /bin/sh

nCPUs=$(grep -c '^processor' /proc/cpuinfo)
pref=$(basename $0 .sh)
echo "Counting 22-mers on ${nCPUs} CPU"
../jellyfish/jellyfish count --matrix seq10m_matrix_22 -m 22 -c 2 -t $nCPUs -o $pref -s 10000000 --raw \
    --timing ${pref}.timing seq10m.fa
echo "5c5d07dfb4e3de89b7fe4c72c714b921  -" > ${pref}.md5sum
../jellyfish/jellyfish rstats ${pref}_0 | md5sum -c ${pref}.md5sum
RET=$?
rm ${pref}.md5sum

if [ -f ${pref}.timing ]; then
    cat ${pref}.timing
    rm ${pref}.timing
fi


rm -f ${pref}_*
exit $RET
