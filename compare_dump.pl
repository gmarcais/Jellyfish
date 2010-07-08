#! /usr/bin/perl

use strict;
use warnings;
use POSIX qw(_exit);
use Getopt::Long;
use GenomeUtils::FDProgress;

my ($help, $debug, $o_diff, $o_duplicated);

my $usage = <<EOS
$0 [options] dump1 dump2

  --diff FILE           Output difference in FILE
  --duplicated FILE	Report duplicated entries in dump1
  --debug               Show progress
  --help                This message
EOS
    ;

GetOptions("diff=s"             => \$o_diff,
	   "duplicated=s"	=> \$o_duplicated,
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

my (%h1, $dup_io);
my ($common, $equal) = (0, 0);

if($o_duplicated) {
  open($dup_io, ">", $o_duplicated) or die("Can't open '$o_duplicated': $!");
}

my $io1 = $d->open_read($dump1);
my $count;
while(<$io1>) {
  if(/^>(\d+)/) {
    $count = $1;
  } else {
    chomp;
    if($o_duplicated && defined($h1{$_})) {
      print($dup_io "$_ $h1{$_} $count\n");
    }
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
close($dup_io) if defined($dup_io);

my $success = $size1 == $size2 && $common == $size1 && $equal == $size1;
_exit(!$success);
