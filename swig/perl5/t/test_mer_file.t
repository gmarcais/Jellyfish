use strict;
use warnings;
use Test::More;

require_ok('jellyfish');
my $data = shift(@ARGV);

# Compare histo
{
  my $rf = jellyfish::ReadMerFile->new($data . "/swig_perl.jf");
  my @histo;
  $histo[$rf->count]++ while($rf->next_mer);

  open(my $io, "<", $data . "/swig_perl.histo");
  my @jf_histo;
  while(<$io>) {
    my ($freq, $count) = split;
    $jf_histo[$freq] = $count;
  }
  
  is_deeply(\@histo, \@jf_histo, "Histogram");
}

# Compare dump
{
  my $rf = jellyfish::ReadMerFile->new($data . "/swig_perl.jf");
  my $equal = open(my $io, "<", $data . "/swig_perl.dump");
  while(<$io>) {
    my ($mer, $count) = split;
    $equal &&= $rf->next_mer;
    $equal &&= ($mer eq $rf->mer);
    $equal &&= ($count == $rf->count);
    last unless $equal;
  }
  $equal &&= !$rf->next_mer;
  ok($equal, "Dump");
}

# Query
{
  my $rf   = jellyfish::ReadMerFile->new($data . "/swig_perl.jf");
  my $qf   = jellyfish::QueryMerFile->new($data . "/swig_perl.jf");
  my $good = 1;
  while(my ($mer, $count) = $rf->each) {
    $good &&= $count == $qf->get($mer) or
        (ok($good, "Query mer") || last);
  }
  ok($good, "Query identical to read");
}

done_testing;
