Jellyfish
=========

Overview
--------

Jellyfish is a tool for fast, memory-efficient counting of k-mers in DNA. A k-mer is a substring of length k, and counting the occurrences of all such substrings is a central step in many analyses of DNA sequence. Jellyfish can count k-mers using an order of magnitude less memory and an order of magnitude faster than other k-mer counting packages by using an efficient encoding of a hash table and by exploiting the "compare-and-swap" CPU instruction to increase parallelism.

JELLYFISH is a command-line program that reads FASTA and multi-FASTA files containing DNA sequences. It outputs its k-mer counts in an binary format, which can be translated into a human-readable text format using the "jellyfish dump" command. See the documentation below for more details.

If you use Jellyfish in your research, please cite:

  Guillaume Marcais and Carl Kingsford, A fast, lock-free approach for efficient parallel counting of occurrences of k-mers. Bioinformatics (2011) 27(6): 764-770 ([first published online January 7, 2011](http://bioinformatics.oxfordjournals.org/cgi/content/abstract/27/6/764 "Paper on Oxford Bioinformatics website")) doi:10.1093/bioinformatics/btr011

Installation
------------

To get packaged tar ball of the source code, see the [home page of Jellyfish at the University of Maryland](http://www.genome.umd.edu/jellyfish.html "University of Maryland website").

To compile from the git tree, you will need autoconf/automake, make, g++ 4.4 or newer and [yaggo](https://github.com/gmarcais/yaggo "Yaggo on github"). Then compile with:

```Shell
autoreconf -i
./configure
make
sudo make install
```

