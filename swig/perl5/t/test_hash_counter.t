use strict;
use warnings;
use Test::More;
use jellyfish;

require_ok('jellyfish');
my $data = shift(@ARGV);

jellyfish::MerDNA::k(100);
my $hash = jellyfish::HashCounter->new(1024, 5);

# Check info
ok(100 == jellyfish::MerDNA::k, "mer_dna k");
ok(1024 == $hash->size, "Hash size");
ok(5 == $hash->val_len, "Hash value length");

# Test adding mers
{
  my $mer  = jellyfish::MerDNA->new;
  my $good = 1;
  for(my $i = 0; $i < 1000; $i++) {
    $mer->randomize();
    my $val = int(rand(1000));
    $good &&= $hash->add($mer, $val) or
        (ok($good, "Adding to new mer") || last);
    if($i % 3 > 0) {
      my $nval = int(rand(1000));
      $val += $nval;
      if($i % 3 == 1) {
        $good &&= !$hash->add($mer, $nval) or
            (ok($good, "Adding to existing mer") || last);
      } else {
        $good &&= $hash->update_add($mer, $nval) or
            (ok($good, "Updating existing mer") || last);
      }
    }
    $good &&= $val == $hash->get($mer) or
        (ok($good, "Value in hash") || last);

  }
  ok($good, "Adding mer to hash");
}

done_testing;
