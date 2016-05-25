AC_INIT([jellyfish], [2.2.6], [gmarcais@umd.edu])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects foreign parallel-tests color-tests])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([jellyfish])
AC_CONFIG_HEADERS([config.h])
AC_PROG_LIBTOOL

# Change default compilation flags
AC_SUBST([ALL_CXXFLAGS], [-std=c++0x])
CXXFLAGS="-std=c++0x $CXXFLAGS"
AC_LANG(C++)
AC_PROG_CXX

# Major version of the library
AC_SUBST([PACKAGE_LIB], [2.0])

# Check for md5 or md5sum
AC_ARG_VAR([MD5], [Path to md5 hashing program])
AS_IF([test "x$MD5" = "x"], AC_CHECK_PROG([MD5], [md5sum], [md5sum]), [])
AS_IF([test "x$MD5" = "x"], AC_CHECK_PROG([MD5], [md5], [md5 -r]), [])
AS_IF([test "x$MD5" = "x"], AC_MSG_ERROR([Could not find md5 hashing program in your path]), [])

# Check for yaggo
AC_ARG_VAR([YAGGO], [Yaggo switch parser generator])
AS_IF([test "x$YAGGO" = "x"], [AC_PATH_PROG([YAGGO], [yaggo], [false])])

dnl define([concat], $1$2$3)dnl
define([PC_FILE], jellyfish-2.0.pc)
AC_CONFIG_FILES([
    Makefile
    tests/compat.sh
]
    PC_FILE
)


AC_ARG_WITH([sse],
            [AS_HELP_STRING([--with-sse], [enable SSE])],
            [], [with_sse=yes])
AS_IF([test "x$with_sse" != xno],
      [AC_DEFINE([HAVE_SSE], [1], [Define if you have SSE])],
      [])

# Use valgrind to check memory allocation with mmap
AC_ARG_ENABLE([valgrind],
              [AS_HELP_STRING([--enable-valgrind], [Instrument mmap memory allocation with valgrind])])
AS_IF([test "x$enable_valgrind" = xyes],
      [AC_DEFINE([HAVE_VALGRIND], [1], [Define is using Valgrind])]
      [PKG_CHECK_MODULES([VALGRIND], [valgrind >= 1.8.0])])

# Check that type __int128 is supported and if the
# std::numeric_limits<__int128> specialization exists
AC_ARG_WITH([int128],
            [AS_HELP_STRING([--with-int128], [enable int128])],
            [], [with_int128=yes])
AS_IF([test "x$with_int128" != xno],
      [AC_CHECK_TYPE([__int128],
                     [AC_DEFINE([HAVE_INT128], [1], [Define if type __int128 is supported])])
       AC_MSG_CHECKING([for std::numeric_limits<__int128>])
       AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <limits>
                                            template<bool> struct StaticAssert; template<> struct StaticAssert<true> { static void assert() { } };]],
                                          [[StaticAssert<std::numeric_limits<__int128>::is_specialized>::assert();]])],
                         [AC_MSG_RESULT([yes])]
                         [AC_DEFINE([HAVE_NUMERIC_LIMITS128], [1], [Define if numeric limits specialization exists for __int128])],
                         [AC_MSG_RESULT([no])])])

# On MacOS X, use _NSGetExecutablePath to find path to own executable
AC_MSG_CHECKING([for _NSGetExecutablePath])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <mach-o/dyld.h>]],
                                   [[_NSGetExecutablePath(0, 0);]])],
                  [AC_MSG_RESULT([yes])]
                  [AC_DEFINE([HAVE_NSGETEXECUTABLEPATH], [1], [Used to find executable path on MacOS X])],
                  [AC_MSG_RESULT([no])])

# Check the version of strerror_r
AC_CHECK_HEADERS_ONCE([execinfo.h ext/stdio_filebuf.h])
AC_CHECK_MEMBER([siginfo_t.si_int],
                [AC_DEFINE([HAVE_SI_INT], [1], [Define if siginfo_t.si_int exists])],
                [], [[#include <signal.h>]])

# --enable-all-static
# Do not use libtool if building all static
AC_ARG_ENABLE([all-static],
              [AC_HELP_STRING([--enable-all-static], [create statically linked executable])])
STATIC_FLAGS=
AS_IF([test x$enable_all_static = xyes],
      [AC_SUBST([STATIC_FLAGS], [-all-static])])

#
# SWIG and bindings
#
maybe_swig=
# --enable-python-binding
AC_ARG_ENABLE([python-binding],
              [AC_HELP_STRING([--enable-python-binding@<:@=PATH@:>@], [create SWIG python module and install in PATH])])
# --enable-ruby-binding
AC_ARG_ENABLE([ruby-binding],
              [AC_HELP_STRING([--enable-ruby-binding@<:@=PATH@:>@], [create SWIG ruby module and install in PATH])])
# --enable-perl-binding
AC_ARG_ENABLE([perl-binding],
              [AC_HELP_STRING([--enable-perl-binding@<:@=PATH@:>@], [create SWIG perl module and install in PATH])])

# --enable-swig
AC_ARG_ENABLE([swig],
              [AC_HELP_STRING([--enable-swig], [enable development of swig binding])])
AS_IF([test x$enable_swig = xyes],
      [AX_PKG_SWIG([3.0.0], [], [AC_MSG_ERROR([SWIG version 3 is required])])])
AS_IF([test -n "$SWIG"],
      [SWIG_ENABLE_CXX])
AM_CONDITIONAL([HAVE_SWIG], [test -n "$SWIG"])

# Python binding setup
AM_CONDITIONAL(PYTHON_BINDING, [test -n "$enable_python_binding" -a x$enable_python_binding != xno])
AM_COND_IF([PYTHON_BINDING],
           [AS_IF([test x$enable_python_binding != xyes], [PYTHON_SITE_PKG=$enable_python_binding])]
           [AX_PYTHON_DEVEL([], [$prefix])])

# Ruby binding setup
AM_CONDITIONAL([RUBY_BINDING], [test -n "$enable_ruby_binding" -a x$enable_ruby_binding != xno])
AM_COND_IF([RUBY_BINDING],
           [AS_IF([test x$enable_ruby_binding != xyes], [RUBY_EXT_LIB=$enable_ruby_binding])]
           [AX_RUBY_EXT([$prefix])])

# Perl binding setup
AM_CONDITIONAL([PERL_BINDING], [test -n "$enable_perl_binding" -a x$enable_perl_binding != xno])
AM_COND_IF([PERL_BINDING],
           [AS_IF([test x$enable_perl_binding != xyes], [PERL_EXT_LIB=$enable_perl_binding])]
           [AX_PERL_EXT([$prefix])])

AC_OUTPUT


