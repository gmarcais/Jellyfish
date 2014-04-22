#! /usr/bin/perl

use strict;
use warnings;
use Test::More;

require_ok('jellyfish');

# Compare histo
{
  my $rf = jellyfish::ReadMerFile->new("sequence.jf");
  my @histo;
  $histo[$rf->val]++ while($rf->next2);

  open(my $io, "<", "sequence.histo");
  my @jf_histo;
  while(<$io>) {
    my ($freq, $count) = split;
    $jf_histo[$freq] = $count;
  }
  
  is_deeply(\@histo, \@jf_histo, "Histogram");
}

done_testing;
