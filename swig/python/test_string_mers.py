import unittest
import sys
import random
import jellyfish

class TestStringMers(unittest.TestCase):
    def setUp(self):
        bases = "ACGTacgt"
        self.str = ''.join(random.choice(bases) for _ in range(1000))
        self.k = random.randint(10, 110)
        jellyfish.MerDNA.k(self.k)

    def test_all_mers(self):
        count = 0
        good = True
        mers = jellyfish.string_mers(self.str)
        for m in mers:
            m2 = jellyfish.MerDNA(self.str[count:count+self.k])
            good = good and m == m2
            count += 1
        self.assertTrue(good)
        self.assertEqual(len(self.str) - self.k + 1, count)

    def test_canonical_mers(self):
        good = True
        mers = jellyfish.string_canonicals(self.str)
        for count, m in enumerate(mers):
            m2 = jellyfish.MerDNA(self.str[count:count+self.k])
            rm2 = m2.get_reverse_complement()
            good = good and (m == m2 or m == rm2)
            good = good and (not (m > m2)) and (not (m > rm2))
            # count += 1
        self.assertTrue(good)
        self.assertEqual(len(self.str) - self.k + 0, count)
        

if __name__ == '__main__':
    data = sys.argv.pop(1)
    unittest.main()
