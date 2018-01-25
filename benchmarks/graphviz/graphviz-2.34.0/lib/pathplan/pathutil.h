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



#ifndef _PATHUTIL_INCLUDE
#define _PATHUTIL_INCLUDE
#define _BLD_pathplan 1

#include "pathplan.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NOT
#define NOT(x)	(!(x))
#endif
#ifndef FALSE
#define FALSE	0
#define TRUE	(NOT(FALSE))
#endif

/*visual studio*/
#ifdef WIN32_DLL
#ifndef PATHPLAN_EXPORTS
#define extern __declspec(dllimport)
#endif
#endif
/*end visual studio*/
	typedef double COORD;
    extern COORD area2(Ppoint_t, Ppoint_t, Ppoint_t);
    extern int wind(Ppoint_t a, Ppoint_t b, Ppoint_t c);
    extern COORD dist2(Ppoint_t, Ppoint_t);
    extern int intersect(Ppoint_t a, Ppoint_t b, Ppoint_t c, Ppoint_t d);

    int in_poly(Ppoly_t argpoly, Ppoint_t q);
    Ppoly_t copypoly(Ppoly_t);
    void freepoly(Ppoly_t);

#undef extern
#ifdef __cplusplus
}
#endif
#endif
