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

# Compare dump
{
  my $rf = jellyfish::ReadMerFile->new("sequence.jf");
  my $equal = open(my $io, "<", "sequence.dump");
  while(<$io>) {
    my ($mer, $count) = split;
    $equal &&= $rf->next2;
    $equal &&= ($mer eq $rf->key);
    $equal &&= ($count == $rf->val);
    last unless $equal;
  }
  $equal &&= !$rf->next2;
  ok($equal, "Dump");
}

done_testing;
