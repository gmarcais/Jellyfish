import unittest
import sys
import random
import jellyfish

class TestHashCounter(unittest.TestCase):
    def setUp(self):
        jellyfish.MerDNA.k(100)
        self.hash = jellyfish.HashCounter(1024, 5)

    def test_info(self):
        self.assertEqual(100, jellyfish.MerDNA.k())
        self.assertEqual(1024, self.hash.size())
        self.assertEqual(5, self.hash.val_len())

    def test_add(self):
        mer  = jellyfish.MerDNA()
        good = True
        for i in range(1000):
            mer.randomize()
            val = random.randrange(1000)
            good = good and self.hash.add(mer, val)
            if not good: break
            if i % 3 > 0:
                nval = random.randrange(1000)
                val  = val + nval
                if i % 3 == 1:
                    good = good and (not self.hash.add(mer, nval))
                else:
                    good = good and self.hash.update_add(mer, nval)
            if not good: break
            good = good and (val == self.hash.get(mer)) and (val == self.hash[mer])
            if not good: break
        self.assertTrue(good)


if __name__ == '__main__':
    data = sys.argv.pop(1)
    unittest.main()
