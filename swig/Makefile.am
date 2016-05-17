# SWIG
SWIG_SRC = swig/jellyfish.i swig/hash_counter.i swig/hash_set.i	\
           swig/mer_dna.i swig/mer_file.i swig/string_mers.i

if HAVE_SWIG
SWIG_V_GEN = $(swig_v_GEN_$(V))
swig_v_GEN_ = $(swig_v_GEN_$(AM_DEFAULT_VERBOSITY))
swig_v_GEN_0 = @echo "  SWIG    " $@;
%/swig_wrap.cpp: $(SWIG_SRC)
	$(SWIG_V_GEN)$(SWIG) -$(notdir $*) -I$(srcdir)/../include -o $@ $<
else
%/swig_wrap.cpp:
	@echo >&2 SWIG >= 3.x.x not found. Make sure it is install and rerun configure
	@false
endif

# Python support
if PYTHON_BINDING
PYTHON_BUILT = swig/python/swig_wrap.cpp swig/python/jellyfish.py
BUILT_SOURCES += $(PYTHON_BUILT)

pythonextdir = $(PYTHON_SITE_PKG)/jellyfish
pythonext_SCRIPTS = swig/python/__init__.pyc
pythonext_LTLIBRARIES = swig/python/_jellyfish.la
swig_python__jellyfish_la_SOURCES = swig/python/swig_wrap.cpp $(SWIG_SRC)
swig_python__jellyfish_la_CPPFLAGS = $(PYTHON_CPPFLAGS) -I$(srcdir)/include
swig_python__jellyfish_la_LDFLAGS = -module
swig_python__jellyfish_la_LIBADD = libjellyfish-2.0.la
CLEANFILES += $(PYTHON_BUILT) $(pythonext_SCRIPTS)
PYTHONC_V_GEN = $(pythonc_v_GEN_$(V))
pythonc_v_GEN_ = $(pythonc_v_GEN_$(AM_DEFAULT_VERBOSITY))
pythonc_v_GEN_0 = @echo "  PYTHONC " $@;
%/__init__.pyc: %/jellyfish.py
	$(PYTHONC_V_GEN)$(PYTHON) -c 'import py_compile, sys; py_compile.compile(sys.argv[1], sys.argv[2])' $< $@
swig/python/jellyfish.py: swig/python/swig_wrap.cpp
EXTRA_DIST += $(PYTHON_BUILT)
endif

# Ruby support
if RUBY_BINDING
RUBY_BUILT = swig/ruby/swig_wrap.cpp
BUILT_SOURCES += $(RUBY_BUILT)
rubyextdir = $(RUBY_EXT_LIB)
rubyext_LTLIBRARIES = swig/ruby/jellyfish.la
swig_ruby_jellyfish_la_SOURCES = swig/ruby/swig_wrap.cpp $(SWIG_SRC)
swig_ruby_jellyfish_la_CPPFLAGS = $(RUBY_EXT_CFLAGS) -I$(srcdir)/include
swig_ruby_jellyfish_la_LDFLAGS = -module
swig_ruby_jellyfish_la_LIBADD = libjellyfish-2.0.la
CLEANFILES += $(RUBY_BUILT)
endif

# Perl5 support
if PERL_BINDING
PERL_BUILT = swig/perl5/swig_wrap.cpp swig/perl5/jellyfish.pm
BUILT_SOURCES += $(PERL_BUILT)
perlextdir = $(PERL_EXT_LIB)
perlext_SCRIPTS = swig/perl5/jellyfish.pm
perlext_LTLIBRARIES = swig/perl5/jellyfish.la
swig_perl5_jellyfish_la_SOURCES = swig/perl5/swig_wrap.cpp $(SWIG_SRC)
swig_perl5_jellyfish_la_CPPFLAGS = $(PERL_EXT_CPPFLAGS) -I$(PERL_EXT_INC) -I$(srcdir)/include
swig_perl5_jellyfish_la_LDFLAGS = -module
swig_perl5_jellyfish_la_LIBADD = libjellyfish-2.0.la
CLEANFILES += $(PERL_BUILT)
swig/perl5/jellyfish.pm: swig/perl5/swig_wrap.cpp
EXTRA_DIST += $(PERL_BUILT)
endif
