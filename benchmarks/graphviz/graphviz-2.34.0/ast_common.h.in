#ifndef _AST_COMMON_H
#define _AST_COMMON_H   1

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#  define _sys_types 1
#endif

#ifdef HAVE_LIMITS_H
#  define _hdr_limits 1
#endif

#ifdef HAVE_FLOAT_H
#  define _hdr_float 1
#endif

#ifdef HAVE_VALUES_H
#  define _hdr_values 1
#endif

#ifdef HAVE_SYS_TYPES_H
#  define _sys_types 1
#endif

#ifdef HAVE_PTHREAD_H
#  define _hdr_pthread 1
#endif

#ifdef HAVE_STDDEF_H
#  define hdr_stddef 1
#endif

#ifdef HAVE_TIME_H
#  define _hdr_time 1
#endif

#ifdef HAVE_SYS_TIME_H
#  define _sys_time 1
#endif

#ifdef HAVE_SYS_TIMES_H
#  define _sys_times 1
#endif

#ifdef HAVE_STDLIB_H
#  define _hdr_stdlib 1
#endif

#ifdef HAVE_SYS_SOCKET_H
#  define _sys_socket 1
#endif

#ifdef HAVE_SYS_STAT_H
#  define _sys_stat 1
#endif

#ifdef HAVE_STRING_H
#  define _hdr_string 1
#endif

#ifdef HAVE_STDINT_H
#  define _hdr_stdint 1
#endif

#ifdef HAVE_STDARG_H
#  define _hdr_stdarg 1
#endif

#ifdef HAVE_VARARGS_H
#  define _hdr_varargs 1
#endif

/* FIXME - need proper config test for these */
#define _typ_size_t     1	/* size_t is a type */
#define _typ_ssize_t    1	/* ssize_t is a type */
#define _typ_long_double        1	/* long double is a type */
#define _proto_stdc 1		/* Standard-C prototypes ok  */
/* */

/* __STD_C indicates that the language is ANSI-C or C++ */
#if !defined(__STD_C) && __STDC__
#define __STD_C         1
#endif
#if !defined(__STD_C) && (__cplusplus || c_plusplus)
#define __STD_C         1
#endif
#if !defined(__STD_C) && _proto_stdc
#define __STD_C         1
#endif
#if !defined(__STD_C)
#define __STD_C         0
#endif

/* extern symbols must be protected against C++ name mangling */
#ifndef _BEGIN_EXTERNS_
#  if __cplusplus || c_plusplus
#    define _BEGIN_EXTERNS_ extern "C" {
#    define _END_EXTERNS_   }
#  else
#    define _BEGIN_EXTERNS_
#    define _END_EXTERNS_
#  endif
#endif /*_BEGIN_EXTERNS_*/

/* _ARG_ simplifies function prototyping among flavors of C */
#ifndef _ARG_
#if __STD_C
#define _ARG_(x)        x
#else
#define _ARG_(x)        ()
#endif
#endif /*_ARG_*/

/* __INLINE__ is the inline keyword */
#if !defined(__INLINE__) && defined(__cplusplus)
#define __INLINE__      inline
#endif
#if !defined(__INLINE__) && defined(_WIN32) && !defined(__GNUC__)
#define __INLINE__      __inline
#endif

/* Void_t is defined so that Void_t* can address any type */
#ifndef Void_t
#if __STD_C
#define Void_t          void
#else
#define Void_t          char
#endif
#endif				/*Void_t */

/* dynamic linked library external scope handling */
#undef extern
#if _dll_import && !defined(__EXPORT__) && _DLL_BLD
#define __EXPORT__      __declspec(dllexport)
#endif
#if _dll_import && !defined(__IMPORT__)
#define __IMPORT__      __declspec(dllimport)
#endif
#if !defined(_astimport)
#if defined(__IMPORT__) && _DLL_BLD
#define _astimport      __IMPORT__
#else
#define _astimport      extern
#endif
#endif /*_astimport*/

#if !_DLL_BLD && _dll_import
#define __EXTERN__(T,obj)       extern T obj; T* _imp__ ## obj = &obj
#define __DEFINE__(T,obj,val)   T obj = val; T* _imp__ ## obj = &obj
#else
#define __EXTERN__(T,obj)       extern T obj
#define __DEFINE__(T,obj,val)   T obj = val
#endif

#ifndef _AST_STD_H
#       if _hdr_stddef
#       include <stddef.h>
#       endif
#       if _hdr_stdarg
#       include <stdarg.h>
#       endif
#       if _hdr_varargs
#       include <varargs.h>
#       endif
#       if _sys_types
#       include <sys/types.h>
#       endif
#endif
#if !_typ_size_t
#       define _typ_size_t      1
typedef int size_t;
#endif
#if !_typ_ssize_t
#       define _typ_ssize_t     1
typedef int ssize_t;
#endif

/* FIXME - need proper configure tests for these */
#define _ast_int1_t             char
#define _ast_int2_t             short
#define _ast_int4_t             int
#define _ast_int8_t             long long
#define _ast_intmax_t           _ast_int8_t
#define _ast_intswap            7

#define _ast_flt4_t             float
#define _ast_flt8_t             double
#define _ast_flt12_t            long double
#define _ast_fltmax_t           _ast_flt12_t
/* */

#ifndef va_listref
#define va_listref(p) (p)	/* pass va_list to varargs function */
#define va_listval(p) (p)	/* retrieve va_list from va_arg(ap,va_listarg) */
#define va_listarg va_list	/* va_arg() va_list type */
#ifndef va_copy
#define va_copy(to,fr) ((to)=(fr))	/* copy va_list fr -> to */
#endif
#undef  _ast_va_list
#ifdef  va_start
#define _ast_va_list va_list
#else
#define _ast_va_list void*	/* va_list that avoids #include */
#endif
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
#ifdef HAVE_VALUES_H
#include <values.h>
#endif
#endif

#ifndef MAXINT
#define MAXINT INT_MAX
#endif

#endif				/*AST_COMMON_H */
