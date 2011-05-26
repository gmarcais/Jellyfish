#! /bin/sh

if [ -z "$BIG" ]; then
    echo "Skip big test"
    exit 77
fi

. ./compat.sh

sort > ${pref}.md5sum <<EOF 
019a57f22208f638f58b68a3131c4692 ${pref}.histo
EOF

echo "Generate 30 Gigs of sequence and count k-mers on it" && \
    ([ -f seq30g.fa ] || ../jellyfish/generate_sequence -v -o seq30g -r 1000 -s 1602176487 30000000000) && \
    $JF count -m 16 -s 4000000000 -o ${pref} -c 4 -p 254 -C --out-counter-len 2 \
    -t $nCPUs --timing ${pref}.timing seq30g.fa && \
    $JF histo ${pref}_0 > ${pref}.histo && \
    check ${pref}.md5sum
RET=$?

cat ${pref}.timing

exit $RET
