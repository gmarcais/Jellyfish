#! /usr/bin/env python

import os
import re
from distutils.core import setup, Extension

# Don't use Extension support for swig, it is more annoying than
# anything!  (1)It does not support very well having the .i in a
# different directory. (2)It runs swig every single time, even if it
# ran successfully before. So (3)doing first build, then install as
# root rebuilds the modules as root.
swig_time = os.path.getmtime('../jellyfish.i')
older = True
try:
    older = os.path.getmtime('jellyfish_wrap.cxx') < swig_time or os.path.getmtime('jellyfish.py') < swig_time
except FileNotFoundError:
    older = True

if older:
    print("Running swig: swig -c++ -python -o jellyfish_wrap.cxx ../jellyfish.i")
    os.system("swig -c++ -python -o jellyfish_wrap.cxx ../jellyfish.i")

jf_include = [re.sub(r'-I', '', x) for x in os.popen("pkg-config --cflags-only-I jellyfish-2.0").read().rstrip().split()]
jf_cflags  = os.popen("pkg-config --cflags-only-other").read().rstrip().split()

jf_libs    = [re.sub(r'-l', '', x) for x in os.popen("pkg-config --libs-only-l jellyfish-2.0").read().rstrip().split()]
jf_libdir  = [re.sub(r'-L', '', x) for x in os.popen("pkg-config --libs-only-L jellyfish-2.0").read().rstrip().split()]
jf_rpath   = [re.sub(r'^', '-Wl,-rpath,', x) for x in jf_libdir]
jf_ldflags = os.popen("pkg-config --libs-only-other jellyfish-2.0").read().rstrip().split()


jellyfish_module = Extension('_dna_jellyfish',
                             sources = ['jellyfish_wrap.cxx'],
                             include_dirs = jf_include,
                             libraries = jf_libs,
                             library_dirs = jf_libdir,
                             extra_compile_args = ["-std=c++0x"] + jf_cflags,
                             extra_link_args = jf_ldflags + jf_rpath,
                             language = "c++")
setup(name = 'dna_jellyfish',
      version = '0.0.1',
      author = 'Guillaume Marcais',
      description = 'Access to jellyfish k-mer counting',
      ext_modules = [jellyfish_module],
      py_modules = ["dna_jellyfish"])
