#! /usr/bin/env perl

use strict;
use warnings;
use jellyfish;

# jellyfish::MerDNA::k(10);
# my $m = jellyfish::MerDNA->new("ACGTACGTAC");
# print($m, " ", $m->get_reverse_complement(), "\n");
# for(my $i = 0; $i < jellyfish::MerDNA::k; $i++) {
#   print($m->get_base($i), " ");
# }
# print("\n");

my $qf = jellyfish::QueryMerFile->new("../tests/parallel_hashing_binary.jf");
print(jellyfish::MerDNA::k(), "\n");

my $rf = jellyfish::ReadMerFile->new("../tests/parallel_hashing_binary.jf");

while($rf->next2()) {
  print($rf->key(), " ", $rf->val(), " ", $qf->get($rf->key()), "\n");
}
