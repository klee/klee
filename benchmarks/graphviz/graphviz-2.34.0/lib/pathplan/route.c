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


#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <math.h>
#include "pathutil.h"
#include "solvers.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define EPSILON1 1E-3
#define EPSILON2 1E-6

#define ABS(a) ((a) >= 0 ? (a) : -(a))

typedef struct tna_t {
    double t;
    Ppoint_t a[2];
} tna_t;

#define prerror(msg) \
        fprintf (stderr, "libpath/%s:%d: %s\n", __FILE__, __LINE__, (msg))

#define DISTSQ(a, b) ( \
    (((a).x - (b).x) * ((a).x - (b).x)) + (((a).y - (b).y) * ((a).y - (b).y)) \
)

#define POINTSIZE sizeof (Ppoint_t)

#define LT(pa, pbp) ((pa.y > pbp->y) || ((pa.y == pbp->y) && (pa.x < pbp->x)))
#define GT(pa, pbp) ((pa.y < pbp->y) || ((pa.y == pbp->y) && (pa.x > pbp->x)))

typedef struct p2e_t {
    Ppoint_t *pp;
    Pedge_t *ep;
} p2e_t;

typedef struct elist_t {
    Pedge_t *ep;
    struct elist_t *next, *prev;
} elist_t;

static jmp_buf jbuf;

#if 0
static p2e_t *p2es;
static int p2en;
#endif

#if 0
static elist_t *elist;
#endif

static Ppoint_t *ops;
static int opn, opl;

static int reallyroutespline(Pedge_t *, int,
			     Ppoint_t *, int, Ppoint_t, Ppoint_t);
static int mkspline(Ppoint_t *, int, tna_t *, Ppoint_t, Ppoint_t,
		    Ppoint_t *, Ppoint_t *, Ppoint_t *, Ppoint_t *);
static int splinefits(Pedge_t *, int, Ppoint_t, Pvector_t, Ppoint_t,
		      Pvector_t, Ppoint_t *, int);
static int splineisinside(Pedge_t *, int, Ppoint_t *);
static int splineintersectsline(Ppoint_t *, Ppoint_t *, double *);
static void points2coeff(double, double, double, double, double *);
static void addroot(double, double *, int *);

static Pvector_t normv(Pvector_t);

static void growops(int);

static Ppoint_t add(Ppoint_t, Ppoint_t);
static Ppoint_t sub(Ppoint_t, Ppoint_t);
static double dist(Ppoint_t, Ppoint_t);
static Ppoint_t scale(Ppoint_t, double);
static double dot(Ppoint_t, Ppoint_t);
static double B0(double t);
static double B1(double t);
static double B2(double t);
static double B3(double t);
static double B01(double t);
static double B23(double t);
#if 0
static int cmpp2efunc(const void *, const void *);

static void listdelete(Pedge_t *);
static void listreplace(Pedge_t *, Pedge_t *);
static void listinsert(Pedge_t *, Ppoint_t);
#endif

/* Proutespline:
 * Given a set of edgen line segments edges as obstacles, a template
 * path input, and endpoint vectors evs, construct a spline fitting the
 * input and endpoing vectors, and return in output.
 * Return 0 on success and -1 on failure, including no memory.
 */
