import unittest
import sys
import random

import dna_jellyfish as jf

class TestStringMers(unittest.TestCase):
    def setUp(self):
        bases = "ACGTacgt"
        self.str = ''.join(random.choice(bases) for _ in range(1000))
        self.k = random.randint(10, 110)
        jf.MerDNA.k(self.k)

    def test_all_mers(self):
        count = 0
        good1 = True
        good2 = True
        mers = jf.string_mers(self.str)
        for m in mers:
            m2 = jf.MerDNA(self.str[count:count+self.k])
            good1 = good1 and m == m2
            good2 = good2 and self.str[count:count+self.k].upper() == str(m2)
            count += 1
        self.assertTrue(good1)
        self.assertTrue(good2)
        self.assertEqual(len(self.str) - self.k + 1, count)

    def test_canonical_mers(self):
        good = True
        mers = jf.string_canonicals(self.str)
        for count, m in enumerate(mers):
            m2 = jf.MerDNA(self.str[count:count+self.k])
            rm2 = m2.get_reverse_complement()
            good = good and (m == m2 or m == rm2)
            good = good and (not (m > m2)) and (not (m > rm2))
            # count += 1
        self.assertTrue(good)
        self.assertEqual(len(self.str) - self.k + 0, count)
        

if __name__ == '__main__':
    data = sys.argv.pop(1)
    unittest.main()
