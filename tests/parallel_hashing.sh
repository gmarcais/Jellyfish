#! /bin/sh

cd tests
. ./compat.sh

sort > ${pref}.md5sum <<EOF 
376761a6e273b57b3428c14e3b536edf ${pref}_text0_dump
EOF
echo "Counting 22-mers on ${nCPUs} CPU"
pwd

$JF count -m 40 -t $nCPUs -o ${pref}_text -s 2M --text seq1m_0.fa
$JF info -s ${pref}_text0 | sort >  ${pref}_text0_dump

$JF count -m 40 -t $nCPUs -o ${pref}_bin -s 2M seq1m_0.fa

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

