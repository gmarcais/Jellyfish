#! /usr/bin/env python

import jellyfish
import sys

qf = jellyfish.QueryMerFile(sys.argv[1])
for str in sys.argv[2:]:
    print("%s %d" % (str, qf[jellyfish.MerDNA(str)]))

