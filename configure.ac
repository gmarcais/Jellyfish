AC_INIT([jellyfish], [1.1.11], [gmarcais@umd.edu])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects foreign parallel-tests color-tests])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([jellyfish])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL

# Change default compilation flags
#AC_SUBST([ALL_CXXFLAGS], [-std=c++0x])
#CXXFLAGS="-std=c++0x $CXXFLAGS"
AC_LANG(C++)
AC_PROG_CXX


dnl define([concat], $1$2$3)dnl
dnl define([PC_FILE], concat([jellyfish-],AC_PACKAGE_VERSION,[.pc]))
define([PC_FILE], jellyfish-1.1.pc)
AC_CONFIG_FILES([
    Makefile
    tests/compat.sh
]
    PC_FILE
)

AC_ARG_VAR([MD5], [Path to md5 hashing program])
AS_IF([test "x$MD5" = "x"], AC_CHECK_PROG([MD5], [md5sum], [md5sum]), [])
AS_IF([test "x$MD5" = "x"], AC_CHECK_PROG([MD5], [md5], [md5 -r]), [])
AS_IF([test "x$MD5" = "x"], AC_MSG_ERROR([Could not find md5 hashing program in your path]), [])

AC_ARG_WITH([sse],
            [AS_HELP_STRING([--with-sse], [enable SSE])],
            [], [with_sse=no])
AS_IF([test "x$with_sse" != xno],
      [AC_DEFINE([HAVE_SSE], [1], [Define if you have SSE])],
      [])

AC_ARG_WITH([half],
            [AS_HELP_STRING([--with-half], [enable half float (16 bits)])],
            [], [with_half=no])
AS_IF([test "x$with_half" = "xyes"],
      [AC_DEFINE([HALF_FLOATS], [1], [Define if you use half floats for qmer counting])],
      [])

AC_CHECK_TYPE([__int128],
              [AC_DEFINE([HAVE_INT128], [1], [Define if type __int128 is supported])])


# Check the version of strerror_r
AC_FUNC_STRERROR_R      

AC_CHECK_HEADERS([execinfo.h sys/syscall.h])

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

AC_OUTPUT


