use strict;
use warnings;
use Test::More;

require_ok('jellyfish');
jellyfish::MerDNA::k(int(rand(100)) + 10);

my @bases = ("A", "C", "G", "T", "a", "c", "g", "t");
my $str;
for(my $i = 0; $i < 1000; $i++) {
  $str .= $bases[int(rand(8))];
}

my $count = 0;
my $m2 = jellyfish::MerDNA->new;
my $mers = jellyfish::string_mers($str);
my $good_mers = 1;
my $good_strs = 1;
while($mers->next_mer) {
  my $mstr = uc(substr($str, $count, jellyfish::MerDNA::k()));
  $m2->set($mstr);
  $good_strs &&= $mers->mer == $m2;
  $good_mers &&= $mers->mer eq $mstr;
  $count++;
}
ok($good_mers, "Mers equal");
ok($good_strs, "Strs equal");
ok($count == length($str) - jellyfish::MerDNA::k() + 1, "Number of mers");

done_testing;
