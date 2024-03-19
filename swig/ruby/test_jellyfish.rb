#  This file is part of Jellyfish.
#
#  This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
#  You can choose between one of them if you use this work.
#
#  `SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`

require 'test/unit'
require 'jellyfish'


class TestMerFile < Test::Unit::TestCase
    def test_version
        m = /^(\d+)\.(\d+)\.(\d+)$/ =~ Jellyfish::VERSION
        assert_not_nil m
        assert_true($1.to_i > 0)
        assert_true($2.to_i >= 0)
        assert_true($3.to_i >= 0)
    end
end