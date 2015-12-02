#! /bin/sh

set -e

NCPUS=$(grep -c '^processor' /proc/cpuinfo)

autoreconf -i
./configure YAGGO=~/bin/yaggo
make
make check || { cat test-suite.log; false; }
