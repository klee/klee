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

/* tlayout.c:
 * Written by Emden R. Gansner
 *
 * Module for initial layout, using point nodes and ports.
 * 
 * Note: If interior nodes are not connected, they tend to fly apart,
 * despite being tied to port nodes. This occurs because, as initially
 * coded, as the nodes tend to straighten into a line, the radius
 * grows causing more expansion. Is the problem really here and not
 * with disconnected nodes in xlayout? If here, we can either forbid
 * expansion or eliminate repulsion between nodes only connected
 * via port nodes.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* uses PRIVATE interface */
#define FDP_PRIVATE 1

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <time.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <ctype.h>
#include <dbg.h>
#include <grid.h>
#include <neato.h>

#ifndef HAVE_SRAND48
#define srand48 srand
#endif
#ifndef HAVE_DRAND48
extern double drand48(void);
#endif

#include "tlayout.h"
#include "globals.h"

#define D_useGrid   (fdp_parms->useGrid)
#define D_useNew    (fdp_parms->useNew)
#define D_numIters  (fdp_parms->numIters)
#define D_unscaled  (fdp_parms->unscaled)
#define D_C         (fdp_parms->C)
#define D_Tfact     (fdp_parms->Tfact)
#define D_K         (fdp_parms->K)
#define D_T0        (fdp_parms->T0)

  /* Actual parameters used; initialized using fdp_parms, then possibly
   * updated with graph-specific values.
   */
typedef struct {
    int useGrid;	/* use grid for speed up */
    int useNew;		/* encode x-K into attractive force */
    long seed;		/* seed for position RNG */
    int numIters;	/* actual iterations in layout */
    int maxIters;	/* max iterations in layout */
    int unscaled;	/* % of iterations used in pass 1 */
    double C;		/* Repulsion factor in xLayout */
    double Tfact;	/* scale temp from default expression */
    double K;		/* spring constant; ideal distance */
    double T0;          /* initial temperature */
    int smode;          /* seed mode */
    double Cell;	/* grid cell size */
    double Cell2;	/* Cell*Cell */
    double K2;		/* K*K */
    double Wd;		/* half-width of boundary */
    double Ht;		/* half-height of boundary */
    double Wd2;		/* Wd*Wd */
    double Ht2;		/* Ht*Ht */
    int pass1;		/* iterations used in pass 1 */
    int loopcnt;        /* actual iterations in this pass */
} parms_t;

static parms_t parms;

#define T_useGrid   (parms.useGrid)
#define T_useNew    (parms.useNew)
#define T_seed      (parms.seed)
#define T_numIters  (parms.numIters)
#define T_maxIters  (parms.maxIters)
#define T_unscaled  (parms.unscaled)
#define T_C         (parms.C)
#define T_Tfact     (parms.Tfact)
#define T_K         (parms.K)
#define T_T0        (parms.T0)
#define T_smode     (parms.smode)
#define T_Cell      (parms.Cell)
#define T_Cell2     (parms.Cell2)
#define T_K2        (parms.K2)
#define T_Wd        (parms.Wd)
#define T_Ht        (parms.Ht)
#define T_Wd2       (parms.Wd2)
#define T_Ht2       (parms.Ht2)
#define T_pass1     (parms.pass1)
#define T_loopcnt   (parms.loopcnt)

#define EXPFACTOR  1.2
#define DFLT_maxIters 600
#define DFLT_K  0.3
#define DFLT_Cell  0.0
#define DFLT_seed  1
#define DFLT_smode INIT_RANDOM

static double cool(double temp, int t)
{
    return (T_T0 * (T_maxIters - t)) / T_maxIters;
}

/* reset_params:
 */
static void reset_params(void)
{
    T_T0 = -1.0;
}

/* init_params:
 * Set parameters for expansion phase based on initial
 * layout parameters. If T0 is not set, we set it here
 * based on the size of the graph. In this case, we
 * return 1, so that fdp_tLayout can unset T0, to be
 * reset by a recursive call to fdp_tLayout. 
 */
