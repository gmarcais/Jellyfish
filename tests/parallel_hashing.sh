#! /bin/sh

cd tests
. ./compat.sh

sort > ${pref}.md5sum <<EOF 
4fd24c05f7c18c47e7b69f77aa071f1f ${pref}_0
7059a4e90b6670b2d814e44e2bc7d429 ${pref}.histo
c3233e107bb6b42d0c979707f156264c ${pref}.query
dbe881e4649406321d0e481da08eab5c ${pref}_L.dump
dbe881e4649406321d0e481da08eab5c ${pref}.dump
dbe881e4649406321d0e481da08eab5c ${pref}_stream.dump
77054a9564aaf59fb330dfa6af92b428 ${pref}_all.dump
77054a9564aaf59fb330dfa6af92b428 ${pref}_S_all.dump
a210906960cf36c09eecad62a4c04973 ${pref}.stats
EOF
echo "Counting 22-mers on ${nCPUs} CPU"

# Count all k-mers
$JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs -o $pref \
    -s 10000000 --timing ${pref}.timing --stats ${pref}.stats seq10m.fa
# Output only counts >= 2
$JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs -o ${pref}_L \
    -s 10000000 --timing ${pref}.timing -L 2 seq10m.fa
# Stream output
$JF count --matrix seq10m_matrix_22 -m 22 -t $nCPUs -o /dev/fd/1 -O \
    -s 10000000 --timing ${pref}.timing --stats ${pref}.stats seq10m.fa | \
    cat > ${pref}_S

$JF histo -f ${pref}_0 > ${pref}.histo
$JF dump -c ${pref}_L_0 > ${pref}_L.dump
$JF dump -c -L 2 ${pref}_0 > ${pref}.dump
cat ${pref}_0 | $JF dump -c -L 2 /dev/fd/0 > ${pref}_stream.dump
echo "GCCATTTCGATTAAAGAATGAT TAGGCATGCAACGCTTCCCTTT" | $JF query ${pref}_0 > ${pref}.query
$JF dump -c ${pref}_0 > ${pref}_all.dump
$JF dump -c ${pref}_S > ${pref}_S_all.dump

check ${pref}.md5sum

cat ${pref}.timing