int Proutespline(Pedge_t * edges, int edgen, Ppolyline_t input,
		 Ppoint_t * evs, Ppolyline_t * output)
{
#if 0
    Ppoint_t p0, p1, p2, p3;
    Ppoint_t *pp;
    Pvector_t v1, v2, v12, v23;
    int ipi, opi;
    int ei, p2ei;
    Pedge_t *e0p, *e1p;
#endif
    Ppoint_t *inps;
    int inpn;

    /* unpack into previous format rather than modify legacy code */
    inps = input.ps;
    inpn = input.pn;

#if 0
    if (!(p2es = (p2e_t *) malloc(sizeof(p2e_t) * (p2en = edgen * 2)))) {
	prerror("cannot malloc p2es");
	return -1;
    }
    for (ei = 0, p2ei = 0; ei < edgen; ei++) {
	if (edges[ei].a.x == edges[ei].b.x
	    && edges[ei].a.y == edges[ei].b.y)
	    continue;
	p2es[p2ei].pp = &edges[ei].a;
	p2es[p2ei++].ep = &edges[ei];
	p2es[p2ei].pp = &edges[ei].b;
	p2es[p2ei++].ep = &edges[ei];
    }
    p2en = p2ei;
    qsort(p2es, p2en, sizeof(p2e_t), cmpp2efunc);
    elist = NULL;
    for (p2ei = 0; p2ei < p2en; p2ei += 2) {
	pp = p2es[p2ei].pp;
#if DEBUG >= 1
	fprintf(stderr, "point: %d %lf %lf\n", p2ei, pp->x, pp->y);
#endif
	e0p = p2es[p2ei].ep;
	e1p = p2es[p2ei + 1].ep;
	p0 = (&e0p->a == p2es[p2ei].pp) ? e0p->b : e0p->a;
	p1 = (&e0p->a == p2es[p2ei + 1].pp) ? e1p->b : e1p->a;
	if (LT(p0, pp) && LT(p1, pp)) {
	    listdelete(e0p), listdelete(e1p);
	} else if (GT(p0, pp) && GT(p1, pp)) {
	    listinsert(e0p, *pp), listinsert(e1p, *pp);
	} else {
	    if (LT(p0, pp))
		listreplace(e0p, e1p);
	    else
		listreplace(e1p, e0p);
	}
    }
#endif
    if (setjmp(jbuf))
	return -1;

    /* generate the splines */
    evs[0] = normv(evs[0]);
    evs[1] = normv(evs[1]);
    opl = 0;
    growops(4);
    ops[opl++] = inps[0];
    if (reallyroutespline(edges, edgen, inps, inpn, evs[0], evs[1]) == -1)
	return -1;
    output->pn = opl;
    output->ps = ops;

#if 0
    fprintf(stderr, "edge\na\nb\n");
    fprintf(stderr, "points\n%d\n", inpn);
    for (ipi = 0; ipi < inpn; ipi++)
	fprintf(stderr, "%f %f\n", inps[ipi].x, inps[ipi].y);
    fprintf(stderr, "splpoints\n%d\n", opl);
    for (opi = 0; opi < opl; opi++)
	fprintf(stderr, "%f %f\n", ops[opi].x, ops[opi].y);
#endif

    return 0;
}

static int reallyroutespline(Pedge_t * edges, int edgen,
			     Ppoint_t * inps, int inpn, Ppoint_t ev0,
			     Ppoint_t ev1)
{
    Ppoint_t p1, p2, cp1, cp2, p;
    Pvector_t v1, v2, splitv, splitv1, splitv2;
    double maxd, d, t;
    int maxi, i, spliti;

    static tna_t *tnas;
    static int tnan;

    if (tnan < inpn) {
	if (!tnas) {
	    if (!(tnas = malloc(sizeof(tna_t) * inpn)))
		return -1;
	} else {
	    if (!(tnas = realloc(tnas, sizeof(tna_t) * inpn)))
		return -1;
	}
	tnan = inpn;
    }
    tnas[0].t = 0;
    for (i = 1; i < inpn; i++)
	tnas[i].t = tnas[i - 1].t + dist(inps[i], inps[i - 1]);
    for (i = 1; i < inpn; i++)
	tnas[i].t /= tnas[inpn - 1].t;
    for (i = 0; i < inpn; i++) {
	tnas[i].a[0] = scale(ev0, B1(tnas[i].t));
	tnas[i].a[1] = scale(ev1, B2(tnas[i].t));
    }
    if (mkspline(inps, inpn, tnas, ev0, ev1, &p1, &v1, &p2, &v2) == -1)
	return -1;
    if (splinefits(edges, edgen, p1, v1, p2, v2, inps, inpn))
	return 0;
    cp1 = add(p1, scale(v1, 1 / 3.0));
    cp2 = sub(p2, scale(v2, 1 / 3.0));
    for (maxd = -1, maxi = -1, i = 1; i < inpn - 1; i++) {
	t = tnas[i].t;
	p.x = B0(t) * p1.x + B1(t) * cp1.x + B2(t) * cp2.x + B3(t) * p2.x;
	p.y = B0(t) * p1.y + B1(t) * cp1.y + B2(t) * cp2.y + B3(t) * p2.y;
	if ((d = dist(p, inps[i])) > maxd)
	    maxd = d, maxi = i;
    }
    spliti = maxi;
    splitv1 = normv(sub(inps[spliti], inps[spliti - 1]));
    splitv2 = normv(sub(inps[spliti + 1], inps[spliti]));
    splitv = normv(add(splitv1, splitv2));
    reallyroutespline(edges, edgen, inps, spliti + 1, ev0, splitv);
    reallyroutespline(edges, edgen, &inps[spliti], inpn - spliti, splitv,
		      ev1);
    return 0;
}

