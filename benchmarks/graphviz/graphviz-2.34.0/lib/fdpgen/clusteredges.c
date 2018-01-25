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


/* clusteredges.c:
 * Written by Emden R. Gansner
 *
 * Code for handling spline edges around clusters.
 */

/* uses PRIVATE interface */
#define FDP_PRIVATE 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <clusteredges.h>
#include <fdp.h>
#include <neatoprocs.h>
#include "vispath.h"

typedef struct {
    int cnt;
    int sz;
    Ppoly_t **obs;
} objlist;

/* addObj:
 * Add an object to the list. The array is increased if necessary.
 */
#define INIT_SZ 100

#ifdef DEBUG
static void dumpObj(Ppoly_t * p)
{
    int j;
    Ppoint_t pt;
    for (j = 0; j < p->pn; j++) {
	pt = p->ps[j];
	fprintf(stderr, " %.5g %.5g", pt.x, pt.y);
    }
    fputs("\n", stderr);
}

static void dumpObjlist(objlist * l)
{
    int i;
    for (i = 0; i < l->cnt; i++) {
	dumpObj(l->obs[i]);
    }
}
#endif

static void addObj(objlist * l, Ppoly_t * obj)
{
    if (l->sz == l->cnt) {
	if (l->obs) {
	    l->sz *= 2;
	    l->obs = RALLOC(l->sz, l->obs, Ppoly_t *);
	} else {
	    l->obs = N_GNEW(INIT_SZ, Ppoly_t *);
	    l->sz = INIT_SZ;
	}
    }
    l->obs[l->cnt++] = obj;
}

/* freeObjlist:
 * Release memory.
 */
static void freeObjlist(objlist * l)
{
    if (l) {
	free(l->obs);
	free(l);
    }
}

/* resetObjlist:
 * Reset objlist so it can be reused, using
 * the same memory.
 */
static void resetObjlist(objlist * l)
{
    l->cnt = 0;
}

/* makeClustObs:
 * Create an obstacle corresponding to a cluster's bbox.
 */
static Ppoly_t *makeClustObs(graph_t * g, expand_t* pm)
{
    Ppoly_t *obs = NEW(Ppoly_t);
    boxf bb;
    boxf newbb;
    Ppoint_t ctr;

    bb = GD_bb(g);
    obs->pn = 4;
    obs->ps = N_NEW(4, Ppoint_t);

    ctr.x = (bb.UR.x + bb.LL.x) / 2.0;
    ctr.y = (bb.UR.y + bb.LL.y) / 2.0;

    if (pm->doAdd) {
	newbb.UR.x = bb.UR.x + pm->x;
	newbb.UR.y = bb.UR.y + pm->y;
	newbb.LL.x = bb.LL.x - pm->x;
	newbb.LL.y = bb.LL.y - pm->y;
    }
    else {
	double deltax = pm->x - 1.0;
	double deltay = pm->y - 1.0;
	newbb.UR.x = pm->x * bb.UR.x - deltax * ctr.x;
	newbb.UR.y = pm->y * bb.UR.y - deltay * ctr.y;
	newbb.LL.x = pm->x * bb.LL.x - deltax * ctr.x;
	newbb.LL.y = pm->y * bb.LL.y - deltay * ctr.y;
    }

    /* CW order */
    obs->ps[0].x = newbb.LL.x;
    obs->ps[0].y = newbb.LL.y;
    obs->ps[1].x = newbb.LL.x;
    obs->ps[1].y = newbb.UR.y;
    obs->ps[2].x = newbb.UR.x;
    obs->ps[2].y = newbb.UR.y;
    obs->ps[3].x = newbb.UR.x;
    obs->ps[3].y = newbb.LL.y;

    return obs;
}

/* addGraphObjs:
 * Add all top-level clusters and nodes with g as their smallest
 * containing graph to the list l.
 * Don't add any objects equal to tex or hex.
 * Return the list.
 */
static void
addGraphObjs(objlist * l, graph_t * g, void *tex, void *hex, expand_t* pm)
{
    node_t *n;
    graph_t *sg;
    int i;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if ((PARENT(n) == g) && (n != tex) && (n != hex)
	    && !IS_CLUST_NODE(n)) {
	    addObj(l, makeObstacle(n, pm));
	}
    }
    for (i = 1; i <= GD_n_cluster(g); i++) {
	sg = GD_clust(g)[i];
	if ((sg != tex) && (sg != hex)) {
	    addObj(l, makeClustObs(sg, pm));
	}
    }
}

