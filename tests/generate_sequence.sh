#! /bin/sh

cd tests
. ./compat.sh

${DIR}/generate_sequence -v -o seq10m -m 10 -m 22 -s 3141592653 10000000
${DIR}/generate_sequence -v -o seq1m -s 1040104553 1000000 1000000 1000000 1000000 1000000

for i in 0 1 2 3 4; do
    gzip -c seq1m_$i.fa > seq1m_$i.fa.gz
done

${DIR}/generate_sequence -v -q -o seq10m -s 1473540700 10000000
ln -sf seq10m.fq seq10m.fastq
${DIR}/fastq2sam seq10m.fastq
if [ -n "$SAMTOOLS" ]; then
    $SAMTOOLS view -b -o seq10m.bam seq10m.sam
    $SAMTOOLS view -C -o seq10m.cram seq10m.sam
fi
