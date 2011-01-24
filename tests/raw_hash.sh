#! /bin/sh

nCPUs=$(grep -c '^processor' /proc/cpuinfo)
pref=$(basename $0 .sh)
echo "Counting 22-mers on ${nCPUs} CPU"
../jellyfish/jellyfish count --matrix seq10m_matrix_22 -m 22 -c 2 -t $nCPUs -o $pref -s 10000000 --raw \
    --timing ${pref}.timing seq10m.fa
echo "b4aacdd081e328f74c7d1e6700752ec8  ${pref}_0" | md5sum -c
RET=$?

if [ -f ${pref}.timing ]; then
    cat ${pref}.timing
    rm ${pref}.timing
fi


#rm -f ${pref}_*
exit $RET
