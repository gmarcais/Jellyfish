#! /usr/bin/env perl

use jellyfish;

my $qf = jellyfish::QueryMerFile->new(shift @ARGV);
for my $s (@ARGV) {
  print($s, " ", $qf->get(jellyfish::MerDNA->new($s)), "\n");
}
