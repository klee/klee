/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

/* : : generated from features/common by iffe version 1999-08-11 : : */
#ifndef _AST_COMMON_H
#define _AST_COMMON_H	1
#define _hdr_pthread	1	/* #include <pthread.h> ok */
#define _hdr_stddef	1	/* #include <stddef.h> ok */
#define _sys_types	1	/* #include <sys/types.h> ok */
#define _hdr_time	1	/* #include <time.h> ok */
#define _sys_time	0	/* #include <sys/time.h> ok */
#define _sys_times	0	/* #include <sys/times.h> ok */
#define _hdr_stdlib	0	/* #include <stdlib.h> ok */
#define _typ_size_t	1	/* size_t is a type */
#define _typ_ssize_t	0	/* ssize_t is a type */
#define _typ_long_double	1	/* long double is a type */
#define _proto_stdc	1	/* Standard-C prototypes ok */

/* __STD_C indicates that the language is ANSI-C or C++ */
#if !defined(__STD_C) && __STDC__
#define	__STD_C		1
#endif
#if !defined(__STD_C) && (__cplusplus || c_plusplus)
#define __STD_C		1
#endif
#if !defined(__STD_C) && _proto_stdc
#define __STD_C		1
#endif
#if !defined(__STD_C)
#define __STD_C		0
#endif

/* extern symbols must be protected against C++ name mangling */
#ifndef _BEGIN_EXTERNS_
#if __cplusplus || c_plusplus
#define _BEGIN_EXTERNS_	extern "C" {
#define _END_EXTERNS_	}
#else
#define _BEGIN_EXTERNS_
#define _END_EXTERNS_
#endif
#endif /*_BEGIN_EXTERNS_*/

/* _ARG_ simplifies function prototyping among flavors of C */
#ifndef _ARG_
#if __STD_C
#define _ARG_(x)	x
#else
#define _ARG_(x)	()
#endif
#endif /*_ARG_*/

/* __INLINE__ is the inline keyword */
#if !defined(__INLINE__) && defined(__cplusplus)
#define __INLINE__	inline
#endif
#if !defined(__INLINE__) && defined(_WIN32) && !defined(__GNUC__)
#define __INLINE__	__inline
#endif

/* Void_t is defined so that Void_t* can address any type */
#ifndef Void_t
#if __STD_C
#define Void_t		void
#else
#define Void_t		char
#endif
#endif				/*Void_t */

/* dynamic linked library external scope handling */
#undef extern
#if _dll_import && !defined(__EXPORT__) && _DLL_BLD
#define __EXPORT__	__declspec(dllexport)
#endif
#if _dll_import && !defined(__IMPORT__)
#define __IMPORT__	__declspec(dllimport)
#endif
#if !defined(_astimport)
#if defined(__IMPORT__) && _DLL_BLD
#define _astimport	__IMPORT__
#else
#define _astimport	extern
#endif
#endif /*_astimport*/
#if !_DLL_BLD && _dll_import
#define __EXTERN__(T,obj)	extern T obj; T* _imp__ ## obj = &obj
#define __DEFINE__(T,obj,val)	T obj = val; T* _imp__ ## obj = &obj
#else
#define __EXTERN__(T,obj)	extern T obj
#define __DEFINE__(T,obj,val)	T obj = val
#endif
#ifndef _AST_STD_H
#	if _hdr_stddef
#	include	<stddef.h>
#	endif
#	if _sys_types
#	include	<sys/types.h>
#	endif
#endif
#if !_typ_size_t
#	define _typ_size_t	1
typedef int size_t;
#endif
#if !_typ_ssize_t
#	define _typ_ssize_t	1
typedef int ssize_t;
#endif
#define _ast_int1_t		char
#define _ast_int2_t		short
#define _ast_int4_t		int
#define _ast_int8_t		long long
#define _ast_intmax_t		_ast_int8_t
#define _ast_intswap		7

#define _ast_flt4_t		float
#define _ast_flt8_t		double
#define _ast_flt12_t		long double
#define _ast_fltmax_t		_ast_flt12_t

#ifndef va_listref
#define va_listref(p) (p)	/* pass va_list to varargs function */
#define va_listval(p) (p)	/* retrieve va_list from va_arg(ap,va_listarg) */
#define va_listarg va_list	/* va_arg() va_list type */
#ifndef va_copy
#define va_copy(to,fr) ((to)=(fr))	/* copy va_list fr -> to */
#endif
#undef	_ast_va_list
#ifdef	va_start
#define _ast_va_list va_list
#else
#define _ast_va_list void*	/* va_list that avoids #include */
#endif

#endif
#endif
