#! /usr/bin/env python

import jellyfish
import sys

mf = jellyfish.ReadMerFile(sys.argv[1])
for mer, count in mf:
    print("%s %d" % (mer, count))
