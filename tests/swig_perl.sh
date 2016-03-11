#! /bin/sh

cd tests
. ./compat.sh
[ -z "$ENABLE_PERL_BINDING" ] && exit 77

LOADPATH="$BUILDDIR/swig/perl5"
K=$($PERL -e 'print(int(rand(16)) + 6)')
I=$($PERL -e 'print(int(rand(5)))')
$JF count -m $K -s 10M -t $nCPUs -C -o ${pref}.jf seq1m_$I.fa
$JF dump -c ${pref}.jf > ${pref}.dump
$JF histo ${pref}.jf > ${pref}.histo

for i in test_mer_file.t test_hash_counter.t test_string_mers.t; do
    echo Test $i
    $PERL "-I$LOADPATH/.libs" "-I$LOADPATH" "-I$SRCDIR/swig/perl5" "$SRCDIR/swig/perl5/t/$i" .
done
