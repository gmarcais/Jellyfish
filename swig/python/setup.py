#! /usr/bin/env python

import os
import sys
import re
from distutils.core import setup, Extension

def pkgconfig(s):
    pipe = os.popen("pkg-config {}".format(s))
    res = pipe.read().rstrip().split()
    ret = pipe.close()
    if ret is not None:
        sys.stderr.write("\nCan't get compilation information for Jellyfish, check you PKG_CONFIG_PATH\n")
        exit(1)
    return res

jf_libs    = [re.sub(r'-l', '', x) for x in pkgconfig("--libs-only-l jellyfish-2.0")]
jf_include = [re.sub(r'-I', '', x) for x in pkgconfig("--cflags-only-I jellyfish-2.0")]
jf_cflags  = pkgconfig("--cflags-only-other jellyfish-2.0")
jf_libdir  = [re.sub(r'-L', '', x) for x in pkgconfig("--libs-only-L jellyfish-2.0")]
jf_rpath   = [re.sub(r'^', '-Wl,-rpath,', x) for x in jf_libdir]
jf_ldflags = pkgconfig("--libs-only-other jellyfish-2.0")

jellyfish_module = Extension('_dna_jellyfish',
                             sources = ['swig_wrap.cpp'],
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
