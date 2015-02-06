What is it?
===========

Bindings of the Jellyfish library to various scripting languages
(Python, Ruby and Perl currently). This allows to query the Jellyfish
output files and use the Jellyfish hash within these scripting
languages, which is much more convenient than in C++, although
somewhat slower.

Installation
============

Requirements
------------

In the following, it is assumed that Jellyfish has been properly
installed and is visible to the 'pkg-config' tool. The following
command:

```Shell
pkg-config --exists jellyfish-2.0 && echo yes
```

Must print 'yes'. If not, see the README in the Jellyfish on how to
install and, if necessary, setup the 'PKG\_CONFIG\_PATH' variable.

The [swig](http://www.swig.org/) software package must be
installed. All the testing is done with version 3.x. Version 2.x MAY
work, but is not tested.

Configure
---------

To compile the bindings, use, according to taste, some of the the following switches with configure:

```Shell
./configure --enable-swig --enable-python-binding --enable-ruby-binding --enable-perl-binding
```

In addition, each of the `--enable-*-binding` switch can take a path where to install the binding. This allows to install without root privilegies. For example:

```Shell
./configure --prefix=`pwd`/inst --enable-swig --enable-python-binding=`pwd`/inst/python
make
make install
```

will install `jellyfish` in `./inst/bin` and the python files in `./inst/python`. Then, one needs to add `$(pwd)/inst/python` to `PYTHONPATH` to use the binding. Similarly with ruby and `RUBYLIB`, perl and `PERL5LIB`.

The swig bindings were tested with Python 3.3.3, Ruby 1.9.3 and Perl 5.18.1. The Perl headers may not compile properly with recent version of g++. It compiles properly with g++ version 4.4. Hence, you may need to pass the path to `g++` version 4.4 to the configure command line. For example:

```Shell
./configure --enable-swig --enable-perl-binding CXX=g++-4.4
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
