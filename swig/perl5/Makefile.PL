#! /usr/bin/env perl

use strict;
use warnings;
use ExtUtils::MakeMaker;

sub pkgconfig {
  my ($cmd) = @_;
  my $res = `pkg-config $cmd`;
  if($?) {
    print(STDERR "\nCan't get compilation information for Jellyfish, check you PKG_CONFIG_PATH\n");
    exit(1);
  }
  chomp $res;
  return $res;
}

my $pkg = "jellyfish-2.0";
my $jf_cflags = pkgconfig("--cflags $pkg");
my $jf_libs   = pkgconfig("--libs $pkg");
my $jf_rpath  = pkgconfig("--libs-only-L $pkg");
$jf_rpath =~ s/-L/-Wl,-rpath,/g;

print("$jf_cflags\n$jf_libs\n$jf_rpath\n");

WriteMakefile(NAME      => 'jellyfish',
              CC        => 'g++',
              LD        => 'g++',
              CCFLAGS   => "-std=c++0x $jf_cflags",
              LIBS      => "$jf_libs",
              OBJECT    => 'swig_wrap.o',
              VERSION   => '0.0.1');
