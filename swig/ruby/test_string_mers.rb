#  This file is part of Jellyfish.
#
#  This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
#  You can choose between one of them if you use this work.
#
#  `SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`

require 'test/unit'
require 'jellyfish'

class TestStringMers < Test::Unit::TestCase
  def setup
    bases = "ACGTacgt"
    @str = (0..1000).map { bases[rand(bases.size())] }.join("")
    Jellyfish::MerDNA::k(rand(100) + 10)
  end

  def test_all_mers
    count = 0
    m2 = Jellyfish::MerDNA.new

    @str.mers.each_with_index { |m, i|
      assert_equal m.to_s, @str[i, Jellyfish::MerDNA::k()].upcase
      m2.set @str[i, Jellyfish::MerDNA::k()]
      assert_equal m2, m
      count += 1
    }
    assert_equal @str.size - Jellyfish::MerDNA::k() + 1, count
  end

  def test_canonical_mers
    count = 0
    m2 = Jellyfish::MerDNA.new
    @str.canonicals { |m|
      m2.set @str[count, Jellyfish::MerDNA::k()]
      cm2 = m2.get_reverse_complement
      assert(m2 == m || m == cm2)
      assert(!(m > m2) && !(m > cm2));
      count += 1
    }      
    assert_equal @str.size - Jellyfish::MerDNA::k() + 1, count
  end
end
