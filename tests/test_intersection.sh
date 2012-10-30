#! /bin/sh

cd tests
. ./compat.sh

RLEN=200
MLEN=40
ALL=2000000
UNIQ=1000000
MULT=1000000

$GS -o intersection_all -s 2703731828 -r $RLEN $ALL
$GS -o intersection_uniq -s 2139959797 -r $RLEN $UNIQ
$GS -o intersection_mult -s 3348383312 -r $RLEN $MULT

cat intersection_all.fa intersection_mult.fa intersection_uniq.fa > uniq.fa
cat intersection_mult.fa intersection_all.fa > mult.fa

time $JF -s 2M -m $MLEN -t 1 uniq.fa mult.fa intersection_all.fa

test "$((($RLEN - $MLEN + 1) * ($ALL / $RLEN)))" = "$(wc -l < intersection)"
test "$((($RLEN - $MLEN + 1) * ($UNIQ / $RLEN)))" = "$(wc -l < uniq_uniq.fa)"
test "0" = "$(wc -l < uniq_mult.fa)"
test "0" = "$(wc -l < uniq_intersection_all.fa)"