static int mkspline(Ppoint_t * inps, int inpn, tna_t * tnas, Ppoint_t ev0,
		    Ppoint_t ev1, Ppoint_t * sp0, Ppoint_t * sv0,
		    Ppoint_t * sp1, Ppoint_t * sv1)
{
    Ppoint_t tmp;
    double c[2][2], x[2], det01, det0X, detX1;
    double d01, scale0, scale3;
    int i;

    scale0 = scale3 = 0.0;
    c[0][0] = c[0][1] = c[1][0] = c[1][1] = 0.0;
    x[0] = x[1] = 0.0;
    for (i = 0; i < inpn; i++) {
	c[0][0] += dot(tnas[i].a[0], tnas[i].a[0]);
	c[0][1] += dot(tnas[i].a[0], tnas[i].a[1]);
	c[1][0] = c[0][1];
	c[1][1] += dot(tnas[i].a[1], tnas[i].a[1]);
	tmp = sub(inps[i], add(scale(inps[0], B01(tnas[i].t)),
			       scale(inps[inpn - 1], B23(tnas[i].t))));
	x[0] += dot(tnas[i].a[0], tmp);
	x[1] += dot(tnas[i].a[1], tmp);
    }
    det01 = c[0][0] * c[1][1] - c[1][0] * c[0][1];
    det0X = c[0][0] * x[1] - c[0][1] * x[0];
    detX1 = x[0] * c[1][1] - x[1] * c[0][1];
    if (ABS(det01) >= 1e-6) {
	scale0 = detX1 / det01;
	scale3 = det0X / det01;
    }
    if (ABS(det01) < 1e-6 || scale0 <= 0.0 || scale3 <= 0.0) {
	d01 = dist(inps[0], inps[inpn - 1]) / 3.0;
	scale0 = d01;
	scale3 = d01;
    }
    *sp0 = inps[0];
    *sv0 = scale(ev0, scale0);
    *sp1 = inps[inpn - 1];
    *sv1 = scale(ev1, scale3);
    return 0;
}

static double dist_n(Ppoint_t * p, int n)
{
    int i;
    double rv;

    rv = 0.0;
    for (i = 1; i < n; i++) {
	rv +=
	    sqrt((p[i].x - p[i - 1].x) * (p[i].x - p[i - 1].x) +
		 (p[i].y - p[i - 1].y) * (p[i].y - p[i - 1].y));
    }
    return rv;
}

