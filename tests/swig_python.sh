#! /bin/sh


export PYTHONPATH=$(pwd)/swig/python/.libs:$(pwd)/swig/python${PYTHONPATH+:$PYTHONPATH}

cd tests
. ./compat.sh
[ -z "$ENABLE_PYTHON_BINDING" ] && exit 77

K=$($PYTHON -c 'import random; print(random.randint(6, 20))')
$JF count -m $K -s 10M -t $nCPUs -C -o ${pref}.jf
$JF dump ${pref}.jf > ${pref}.dump
$JF histo ${pref}.jf > ${pref}.histo

for i in test_mer_file.py test_hash_counter.py; do
    echo Test $i
    $PYTHON ../$(dirname $0)/../swig/python/$i .
done
