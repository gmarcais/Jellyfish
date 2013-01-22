#! /bin/sh

cd tests
. ./compat.sh

sort > ${pref}.md5sum <<EOF 
864c0b0826854bdc72a85d170549b64b ${pref}_m15_s2M.histo
864c0b0826854bdc72a85d170549b64b ${pref}_m15_s16M.histo
376761a6e273b57b3428c14e3b536edf ${pref}_binary.dump
376761a6e273b57b3428c14e3b536edf ${pref}_text.dump
9251799dd5dbd3f617124aa2ff72112a ${pref}_binary.histo
9251799dd5dbd3f617124aa2ff72112a ${pref}_text.histo
EOF

# Count with in memory hash doubling
$JF count -t $nCPUs -o ${pref}_m15_s2M -s 2M -C -m 15 seq10m.fa
$JF histo ${pref}_m15_s2M > ${pref}_m15_s2M.histo
rm ${pref}_m15_s2M

# Count without size doubling
$JF count -t $nCPUs -o ${pref}_m15_s16M -s 16M -C -m 15 seq10m.fa
$JF histo ${pref}_m15_s16M > ${pref}_m15_s16M.histo
rm ${pref}_m15_s16M

# Count large merges in binary and text. Should agree
$JF count -m 40 -t $nCPUs -o ${pref}_text -s 2M --text seq1m_0.fa
$JF count -m 40 -t $nCPUs -o ${pref}_binary -s 2M seq1m_0.fa
$JF histo ${pref}_text > ${pref}_text.histo
$JF histo ${pref}_binary > ${pref}_binary.histo
$JF dump -c ${pref}_text | sort > ${pref}_text.dump
$JF dump -c ${pref}_binary | sort > ${pref}_binary.dump

# $JF count -m 40 -t $nCPUs -o ${pref}_text -s 2M --text seq1m_0.fa
# $JF info -s ${pref}_text0 | sort >  ${pref}_text0_dump

# $JF count -m 40 -t $nCPUs -o ${pref}_bin -s 2M seq1m_0.fa
# $JF dump -c ${pref}_bin0 | sort > ${pref}_bin0_dump

# $JF histo ${pref}_bin0 > ${pref}.histo

# Count all k-mers
# $JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs -o $pref \
#     -s 10000000 --timing ${pref}.timing --stats ${pref}.stats seq10m.fa
# # Output only counts >= 2
# $JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs -o ${pref}_L \
#     -s 10000000 --timing ${pref}.timing -L 2 seq10m.fa
# # Stream output
# $JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs -o /dev/fd/1 -O \
#     -s 10000000 --timing ${pref}.timing --stats ${pref}.stats seq10m.fa | \
#     cat > ${pref}_S

# $JF histo -f ${pref}_0 > ${pref}.histo
# $JF dump -c ${pref}_L_0 > ${pref}_L.dump
# $JF dump -c -L 2 ${pref}_0 > ${pref}.dump
# cat ${pref}_0 | $JF dump -c -L 2 /dev/fd/0 > ${pref}_stream.dump
# echo "GCCATTTCGATTAAAGAATGAT TAGGCATGCAACGCTTCCCTTT" | $JF query ${pref}_0 > ${pref}.query
# $JF dump -c ${pref}_0 > ${pref}_all.dump
# $JF dump -c ${pref}_S > ${pref}_S_all.dump

check ${pref}.md5sum

# cat ${pref}.timing

