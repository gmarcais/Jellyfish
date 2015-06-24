Jellyfish
=========

Overview
--------

Jellyfish is a tool for fast, memory-efficient counting of k-mers in DNA. A k-mer is a substring of length k, and counting the occurrences of all such substrings is a central step in many analyses of DNA sequence. Jellyfish can count k-mers using an order of magnitude less memory and an order of magnitude faster than other k-mer counting packages by using an efficient encoding of a hash table and by exploiting the "compare-and-swap" CPU instruction to increase parallelism.

JELLYFISH is a command-line program that reads FASTA and multi-FASTA files containing DNA sequences. It outputs its k-mer counts in a binary format, which can be translated into a human-readable text format using the "jellyfish dump" command, or queried for specific k-mers with "jellyfish query". See the UserGuide provided on [Jellyfish's home page][1] for more details.

If you use Jellyfish in your research, please cite:

  Guillaume Marcais and Carl Kingsford, A fast, lock-free approach for efficient parallel counting of occurrences of k-mers. Bioinformatics (2011) 27(6): 764-770 ([first published online January 7, 2011](http://bioinformatics.oxfordjournals.org/cgi/content/abstract/27/6/764 "Paper on Oxford Bioinformatics website")) doi:10.1093/bioinformatics/btr011

Installation
------------

To get an easier to compiled packaged tar ball of the source code, download a release from [home page of Jellyfish at the University of Maryland][1] or from the [github release][3].

To compile from the git tree, you will need autoconf/automake, make, g++ 4.4 or newer and [yaggo](https://github.com/gmarcais/yaggo "Yaggo on github"). Then compile with:

```Shell
autoreconf -i
./configure
make
sudo make install
```

Extra / Examples
----------------

In the examples directory are potentially useful extra programs to query/manipulates output files of Jellyfish, using the shared library of Jellyfish in C++ or with scripting languages. The examples are not compiled by default. Each subdirectory of examples is independent and is compiled with a simple invocation of 'make'.


Binding to script languages
---------------------------

Bindings to Ruby, Python and Perl are provided. This binding allows to read the output file of Jellyfish directly in a scripting language. Compilation of the bindings is easier from the [release tarball][3]: [SWIG][2] is not required and in the command lines shown below, remove the `--enable-swig` switch. Only the development files of the scripting languages are required.

Compilation of the bindings from the git tree requires [SWIG][2] version 3, and the development files of the scripting languages. To compile all three bindings, configure with:

```Shell
./configure --enable-swig --enable-ruby-binding --enable-python-binding --enable-perl-binding
```

Note that the headers of older version of Perl 5 do not compile with recent compilers (g++ > 4.4, clang++) and C++11 mode enable. One may have to specify the path to version 4.4 of gcc by adding, for example, `CXX=g++4.4` to the configure commande line.

The binding can installed in a different location than the default (which may require root privileges for example) by passing a path to the `--enable` switches. Then, for Python, Ruby or Perl to find the binding, an environment variable may need to be adjusted (`PYTHONPATH`, `RUBYLIB` and `PERL5LIB` respectively). For example:

```Shell
./configure --prefix=$HOME --enable-swig --enable-python-binding=$HOME/lib/python
export PYTHONPATH=$HOME/lib/python
```

See the `swig` directory for examples on how to use the bindings.

[1]: http://www.genome.umd.edu/jellyfish.html "Genome group at University of Maryland"
[2]: http://www.swig.org/
[3]: https://github.com/gmarcais/Jellyfish/releases "Jellyfish release"
