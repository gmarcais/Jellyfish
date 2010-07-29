/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
/* A substitute for ISO C99 <wchar.h>, for platforms that have issues.

   Copyright (C) 2007-2010 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Eric Blake.  */

/*
 * ISO C 99 <wchar.h> for platforms that have issues.
 * <http://www.opengroup.org/susv3xbd/wchar.h.html>
 *
 * For now, this just ensures proper prerequisite inclusion order and
 * the declaration of wcwidth().
 */

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#if defined __need_mbstate_t || defined __need_wint_t || (defined __hpux && ((defined _INTTYPES_INCLUDED && !defined strtoimax) || defined _GL_JUST_INCLUDE_SYSTEM_WCHAR_H)) || defined _GL_ALREADY_INCLUDING_WCHAR_H
/* Special invocation convention:
   - Inside glibc and uClibc header files.
   - On HP-UX 11.00 we have a sequence of nested includes
     <wchar.h> -> <stdlib.h> -> <stdint.h>, and the latter includes <wchar.h>,
     once indirectly <stdint.h> -> <sys/types.h> -> <inttypes.h> -> <wchar.h>
     and once directly.  In both situations 'wint_t' is not yet defined,
     therefore we cannot provide the function overrides; instead include only
     the system's <wchar.h>.
   - On IRIX 6.5, similarly, we have an include <wchar.h> -> <wctype.h>, and
     the latter includes <wchar.h>.  But here, we have no way to detect whether
     <wctype.h> is completely included or is still being included.  */

#include_next <wchar.h>

#else
/* Normal invocation convention.  */

#ifndef _GL_WCHAR_H

#define _GL_ALREADY_INCLUDING_WCHAR_H

/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be
   included before <wchar.h>.
   But avoid namespace pollution on glibc systems.  */
#ifndef __GLIBC__
# include <stddef.h>
# include <stdio.h>
# include <time.h>
#endif

/* Include the original <wchar.h> if it exists.
   Some builds of uClibc lack it.  */
/* The include_next requires a split double-inclusion guard.  */
#if 1
# include_next <wchar.h>
#endif

#undef _GL_ALREADY_INCLUDING_WCHAR_H

#ifndef _GL_WCHAR_H
#define _GL_WCHAR_H

/* The definitions of _GL_FUNCDECL_RPL etc. are copied here.  */
#ifndef _GL_CXXDEFS_H
#define _GL_CXXDEFS_H

/* The three most frequent use cases of these macros are:

   * For providing a substitute for a function that is missing on some
     platforms, but is declared and works fine on the platforms on which
     it exists:

       #if @GNULIB_FOO@
       # if !@HAVE_FOO@
       _GL_FUNCDECL_SYS (foo, ...);
       # endif
       _GL_CXXALIAS_SYS (foo, ...);
       _GL_CXXALIASWARN (foo);
       #elif defined GNULIB_POSIXCHECK
       ...
       #endif

   * For providing a replacement for a function that exists on all platforms,
     but is broken/insufficient and needs to be replaced on some platforms:

       #if @GNULIB_FOO@
       # if @REPLACE_FOO@
       #  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
       #   undef foo
       #   define foo rpl_foo
       #  endif
       _GL_FUNCDECL_RPL (foo, ...);
       _GL_CXXALIAS_RPL (foo, ...);
       # else
       _GL_CXXALIAS_SYS (foo, ...);
       # endif
       _GL_CXXALIASWARN (foo);
       #elif defined GNULIB_POSIXCHECK
       ...
       #endif

   * For providing a replacement for a function that exists on some platforms
     but is broken/insufficient and needs to be replaced on some of them and
     is additionally either missing or undeclared on some other platforms:

       #if @GNULIB_FOO@
       # if @REPLACE_FOO@
       #  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
       #   undef foo
       #   define foo rpl_foo
       #  endif
       _GL_FUNCDECL_RPL (foo, ...);
       _GL_CXXALIAS_RPL (foo, ...);
       # else
       #  if !@HAVE_FOO@   or   if !@HAVE_DECL_FOO@
       _GL_FUNCDECL_SYS (foo, ...);
       #  endif
       _GL_CXXALIAS_SYS (foo, ...);
       # endif
       _GL_CXXALIASWARN (foo);
       #elif defined GNULIB_POSIXCHECK
       ...
       #endif
*/

