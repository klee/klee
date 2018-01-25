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

/* 
 * Force the vertices of a polygon to be in CW order.
 * 
 * Works for polygons with concavities.
 * Does not work for twisted polygons.
 *
 * ellson@graphviz.org    October 2nd, 1996
 */

#include <pathutil.h>

void make_CW(Ppoly_t * poly)
{
    int i, j, n;
    Ppoint_t *P;
    Ppoint_t tP;
    double area = 0.0;

    P = poly->ps;
    n = poly->pn;
    /* points or lines don't have a rotation */
    if (n > 2) {
	/* check CW or CCW by computing (twice the) area of poly */
	for (i = 1; i < n - 1; i++) {
	    area += area2(P[0], P[i + 1], P[i]);
	}
	/* if the area is -ve then the rotation needs to be reversed */
	/* the starting point is left unchanged */
	if (area < 0.0) {
	    for (i = 1, j = n - 1; i < 1 + n / 2; i++, j--) {
		tP = P[i];
		P[i] = P[j];
		P[j] = tP;
	    }
	}
    }
}
