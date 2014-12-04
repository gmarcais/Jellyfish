BUILT_SOURCES = swig_wrap.cpp
SWIG_SOURCES = ../jellyfish.i ../hash_counter.i ../hash_set.i ../mer_dna.i ../mer_file.i ../string_mers.i

pkgpython_PYTHON = jellyfish.py
pkgpyexec_LTLIBRARIES = _jellyfish.la
_jellyfish_la_SOURCES = swig_wrap.cpp $(SWIG_SOURCES)
_jellyfish_la_CPPFLAGS = $(SWIG_PYTHON_CPPFLAGS) $(PYTHON_CPPFLAGS) -I$(top_srcdir)/include
_jellyfish_la_LDFLAGS = -module $(PYTHON_LDFLAGS)
_jellyfish_la_LIBADD = ../../libjellyfish-2.0.la

swig_wrap.cpp : $(SWIG_SOURCES)
	$(SWIG) -python $(SWIG_PYTHON_OPT) -I$(top_srcdir)/include -o $@ $<
