AC_INIT([jellyfish], [1.1], [gmarcais@umd.edu])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([-Wall -Werror foreign parallel-tests color-tests])
AC_PROG_CXX

AC_CONFIG_HEADERS([config.h])
gl_EARLY 
gl_INIT

AC_CONFIG_FILES([
    Makefile
    jellyfish/Makefile
    tests/Makefile
    tests/compat.sh
    jellyfish-1.0.pc
])

AC_ARG_VAR([MD5], [Path to md5 hashing program])
if test "x$MD5" = "x"; then
  AC_CHECK_PROG([MD5], [md5sum], [md5sum])
fi
if test "x$MD5" = "x"; then
   AC_CHECK_PROG([MD5], [md5], [md5 -r])
fi
if test "x$MD5" = "x"; then
   AC_MSG_ERROR([Could not find md5 hashing program in your path])
fi

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


