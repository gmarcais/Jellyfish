#! /usr/bin/env ruby

require 'minitest/autorun'
require 'jellyfish'


class TestMerFile < MiniTest::Unit::TestCase
  def setup
    @mf = Jellyfish::ReadMerFile.new("sequence.jf")
  end

  def test_histo
    histo = []
    histo[@mf.val] = (histo[@mf.val] || 0) + 1 while @mf.next2

    jf_histo = []
    open("sequence.histo") { |f|
      f.lines.each { |l|
        freq, count = l.split.map {|x| x.to_i }
        jf_histo[freq] = count
      }
    }
    
    assert_equal jf_histo, histo
  end

  def test_dump
    open("sequence.dump") { |f|
      f.lines.each { |l|
        mer, count = l.split
        assert @mf.next2
        assert_equal(mer, @mf.key.to_s)
        assert_equal(count.to_i, @mf.val)
      }
    }
    assert !@mf.next2
  end
end