static int splinefits(Pedge_t * edges, int edgen, Ppoint_t pa,
		      Pvector_t va, Ppoint_t pb, Pvector_t vb,
		      Ppoint_t * inps, int inpn)
{
    Ppoint_t sps[4];
    double a, b;
#if 0
    double d;
#endif
    int pi;
    int forceflag;
    int first = 1;

    forceflag = (inpn == 2 ? 1 : 0);

#if 0
    d = sqrt((pb.x - pa.x) * (pb.x - pa.x) +
	     (pb.y - pa.y) * (pb.y - pa.y));
    a = d, b = d;
#else
    a = b = 4;
#endif
    for (;;) {
	sps[0].x = pa.x;
	sps[0].y = pa.y;
	sps[1].x = pa.x + a * va.x / 3.0;
	sps[1].y = pa.y + a * va.y / 3.0;
	sps[2].x = pb.x - b * vb.x / 3.0;
	sps[2].y = pb.y - b * vb.y / 3.0;
	sps[3].x = pb.x;
	sps[3].y = pb.y;

	/* shortcuts (paths shorter than the shortest path) not allowed -
	 * they must be outside the constraint polygon.  this can happen
	 * if the candidate spline intersects the constraint polygon exactly
	 * on sides or vertices.  maybe this could be more elegant, but
	 * it solves the immediate problem. we could also try jittering the
	 * constraint polygon, or computing the candidate spline more carefully,
	 * for example using the path. SCN */

	if (first && (dist_n(sps, 4) < (dist_n(inps, inpn) - EPSILON1)))
	    return 0;
	first = 0;

	if (splineisinside(edges, edgen, &sps[0])) {
	    growops(opl + 4);
	    for (pi = 1; pi < 4; pi++)
		ops[opl].x = sps[pi].x, ops[opl++].y = sps[pi].y;
#if DEBUG >= 1
	    fprintf(stderr, "success: %f %f\n", a, b);
#endif
	    return 1;
	}
	if (a == 0 && b == 0) {
	    if (forceflag) {
		growops(opl + 4);
		for (pi = 1; pi < 4; pi++)
		    ops[opl].x = sps[pi].x, ops[opl++].y = sps[pi].y;
#if DEBUG >= 1
		fprintf(stderr, "forced straight line: %f %f\n", a, b);
#endif
		return 1;
	    }
	    break;
	}
	if (a > .01)
	    a /= 2, b /= 2;
	else
	    a = b = 0;
    }
#if DEBUG >= 1
    fprintf(stderr, "failure\n");
#endif
    return 0;
}

static int splineisinside(Pedge_t * edges, int edgen, Ppoint_t * sps)
{
    double roots[4];
    int rooti, rootn;
    int ei;
    Ppoint_t lps[2], ip;
    double t, ta, tb, tc, td;

    for (ei = 0; ei < edgen; ei++) {
	lps[0] = edges[ei].a, lps[1] = edges[ei].b;
	/* if ((rootn = splineintersectsline (sps, lps, roots)) == 4)
	   return 1; */
	if ((rootn = splineintersectsline(sps, lps, roots)) == 4)
	    continue;
	for (rooti = 0; rooti < rootn; rooti++) {
	    if (roots[rooti] < EPSILON2 || roots[rooti] > 1 - EPSILON2)
		continue;
	    t = roots[rooti];
	    td = t * t * t;
	    tc = 3 * t * t * (1 - t);
	    tb = 3 * t * (1 - t) * (1 - t);
	    ta = (1 - t) * (1 - t) * (1 - t);
	    ip.x = ta * sps[0].x + tb * sps[1].x +
		tc * sps[2].x + td * sps[3].x;
	    ip.y = ta * sps[0].y + tb * sps[1].y +
		tc * sps[2].y + td * sps[3].y;
	    if (DISTSQ(ip, lps[0]) < EPSILON1 ||
		DISTSQ(ip, lps[1]) < EPSILON1)
		continue;
	    return 0;
	}
    }
    return 1;
}