/* raiseLevel:
 * Add barrier objects for node n, in graph *gp of level maxlvl, up to
 * level minlvl. 
 * Assume maxlvl > minlvl.
 * Return appended list, plus pass back last cluster processed in gp.
 */
static void
raiseLevel(objlist * l, int maxlvl, void *ex, int minlvl, graph_t ** gp,
	   expand_t* pm)
{
    graph_t *g = *gp;
    int i;

    for (i = maxlvl; i > minlvl; i--) {
	addGraphObjs(l, g, ex, NULL, pm);
	ex = g;
	g = GPARENT(g);
    }
    *gp = (graph_t *) ex;
}

/* objectList:
 * Create array of all objects (nodes and clusters) to be avoided
 * when routing edge e. Make sure it never adds the endpoints of the
 * edge, or any graph containing the endpoints.
 * Return the list.
 * Assume e is not a loop.
 */
static objlist *objectList(edge_t * ep, expand_t* pm)
{
    node_t *h = aghead(ep);
    node_t *t = agtail(ep);
    graph_t *hg = PARENT(h);
    graph_t *tg = PARENT(t);
    int hlevel;
    int tlevel;
    void *hex;			/* Objects to be excluded from list */
    void *tex;
    objlist *list = NEW(objlist);

    /* If either endpoint is a cluster node, we move up one level */
    if (IS_CLUST_NODE(h)) {
	hex = hg;
	hg = GPARENT(hg);
    } else
	hex = h;
    if (IS_CLUST_NODE(t)) {
	tex = tg;
	tg = GPARENT(tg);
    } else
	tex = t;

    hlevel = LEVEL(hg);
    tlevel = LEVEL(tg);
    if (hlevel > tlevel) {
	raiseLevel(list, hlevel, hex, tlevel, &hg, pm);
	hex = hg;
	hg = GPARENT(hg);
    } else if (tlevel > hlevel) {
	raiseLevel(list, tlevel, tex, hlevel, &tg, pm);
	tex = tg;
	tg = GPARENT(tg);
    }

    /* hg and tg always have the same level */
    while (hg != tg) {
	addGraphObjs(list, hg, NULL, hex, pm);
	addGraphObjs(list, tg, tex, NULL, pm);
	hex = hg;
	hg = GPARENT(hg);
	tex = tg;
	tg = GPARENT(tg);
    }
    addGraphObjs(list, tg, tex, hex, pm);

    return list;
}

/* compoundEdges:
 * Construct edges as splines, avoiding clusters when required.
 * We still don't implement spline multiedges, so we just copy
 * one spline to all the other edges.
 * Returns 0 on success. Failure indicates the obstacle configuration
 * for some edge had overlaps.
 */
int compoundEdges(graph_t * g, expand_t* pm, int edgetype)
{
    node_t *n;
    node_t *head;
    edge_t *e;
    edge_t *e0;
    objlist *objl = NULL;
    path *P = NULL;
    vconfig_t *vconfig;
    int rv = 0;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    head = aghead(e);
	    if ((n == head) && ED_count(e)) {	/* self arc */
		if (!P) {
		    P = NEW(path);
		    P->boxes = N_NEW(agnnodes(g) + 20 * 2 * 9, boxf);
		}
		makeSelfArcs(P, e, GD_nodesep(g));
	    } else if (ED_count(e)) {
		objl = objectList(e, pm);
		if (Plegal_arrangement(objl->obs, objl->cnt)) {
		    vconfig = Pobsopen(objl->obs, objl->cnt);
		    if (!vconfig) {
			agerr(AGWARN, "compoundEdges: could not construct obstacles - falling back to straight line edges\n");
			rv = 1;
			continue;
		    }
		}
		else {
		    if (Verbose)
			fprintf(stderr,
				"nodes touch - falling back to straight line edges\n");
		    rv = 1;
		    continue;
		}

		/* For efficiency, it should be possible to copy the spline
		 * from the first edge to the rest. However, one has to deal
		 * with change in direction, different arrowheads, labels, etc.
		 */
		for (e0 = e; e0; e0 = ED_to_virt(e0)) {
		    ED_path(e0) =
			getPath(e0, vconfig, 0, objl->obs, objl->cnt);
		    makeSpline(g, e0, objl->obs, objl->cnt, FALSE);
		}
		resetObjlist(objl);
	    }
	}
    }
    freeObjlist(objl);
    if (P) {
	free(P->boxes);
	free(P);
    }
    return rv;
}
