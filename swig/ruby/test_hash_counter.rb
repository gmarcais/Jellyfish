require 'minitest/autorun'
require 'jellyfish'

class TestHashCounter < MiniTest::Unit::TestCase
  def setup
    @hash = Jellyfish::HashCounter.new(1024, 5)
  end

  def test_info
    assert_equal(1024, @hash.size)
    assert_equal(5, @hash.val_len)
#    assert_equal(1, @hash.nb_threads)
  end
end
