#  This file is part of Jellyfish.
#
#  This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
#  You can choose between one of them if you use this work.
#
#  `SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`

import warnings

warnings.warn("Module 'jellyfish' is deprecated. Use 'dna_jellyfish'", DeprecationWarning)
import dna_jellyfish as jellyfish
