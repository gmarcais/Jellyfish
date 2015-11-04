What is it?
===========

Bindings of the Jellyfish library to various scripting languages
(Python, Ruby and Perl currently). This allows to query the Jellyfish
output files and use the Jellyfish hash within these scripting
languages, which is much more convenient than in C++, although
somewhat slower.


Examples
========

Some simplified examples on how to use the Jellyfish binding.

jellyfish dump
--------------

The following is roughly equivalent to the dump subcommand. It dumps
in text format the content of a Jellyfish database.

----
##### Python
```Python
import jellyfish

mf = jellyfish.ReadMerFile(sys.argv[1])
for mer, count in mf:
    print(mer, " ", count)
```

----
##### Ruby
```Ruby
require 'jellyfish'

mf = Jellyfish::ReadMerFile.new ARGV[0]
mf.each { |mer, count|
    puts("#{mer} #{count}")
}
```

----
##### Perl
```Perl
use jellyfish;

my $mf = jellyfish::ReadMerFile->new($ARGV[0]);
while($mf->next_mer) {
    print($mf->mer, " ", $mf->count, "\n");
}
```
----

jellyfish query
---------------

The following is roughly equivalent to the command query
subcommand. For every mer passed on the command line, it prints its
count in the Jellyfish database, where the mer is in canonical form.

----
##### Python
```Python
import jellyfish

qf = jellyfish.QueryMerFile(sys.argv.[1])
for m in sys.argv[2:]:
    mer = jellyfish.MerDNA(m)
    mer.canonicalize()
    print(mer, " ", qf[mer])
```

----
##### Ruby
```Ruby
require 'jellyfish'

qf = Jellyfish::QueryMerFile(ARGV.shift)
ARGV.each { |m|
    mer = Jellyfish::MerDNA.new(m)
    mer.canonicalize!
    puts("#{mer} #{qf[mer]}")
}
```

----
##### Perl
```Perl
use jellyfish;

my $qf = jellyfish::QueryMerFile->new(shift(@ARGV));
foreach my $m (@ARGV) {
  my $mer = Jellyfish::MerDNA->new($m);
  $mer->canonicalize;
  print($mer, " ", $qf->get($mer), "\n");
}
```
----
