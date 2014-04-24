#! /usr/bin/env python

import jellyfish
import unittest
import sys
import os
from collections import Counter

class TestMerFile(unittest.TestCase):
    def setUp(self):
        self.mf = jellyfish.ReadMerFile(os.path.join(data, "sequence.jf"))

    def test_histo(self):
        histo = Counter()
        while self.mf.next_mer():
            histo[self.mf.val()] += 1

        jf_histo = Counter()
        with open(os.path.join(data, "sequence.histo")) as f:
            for line in f:
                a = [int(n) for n in line.split()]
                self.failUnlessEqual(a[1], histo[a[0]])

    def test_dump(self):
        good = True
        with open(os.path.join(data, "sequence.dump")) as f:
            for line in f:
                good = good and self.mf.next_mer()
                if not good: break
                a = line.split()
                goot = good and a[0] == str(self.mf.key()) and int(a[1]) == self.mf.val()
                if not good: break;
        self.assertTrue(good)

if __name__ == '__main__':
    data = sys.argv.pop(1)
    unittest.main()
