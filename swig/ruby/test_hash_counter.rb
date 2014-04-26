require 'minitest/autorun'
require 'jellyfish'

class TestHashCounter < MiniTest::Unit::TestCase
  def setup
    Jellyfish::MerDNA::k(100)
    @hash = Jellyfish::HashCounter.new(1024, 5)
  end

  def test_info
    assert_equal(100, Jellyfish::MerDNA::k)
    assert_equal(1024, @hash.size)
    assert_equal(5, @hash.val_len)
#    assert_equal(1, @hash.nb_threads)
  end

  def test_add
    mer = Jellyfish::MerDNA.new

    1000.times { |i|
      mer.randomize!
      val = rand(1000)
      assert(@hash.add(mer, val))
      if i % 3 > 0
        nval = rand(1000)
        val += nval
        if i % 3 == 1
          assert(!@hash.add(mer, nval))
        else
          assert(@hash.update_add(mer, nval))
        end
      end
      assert_equal(val, @hash.get(mer))
      assert_equal(val, @hash[mer])
    }

    mer.randomize!
    assert_nil(@hash.get(mer))
    assert_nil(@hash[mer])
    assert(!@hash.update_add(mer, 1))
  end
end