static int init_params(graph_t * g, xparams * xpms)
{
    int ret = 0;

    if (T_T0 == -1.0) {
	int nnodes = agnnodes(g);

	T_T0 = T_Tfact * T_K * sqrt(nnodes) / 5;
#ifdef DEBUG
	if (Verbose) {
	    prIndent();
	    fprintf(stderr, "tlayout %s", agnameof(g));
	    fprintf(stderr, "(%s) : T0 %f\n", agnameof(GORIG(g->root)), T_T0);
	}
#endif
	ret = 1;
    }

    xpms->T0 = cool(T_T0, T_pass1);
    xpms->K = T_K;
    xpms->C = T_C;
    xpms->numIters = T_maxIters - T_pass1;

    if (T_numIters >= 0) {
	if (T_numIters <= T_pass1) {
	    T_loopcnt = T_numIters;
	    xpms->loopcnt = 0;
	} else if (T_numIters <= T_maxIters) {
	    T_loopcnt = T_pass1;
	    xpms->loopcnt = T_numIters - T_pass1;
	}
    } else {
	T_loopcnt = T_pass1;
	xpms->loopcnt = xpms->numIters;
    }
    return ret;
}

/* fdp_initParams:
 * Initialize parameters based on root graph attributes.
 */
void fdp_initParams(graph_t * g)
{
    T_useGrid = D_useGrid;
    T_useNew = D_useNew;
    T_numIters = D_numIters;
    T_unscaled = D_unscaled;
    T_Cell = DFLT_Cell;
    T_C = D_C;
    T_Tfact = D_Tfact;
#ifndef WITH_CGRAPH
    T_maxIters = late_int(g, agfindattr(g, "maxiter"), DFLT_maxIters, 0);
    D_K = T_K = late_double(g, agfindattr(g, "K"), DFLT_K, 0.0);
#else /* WITH_CGRAPH */
    T_maxIters = late_int(g, agattr(g,AGRAPH, "maxiter", NULL), DFLT_maxIters, 0);
    D_K = T_K = late_double(g, agattr(g,AGRAPH, "K", NULL), DFLT_K, 0.0);
#endif /* WITH_CGRAPH */
    if (D_T0 == -1.0) {
#ifndef WITH_CGRAPH
	T_T0 = late_double(g, agfindattr(g, "T0"), -1.0, 0.0);
#else /* WITH_CGRAPH */
	T_T0 = late_double(g, agattr(g,AGRAPH, "T0", NULL), -1.0, 0.0);
#endif /* WITH_CGRAPH */
    } else
	T_T0 = D_T0;
    T_seed = DFLT_seed;
    T_smode = setSeed (g, DFLT_smode, &T_seed);
    if (T_smode == INIT_SELF) {
	agerr(AGWARN, "fdp does not support start=self - ignoring\n");
	T_seed = DFLT_smode;
    }

    T_pass1 = (T_unscaled * T_maxIters) / 100;
    T_K2 = T_K * T_K;

    if (T_useGrid) {
	if (T_Cell <= 0.0)
	    T_Cell = 3 * T_K;
	T_Cell2 = T_Cell * T_Cell;
    }
#ifdef DEBUG
    if (Verbose) {
	prIndent();
	fprintf(stderr,
		"Params %s : K %f T0 %f Tfact %f maxIters %d unscaled %d\n",
		agnameof(g),
                T_K, T_T0, T_Tfact, T_maxIters, T_unscaled);
    }
#endif
}

static void
doRep(node_t * p, node_t * q, double xdelta, double ydelta, double dist2)
{
    double force;
    double dist;

    while (dist2 == 0.0) {
	xdelta = 5 - rand() % 10;
	ydelta = 5 - rand() % 10;
	dist2 = xdelta * xdelta + ydelta * ydelta;
    }
    if (T_useNew) {
	dist = sqrt(dist2);
	force = T_K2 / (dist * dist2);
    } else
	force = T_K2 / dist2;
    if (IS_PORT(p) && IS_PORT(q))
	force *= 10.0;
    DISP(q)[0] += xdelta * force;
    DISP(q)[1] += ydelta * force;
    DISP(p)[0] -= xdelta * force;
    DISP(p)[1] -= ydelta * force;
}

