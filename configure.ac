AC_INIT([jellyfish], [1.0], [guillaume@marcais.net])
AC_CANONICAL_HOST
AM_CONDITIONAL([MACOSX], [test x$host_vendor = xapple])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX

AC_CONFIG_HEADERS([config.h])
gl_EARLY 
gl_INIT
#    gnulib/Makefile
AC_CONFIG_FILES([
    Makefile
    jellyfish/Makefile
    jellyfish-1.0.pc
])

AC_ARG_WITH([sse],
            [AS_HELP_STRING([--without-sse], [disable SSE])],
            [], [with_sse=yes])
AS_IF([test "x$with_sse" != xno],
      [AC_DEFINE([HAVE_SSE], [1], [Define if you have SSE])],
      [])

# Check the version of strerror_r
AC_FUNC_STRERROR_R      

LT_INIT
AC_OUTPUT


