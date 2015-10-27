require 'minitest/autorun'
require 'jellyfish'

$data = ARGV.shift

class TestMerFile < MiniTest::Unit::TestCase
  def setup
    @mf = Jellyfish::ReadMerFile.new(File.join($data, "swig_ruby.jf"))
  end

  def test_histo
    histo = []
    histo[@mf.count] = (histo[@mf.count] || 0) + 1 while @mf.next_mer

    jf_histo = []
    open(File.join($data, "swig_ruby.histo")) { |f|
      f.each_line.each { |l|
        freq, count = l.split.map {|x| x.to_i }
        jf_histo[freq] = count
      }
    }
    
    assert_equal jf_histo, histo
  end

  def test_each
    open(File.join($data, "swig_ruby.dump")) { |f|
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
    open(File.join($data, "swig_ruby.dump")) { |f|
      f.each_line.each { |l|
        mer, count = l.split
        assert @mf.next_mer
        assert_equal(mer, @mf.mer.to_s)
        assert_equal(count.to_i, @mf.count)
      }
    }
    assert !@mf.next_mer
  end

  def test_query
    query = Jellyfish::QueryMerFile.new(File.join($data, "swig_ruby.jf"))
    @mf.each { |m, c|
      assert_equal c, query[m]
    }
  end
end