/* applyRep:
 * Repulsive force = (K*K)/d
 *  or K*K/d*d
 */
static void applyRep(Agnode_t * p, Agnode_t * q)
{
    double xdelta, ydelta;

    xdelta = ND_pos(q)[0] - ND_pos(p)[0];
    ydelta = ND_pos(q)[1] - ND_pos(p)[1];
    doRep(p, q, xdelta, ydelta, xdelta * xdelta + ydelta * ydelta);
}

static void doNeighbor(Grid * grid, int i, int j, node_list * nodes)
{
    cell *cellp = findGrid(grid, i, j);
    node_list *qs;
    Agnode_t *p;
    Agnode_t *q;
    double xdelta, ydelta;
    double dist2;

    if (cellp) {
#ifdef DEBUG
	if (Verbose >= 3) {
	    prIndent();
	    fprintf(stderr, "  doNeighbor (%d,%d) : %d\n", i, j,
		    gLength(cellp));
	}
#endif
	for (; nodes != 0; nodes = nodes->next) {
	    p = nodes->node;
	    for (qs = cellp->nodes; qs != 0; qs = qs->next) {
		q = qs->node;
		xdelta = (ND_pos(q))[0] - (ND_pos(p))[0];
		ydelta = (ND_pos(q))[1] - (ND_pos(p))[1];
		dist2 = xdelta * xdelta + ydelta * ydelta;
		if (dist2 < T_Cell2)
		    doRep(p, q, xdelta, ydelta, dist2);
	    }
	}
    }
}

static int gridRepulse(Dt_t * dt, cell * cellp, Grid * grid)
{
    node_list *nodes = cellp->nodes;
    int i = cellp->p.i;
    int j = cellp->p.j;
    node_list *p;
    node_list *q;

    NOTUSED(dt);
#ifdef DEBUG
    if (Verbose >= 3) {
	prIndent();
	fprintf(stderr, "gridRepulse (%d,%d) : %d\n", i, j,
		gLength(cellp));
    }
#endif
    for (p = nodes; p != 0; p = p->next) {
	for (q = nodes; q != 0; q = q->next)
	    if (p != q)
		applyRep(p->node, q->node);
    }

    doNeighbor(grid, i - 1, j - 1, nodes);
    doNeighbor(grid, i - 1, j, nodes);
    doNeighbor(grid, i - 1, j + 1, nodes);
    doNeighbor(grid, i, j - 1, nodes);
    doNeighbor(grid, i, j + 1, nodes);
    doNeighbor(grid, i + 1, j - 1, nodes);
    doNeighbor(grid, i + 1, j, nodes);
    doNeighbor(grid, i + 1, j + 1, nodes);

    return 0;
}

/* applyAttr:
 * Attractive force = weight*(d*d)/K
 *  or        force = (d - L(e))*weight(e)
 */
static void applyAttr(Agnode_t * p, Agnode_t * q, Agedge_t * e)
{
    double xdelta, ydelta;
    double force;
    double dist;
    double dist2;

    xdelta = ND_pos(q)[0] - ND_pos(p)[0];
    ydelta = ND_pos(q)[1] - ND_pos(p)[1];
    dist2 = xdelta * xdelta + ydelta * ydelta;
    while (dist2 == 0.0) {
	xdelta = 5 - rand() % 10;
	ydelta = 5 - rand() % 10;
	dist2 = xdelta * xdelta + ydelta * ydelta;
    }
    dist = sqrt(dist2);
    if (T_useNew)
	force = (ED_factor(e) * (dist - ED_dist(e))) / dist;
    else
	force = (ED_factor(e) * dist) / ED_dist(e);
    DISP(q)[0] -= xdelta * force;
    DISP(q)[1] -= ydelta * force;
    DISP(p)[0] += xdelta * force;
    DISP(p)[1] += ydelta * force;
}

