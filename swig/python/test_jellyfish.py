#  This file is part of Jellyfish.
#
#  This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
#  You can choose between one of them if you use this work.
#
#  `SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`

import unittest
import sys
import re

import dna_jellyfish as jf

class TestMerFile(unittest.TestCase):
    def test_version(self):
        version = jf.__version__
        m = re.match('^(\d+)\.(\d+).(\d+)$', version)
        self.assertIsNotNone(m)
        self.assertGreater(int(m[1]), 0)
        self.assertGreaterEqual(int(m[2]), 0)
        self.assertGreaterEqual(int(m[3]), 0)

if __name__ == '__main__':
    data = sys.argv.pop(1)
    unittest.main()