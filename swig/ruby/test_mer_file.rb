require 'minitest/autorun'
require 'jellyfish'

$data = ARGV.shift

class TestMerFile < MiniTest::Unit::TestCase
  def setup
    @mf = Jellyfish::ReadMerFile.new(File.join($data, "sequence.jf"))
  end

  def test_histo
    histo = []
    histo[@mf.val] = (histo[@mf.val] || 0) + 1 while @mf.next_mer

    jf_histo = []
    open(File.join($data, "sequence.histo")) { |f|
      f.lines.each { |l|
        freq, count = l.split.map {|x| x.to_i }
        jf_histo[freq] = count
      }
    }
    
    assert_equal jf_histo, histo
  end

  def test_each
    open(File.join($data, "sequence.dump")) { |f|
      @mf.each { |m, c|
        l = f.readline
        assert l
        fm, fc = l.split
        assert_equal fm, m.to_s
        assert_equal fc.to_i, c
      }
      assert_raises(EOFError) { f.readline }
    }
  end

  def test_dump
    open(File.join($data, "sequence.dump")) { |f|
      f.lines.each { |l|
        mer, count = l.split
        assert @mf.next_mer
        assert_equal(mer, @mf.key.to_s)
        assert_equal(count.to_i, @mf.val)
      }
    }
    assert !@mf.next_mer
  end
end
