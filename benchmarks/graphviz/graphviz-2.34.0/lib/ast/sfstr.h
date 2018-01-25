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

/*
 * macro interface for sfio write strings
 *
 * NOTE: see <stak.h> for an alternative interface
 *	 read operations require sfseek()
 */

#ifndef _SFSTR_H
#define _SFSTR_H

#include <sfio.h>

#define sfstropen()	sfnew((Sfio_t*)0,(char*)0,-1,-1,SF_WRITE|SF_STRING)
#define sfstrnew(m)	sfnew((Sfio_t*)0,(char*)0,-1,-1,(m)|SF_STRING)
#define sfstrclose(f)	sfclose(f)

#define sfstrtell(f)	((f)->next - (f)->data)
#define sfstrrel(f,p)	((p) == (0) ? (char*)(f)->next : \
			 ((f)->next += (p), \
			  ((f)->next >= (f)->data && (f)->next  <= (f)->endb) ? \
				(char*)(f)->next : ((f)->next -= (p), (char*)0) ) )

#define sfstrset(f,p)	(((p) >= 0 && (p) <= (f)->size) ? \
				(char*)((f)->next = (f)->data+(p)) : (char*)0 )

#define sfstrbase(f)	((char*)(f)->data)
#define sfstrsize(f)	((f)->size)

#define sfstrrsrv(f,n)	(sfreserve(f,(long)(n),1)?(sfwrite(f,(char*)(f)->next,0),(char*)(f)->next):(char*)0)

#define sfstruse(f)	(sfputc(f,0), (char*)((f)->next = (f)->data) )

#if _BLD_ast && defined(__EXPORT__)
#define extern		__EXPORT__
#endif

    extern int sfstrtmp(Sfio_t *, int, void *, size_t);

#undef	extern

#endif

#ifdef __cplusplus
}
#endif
