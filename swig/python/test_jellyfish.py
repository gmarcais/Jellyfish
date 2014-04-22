#! /usr/bin/env python

import jellyfish

jellyfish.MerDNA.k(10)
m = jellyfish.MerDNA("ACGTACGTAC")
print(m, m.get_reverse_complement())
print(m[0])