static void updatePos(Agraph_t * g, double temp, bport_t * pp)
{
    Agnode_t *n;
    double temp2;
    double len2;
    double x, y, d;
    double dx, dy;

    temp2 = temp * temp;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (ND_pinned(n) & P_FIX)
	    continue;
	dx = DISP(n)[0];
	dy = DISP(n)[1];
	len2 = dx * dx + dy * dy;

	/* limit by temperature */
	if (len2 < temp2) {
	    x = ND_pos(n)[0] + dx;
	    y = ND_pos(n)[1] + dy;
	} else {
	    double fact = temp / (sqrt(len2));
	    x = ND_pos(n)[0] + dx * fact;
	    y = ND_pos(n)[1] + dy * fact;
	}

	/* if ports, limit by boundary */
	if (pp) {
	    d = sqrt((x * x) / T_Wd2 + (y * y) / T_Ht2);
	    if (IS_PORT(n)) {
		ND_pos(n)[0] = x / d;
		ND_pos(n)[1] = y / d;
	    } else if (d >= 1.0) {
		ND_pos(n)[0] = 0.95 * x / d;
		ND_pos(n)[1] = 0.95 * y / d;
	    } else {
		ND_pos(n)[0] = x;
		ND_pos(n)[1] = y;
	    }
	} else {
	    ND_pos(n)[0] = x;
	    ND_pos(n)[1] = y;
	}
    }
}

#define FLOOR(d) ((int)floor(d))

/* gAdjust:
 */
static void gAdjust(Agraph_t * g, double temp, bport_t * pp, Grid * grid)
{
    Agnode_t *n;
    Agedge_t *e;

    if (temp <= 0.0)
	return;

    clearGrid(grid);

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	DISP(n)[0] = DISP(n)[1] = 0;
	addGrid(grid, FLOOR((ND_pos(n))[0] / T_Cell), FLOOR((ND_pos(n))[1] / T_Cell),
		n);
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e))
	    if (n != aghead(e))
		applyAttr(n, aghead(e), e);
    }
    walkGrid(grid, gridRepulse);


    updatePos(g, temp, pp);
}

/* adjust:
 */
static void adjust(Agraph_t * g, double temp, bport_t * pp)
{
    Agnode_t *n;
    Agnode_t *n1;
    Agedge_t *e;

    if (temp <= 0.0)
	return;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	DISP(n)[0] = DISP(n)[1] = 0;
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (n1 = agnxtnode(g, n); n1; n1 = agnxtnode(g, n1)) {
	    applyRep(n, n1);
	}
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (n != aghead(e))
		applyAttr(n, aghead(e), e);
	}
    }

    updatePos(g, temp, pp);
}

/* initPositions:
 * Create initial layout of nodes
 * TODO :
 *  Position nodes near neighbors with positions.
 *  Use bbox to reset K.
 */
