What is it?
===========

Bindings of the Jellyfish library to various scripting languages
(Python, Ruby and Perl currently). This allows to query the Jellyfish
output files and use the Jellyfish hash within these scripting
languages, which is much more convenient than in C++, although
somewhat slower.

Installation
============

See the installation instructions on the
[main page](../../tree/master) on how to install the bindings at the
same times as Jellyfish.

Additionally, it is possible to install the bindings from the
distribution tarball. If Jellyfish is already installed (that is
`pkg-config --exists jellyfish-2.0 && echo OK` prints `OK`), then the
bindings can be installed as follows. (`$PREFIX` should be set to the
desired installation location)

Python:
```Shell
cd swig/python
python setup.py build
python setup.py install --prefix=$PREFIX
```

Ruby:
```Shell
cd swig/ruby
ruby extconf.rb
make
make install prefix=$PREFIX
```

Perl 5:
```Shell
cd swig/perl5
perl Makefile.PL SITEPREFIX=$PREFIX
make
make install
```

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
    print mer, count
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
    print mer, qf[mer])
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
  my $mer = jellyfish::MerDNA->new($m);
  $mer->canonicalize;
  print($mer, " ", $qf->get($mer), "\n");
}
```
----

jellyfish count
===============

The following will parse all the 30-mers in a string (`str` in the examples) and store them in a hash counter. In the following code, by replacing `string_mers` by `string_canonicals`, one gets only the canonical mers.

----
##### Ruby
```Ruby
require 'jellyfish'

Jellyfish::MerDNA::k(30)
h = Jellyfish::HashCounter.new(1024, 5)
str.mers { |m|
  h.add(m, 1)
}
```

----
##### Python
```Python
import jellyfish

jellyfish.MerDNA.k(30)
h = jellyfish.HashCounter(1024, 5)
mers = jellyfish.string_mers(str)
for m in mers:
    h.add(m, 1)
```

---
##### Perl
```Perl
use jellyfish;

jellyfish::MerDNA::k(30);
my $h = jellyfish::HashCounter->new(1024, 5);
my $mers = jellyfish::string_mers($str);
while($mers->next_mer) {
  $h->add($mers->mer, 1);
}
```

The argument to the `HashCounter` constructor are the initial size of the hash and the number of bits in the value field (call it `b`). Note that the number `b` of bits in the value field does not create an upper limit on the value of the count. For best performance, `b` should be chosen so that most k-mers in the hash have a count less than 2^b (2 to the power of b).
