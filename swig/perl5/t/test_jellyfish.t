#  This file is part of Jellyfish.
#
#  This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
#  You can choose between one of them if you use this work.
#
#  `SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`

use strict;
use warnings;
use Test::More;

require_ok('jellyfish');

# Test version
{
my $version = $jellyfish::VERSION;
my $m = $version =~ m/^(\d+)\.(\d+)\.(\d+)$/;
ok($m, , "Version match regexp");
ok($1 > 0, "Major positive");
ok($2 >= 0, "Minor");
ok($3 >= 0, "Revision");
}

done_testing;