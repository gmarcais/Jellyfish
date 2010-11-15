#! /bin/sh

nCPUs=$(grep -c '^processor' /proc/cpuinfo)
pref=$(basename $0 .sh)
echo "Counting 10-mers on ${nCPUs} CPU"
../jellyfish/jellyfish count --matrix seq10m_matrix_10 -m 10 -t $nCPUs -o $pref -s 10000000 seq10m.fa && \
    echo "dcbb23c4a74a923c37a3b059f6a6d89a  ${pref}_0" | md5sum -c
RET=$?
rm -f ${pref}_*
exit $RET
