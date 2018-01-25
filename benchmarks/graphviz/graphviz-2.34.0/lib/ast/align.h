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

#ifdef __cplusplus
extern "C" {
#endif

/* : : generated from features/align.c by iffe version 1999-08-11 : : */
#ifndef _def_align_ast
#define _def_align_ast	1
    typedef unsigned long ALIGN_INTEGRAL;

#define ALIGN_CHUNK		8192
#define ALIGN_INTEGRAL		long
#define ALIGN_INTEGER(x)	((ALIGN_INTEGRAL)(x))
#define ALIGN_POINTER(x)	((char*)(x))
#define ALIGN_ROUND(x,y)	ALIGN_POINTER(ALIGN_INTEGER((x)+(y)-1)&~((y)-1))

#define ALIGN_BOUND		ALIGN_BOUND2
#define ALIGN_ALIGN(x)		ALIGN_ALIGN2(x)
#define ALIGN_TRUNC(x)		ALIGN_TRUNC2(x)

#define ALIGN_BIT1		0x1
#define ALIGN_BOUND1		ALIGN_BOUND2
#define ALIGN_ALIGN1(x)		ALIGN_ALIGN2(x)
#define ALIGN_TRUNC1(x)		ALIGN_TRUNC2(x)
#define ALIGN_CLRBIT1(x)	ALIGN_POINTER(ALIGN_INTEGER(x)&0xfffffffe)
#define ALIGN_SETBIT1(x)	ALIGN_POINTER(ALIGN_INTEGER(x)|0x1)
#define ALIGN_TSTBIT1(x)	ALIGN_POINTER(ALIGN_INTEGER(x)&0x1)

#define ALIGN_BIT2		0x2
#define ALIGN_BOUND2		8
#define ALIGN_ALIGN2(x)		ALIGN_TRUNC2((x)+7)
#define ALIGN_TRUNC2(x)		ALIGN_POINTER(ALIGN_INTEGER(x)&0xfffffff8)
#define ALIGN_CLRBIT2(x)	ALIGN_POINTER(ALIGN_INTEGER(x)&0xfffffffd)
#define ALIGN_SETBIT2(x)	ALIGN_POINTER(ALIGN_INTEGER(x)|0x2)
#define ALIGN_TSTBIT2(x)	ALIGN_POINTER(ALIGN_INTEGER(x)&0x2)

#endif

#ifdef __cplusplus
}
#endif
