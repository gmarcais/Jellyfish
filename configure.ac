AC_INIT([jellyfish], [1.0], [guillaume@marcais.net])
AC_CANONICAL_HOST
AM_CONDITIONAL([MACOSX], [test x$host_vendor = xapple])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX
AC_CONFIG_HEADERS([config.h])
m4_bmatch([$host_vendor], [apple], [
gl_EARLY 
gl_INIT
AC_CONFIG_FILES([
    Makefile
    gnulib/Makefile
    jellyfish/Makefile
])], [
AC_CONFIG_FILES([
    Makefile
    jellyfish/Makefile
])])
AC_OUTPUT


