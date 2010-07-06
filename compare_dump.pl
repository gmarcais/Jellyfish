#! /usr/bin/perl

use strict;
use warnings;
use POSIX qw(_exit);
use Getopt::Long;
use GenomeUtils::FDProgress;

my ($help, $debug, $o_diff);

my $usage = <<EOS
$0 [options] dump1 dump2

  --diff FILE           Output difference in FILE.
  --debug               Show progress.
  --help                This message.
EOS
    ;

GetOptions("diff=s"             => \$o_diff,
           "debug"              => \$debug,
           "help"               => \$help) or die($usage);

if($help) { print($usage); exit(0); }
if(@ARGV != 2) { die("Missing arguments\n", $usage); }

my $d = GenomeUtils::FDProgress->debug($debug);

my ($dump1, $dump2) = @ARGV;
my ($diff_io);

if($o_diff) {
  open($diff_io, ">", $o_diff) or
    die("Can't open '$o_diff': $!");
}

my %h1;
my ($common, $equal) = (0, 0);

my $io1 = $d->open_read($dump1);
my $count;
while(<$io1>) {
  if(/^>(\d+)/) {
    $count = $1;
  } else {
    chomp;
    $h1{$_} += $count;
  }
}
$d->close($io1);
my $size1 = scalar(keys %h1);

my $io2 = $d->open_read($dump2);
my $size2 = 0;
while(<$io2>) {
  if(/^>(\d+)/) {
    $count = $1;
  } else {
    chomp;
    $size2++;
    my $c1 = delete($h1{$_});
    if(defined($c1)) {
      $common++;
      if($c1 == $count) {
        $equal++;
      } elsif(defined($diff_io)) {
        print($diff_io "$_ $c1 $count\n");
      }
    } else {
      print($diff_io "$_ - $count\n") if defined($diff_io);
    }
  }
}
$d->close($io2);

if(defined($diff_io)) {
  while(my ($k, $v) = each %h1) {
    print($diff_io "$k $v -\n");
  }
}

print("Sizes $size1 $size2\n");
print("Common $common Equal $equal\n");

close($diff_io) if defined($diff_io);

close(STDOUT);
close(STDERR);

my $success = $size1 == $size2 && $common == $size1 && $equal == $size1;
_exit(!$success);
