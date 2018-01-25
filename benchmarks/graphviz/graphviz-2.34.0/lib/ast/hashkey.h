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
 * Glenn Fowler
 * AT&T Research
 *
 * 1-6 char lower-case keyword -> long hash
 * digit args passed as HASHKEYN('2')
 */

#ifndef _HASHKEY_H
#define _HASHKEY_H

#define HASHKEYMAX			6
#define HASHKEYBIT			5
#define HASHKEYOFF			('a'-1)
#define HASHKEYPART(h,c)		(((h)<<HASHKEYBIT)+HASHKEY1(c))

#define HASHKEYN(n)			((n)-'0'+'z'+1)

#define HASHKEY1(c1)			((c1)-HASHKEYOFF)
#define HASHKEY2(c1,c2)			HASHKEYPART(HASHKEY1(c1),c2)
#define HASHKEY3(c1,c2,c3)		HASHKEYPART(HASHKEY2(c1,c2),c3)
#define HASHKEY4(c1,c2,c3,c4)		HASHKEYPART(HASHKEY3(c1,c2,c3),c4)
#define HASHKEY5(c1,c2,c3,c4,c5)	HASHKEYPART(HASHKEY4(c1,c2,c3,c4),c5)
#define HASHKEY6(c1,c2,c3,c4,c5,c6)	HASHKEYPART(HASHKEY5(c1,c2,c3,c4,c5),c6)

#define HASHNKEY1(n,c1)			HASHKEY2((n)+HASHKEYOFF,c1)
#define HASHNKEY2(n,c2,c1)		HASHKEY3((n)+HASHKEYOFF,c2,c1)
#define HASHNKEY3(n,c3,c2,c1)		HASHKEY4((n)+HASHKEYOFF,c3,c2,c1)
#define HASHNKEY4(n,c4,c3,c2,c1)	HASHKEY5((n)+'a',c4,c3,c2,c1)
#define HASHNKEY5(n,c5,c4,c3,c2,c1)	HASHKEY6((n)+'a',c5,c4,c3,c2,c1)

#if _BLD_ast && defined(__EXPORT__)
#define extern		__EXPORT__
#endif

    extern long strkey(const char *);

#undef	extern

#endif

#ifdef __cplusplus
}
#endif
