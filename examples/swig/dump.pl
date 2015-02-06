#! /usr/bin/env perl

use jellyfish;

my $mf = jellyfish::ReadMerFile->new($ARGV[0]);
while($mf->next_mer) {
  print($mf->mer, " ", $mf->count, "\n");
}