static int splineintersectsline(Ppoint_t * sps, Ppoint_t * lps,
				double *roots)
{
    double scoeff[4], xcoeff[2], ycoeff[2];
    double xroots[3], yroots[3], tv, sv, rat;
    int rootn, xrootn, yrootn, i, j;

    xcoeff[0] = lps[0].x;
    xcoeff[1] = lps[1].x - lps[0].x;
    ycoeff[0] = lps[0].y;
    ycoeff[1] = lps[1].y - lps[0].y;
    rootn = 0;
    if (xcoeff[1] == 0) {
	if (ycoeff[1] == 0) {
	    points2coeff(sps[0].x, sps[1].x, sps[2].x, sps[3].x, scoeff);
	    scoeff[0] -= xcoeff[0];
	    xrootn = solve3(scoeff, xroots);
	    points2coeff(sps[0].y, sps[1].y, sps[2].y, sps[3].y, scoeff);
	    scoeff[0] -= ycoeff[0];
	    yrootn = solve3(scoeff, yroots);
	    if (xrootn == 4)
		if (yrootn == 4)
		    return 4;
		else
		    for (j = 0; j < yrootn; j++)
			addroot(yroots[j], roots, &rootn);
	    else if (yrootn == 4)
		for (i = 0; i < xrootn; i++)
		    addroot(xroots[i], roots, &rootn);
	    else
		for (i = 0; i < xrootn; i++)
		    for (j = 0; j < yrootn; j++)
			if (xroots[i] == yroots[j])
			    addroot(xroots[i], roots, &rootn);
	    return rootn;
	} else {
	    points2coeff(sps[0].x, sps[1].x, sps[2].x, sps[3].x, scoeff);
	    scoeff[0] -= xcoeff[0];
	    xrootn = solve3(scoeff, xroots);
	    if (xrootn == 4)
		return 4;
	    for (i = 0; i < xrootn; i++) {
		tv = xroots[i];
		if (tv >= 0 && tv <= 1) {
		    points2coeff(sps[0].y, sps[1].y, sps[2].y, sps[3].y,
				 scoeff);
		    sv = scoeff[0] + tv * (scoeff[1] + tv *
					   (scoeff[2] + tv * scoeff[3]));
		    sv = (sv - ycoeff[0]) / ycoeff[1];
		    if ((0 <= sv) && (sv <= 1))
			addroot(tv, roots, &rootn);
		}
	    }
	    return rootn;
	}
    } else {
	rat = ycoeff[1] / xcoeff[1];
	points2coeff(sps[0].y - rat * sps[0].x, sps[1].y - rat * sps[1].x,
		     sps[2].y - rat * sps[2].x, sps[3].y - rat * sps[3].x,
		     scoeff);
	scoeff[0] += rat * xcoeff[0] - ycoeff[0];
	xrootn = solve3(scoeff, xroots);
	if (xrootn == 4)
	    return 4;
	for (i = 0; i < xrootn; i++) {
	    tv = xroots[i];
	    if (tv >= 0 && tv <= 1) {
		points2coeff(sps[0].x, sps[1].x, sps[2].x, sps[3].x,
			     scoeff);
		sv = scoeff[0] + tv * (scoeff[1] +
				       tv * (scoeff[2] + tv * scoeff[3]));
		sv = (sv - xcoeff[0]) / xcoeff[1];
		if ((0 <= sv) && (sv <= 1))
		    addroot(tv, roots, &rootn);
	    }
	}
	return rootn;
    }
}

static void points2coeff(double v0, double v1, double v2, double v3,
			 double *coeff)
{
    coeff[3] = v3 + 3 * v1 - (v0 + 3 * v2);
    coeff[2] = 3 * v0 + 3 * v2 - 6 * v1;
    coeff[1] = 3 * (v1 - v0);
    coeff[0] = v0;
}

static void addroot(double root, double *roots, int *rootnp)
{
    if (root >= 0 && root <= 1)
	roots[*rootnp] = root, (*rootnp)++;
}

static Pvector_t normv(Pvector_t v)
{
    double d;

    d = v.x * v.x + v.y * v.y;
    if (d > 1e-6) {
	d = sqrt(d);
	v.x /= d, v.y /= d;
    }
    return v;
}

static void growops(int newopn)
{
    if (newopn <= opn)
	return;
    if (!ops) {
	if (!(ops = (Ppoint_t *) malloc(POINTSIZE * newopn))) {
	    prerror("cannot malloc ops");
	    longjmp(jbuf,1);
	}
    } else {
	if (!(ops = (Ppoint_t *) realloc((void *) ops,
					 POINTSIZE * newopn))) {
	    prerror("cannot realloc ops");
	    longjmp(jbuf,1);
	}
    }
    opn = newopn;
}

static Ppoint_t add(Ppoint_t p1, Ppoint_t p2)
{
    p1.x += p2.x, p1.y += p2.y;
    return p1;
}

static Ppoint_t sub(Ppoint_t p1, Ppoint_t p2)
{
    p1.x -= p2.x, p1.y -= p2.y;
    return p1;
}

