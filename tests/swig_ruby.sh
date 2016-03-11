#! /bin/sh

cd tests
. ./compat.sh
[ -z "$ENABLE_RUBY_BINDING" ] && exit 77

LOADPATH="$BUILDDIR/swig/ruby/.libs"
K=$($RUBY -e 'print(rand(15) + 6)')
I=$($RUBY -e 'print(rand(5))')
$JF count -m $K -s 10M -t $nCPUs -C -o ${pref}.jf seq1m_$I.fa
$JF dump -c ${pref}.jf > ${pref}.dump
$JF histo ${pref}.jf > ${pref}.histo



for i in test_mer_file.rb test_hash_counter.rb test_string_mers.rb; do
    echo Test $i
    $RUBY "-I$LOADPATH" "$SRCDIR/swig/ruby/$i" .
done
