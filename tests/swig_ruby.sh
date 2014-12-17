#! /bin/sh

LOADPATH=$(pwd)/swig/ruby/.libs

cd tests
. ./compat.sh
[ -z "$ENABLE_RUBY_BINDING" ] && exit 77

K=$($RUBY -e 'print(rand(15) + 1)')
$JF count -m $K -s 10M -t $nCPUs -C -o ${pref}.jf
$JF dump ${pref}.jf > ${pref}.dump
$JF histo ${pref}.jf > ${pref}.histo

for i in test_mer_file.rb test_hash_counter.rb; do
    echo Test $i
    $RUBY "-I$LOADPATH" ../$(dirname $0)/../swig/ruby/$i .
done
