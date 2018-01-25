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


#ifndef VISIBILITY_H
#define VISIBILITY_H

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include "vispath.h"
#include "pathutil.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef COORD **array2;

#define	OBSCURED	0.0
#define EQ(p,q)		((p.x == q.x) && (p.y == q.y))
#define NEQ(p,q)	(!EQ(p,q))
#define NIL(p)		((p)0)
#define	CW			0
#define	CCW			1

    struct vconfig_s {
	int Npoly;
	int N;			/* number of points in walk of barriers */
	Ppoint_t *P;		/* barrier points */
	int *start;
	int *next;
	int *prev;

	/* this is computed from the above */
	array2 vis;
    };
#ifdef WIN32_DLL
#ifndef PATHPLAN_EXPORTS
#define extern __declspec(dllimport)
#endif
#endif
/*end visual studio*/

	extern COORD *ptVis(vconfig_t *, int, Ppoint_t);
    extern int directVis(Ppoint_t, int, Ppoint_t, int, vconfig_t *);
    extern void visibility(vconfig_t *);
    extern int *makePath(Ppoint_t p, int pp, COORD * pvis,
			 Ppoint_t q, int qp, COORD * qvis,
			 vconfig_t * conf);

#undef extern

#ifdef __cplusplus
}
#endif
#endif
