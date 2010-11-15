#! /bin/sh

nCPUs=$(grep -c '^processor' /proc/cpuinfo)
pref=$(basename $0 .sh)
echo "Counting 22-mers on ${nCPUs} CPU"
../jellyfish/jellyfish count --matrix seq10m_matrix_22 -m 22 -t $nCPUs -o $pref -s 10000000 seq10m.fa && \
    echo "c52c09131223cad72d4417e3227f84fe  ${pref}_0" | md5sum -c
RET=$?
rm -f ${pref}_*
exit $RET
