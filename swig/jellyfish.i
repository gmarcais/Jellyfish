/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifdef SWIGPYTHON
// Default Python loading code does not seem to work. Use our own.
%define MODULEIMPORT
"
import os
if os.path.basename(__file__) == \"__init__.pyc\" or os.path.basename(__file__) == \"__init__.py\":
  import dna_jellyfish.$module
else:
  import $module
"
%enddef
%module(docstring="Jellyfish binding", moduleimport=MODULEIMPORT) dna_jellyfish
#else
%module(docstring="Jellyfish binding") jellyfish
#endif

%naturalvar; // Use const reference instead of pointers
%include "std_string.i"

%include "exception.i"
%include "std_except.i"
%include "typemaps.i"
%feature("autodoc", "2");

%{
#ifdef SWIGPYTHON
#endif

#ifdef SWIGPERL
#undef seed
#undef random
#endif

#include <fstream>
#include <stdexcept>
#undef die
#include <jellyfish/mer_dna.hpp>
#include <jellyfish/file_header.hpp>
#include <jellyfish/mer_dna_bloom_counter.hpp>
#include <jellyfish/hash_counter.hpp>
#include <jellyfish/jellyfish.hpp>
#undef die
%}

#ifdef SWIGPYTHON
%constant const char* __version__ = PACKAGE_VERSION;
#else
%constant const char* VERSION = PACKAGE_VERSION;
#endif

#ifdef SWIGRUBY
#endif


%include "mer_dna.i"
%include "mer_file.i"
%include "hash_counter.i"
%include "hash_set.i"
%include "string_mers.i"