static double dist(Ppoint_t p1, Ppoint_t p2)
{
    double dx, dy;

    dx = p2.x - p1.x, dy = p2.y - p1.y;
    return sqrt(dx * dx + dy * dy);
}

static Ppoint_t scale(Ppoint_t p, double c)
{
    p.x *= c, p.y *= c;
    return p;
}

static double dot(Ppoint_t p1, Ppoint_t p2)
{
    return p1.x * p2.x + p1.y * p2.y;
}

static double B0(double t)
{
    double tmp = 1.0 - t;
    return tmp * tmp * tmp;
}

static double B1(double t)
{
    double tmp = 1.0 - t;
    return 3 * t * tmp * tmp;
}

static double B2(double t)
{
    double tmp = 1.0 - t;
    return 3 * t * t * tmp;
}

static double B3(double t)
{
    return t * t * t;
}

static double B01(double t)
{
    double tmp = 1.0 - t;
    return tmp * tmp * (tmp + 3 * t);
}

static double B23(double t)
{
    double tmp = 1.0 - t;
    return t * t * (3 * tmp + t);
}

#if 0
static int cmpp2efunc(const void *v0p, const void *v1p)
{
    p2e_t *p2e0p, *p2e1p;
    double x0, x1;

    p2e0p = (p2e_t *) v0p, p2e1p = (p2e_t *) v1p;
    if (p2e0p->pp->y > p2e1p->pp->y)
	return -1;
    else if (p2e0p->pp->y < p2e1p->pp->y)
	return 1;
    if (p2e0p->pp->x < p2e1p->pp->x)
	return -1;
    else if (p2e0p->pp->x > p2e1p->pp->x)
	return 1;
    x0 = (p2e0p->pp == &p2e0p->ep->a) ? p2e0p->ep->b.x : p2e0p->ep->a.x;
    x1 = (p2e1p->pp == &p2e1p->ep->a) ? p2e1p->ep->b.x : p2e1p->ep->a.x;
    if (x0 < x1)
	return -1;
    else if (x0 > x1)
	return 1;
    return 0;
}

static void listdelete(Pedge_t * ep)
{
    elist_t *lp;

    for (lp = elist; lp; lp = lp->next) {
	if (lp->ep != ep)
	    continue;
	if (lp->prev)
	    lp->prev->next = lp->next;
	if (lp->next)
	    lp->next->prev = lp->prev;
	if (elist == lp)
	    elist = lp->next;
	free(lp);
	return;
    }
    if (!lp) {
	prerror("cannot find list element to delete");
	abort();
    }
}

static void listreplace(Pedge_t * oldep, Pedge_t * newep)
{
    elist_t *lp;

    for (lp = elist; lp; lp = lp->next) {
	if (lp->ep != oldep)
	    continue;
	lp->ep = newep;
	return;
    }
    if (!lp) {
	prerror("cannot find list element to replace");
	abort();
    }
}

static void listinsert(Pedge_t * ep, Ppoint_t p)
{
    elist_t *lp, *newlp, *lastlp;
    double lx;

    if (!(newlp = (elist_t *) malloc(sizeof(elist_t)))) {
	prerror("cannot malloc newlp");
	abort();
    }
    newlp->ep = ep;
    newlp->next = newlp->prev = NULL;
    if (!elist) {
	elist = newlp;
	return;
    }
    for (lp = elist; lp; lp = lp->next) {
	lastlp = lp;
	lx = lp->ep->a.x + (lp->ep->b.x - lp->ep->a.x) * (p.y -
							  lp->ep->a.y) /
	    (lp->ep->b.y - lp->ep->a.y);
	if (lx <= p.x)
	    continue;
	if (lp->prev)
	    lp->prev->next = newlp;
	newlp->prev = lp->prev;
	newlp->next = lp;
	lp->prev = newlp;
	if (elist == lp)
	    elist = newlp;
	return;
    }
    lastlp->next = newlp;
    newlp->prev = lastlp;
    if (!elist)
	elist = newlp;
}
#endif