static pointf initPositions(graph_t * g, bport_t * pp)
{
    int nG = agnnodes(g) - NPORTS(g);
    double size;
    Agnode_t *np;
    int n_pos = 0;		/* no. of nodes with position info */
    box bb = { {0, 0}, {0, 0} };
    pointf ctr;			/* center of boundary ellipse */
    long local_seed;
    double PItimes2 = M_PI * 2.0;

    for (np = agfstnode(g); np; np = agnxtnode(g, np)) {
	if (ND_pinned(np)) {
	    if (n_pos) {
		bb.LL.x = MIN(ND_pos(np)[0], bb.LL.x);
		bb.LL.y = MIN(ND_pos(np)[1], bb.LL.y);
		bb.UR.x = MAX(ND_pos(np)[0], bb.UR.x);
		bb.UR.y = MAX(ND_pos(np)[1], bb.UR.y);
	    } else {
		bb.UR.x = bb.LL.x = ND_pos(np)[0];
		bb.UR.y = bb.LL.y = ND_pos(np)[1];
	    }
	    n_pos++;
	}
    }

    size = T_K * (sqrt((double) nG) + 1.0);
    T_Wd = T_Ht = EXPFACTOR * (size / 2.0);
    if (n_pos == 1) {
	ctr.x = bb.LL.x;
	ctr.y = bb.LL.y;
    } else if (n_pos > 1) {
	double alpha, area, width, height, quot;
	ctr.x = (bb.LL.x + bb.UR.x) / 2.0;
	ctr.y = (bb.LL.y + bb.UR.y) / 2.0;
	width = EXPFACTOR * (bb.UR.x - bb.LL.x);
	height = EXPFACTOR * (bb.UR.y - bb.LL.y);
	area = 4.0 * T_Wd * T_Ht;
	quot = (width * height) / area;
	if (quot >= 1.0) {	/* If bbox has large enough area, use it */
	    T_Wd = width / 2.0;
	    T_Ht = height / 2.0;
	} else if (quot > 0.0) {	/* else scale up to have enough area */
	    quot = 2.0 * sqrt(quot);
	    T_Wd = width / quot;
	    T_Ht = height / quot;
	} else {		/* either width or height is 0 */
	    if (width > 0) {
		height = area / width;
		T_Wd = width / 2.0;
		T_Ht = height / 2.0;
	    } else if (height > 0) {
		width = area / height;
		T_Wd = width / 2.0;
		T_Ht = height / 2.0;
	    }
	    /* If width = height = 0, use Wd and Ht as defined above for
	     * the case the n_pos == 0.
	     */
	}

	/* Construct enclosing ellipse */
	alpha = atan2(T_Ht, T_Wd);
	T_Wd = T_Wd / cos(alpha);
	T_Ht = T_Ht / sin(alpha);
    } else {
	ctr.x = ctr.y = 0;
    }
    T_Wd2 = T_Wd * T_Wd;
    T_Ht2 = T_Ht * T_Ht;

    /* Set seed value */
    if (T_smode == INIT_RANDOM)
	local_seed = T_seed;
    else {
#if defined(MSWIN32) || defined(WIN32)
	local_seed = time(NULL);
#else
	local_seed = getpid() ^ time(NULL);
#endif
    }
    srand48(local_seed);

    /* If ports, place ports on and nodes within an ellipse centered at origin
     * with halfwidth Wd and halfheight Ht. 
     * If no ports, place nodes within a rectangle centered at origin
     * with halfwidth Wd and halfheight Ht. Nodes with a given position
     * are translated. Wd and Ht are set to contain all positioned points.
     * The reverse translation will be applied to all
     * nodes at the end of the layout.
     * TODO: place unfixed points using adjacent ports or fixed pts.
     */
    if (pp) {
/* fprintf (stderr, "initPos %s ctr (%g,%g) Wd %g Ht %g\n", g->name, ctr.x, ctr.y, Wd, Ht); */
	while (pp->e) {		/* position ports on ellipse */
	    np = pp->n;
	    ND_pos(np)[0] = T_Wd * cos(pp->alpha) + ctr.x;
	    ND_pos(np)[1] = T_Ht * sin(pp->alpha) + ctr.y;
	    ND_pinned(np) = P_SET;
/* fprintf (stderr, "%s pt (%g,%g) %g\n", np->name, ND_pos(np)[0], ND_pos(np)[1], pp->alpha); */
	    pp++;
	}
	for (np = agfstnode(g); np; np = agnxtnode(g, np)) {
	    if (IS_PORT(np))
		continue;
	    if (ND_pinned(np)) {
		ND_pos(np)[0] -= ctr.x;
		ND_pos(np)[1] -= ctr.y;
	    } else {
		pointf p = { 0.0, 0.0 };
		int cnt = 0;
		node_t *op;
		edge_t *ep;
		for (ep = agfstedge(g, np); ep; ep = agnxtedge(g, ep, np)) {
		    if (aghead(ep) == agtail(ep))
			continue;
		    op = (aghead(ep) == np ? agtail(ep) : aghead(ep));
		    if (!hasPos(op))
			continue;
		    if (cnt) {
			p.x = (p.x * cnt + ND_pos(op)[0]) / (cnt + 1);
			p.y = (p.y * cnt + ND_pos(op)[1]) / (cnt + 1);
		    } else {
			p.x = ND_pos(op)[0];
			p.y = ND_pos(op)[1];
		    }
		    cnt++;
		}
		if (cnt > 1) {
		    ND_pos(np)[0] = p.x;
		    ND_pos(np)[1] = p.y;
/* fprintf (stderr, "%s 1 (%g,%g)\n", np->name, p.x, p.y); */
		} else if (cnt == 1) {
		    ND_pos(np)[0] = 0.98 * p.x + 0.1 * ctr.x;
		    ND_pos(np)[1] = 0.9 * p.y + 0.1 * ctr.y;
/* fprintf (stderr, "%s %d (%g,%g)\n", np->name, cnt, ND_pos(np)[0], ND_pos(np)[1]); */
		} else {
		    double angle = PItimes2 * drand48();
		    double radius = 0.9 * drand48();
		    ND_pos(np)[0] = radius * T_Wd * cos(angle);
		    ND_pos(np)[1] = radius * T_Ht * sin(angle);
/* fprintf (stderr, "%s 0 (%g,%g)\n", np->name, ND_pos(np)[0], ND_pos(np)[1]); */
		}
		ND_pinned(np) = P_SET;
	    }
	}
    } else {
	if (n_pos) {		/* If positioned nodes */
	    for (np = agfstnode(g); np; np = agnxtnode(g, np)) {
		if (ND_pinned(np)) {
		    ND_pos(np)[0] -= ctr.x;
		    ND_pos(np)[1] -= ctr.y;
		} else {
		    ND_pos(np)[0] = T_Wd * (2.0 * drand48() - 1.0);
		    ND_pos(np)[1] = T_Ht * (2.0 * drand48() - 1.0);
		}
	    }
	} else {		/* No ports or positions; place randomly */
	    for (np = agfstnode(g); np; np = agnxtnode(g, np)) {
		ND_pos(np)[0] = T_Wd * (2.0 * drand48() - 1.0);
		ND_pos(np)[1] = T_Ht * (2.0 * drand48() - 1.0);
	    }
	}
    }

    return ctr;
}

