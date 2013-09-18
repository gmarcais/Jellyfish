#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF
bbceae352707eaa945ce2057137f3a7a ${pref}.histo
c697577f78593fef303b5fcaad86e336 ${pref}.stats
EOF

echo "Count on a very small file" && \
    ${DIR}/generate_sequence -v -s 2609132522 -o small 148 && \
    $JF count -m 20 -s 1M -t $nCPUs -o ${pref} -C --timing ${pref}.timing --stats ${pref}.stats small.fa && \
    $JF histo ${pref}_0 > ${pref}.histo && \
    check ${pref}.md5sum
RET=$?
  
echo "Small timing"; cat ${pref}.timing

exit $RET
