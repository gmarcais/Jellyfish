#! /bin/sh

. ./compat.sh

../jellyfish/test_double_fifo_input 1 seq1m_0.fa
RET=$?

exit $RET
