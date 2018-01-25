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


#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <setjmp.h>
#include "pathutil.h"
#include "tri.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

typedef struct lvertex_2_t {
    double x, y;
} lvertex_2_t;

typedef struct dpd_triangle {
    Ppoint_t *v[3];
} ltriangle_t;

#define ISCCW 1
#define ISCW  2
#define ISON  3

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

static jmp_buf jbuf;
static int dpd_ccw(Ppoint_t *, Ppoint_t *, Ppoint_t *);
static int dpd_isdiagonal(int, int, Ppoint_t **, int);
static int dpd_intersects(Ppoint_t *, Ppoint_t *, Ppoint_t *, Ppoint_t *);
static int dpd_between(Ppoint_t *, Ppoint_t *, Ppoint_t *);
static void triangulate(Ppoint_t ** pointp, int pointn,
			void (*fn) (void *, Ppoint_t *), void *vc);

static int dpd_ccw(Ppoint_t * p1, Ppoint_t * p2, Ppoint_t * p3)
{
    double d =
	((p1->y - p2->y) * (p3->x - p2->x)) -
	((p3->y - p2->y) * (p1->x - p2->x));
    return (d > 0) ? ISCW : ((d < 0) ? ISCCW : ISON);
}

/* Ptriangulate:
 * Return 0 on success; non-zero on error.
 */
int Ptriangulate(Ppoly_t * polygon, void (*fn) (void *, Ppoint_t *),
		  void *vc)
{
    int i;
    int pointn;
    Ppoint_t **pointp;

    pointn = polygon->pn;

    pointp = (Ppoint_t **) malloc(pointn * sizeof(Ppoint_t *));

    for (i = 0; i < pointn; i++)
	pointp[i] = &(polygon->ps[i]);

    if (setjmp(jbuf)) {
	free(pointp);
	return 1;
    }
    triangulate(pointp, pointn, fn, vc);

    free(pointp);
    return 0;
}

/* triangulate:
 * Triangulates the given polygon. 
 * Throws an exception if no diagonal exists.
 */
static void
triangulate(Ppoint_t ** pointp, int pointn,
	    void (*fn) (void *, Ppoint_t *), void *vc)
{
    int i, ip1, ip2, j;
    Ppoint_t A[3];
    if (pointn > 3) {
	for (i = 0; i < pointn; i++) {
	    ip1 = (i + 1) % pointn;
	    ip2 = (i + 2) % pointn;
	    if (dpd_isdiagonal(i, ip2, pointp, pointn)) {
		A[0] = *pointp[i];
		A[1] = *pointp[ip1];
		A[2] = *pointp[ip2];
		fn(vc, A);
		j = 0;
		for (i = 0; i < pointn; i++)
		    if (i != ip1)
			pointp[j++] = pointp[i];
		triangulate(pointp, pointn - 1, fn, vc);
		return;
	    }
	}
	longjmp(jbuf,1);
    } else {
	A[0] = *pointp[0];
	A[1] = *pointp[1];
	A[2] = *pointp[2];
	fn(vc, A);
    }
}

/* check if (i, i + 2) is a diagonal */
static int dpd_isdiagonal(int i, int ip2, Ppoint_t ** pointp, int pointn)
{
    int ip1, im1, j, jp1, res;

    /* neighborhood test */
    ip1 = (i + 1) % pointn;
    im1 = (i + pointn - 1) % pointn;
    /* If P[i] is a convex vertex [ i+1 left of (i-1,i) ]. */
    if (dpd_ccw(pointp[im1], pointp[i], pointp[ip1]) == ISCCW)
	res = (dpd_ccw(pointp[i], pointp[ip2], pointp[im1]) == ISCCW) &&
	    (dpd_ccw(pointp[ip2], pointp[i], pointp[ip1]) == ISCCW);
    /* Assume (i - 1, i, i + 1) not collinear. */
    else
	res = ((dpd_ccw(pointp[i], pointp[ip2], pointp[ip1]) == ISCW)
	    );
/*
		&&
                (dpd_ccw (pointp[ip2], pointp[i], pointp[im1]) != ISCW));
*/
    if (!res) {
	return FALSE;
    }

    /* check against all other edges */
    for (j = 0; j < pointn; j++) {
	jp1 = (j + 1) % pointn;
	if (!((j == i) || (jp1 == i) || (j == ip2) || (jp1 == ip2)))
	    if (dpd_intersects
		(pointp[i], pointp[ip2], pointp[j], pointp[jp1])) {
		return FALSE;
	    }
    }
    return TRUE;
}

/* line to line intersection */
static int dpd_intersects(Ppoint_t * pa, Ppoint_t * pb, Ppoint_t * pc,
			  Ppoint_t * pd)
{
    int ccw1, ccw2, ccw3, ccw4;

    if (dpd_ccw(pa, pb, pc) == ISON || dpd_ccw(pa, pb, pd) == ISON ||
	dpd_ccw(pc, pd, pa) == ISON || dpd_ccw(pc, pd, pb) == ISON) {
	if (dpd_between(pa, pb, pc) || dpd_between(pa, pb, pd) ||
	    dpd_between(pc, pd, pa) || dpd_between(pc, pd, pb))
	    return TRUE;
    } else {
	ccw1 = (dpd_ccw(pa, pb, pc) == ISCCW) ? 1 : 0;
	ccw2 = (dpd_ccw(pa, pb, pd) == ISCCW) ? 1 : 0;
	ccw3 = (dpd_ccw(pc, pd, pa) == ISCCW) ? 1 : 0;
	ccw4 = (dpd_ccw(pc, pd, pb) == ISCCW) ? 1 : 0;
	return (ccw1 ^ ccw2) && (ccw3 ^ ccw4);
    }
    return FALSE;
}

static int dpd_between(Ppoint_t * pa, Ppoint_t * pb, Ppoint_t * pc)
{
    Ppoint_t pba, pca;

    pba.x = pb->x - pa->x, pba.y = pb->y - pa->y;
    pca.x = pc->x - pa->x, pca.y = pc->y - pa->y;
    if (dpd_ccw(pa, pb, pc) != ISON)
	return FALSE;
    return (pca.x * pba.x + pca.y * pba.y >= 0) &&
	(pca.x * pca.x + pca.y * pca.y <= pba.x * pba.x + pba.y * pba.y);
}
