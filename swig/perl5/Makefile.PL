#! /usr/bin/env perl

use strict;
use warnings;
use ExtUtils::MakeMaker;

mkdir('lib');
system('swig', '-c++', '-perl5', '-o', 'jellyfish_wrap.cxx', '../jellyfish.i');

my $jf_cflags = `pkg-config --cflags jellyfish-2.0`;
my $jf_libs   = `pkg-config --libs jellyfish-2.0`;
my $jf_rpath  = `pkg-config --libs-only-L jellyfish-2.0 | sed -e 's/-L/-Wl,-rpath,/g'`;

WriteMakefile(NAME      => 'jellyfish',
              CC        => 'g++-4.4',
              LD        => 'g++-4.4',
              CCFLAGS   => "-std=c++0x $jf_cflags",
              LIBS      => "$jf_libs",
              OBJECT    => 'jellyfish_wrap.o',
              VERSION   => '0.0.1');
