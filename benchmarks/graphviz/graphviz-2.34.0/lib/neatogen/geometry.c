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

#include "geometry.h"
#include <math.h>


Point origin = { 0, 0 };

double xmin, xmax, ymin, ymax;	/* min and max x and y values of sites */
double deltax,			/* xmax - xmin */
 deltay;			/* ymax - ymin */

int nsites;
int sqrt_nsites;

void geominit()
{
    double sn;

    sn = nsites + 4;
    sqrt_nsites = (int) sqrt(sn);
    /* deltay = ymax - ymin; */
    /* deltax = xmax - xmin; */
}

double dist_2(Point * pp, Point * qp)
{
    double dx = pp->x - qp->x;
    double dy = pp->y - qp->y;

    return (dx * dx + dy * dy);
}

void subpt(Point * a, Point b, Point c)
{
    a->x = b.x - c.x;
    a->y = b.y - c.y;
}

void addpt(Point * c, Point a, Point b)
{
    c->x = a.x + b.x;
    c->y = a.y + b.y;
}

double area_2(Point a, Point b, Point c)
{
    return ((a.y - b.y) * (c.x - b.x) - (c.y - b.y) * (a.x - b.x));
}

int leftOf(Point a, Point b, Point c)
{
    return (area_2(a, b, c) > 0);
}

int intersection(Point a, Point b, Point c, Point d, Point * p)
{
    double s, t;		/* The two parameters of the parametric eqns. */
    double denom;		/* Denominator of solutions. */

    denom =
	a.x * (d.y - c.y) +
	b.x * (c.y - d.y) + d.x * (b.y - a.y) + c.x * (a.y - b.y);

    /* If denom is zero, then the line segments are parallel. */
    /* In this case, return false even though the segments might overlap. */
    if (denom == 0.0)
	return 0;

    s = (a.x * (d.y - c.y) + c.x * (a.y - d.y) + d.x * (c.y - a.y)
	) / denom;
    t = -(a.x * (c.y - b.y) + b.x * (a.y - c.y) + c.x * (b.y - a.y)
	) / denom;

    p->x = a.x + s * (b.x - a.x);
    p->y = a.y + s * (b.y - a.y);

    if ((0.0 <= s) && (s <= 1.0) && (0.0 <= t) && (t <= 1.0))
	return 1;
    else
	return 0;
}
