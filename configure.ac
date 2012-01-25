AC_INIT([jellyfish], [1.1.4], [gmarcais@umd.edu])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror subdir-objects foreign parallel-tests color-tests])
AM_SILENT_RULES([yes])

: ${CXXFLAGS=""}
AC_PROG_CXX

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

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

# Check the version of strerror_r
AC_FUNC_STRERROR_R      

AC_CHECK_HEADERS([execinfo.h])

AC_CHECK_MEMBER([siginfo_t.si_int],
                [AC_DEFINE([HAVE_SI_INT], [1], [Define if siginfo_t.si_int exists])],
                [], [[#include <signal.h>]])

LT_INIT
AC_OUTPUT


