use strict;
use warnings;
use Test::More;

require_ok('jellyfish');
my $data = shift(@ARGV);

# Compare histo
{
  my $rf = jellyfish::ReadMerFile->new($data . "/sequence.jf");
  my @histo;
  $histo[$rf->val]++ while($rf->next_mer);

  open(my $io, "<", $data . "/sequence.histo");
  my @jf_histo;
  while(<$io>) {
    my ($freq, $count) = split;
    $jf_histo[$freq] = $count;
  }
  
  is_deeply(\@histo, \@jf_histo, "Histogram");
}

# Compare dump
{
  my $rf = jellyfish::ReadMerFile->new($data . "/sequence.jf");
  my $equal = open(my $io, "<", $data . "/sequence.dump");
  while(<$io>) {
    my ($mer, $count) = split;
    $equal &&= $rf->next_mer;
    $equal &&= ($mer eq $rf->key);
    $equal &&= ($count == $rf->val);
    last unless $equal;
  }
  $equal &&= !$rf->next_mer;
  ok($equal, "Dump");
}

# Query
{
  my $rf   = jellyfish::ReadMerFile->new($data . "/sequence.jf");
  my $qf   = jellyfish::QueryMerFile->new($data . "/sequence.jf");
  my $good = 1;
  while($rf->next_mer) {
    $good &&= $rf->val == $qf->get($rf->key) or
        (ok($good, "Query mer") || last);
  }
  ok($good, "Query identical to read");
}

done_testing;
