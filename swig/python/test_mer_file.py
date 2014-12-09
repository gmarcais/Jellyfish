import jellyfish
import unittest
import sys
import os
from collections import Counter

class TestMerFile(unittest.TestCase):
    def setUp(self):
        self.mf = jellyfish.ReadMerFile(os.path.join(data, "swig_python.jf"))

    def test_histo(self):
        histo = Counter()
        while self.mf.next_mer():
            histo[self.mf.count()] += 1

        jf_histo = Counter()
        with open(os.path.join(data, "swig_python.histo")) as f:
            for line in f:
                num, count = [int(n) for n in line.split()]
                self.assertEqual(count, histo[num])

    def test_dump(self):
        good = True
        with open(os.path.join(data, "swig_python.dump")) as f:
            for line in f:
                good = good and self.mf.next_mer()
                if not good: break
                a = line.split()
                good = good and a[0] == str(self.mf.mer()) and int(a[1]) == self.mf.count()
                if not good: break
        self.assertTrue(good)

    def test_iter(self):
        good = True
        with open(os.path.join(data, "swig_python.dump")) as f:
            for mer, count in self.mf:
                line = f.readline()
                good = good and line
                if not good: break
                fmer, fcount = line.split()
                good = good and fmer == str(mer) and int(fcount) == count
                if not good: break
            self.assertTrue(good)
            line = f.readline()
            self.assertTrue(not line)

    def test_query(self):
        good = True
        qf   = jellyfish.QueryMerFile(os.path.join(data, "swig_python.jf"))
        for mer, count in self.mf:
            good = good and count == qf[mer]
            if not good: break
        self.assertTrue(good)

if __name__ == '__main__':
    data = sys.argv.pop(1)
    unittest.main()
