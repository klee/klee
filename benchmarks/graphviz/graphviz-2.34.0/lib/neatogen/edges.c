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

#include "neato.h"
#include "mem.h"
#include "info.h"
#include "edges.h"
#include <math.h>


double pxmin, pxmax, pymin, pymax;	/* clipping window */

static int nedges;
static Freelist efl;

void edgeinit()
{
    freeinit(&efl, sizeof(Edge));
    nedges = 0;
}

Edge *bisect(Site * s1, Site * s2)
{
    double dx, dy, adx, ady;
    Edge *newedge;

    newedge = (Edge *) getfree(&efl);

    newedge->reg[0] = s1;
    newedge->reg[1] = s2;
    ref(s1);
    ref(s2);
    newedge->ep[0] = (Site *) NULL;
    newedge->ep[1] = (Site *) NULL;

    dx = s2->coord.x - s1->coord.x;
    dy = s2->coord.y - s1->coord.y;
    adx = dx > 0 ? dx : -dx;
    ady = dy > 0 ? dy : -dy;
    newedge->c =
	s1->coord.x * dx + s1->coord.y * dy + (dx * dx + dy * dy) * 0.5;
    if (adx > ady) {
	newedge->a = 1.0;
	newedge->b = dy / dx;
	newedge->c /= dx;
    } else {
	newedge->b = 1.0;
	newedge->a = dx / dy;
	newedge->c /= dy;
    };

    newedge->edgenbr = nedges;
#ifdef STANDALONE
    out_bisector(newedge);
#endif
    nedges += 1;
    return (newedge);
}


static void doSeg(Edge * e, double x1, double y1, double x2, double y2)
{
    addVertex(e->reg[0], x1, y1);
    addVertex(e->reg[0], x2, y2);
    addVertex(e->reg[1], x1, y1);
    addVertex(e->reg[1], x2, y2);
}

void clip_line(Edge * e)
{
    Site *s1, *s2;
    double x1, x2, y1, y2;

    if (e->a == 1.0 && e->b >= 0.0) {
	s1 = e->ep[1];
	s2 = e->ep[0];
    } else {
	s1 = e->ep[0];
	s2 = e->ep[1];
    }

    if (e->a == 1.0) {
	if (s1 != (Site *) NULL) {
	    y1 = s1->coord.y;
	    if (y1 > pymax)
		return;
	    else if (y1 >= pymin)
		x1 = s1->coord.x;
	    else {
		y1 = pymin;
		x1 = e->c - e->b * y1;
	    }
	} else {
	    y1 = pymin;
	    x1 = e->c - e->b * y1;
	}

	if (s2 != (Site *) NULL) {
	    y2 = s2->coord.y;
	    if (y2 < pymin)
		return;
	    else if (y2 <= pymax)
		x2 = s2->coord.x;
	    else {
		y2 = pymax;
		x2 = e->c - e->b * y2;
	    }
	} else {
	    y2 = pymax;
	    x2 = e->c - e->b * y2;
	}

	if (((x1 > pxmax) & (x2 > pxmax)) | ((x1 < pxmin) & (x2 < pxmin)))
	    return;
	if (x1 > pxmax) {
	    x1 = pxmax;
	    y1 = (e->c - x1) / e->b;
	};
	if (x1 < pxmin) {
	    x1 = pxmin;
	    y1 = (e->c - x1) / e->b;
	};
	if (x2 > pxmax) {
	    x2 = pxmax;
	    y2 = (e->c - x2) / e->b;
	};
	if (x2 < pxmin) {
	    x2 = pxmin;
	    y2 = (e->c - x2) / e->b;
	};
    } else {
	if (s1 != (Site *) NULL) {
	    x1 = s1->coord.x;
	    if (x1 > pxmax)
		return;
	    else if (x1 >= pxmin)
		y1 = s1->coord.y;
	    else {
		x1 = pxmin;
		y1 = e->c - e->a * x1;
	    }
	} else {
	    x1 = pxmin;
	    y1 = e->c - e->a * x1;
	}

	if (s2 != (Site *) NULL) {
	    x2 = s2->coord.x;
	    if (x2 < pxmin)
		return;
	    else if (x2 <= pxmax)
		y2 = s2->coord.y;
	    else {
		x2 = pxmax;
		y2 = e->c - e->a * x2;
	    }
	} else {
	    x2 = pxmax;
	    y2 = e->c - e->a * x2;
	}

	if (((y1 > pymax) & (y2 > pymax)) | ((y1 < pymin) & (y2 < pymin)))
	    return;
	if (y1 > pymax) {
	    y1 = pymax;
	    x1 = (e->c - y1) / e->a;
	};
	if (y1 < pymin) {
	    y1 = pymin;
	    x1 = (e->c - y1) / e->a;
	};
	if (y2 > pymax) {
	    y2 = pymax;
	    x2 = (e->c - y2) / e->a;
	};
	if (y2 < pymin) {
	    y2 = pymin;
	    x2 = (e->c - y2) / e->a;
	};
    }

    doSeg(e, x1, y1, x2, y2);
#ifdef STANDALONE
    if (doPS)
	line(x1, y1, x2, y2);
#endif
}

void endpoint(Edge * e, int lr, Site * s)
{
    e->ep[lr] = s;
    ref(s);
    if (e->ep[re - lr] == (Site *) NULL)
	return;
    clip_line(e);
#ifdef STANDALONE
    out_ep(e);
#endif
    deref(e->reg[le]);
    deref(e->reg[re]);
    makefree(e, &efl);
}
