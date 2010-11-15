ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = m4/gnulib-cache.m4 doc/jellyfish.pdf doc/jellyfish.man README LICENSE
man1_MANS = doc/jellyfish.man

#AM_CPPFLAGS = -I$(top_builddir)/gnulib -I$(top_srcdir)/gnulib 

#if MACOSX
#SUBDIRS = gnulib jellyfish
#else
SUBDIRS = jellyfish tests
#endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = jellyfish-1.0.pc

