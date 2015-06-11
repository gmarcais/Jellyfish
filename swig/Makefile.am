# SWIG
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
SWIG_SRC = jellyfish.i hash_counter.i hash_set.i mer_dna.i mer_file.i string_mers.i

if HAVE_SWIG
SWIG_V_GEN = $(swig_v_GEN_$(V))
swig_v_GEN_ = $(swig_v_GEN_$(AM_DEFAULT_VERBOSITY))
swig_v_GEN_0 = @echo "  SWIG    " $@;
%/swig_wrap.cpp: $(SWIG_SRC)
	$(SWIG_V_GEN)$(SWIG) -$* -I$(top_srcdir)/include -o $@ $<
else
%/swig_wrap.cc:
	@echo >&2 SWIG >= 3.x.x not found. Make sure it is install and rerun configure
	@false
endif

# Python support
if PYTHON_BINDING
PYTHON_BUILT = python/swig_wrap.cpp python/jellyfish.py
BUILT_SOURCES += $(PYTHON_BUILT)

pythonextdir = $(PYTHON_SITE_PKG)/jellyfish
pythonext_SCRIPTS = python/__init__.pyc
pythonext_LTLIBRARIES = python/_jellyfish.la
python__jellyfish_la_SOURCES = python/swig_wrap.cpp $(SWIG_SRC)
python__jellyfish_la_CPPFLAGS = $(PYTHON_CPPFLAGS) -I$(top_srcdir)/include
python__jellyfish_la_LDFLAGS = -module $(PYTHON_LDFLAGS)
python__jellyfish_la_LIBADD = ../libjellyfish-2.0.la
CLEANFILES += $(PYTHON_BUILT) $(dist_pythonext_DATA) python/jellyfish.py
PYTHONC_V_GEN = $(pythonc_v_GEN_$(V))
pythonc_v_GEN_ = $(pythonc_v_GEN_$(AM_DEFAULT_VERBOSITY))
pythonc_v_GEN_0 = @echo "  PYTHONC " $@;
%/__init__.pyc: %/jellyfish.py
	$(PYTHONC_V_GEN)$(PYTHON) -c 'import py_compile, sys; py_compile.compile(sys.argv[1], sys.argv[2])' $< $@
python/jellyfish.py: python/swig_wrap.cpp
EXTRA_DIST += python/jellyfish.py
endif

# Ruby support
if RUBY_BINDING
RUBY_BUILT = ruby/swig_wrap.cpp
BUILT_SOURCES += $(RUBY_BUILT)
rubyextdir = $(RUBY_EXT_LIB)
rubyext_LTLIBRARIES = ruby/jellyfish.la
ruby_jellyfish_la_SOURCES = ruby/swig_wrap.cpp $(SWIG_SRC)
ruby_jellyfish_la_CPPFLAGS = $(RUBY_EXT_CFLAGS) -I$(top_srcdir)/include
ruby_jellyfish_la_LDFLAGS = -module $(RUBY_EXT_LIBS)
ruby_jellyfish_la_LIBADD = ../libjellyfish-2.0.la
CLEANFILES += $(RUBY_BUILT)
endif

# Perl5 support
if PERL_BINDING
PERL_BUILT = perl5/swig_wrap.cpp perl5/jellyfish.pm
BUILT_SOURCES += $(PERL_BUILT)
perlextdir = $(PERL_EXT_LIB)
perlext_SCRIPTS = perl5/jellyfish.pm
perlext_LTLIBRARIES = perl5/jellyfish.la
perl5_jellyfish_la_SOURCES = perl5/swig_wrap.cpp $(SWIG_SRC)
perl5_jellyfish_la_CPPFLAGS = $(PERL_EXT_CPPFLAGS) -I$(PERL_EXT_INC) -I$(top_srcdir)/include
perl5_jellyfish_la_LDFLAGS = -module $(PERL5_EXT_LDFLAGS)
perl5_jellyfish_la_LIBADD = ../libjellyfish-2.0.la
CLEANFILES += $(PERL5_BUILT) perl5/jellyfish.pm
perl5/jellyfish.pm: perl5/swig_wrap.cpp
EXTRA_DIST += perl5/jellyfish.pm
endif
