#! /bin/sh

cd tests
. ./compat.sh

sort -k2,2 > ${pref}.md5sum <<EOF
4fd24c05f7c18c47e7b69f77aa071f1f ${pref}_0
7059a4e90b6670b2d814e44e2bc7d429 ${pref}.histo
EOF
echo "Counting 22-mers on 1 CPU" && \
    $JF count --matrix seq10m_matrix_22 -m 22 -t 1 -o ${pref} \
    --timing ${pref}.timing -s 10000000 seq10m.fa && \
    $JF histo -f ${pref}_0 > ${pref}.histo && \
    check ${pref}.md5sum
RET=$?

cat ${pref}.timing
# [ -z "$NODEL" ] && rm -f ${pref}_* ${pref}.timing ${pref}.md5sum* ${pref}.histo

exit $RET