void dumpstat(graph_t * g)
{
    double dx, dy;
    double l, max2 = 0.0;
    node_t *np;
    edge_t *ep;
    for (np = agfstnode(g); np; np = agnxtnode(g, np)) {
	dx = DISP(np)[0];
	dy = DISP(np)[1];
	l = dx * dx + dy * dy;
	if (l > max2)
	    max2 = l;
	fprintf(stderr, "%s: (%f,%f) (%f,%f)\n", agnameof(np),
		ND_pos(np)[0], ND_pos(np)[1], DISP(np)[0], DISP(np)[1]);
    }
    fprintf(stderr, "max delta = %f\n", sqrt(max2));
    for (np = agfstnode(g); np; np = agnxtnode(g, np)) {
	for (ep = agfstout(g, np); ep; ep = agnxtout(g, ep)) {
	    dx = ND_pos(np)[0] - ND_pos(aghead(ep))[0];
	    dy = ND_pos(np)[1] - ND_pos(aghead(ep))[1];
	    fprintf(stderr, "  %s --  %s  (%f)\n", agnameof(np),
		    agnameof(aghead(ep)), sqrt(dx * dx + dy * dy));
	}
    }
}

/* fdp_tLayout:
 * Given graph g with ports nodes, layout g respecting ports.
 * If some node have position information, it may be useful to
 * reset temperature and other parameters to reflect this.
 */
void fdp_tLayout(graph_t * g, xparams * xpms)
{
    int i;
    int reset;
    bport_t *pp = PORTS(g);
    double temp;
    Grid *grid;
    pointf ctr;
    Agnode_t *n;

    reset = init_params(g, xpms);
    temp = T_T0;

    ctr = initPositions(g, pp);

    if (T_useGrid) {
	grid = mkGrid(agnnodes(g));
	adjustGrid(grid, agnnodes(g));
	for (i = 0; i < T_loopcnt; i++) {
	    temp = cool(temp, i);
	    gAdjust(g, temp, pp, grid);
	}
	delGrid(grid);
    } else {
	for (i = 0; i < T_loopcnt; i++) {
	    temp = cool(temp, i);
	    adjust(g, temp, pp);
	}
    }

    if ((ctr.x != 0.0) || (ctr.y != 0.0)) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    ND_pos(n)[0] += ctr.x;
	    ND_pos(n)[1] += ctr.y;
	}
    }
/* dumpstat (g); */
    if (reset)
	reset_params();
}
