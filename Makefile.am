ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = m4/gnulib-cache.m4 doc/jellyfish.pdf doc/jellyfish.man README LICENSE HalfLICENSE
man1_MANS = doc/jellyfish.man

#AM_CPPFLAGS = -I$(top_builddir)/gnulib -I$(top_srcdir)/gnulib 

SUBDIRS = jellyfish tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = jellyfish-1.1.pc

