AC_INIT([jellyfish], [1.0], [guillaume@marcais.net])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX
gl_EARLY
gl_INIT
AC_CONFIG_HEADERS([config.h])
AM_CONDITIONAL([MACOSX], [test x$target_vendor = xapple])
AC_CONFIG_FILES([
    Makefile
    gnulib/Makefile
    jellyfish/Makefile
])
AC_OUTPUT