/* _GL_EXTERN_C declaration;
   performs the declaration with C linkage.  */
#if defined __cplusplus
# define _GL_EXTERN_C extern "C"
#else
# define _GL_EXTERN_C extern
#endif

/* _GL_FUNCDECL_RPL (func, rettype, parameters_and_attributes);
   declares a replacement function, named rpl_func, with the given prototype,
   consisting of return type, parameters, and attributes.
   Example:
     _GL_FUNCDECL_RPL (open, int, (const char *filename, int flags, ...)
                                  _GL_ARG_NONNULL ((1)));
 */
#define _GL_FUNCDECL_RPL(func,rettype,parameters_and_attributes) \
  _GL_FUNCDECL_RPL_1 (rpl_##func, rettype, parameters_and_attributes)
#define _GL_FUNCDECL_RPL_1(rpl_func,rettype,parameters_and_attributes) \
  _GL_EXTERN_C rettype rpl_func parameters_and_attributes

/* _GL_FUNCDECL_SYS (func, rettype, parameters_and_attributes);
   declares the system function, named func, with the given prototype,
   consisting of return type, parameters, and attributes.
   Example:
     _GL_FUNCDECL_SYS (open, int, (const char *filename, int flags, ...)
                                  _GL_ARG_NONNULL ((1)));
 */
#define _GL_FUNCDECL_SYS(func,rettype,parameters_and_attributes) \
  _GL_EXTERN_C rettype func parameters_and_attributes

/* _GL_CXXALIAS_RPL (func, rettype, parameters);
   declares a C++ alias called GNULIB_NAMESPACE::func
   that redirects to rpl_func, if GNULIB_NAMESPACE is defined.
   Example:
     _GL_CXXALIAS_RPL (open, int, (const char *filename, int flags, ...));
 */
#define _GL_CXXALIAS_RPL(func,rettype,parameters) \
  _GL_CXXALIAS_RPL_1 (func, rpl_##func, rettype, parameters)
#if defined __cplusplus && defined GNULIB_NAMESPACE
# define _GL_CXXALIAS_RPL_1(func,rpl_func,rettype,parameters) \
    namespace GNULIB_NAMESPACE                                \
    {                                                         \
      rettype (*const func) parameters = ::rpl_func;          \
    }                                                         \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#else
# define _GL_CXXALIAS_RPL_1(func,rpl_func,rettype,parameters) \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#endif

/* _GL_CXXALIAS_RPL_CAST_1 (func, rpl_func, rettype, parameters);
   is like  _GL_CXXALIAS_RPL_1 (func, rpl_func, rettype, parameters);
   except that the C function rpl_func may have a slightly different
   declaration.  A cast is used to silence the "invalid conversion" error
   that would otherwise occur.  */
#if defined __cplusplus && defined GNULIB_NAMESPACE
# define _GL_CXXALIAS_RPL_CAST_1(func,rpl_func,rettype,parameters) \
    namespace GNULIB_NAMESPACE                                     \
    {                                                              \
      rettype (*const func) parameters =                           \
        reinterpret_cast<rettype(*)parameters>(::rpl_func);        \
    }                                                              \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#else
# define _GL_CXXALIAS_RPL_CAST_1(func,rpl_func,rettype,parameters) \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#endif

/* _GL_CXXALIAS_SYS (func, rettype, parameters);
   declares a C++ alias called GNULIB_NAMESPACE::func
   that redirects to the system provided function func, if GNULIB_NAMESPACE
   is defined.
   Example:
     _GL_CXXALIAS_SYS (open, int, (const char *filename, int flags, ...));
 */
#if defined __cplusplus && defined GNULIB_NAMESPACE
  /* If we were to write
       rettype (*const func) parameters = ::func;
     like above in _GL_CXXALIAS_RPL_1, the compiler could optimize calls
     better (remove an indirection through a 'static' pointer variable),
     but then the _GL_CXXALIASWARN macro below would cause a warning not only
     for uses of ::func but also for uses of GNULIB_NAMESPACE::func.  */
# define _GL_CXXALIAS_SYS(func,rettype,parameters) \
    namespace GNULIB_NAMESPACE                     \
    {                                              \
      static rettype (*func) parameters = ::func;  \
    }                                              \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#else
# define _GL_CXXALIAS_SYS(func,rettype,parameters) \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#endif

/* _GL_CXXALIAS_SYS_CAST (func, rettype, parameters);
   is like  _GL_CXXALIAS_SYS (func, rettype, parameters);
   except that the C function func may have a slightly different declaration.
   A cast is used to silence the "invalid conversion" error that would
   otherwise occur.  */
#if defined __cplusplus && defined GNULIB_NAMESPACE
# define _GL_CXXALIAS_SYS_CAST(func,rettype,parameters) \
    namespace GNULIB_NAMESPACE                          \
    {                                                   \
      static rettype (*func) parameters =               \
        reinterpret_cast<rettype(*)parameters>(::func); \
    }                                                   \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#else
# define _GL_CXXALIAS_SYS_CAST(func,rettype,parameters) \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#endif

/* _GL_CXXALIAS_SYS_CAST2 (func, rettype, parameters, rettype2, parameters2);
   is like  _GL_CXXALIAS_SYS (func, rettype, parameters);
   except that the C function is picked among a set of overloaded functions,
   namely the one with rettype2 and parameters2.  Two consecutive casts
   are used to silence the "cannot find a match" and "invalid conversion"
   errors that would otherwise occur.  */
#if defined __cplusplus && defined GNULIB_NAMESPACE
  /* The outer cast must be a reinterpret_cast.
     The inner cast: When the function is defined as a set of overloaded
     functions, it works as a static_cast<>, choosing the designated variant.
     When the function is defined as a single variant, it works as a
     reinterpret_cast<>. The parenthesized cast syntax works both ways.  */
# define _GL_CXXALIAS_SYS_CAST2(func,rettype,parameters,rettype2,parameters2) \
    namespace GNULIB_NAMESPACE                                                \
    {                                                                         \
      static rettype (*func) parameters =                                     \
        reinterpret_cast<rettype(*)parameters>(                               \
          (rettype2(*)parameters2)(::func));                                  \
    }                                                                         \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#else
# define _GL_CXXALIAS_SYS_CAST2(func,rettype,parameters,rettype2,parameters2) \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#endif

/* _GL_CXXALIASWARN (func);
   causes a warning to be emitted when ::func is used but not when
   GNULIB_NAMESPACE::func is used.  func must be defined without overloaded
   variants.  */
#if defined __cplusplus && defined GNULIB_NAMESPACE
# define _GL_CXXALIASWARN(func) \
   _GL_CXXALIASWARN_1 (func, GNULIB_NAMESPACE)
# define _GL_CXXALIASWARN_1(func,namespace) \
   _GL_CXXALIASWARN_2 (func, namespace)
/* To work around GCC bug <http://gcc.gnu.org/bugzilla/show_bug.cgi?id=43881>,
   we enable the warning only when not optimizing.  */
# if !__OPTIMIZE__
#  define _GL_CXXALIASWARN_2(func,namespace) \
    _GL_WARN_ON_USE (func, \
                     "The symbol ::" #func " refers to the system function. " \
                     "Use " #namespace "::" #func " instead.")
# elif __GNUC__ >= 3 && GNULIB_STRICT_CHECKING
#  define _GL_CXXALIASWARN_2(func,namespace) \
     extern __typeof__ (func) func
# else
#  define _GL_CXXALIASWARN_2(func,namespace) \
     _GL_EXTERN_C int _gl_cxxalias_dummy
# endif
#else
# define _GL_CXXALIASWARN(func) \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#endif

/* _GL_CXXALIASWARN1 (func, rettype, parameters_and_attributes);
   causes a warning to be emitted when the given overloaded variant of ::func
   is used but not when GNULIB_NAMESPACE::func is used.  */
#if defined __cplusplus && defined GNULIB_NAMESPACE
# define _GL_CXXALIASWARN1(func,rettype,parameters_and_attributes) \
   _GL_CXXALIASWARN1_1 (func, rettype, parameters_and_attributes, \
                        GNULIB_NAMESPACE)
# define _GL_CXXALIASWARN1_1(func,rettype,parameters_and_attributes,namespace) \
   _GL_CXXALIASWARN1_2 (func, rettype, parameters_and_attributes, namespace)
/* To work around GCC bug <http://gcc.gnu.org/bugzilla/show_bug.cgi?id=43881>,
   we enable the warning only when not optimizing.  */
# if !__OPTIMIZE__
#  define _GL_CXXALIASWARN1_2(func,rettype,parameters_and_attributes,namespace) \
    _GL_WARN_ON_USE_CXX (func, rettype, parameters_and_attributes, \
                         "The symbol ::" #func " refers to the system function. " \
                         "Use " #namespace "::" #func " instead.")
# elif __GNUC__ >= 3 && GNULIB_STRICT_CHECKING
#  define _GL_CXXALIASWARN1_2(func,rettype,parameters_and_attributes,namespace) \
     extern __typeof__ (func) func
# else
#  define _GL_CXXALIASWARN1_2(func,rettype,parameters_and_attributes,namespace) \
     _GL_EXTERN_C int _gl_cxxalias_dummy
# endif
#else
# define _GL_CXXALIASWARN1(func,rettype,parameters_and_attributes) \
    _GL_EXTERN_C int _gl_cxxalias_dummy
#endif

#endif /* _GL_CXXDEFS_H */

/* The definition of _GL_ARG_NONNULL is copied here.  */
/* _GL_ARG_NONNULL((n,...,m)) tells the compiler and static analyzer tools
   that the values passed as arguments n, ..., m must be non-NULL pointers.
   n = 1 stands for the first argument, n = 2 for the second argument etc.  */
#ifndef _GL_ARG_NONNULL
# if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || __GNUC__ > 3
#  define _GL_ARG_NONNULL(params) __attribute__ ((__nonnull__ params))
# else
#  define _GL_ARG_NONNULL(params)
# endif
#endif

/* The definition of _GL_WARN_ON_USE is copied here.  */
#ifndef _GL_WARN_ON_USE

# if 4 < __GNUC__ || (__GNUC__ == 4 && 3 <= __GNUC_MINOR__)
/* A compiler attribute is available in gcc versions 4.3.0 and later.  */
#  define _GL_WARN_ON_USE(function, message) \
extern __typeof__ (function) function __attribute__ ((__warning__ (message)))
# elif __GNUC__ >= 3 && GNULIB_STRICT_CHECKING
/* Verify the existence of the function.  */
#  define _GL_WARN_ON_USE(function, message) \
extern __typeof__ (function) function
# else /* Unsupported.  */
#  define _GL_WARN_ON_USE(function, message) \
_GL_WARN_EXTERN_C int _gl_warn_on_use
# endif
#endif

/* _GL_WARN_ON_USE_CXX (function, rettype, parameters_and_attributes, "string")
   is like _GL_WARN_ON_USE (function, "string"), except that the function is
   declared with the given prototype, consisting of return type, parameters,
   and attributes.
   This variant is useful for overloaded functions in C++. _GL_WARN_ON_USE does
   not work in this case.  */
#ifndef _GL_WARN_ON_USE_CXX
# if 4 < __GNUC__ || (__GNUC__ == 4 && 3 <= __GNUC_MINOR__)
#  define _GL_WARN_ON_USE_CXX(function,rettype,parameters_and_attributes,msg) \
extern rettype function parameters_and_attributes \
     __attribute__ ((__warning__ (msg)))
# elif __GNUC__ >= 3 && GNULIB_STRICT_CHECKING
/* Verify the existence of the function.  */
#  define _GL_WARN_ON_USE_CXX(function,rettype,parameters_and_attributes,msg) \
extern rettype function parameters_and_attributes
# else /* Unsupported.  */
#  define _GL_WARN_ON_USE_CXX(function,rettype,parameters_and_attributes,msg) \
_GL_WARN_EXTERN_C int _gl_warn_on_use
# endif
#endif

/* _GL_WARN_EXTERN_C declaration;
   performs the declaration with C linkage.  */
#ifndef _GL_WARN_EXTERN_C
# if defined __cplusplus
#  define _GL_WARN_EXTERN_C extern "C"
# else
#  define _GL_WARN_EXTERN_C extern
# endif
#endif


/* Define wint_t and WEOF.  (Also done in wctype.in.h.)  */
#if !1 && !defined wint_t
# define wint_t int
# ifndef WEOF
#  define WEOF -1
# endif
#else
# ifndef WEOF
#  define WEOF ((wint_t) -1)
# endif
#endif


/* Override mbstate_t if it is too small.
   On IRIX 6.5, sizeof (mbstate_t) == 1, which is not sufficient for
   implementing mbrtowc for encodings like UTF-8.  */
#if !(1 && 1) || 0
typedef int rpl_mbstate_t;
# undef mbstate_t
# define mbstate_t rpl_mbstate_t
# define GNULIB_defined_mbstate_t 1
#endif


/* Convert a single-byte character to a wide character.  */
#if 0
# if 0
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef btowc
#   define btowc rpl_btowc
#  endif
_GL_FUNCDECL_RPL (btowc, wint_t, (int c));
_GL_CXXALIAS_RPL (btowc, wint_t, (int c));
# else
#  if !1
_GL_FUNCDECL_SYS (btowc, wint_t, (int c));
#  endif
_GL_CXXALIAS_SYS (btowc, wint_t, (int c));
# endif
_GL_CXXALIASWARN (btowc);
#elif defined GNULIB_POSIXCHECK
# undef btowc
# if HAVE_RAW_DECL_BTOWC
_GL_WARN_ON_USE (btowc, "btowc is unportable - "
                 "use gnulib module btowc for portability");
# endif
#endif


/* Convert a wide character to a single-byte character.  */
#if 0
# if 0
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef wctob
#   define wctob rpl_wctob
#  endif
_GL_FUNCDECL_RPL (wctob, int, (wint_t wc));
_GL_CXXALIAS_RPL (wctob, int, (wint_t wc));
# else
#  if !defined wctob && !1
/* wctob is provided by gnulib, or wctob exists but is not declared.  */
_GL_FUNCDECL_SYS (wctob, int, (wint_t wc));
#  endif
_GL_CXXALIAS_SYS (wctob, int, (wint_t wc));
# endif
_GL_CXXALIASWARN (wctob);
#elif defined GNULIB_POSIXCHECK
# undef wctob
# if HAVE_RAW_DECL_WCTOB
_GL_WARN_ON_USE (wctob, "wctob is unportable - "
                 "use gnulib module wctob for portability");
# endif
#endif


/* Test whether *PS is in the initial state.  */
#if 0
# if 0
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef mbsinit
#   define mbsinit rpl_mbsinit
#  endif
_GL_FUNCDECL_RPL (mbsinit, int, (const mbstate_t *ps));
_GL_CXXALIAS_RPL (mbsinit, int, (const mbstate_t *ps));
# else
#  if !1
_GL_FUNCDECL_SYS (mbsinit, int, (const mbstate_t *ps));
#  endif
_GL_CXXALIAS_SYS (mbsinit, int, (const mbstate_t *ps));
# endif
_GL_CXXALIASWARN (mbsinit);
#elif defined GNULIB_POSIXCHECK
# undef mbsinit
# if HAVE_RAW_DECL_MBSINIT
_GL_WARN_ON_USE (mbsinit, "mbsinit is unportable - "
                 "use gnulib module mbsinit for portability");
# endif
#endif


/* Convert a multibyte character to a wide character.  */
#if 0
# if 0
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef mbrtowc
#   define mbrtowc rpl_mbrtowc
#  endif
_GL_FUNCDECL_RPL (mbrtowc, size_t,
                  (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps));
_GL_CXXALIAS_RPL (mbrtowc, size_t,
                  (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps));
# else
#  if !1
_GL_FUNCDECL_SYS (mbrtowc, size_t,
                  (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps));
#  endif
_GL_CXXALIAS_SYS (mbrtowc, size_t,
                  (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps));
# endif
_GL_CXXALIASWARN (mbrtowc);
#elif defined GNULIB_POSIXCHECK
# undef mbrtowc
# if HAVE_RAW_DECL_MBRTOWC
_GL_WARN_ON_USE (mbrtowc, "mbrtowc is unportable - "
                 "use gnulib module mbrtowc for portability");
# endif
#endif


/* Recognize a multibyte character.  */
#if 0
# if 0
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef mbrlen
#   define mbrlen rpl_mbrlen
#  endif
_GL_FUNCDECL_RPL (mbrlen, size_t, (const char *s, size_t n, mbstate_t *ps));
_GL_CXXALIAS_RPL (mbrlen, size_t, (const char *s, size_t n, mbstate_t *ps));
# else
#  if !1
_GL_FUNCDECL_SYS (mbrlen, size_t, (const char *s, size_t n, mbstate_t *ps));
#  endif
_GL_CXXALIAS_SYS (mbrlen, size_t, (const char *s, size_t n, mbstate_t *ps));
# endif
_GL_CXXALIASWARN (mbrlen);
#elif defined GNULIB_POSIXCHECK
# undef mbrlen
# if HAVE_RAW_DECL_MBRLEN
_GL_WARN_ON_USE (mbrlen, "mbrlen is unportable - "
                 "use gnulib module mbrlen for portability");
# endif
#endif


/* Convert a string to a wide string.  */
#if 0
# if 0
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef mbsrtowcs
#   define mbsrtowcs rpl_mbsrtowcs
#  endif
_GL_FUNCDECL_RPL (mbsrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t len, mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
_GL_CXXALIAS_RPL (mbsrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t len,
                   mbstate_t *ps));
# else
#  if !1
_GL_FUNCDECL_SYS (mbsrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t len, mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
#  endif
_GL_CXXALIAS_SYS (mbsrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t len,
                   mbstate_t *ps));
# endif
_GL_CXXALIASWARN (mbsrtowcs);
#elif defined GNULIB_POSIXCHECK
# undef mbsrtowcs
# if HAVE_RAW_DECL_MBSRTOWCS
_GL_WARN_ON_USE (mbsrtowcs, "mbsrtowcs is unportable - "
                 "use gnulib module mbsrtowcs for portability");
# endif
#endif


/* Convert a string to a wide string.  */
#if 0
# if 0
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef mbsnrtowcs
#   define mbsnrtowcs rpl_mbsnrtowcs
#  endif
_GL_FUNCDECL_RPL (mbsnrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t srclen, size_t len,
                   mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
_GL_CXXALIAS_RPL (mbsnrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t srclen, size_t len,
                   mbstate_t *ps));
# else
#  if !1
_GL_FUNCDECL_SYS (mbsnrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t srclen, size_t len,
                   mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
#  endif
_GL_CXXALIAS_SYS (mbsnrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t srclen, size_t len,
                   mbstate_t *ps));
# endif
_GL_CXXALIASWARN (mbsnrtowcs);
#elif defined GNULIB_POSIXCHECK
# undef mbsnrtowcs
# if HAVE_RAW_DECL_MBSNRTOWCS
_GL_WARN_ON_USE (mbsnrtowcs, "mbsnrtowcs is unportable - "
                 "use gnulib module mbsnrtowcs for portability");
# endif
#endif


/* Convert a wide character to a multibyte character.  */
#if 0
# if 0
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef wcrtomb
#   define wcrtomb rpl_wcrtomb
#  endif
_GL_FUNCDECL_RPL (wcrtomb, size_t, (char *s, wchar_t wc, mbstate_t *ps));
_GL_CXXALIAS_RPL (wcrtomb, size_t, (char *s, wchar_t wc, mbstate_t *ps));
# else
#  if !1
_GL_FUNCDECL_SYS (wcrtomb, size_t, (char *s, wchar_t wc, mbstate_t *ps));
#  endif
_GL_CXXALIAS_SYS (wcrtomb, size_t, (char *s, wchar_t wc, mbstate_t *ps));
# endif
_GL_CXXALIASWARN (wcrtomb);
#elif defined GNULIB_POSIXCHECK
# undef wcrtomb
# if HAVE_RAW_DECL_WCRTOMB
_GL_WARN_ON_USE (wcrtomb, "wcrtomb is unportable - "
                 "use gnulib module wcrtomb for portability");
# endif
#endif


/* Convert a wide string to a string.  */
#if 0
# if 0
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef wcsrtombs
#   define wcsrtombs rpl_wcsrtombs
#  endif
_GL_FUNCDECL_RPL (wcsrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t len, mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
_GL_CXXALIAS_RPL (wcsrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t len,
                   mbstate_t *ps));
# else
#  if !1
_GL_FUNCDECL_SYS (wcsrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t len, mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
#  endif
_GL_CXXALIAS_SYS (wcsrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t len,
                   mbstate_t *ps));
# endif
_GL_CXXALIASWARN (wcsrtombs);
#elif defined GNULIB_POSIXCHECK
# undef wcsrtombs
# if HAVE_RAW_DECL_WCSRTOMBS
_GL_WARN_ON_USE (wcsrtombs, "wcsrtombs is unportable - "
                 "use gnulib module wcsrtombs for portability");
# endif
#endif


/* Convert a wide string to a string.  */
#if 0
# if 0
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef wcsnrtombs
#   define wcsnrtombs rpl_wcsnrtombs
#  endif
_GL_FUNCDECL_RPL (wcsnrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t srclen, size_t len,
                   mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
_GL_CXXALIAS_RPL (wcsnrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t srclen, size_t len,
                   mbstate_t *ps));
# else
#  if !1
_GL_FUNCDECL_SYS (wcsnrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t srclen, size_t len,
                   mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
#  endif
_GL_CXXALIAS_SYS (wcsnrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t srclen, size_t len,
                   mbstate_t *ps));
# endif
_GL_CXXALIASWARN (wcsnrtombs);
#elif defined GNULIB_POSIXCHECK
# undef wcsnrtombs
# if HAVE_RAW_DECL_WCSNRTOMBS
_GL_WARN_ON_USE (wcsnrtombs, "wcsnrtombs is unportable - "
                 "use gnulib module wcsnrtombs for portability");
# endif
#endif


/* Return the number of screen columns needed for WC.  */
#if 0
# if 0
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef wcwidth
#   define wcwidth rpl_wcwidth
#  endif
_GL_FUNCDECL_RPL (wcwidth, int, (wchar_t));
_GL_CXXALIAS_RPL (wcwidth, int, (wchar_t));
# else
#  if !1
/* wcwidth exists but is not declared.  */
_GL_FUNCDECL_SYS (wcwidth, int, (wchar_t));
#  endif
_GL_CXXALIAS_SYS (wcwidth, int, (wchar_t));
# endif
_GL_CXXALIASWARN (wcwidth);
#elif defined GNULIB_POSIXCHECK
# undef wcwidth
# if HAVE_RAW_DECL_WCWIDTH
_GL_WARN_ON_USE (wcwidth, "wcwidth is unportable - "
                 "use gnulib module wcwidth for portability");
# endif
#endif


#endif /* _GL_WCHAR_H */
#endif /* _GL_WCHAR_H */
#endif
