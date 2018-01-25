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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "neato.h"
#include "adjust.h"
#include "pathplan.h"
#include "vispath.h"
#include "multispline.h"
#ifndef HAVE_DRAND48
extern double drand48(void);
#endif

#ifdef ORTHO
#include <ortho.h>
#endif

extern void printvis(vconfig_t * cp);
extern int in_poly(Ppoly_t argpoly, Ppoint_t q);


static boolean spline_merge(node_t * n)
{
    return FALSE;
}

static boolean swap_ends_p(edge_t * e)
{
    return FALSE;
}

static splineInfo sinfo = { swap_ends_p, spline_merge };

static void
make_barriers(Ppoly_t ** poly, int npoly, int pp, int qp,
	      Pedge_t ** barriers, int *n_barriers)
{
    int i, j, k, n, b;
    Pedge_t *bar;

    n = 0;
    for (i = 0; i < npoly; i++) {
	if (i == pp)
	    continue;
	if (i == qp)
	    continue;
	n = n + poly[i]->pn;
    }
    bar = N_GNEW(n, Pedge_t);
    b = 0;
    for (i = 0; i < npoly; i++) {
	if (i == pp)
	    continue;
	if (i == qp)
	    continue;
	for (j = 0; j < poly[i]->pn; j++) {
	    k = j + 1;
	    if (k >= poly[i]->pn)
		k = 0;
	    bar[b].a = poly[i]->ps[j];
	    bar[b].b = poly[i]->ps[k];
	    b++;
	}
    }
    assert(b == n);
    *barriers = bar;
    *n_barriers = n;
}

/* genPt:
 */
static Ppoint_t genPt(double x, double y, pointf c)
{
    Ppoint_t p;

    p.x = x + c.x;
    p.y = y + c.y;
    return p;
}


/* recPt:
 */
static Ppoint_t recPt(double x, double y, pointf c, expand_t* m)
{
    Ppoint_t p;

    p.x = (x * m->x) + c.x;
    p.y = (y * m->y) + c.y;
    return p;
}

typedef struct {
    node_t *n1;
    pointf p1;
    node_t *n2;
    pointf p2;
} edgeinfo;
typedef struct {
    Dtlink_t link;
    edgeinfo id;
    edge_t *e;
} edgeitem;

static void *newitem(Dt_t * d, edgeitem * obj, Dtdisc_t * disc)
{
    edgeitem *newp;

    NOTUSED(disc);
    newp = NEW(edgeitem);
    newp->id = obj->id;
    newp->e = obj->e;
    ED_count(newp->e) = 1;

    return newp;
}

static void freeitem(Dt_t * d, edgeitem * obj, Dtdisc_t * disc)
{
    free(obj);
}

static int
cmpitems(Dt_t * d, edgeinfo * key1, edgeinfo * key2, Dtdisc_t * disc)
{
    int x;

    if (key1->n1 > key2->n1)
	return 1;
    if (key1->n1 < key2->n1)
	return -1;
    if (key1->n2 > key2->n2)
	return 1;
    if (key1->n2 < key2->n2)
	return -1;

    if ((x = key1->p1.x - key2->p1.x))
	return x;
    if ((x = key1->p1.y - key2->p1.y))
	return x;
    if ((x = key1->p2.x - key2->p2.x))
	return x;
    return (key1->p2.y - key2->p2.y);
}

Dtdisc_t edgeItemDisc = {
    offsetof(edgeitem, id),
    sizeof(edgeinfo),
    offsetof(edgeitem, link),
    (Dtmake_f) newitem,
    (Dtfree_f) freeitem,
    (Dtcompar_f) cmpitems,
    0,
    0,
    0
};

/* equivEdge:
 * See if we have already encountered an edge between the same
 * node:port pairs. If so, return the earlier edge. If not, 
 * this edge is added to map and returned.
 * We first have to canonicalize the key fields using a lexicographic
 * ordering.
 */
static edge_t *equivEdge(Dt_t * map, edge_t * e)
{
    edgeinfo test;
    edgeitem dummy;
    edgeitem *ip;

    if (agtail(e) < aghead(e)) {
	test.n1 = agtail(e);
	test.p1 = ED_tail_port(e).p;
	test.n2 = aghead(e);
	test.p2 = ED_head_port(e).p;
    } else if (agtail(e) > aghead(e)) {
	test.n2 = agtail(e);
	test.p2 = ED_tail_port(e).p;
	test.n1 = aghead(e);
	test.p1 = ED_head_port(e).p;
    } else {
	pointf hp = ED_head_port(e).p;
	pointf tp = ED_tail_port(e).p;
	if (tp.x < hp.x) {
	    test.p1 = tp;
	    test.p2 = hp;
	} else if (tp.x > hp.x) {
	    test.p1 = hp;
	    test.p2 = tp;
	} else if (tp.y < hp.y) {
	    test.p1 = tp;
	    test.p2 = hp;
	} else if (tp.y > hp.y) {
	    test.p1 = hp;
	    test.p2 = tp;
	} else {
	    test.p1 = test.p2 = tp;
	}
	test.n2 = test.n1 = agtail(e);
    }
    dummy.id = test;
    dummy.e = e;
    ip = dtinsert(map, &dummy);
    return ip->e;
}


/* makeSelfArcs:
 * Generate loops. We use the library routine makeSelfEdge
 * which also places the labels.
 * We have to handle port labels here.
 * as well as update the bbox from edge labels.
 */
void makeSelfArcs(path * P, edge_t * e, int stepx)
{
    int cnt = ED_count(e);

    if ((cnt == 1) || Concentrate) {
	edge_t *edges1[1];
	edges1[0] = e;
	makeSelfEdge(P, edges1, 0, 1, stepx, stepx, &sinfo);
	if (ED_label(e))
	    updateBB(agraphof(agtail(e)), ED_label(e));
	makePortLabels(e);
    } else {
	int i;
	edge_t **edges = N_GNEW(cnt, edge_t *);
	for (i = 0; i < cnt; i++) {
	    edges[i] = e;
	    e = ED_to_virt(e);
	}
	makeSelfEdge(P, edges, 0, cnt, stepx, stepx, &sinfo);
	for (i = 0; i < cnt; i++) {
	    e = edges[i];
	    if (ED_label(e))
		updateBB(agraphof(agtail(e)), ED_label(e));
	    makePortLabels(e);
	}
	free(edges);
    }
}

/* makeObstacle:
 * Given a node, return an obstacle reflecting the
 * node's geometry. pmargin specifies how much space to allow
 * around the polygon. 
 * Returns the constructed polygon on success, NULL on failure.
 * Failure means the node shape is not supported. 
 *
 * The polygon has its vertices in CW order.
 * 
 */
Ppoly_t *makeObstacle(node_t * n, expand_t* pmargin)
{
    Ppoly_t *obs;
    polygon_t *poly;
    double adj = 0.0;
    int j, sides;
    pointf polyp;
    boxf b;
    pointf pt;
    field_t *fld;
    epsf_t *desc;

    switch (shapeOf(n)) {
    case SH_POLY:
    case SH_POINT:
	obs = NEW(Ppoly_t);
	poly = (polygon_t *) ND_shape_info(n);
	if (poly->sides >= 3) {
	    sides = poly->sides;
	} else {		/* ellipse */
	    sides = 8;
	    adj = drand48() * .01;
	}
	obs->pn = sides;
	obs->ps = N_NEW(sides, Ppoint_t);
	/* assuming polys are in CCW order, and pathplan needs CW */
	for (j = 0; j < sides; j++) {
	    double xmargin = 0.0, ymargin = 0.0;
	    if (poly->sides >= 3) {
		if (pmargin->doAdd) {
		    if (poly->sides == 4) {
			switch (j) {
			case 0 :
			    xmargin = pmargin->x;
			    ymargin = pmargin->y;
			    break;
			case 1 :
			    xmargin = -pmargin->x;
			    ymargin = pmargin->y;
			    break;
			case 2 :
			    xmargin = -pmargin->x;
			    ymargin = -pmargin->y;
			    break;
			case 3 :
			    xmargin = pmargin->x;
			    ymargin = -pmargin->y;
			    break;
			}
			polyp.x = poly->vertices[j].x + xmargin;
			polyp.y = poly->vertices[j].y + ymargin;
		    }
		    else {
			double h = LEN(poly->vertices[j].x,poly->vertices[j].y);
			polyp.x = poly->vertices[j].x * (1.0 + pmargin->x/h);
			polyp.y = poly->vertices[j].y * (1.0 + pmargin->y/h);
		    }
		}
		else {
		    polyp.x = poly->vertices[j].x * pmargin->x;
		    polyp.y = poly->vertices[j].y * pmargin->y;
		}
	    } else {
		double c, s;
		c = cos(2.0 * M_PI * j / sides + adj);
		s = sin(2.0 * M_PI * j / sides + adj);
		if (pmargin->doAdd) {
		    polyp.x =  c*(ND_lw(n)+ND_rw(n)+pmargin->x) / 2.0;
		    polyp.y =  s*(ND_ht(n)+pmargin->y) / 2.0;
		}
		else {
		    polyp.x = pmargin->x * c * (ND_lw(n) + ND_rw(n)) / 2.0;
		    polyp.y = pmargin->y * s * ND_ht(n) / 2.0;
		}
	    }
	    obs->ps[sides - j - 1].x = polyp.x + ND_coord(n).x;
	    obs->ps[sides - j - 1].y = polyp.y + ND_coord(n).y;
	}
	break;
    case SH_RECORD:
	fld = (field_t *) ND_shape_info(n);
	b = fld->b;
	obs = NEW(Ppoly_t);
	obs->pn = 4;
	obs->ps = N_NEW(4, Ppoint_t);
	/* CW order */
	pt = ND_coord(n);
	if (pmargin->doAdd) {
	    obs->ps[0] = genPt(b.LL.x-pmargin->x, b.LL.y-pmargin->y, pt);
	    obs->ps[1] = genPt(b.LL.x-pmargin->x, b.UR.y+pmargin->y, pt);
	    obs->ps[2] = genPt(b.UR.x+pmargin->x, b.UR.y+pmargin->y, pt);
	    obs->ps[3] = genPt(b.UR.x+pmargin->x, b.LL.y-pmargin->y, pt);
	}
	else {
	    obs->ps[0] = recPt(b.LL.x, b.LL.y, pt, pmargin);
	    obs->ps[1] = recPt(b.LL.x, b.UR.y, pt, pmargin);
	    obs->ps[2] = recPt(b.UR.x, b.UR.y, pt, pmargin);
	    obs->ps[3] = recPt(b.UR.x, b.LL.y, pt, pmargin);
	}
	break;
    case SH_EPSF:
	desc = (epsf_t *) (ND_shape_info(n));
	obs = NEW(Ppoly_t);
	obs->pn = 4;
	obs->ps = N_NEW(4, Ppoint_t);
	/* CW order */
	pt = ND_coord(n);
	if (pmargin->doAdd) {
	    obs->ps[0] = genPt(-ND_lw(n)-pmargin->x, -ND_ht(n)-pmargin->y,pt);
	    obs->ps[1] = genPt(-ND_lw(n)-pmargin->x, ND_ht(n)+pmargin->y,pt);
	    obs->ps[2] = genPt(ND_rw(n)+pmargin->x, ND_ht(n)+pmargin->y,pt);
	    obs->ps[3] = genPt(ND_rw(n)+pmargin->x, -ND_ht(n)-pmargin->y,pt);
	}
	else {
	    obs->ps[0] = recPt(-ND_lw(n), -ND_ht(n), pt, pmargin);
	    obs->ps[1] = recPt(-ND_lw(n), ND_ht(n), pt, pmargin);
	    obs->ps[2] = recPt(ND_rw(n), ND_ht(n), pt, pmargin);
	    obs->ps[3] = recPt(ND_rw(n), -ND_ht(n), pt, pmargin);
	}
	break;
    default:
	obs = NULL;
	break;
    }
    return obs;
}

/* getPath
 * Construct the shortest path from one endpoint of e to the other.
 * The obstacles and their number are given by obs and npoly.
 * vconfig is a precomputed data structure to help in the computation.
 * If chkPts is true, the function finds the polygons, if any, containing
 * the endpoints and tells the shortest path computation to ignore them. 
 * Assumes this info is set in ND_lim, usually from _spline_edges.
 * Returns the shortest path.
 */
Ppolyline_t
getPath(edge_t * e, vconfig_t * vconfig, int chkPts, Ppoly_t ** obs,
	int npoly)
{
    Ppolyline_t line;
    int pp, qp;
    Ppoint_t p, q;

    p = add_pointf(ND_coord(agtail(e)), ED_tail_port(e).p);
    q = add_pointf(ND_coord(aghead(e)), ED_head_port(e).p);

    /* determine the polygons (if any) that contain the endpoints */
    pp = qp = POLYID_NONE;
    if (chkPts) {
	pp = ND_lim(agtail(e));
	qp = ND_lim(aghead(e));
/*
	for (i = 0; i < npoly; i++) {
	    if ((pp == POLYID_NONE) && in_poly(*obs[i], p))
		pp = i;
	    if ((qp == POLYID_NONE) && in_poly(*obs[i], q))
		qp = i;
	}
*/
    }
    Pobspath(vconfig, p, pp, q, qp, &line);
    return line;
}

/* makePolyline:
 */
static void
makePolyline(graph_t* g, edge_t * e)
{
    Ppolyline_t spl, line = ED_path(e);
    Ppoint_t p0, q0;

    p0 = line.ps[0];
    q0 = line.ps[line.pn - 1];
    make_polyline (line, &spl);
    if (Verbose > 1)
	fprintf(stderr, "polyline %s %s\n", agnameof(agtail(e)), agnameof(aghead(e)));
    clip_and_install(e, aghead(e), spl.ps, spl.pn, &sinfo);
    addEdgeLabels(g, e, p0, q0);
}

/* makeSpline:
 * Construct a spline connecting the endpoints of e, avoiding the npoly
 * obstacles obs.
 * The resultant spline is attached to the edge, the positions of any 
 * edge labels are computed, and the graph's bounding box is recomputed.
 * 
 * If chkPts is true, the function checks if one or both of the endpoints 
 * is on or inside one of the obstacles and, if so, tells the shortest path
 * computation to ignore them. 
 */
void makeSpline(graph_t* g, edge_t * e, Ppoly_t ** obs, int npoly, boolean chkPts)
{
    Ppolyline_t line, spline;
    Pvector_t slopes[2];
    int i, n_barriers;
    int pp, qp;
    Ppoint_t p, q;
    Pedge_t *barriers;

    line = ED_path(e);
    p = line.ps[0];
    q = line.ps[line.pn - 1];
    /* determine the polygons (if any) that contain the endpoints */
    pp = qp = POLYID_NONE;
    if (chkPts)
	for (i = 0; i < npoly; i++) {
	    if ((pp == POLYID_NONE) && in_poly(*obs[i], p))
		pp = i;
	    if ((qp == POLYID_NONE) && in_poly(*obs[i], q))
		qp = i;
	}

    make_barriers(obs, npoly, pp, qp, &barriers, &n_barriers);
    slopes[0].x = slopes[0].y = 0.0;
    slopes[1].x = slopes[1].y = 0.0;
    if (Proutespline(barriers, n_barriers, line, slopes, &spline) < 0) {
	agerr (AGERR, "makeSpline: failed to make spline edge (%s,%s)\n", agnameof(agtail(e)), agnameof(aghead(e)));
	return;
    }

    /* north why did you ever use int coords */
    if (Verbose > 1)
	fprintf(stderr, "spline %s %s\n", agnameof(agtail(e)), agnameof(aghead(e)));
    clip_and_install(e, aghead(e), spline.ps, spline.pn, &sinfo);
    free(barriers);
    addEdgeLabels(g, e, p, q);
}

  /* True if either head or tail has a port on its boundary */
#define BOUNDARY_PORT(e) ((ED_tail_port(e).side)||(ED_head_port(e).side))

/* _spline_edges:
 * Basic default routine for creating edges.
 * If splines are requested, we construct the obstacles.
 * If not, or nodes overlap, the function reverts to line segments.
 * NOTE: intra-cluster edges are not constrained to
 * remain in the cluster's bounding box and, conversely, a cluster's box
 * is not altered to reflect intra-cluster edges.
 * If Nop > 1 and the spline exists, it is just copied.
 * NOTE: if edgetype = ET_NONE, we shouldn't be here.
 */
static int _spline_edges(graph_t * g, expand_t* pmargin, int edgetype)
{
    node_t *n;
    edge_t *e;
    edge_t *e0;
    Ppoly_t **obs = 0;
    Ppoly_t *obp;
    int cnt, i = 0, npoly;
    vconfig_t *vconfig = 0;
    path *P = NULL;
    int useEdges = (Nop > 1);
    router_t* rtr = 0;
    int legal = 0;

    /* build configuration */
    if (edgetype >= ET_PLINE) {
	obs = N_NEW(agnnodes(g), Ppoly_t *);
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    obp = makeObstacle(n, pmargin);
	    if (obp) {
		ND_lim(n) = i; 
		obs[i++] = obp;
	    }
	    else
		ND_lim(n) = POLYID_NONE; 
	}
    } else {
	obs = 0;
    }
    npoly = i;
    if (obs) {
	if ((legal = Plegal_arrangement(obs, npoly))) {
	    if (edgetype != ET_ORTHO) vconfig = Pobsopen(obs, npoly);
	}
	else if (Verbose)
	    fprintf(stderr,
		"nodes touch - falling back to straight line edges\n");
    }

    /* route edges  */
    if (Verbose)
	fprintf(stderr, "Creating edges using %s\n",
	    (legal && (edgetype == ET_ORTHO)) ? "orthogonal lines" :
	    (vconfig ? (edgetype == ET_SPLINE ? "splines" : "polylines") : 
		"line segments"));
    if (vconfig) {
	/* path-finding pass */
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
		ED_path(e) = getPath(e, vconfig, TRUE, obs, npoly);
	    }
	}
    }
#ifdef ORTHO
    else if (legal && (edgetype == ET_ORTHO)) {
	orthoEdges (g, 0);
	useEdges = 1;
    }
#endif

    /* spline-drawing pass */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
/* fprintf (stderr, "%s -- %s %d\n", agnameof(agtail(e)), agnameof(aghead(e)), ED_count(e)); */
	    node_t *head = aghead(e);
	    if (useEdges && ED_spl(e)) {
		addEdgeLabels(g, e,
			      add_pointf(ND_coord(n), ED_tail_port(e).p),
			      add_pointf(ND_coord(head), ED_head_port(e).p));
	    } 
	    else if (ED_count(e) == 0) continue;  /* only do representative */
	    else if (n == head) {    /* self arc */
		if (!P) {
		    P = NEW(path);
		    P->boxes = N_NEW(agnnodes(g) + 20 * 2 * 9, boxf);
		}
		makeSelfArcs(P, e, GD_nodesep(g->root));
	    } else if (vconfig) { /* ET_SPLINE or ET_PLINE */
#ifdef HAVE_GTS
		if ((ED_count(e) > 1) || BOUNDARY_PORT(e)) {
		    int fail = 0;
		    if ((ED_path(e).pn == 2) && !BOUNDARY_PORT(e))
			     /* if a straight line can connect the ends */
			makeStraightEdge(g, e, edgetype, &sinfo);
		    else { 
			if (!rtr) rtr = mkRouter (obs, npoly);
			fail = makeMultiSpline(g, e, rtr, edgetype == ET_PLINE);
		    } 
		    if (!fail) continue;
		}
		/* We can probably remove this branch and just use
		 * makeMultiSpline. It can also catch the makeStraightEdge
		 * case. We could then eliminate all of the vconfig stuff.
		 */
#endif
		cnt = ED_count(e);
		if (Concentrate) cnt = 1; /* only do representative */
		e0 = e;
		for (i = 0; i < cnt; i++) {
		    if (edgetype == ET_SPLINE)
			makeSpline(g, e0, obs, npoly, TRUE);
		    else
			makePolyline(g, e0);
		    e0 = ED_to_virt(e0);
		}
	    } else {
		makeStraightEdge(g, e, edgetype, &sinfo);
	    }
	}
    }

#ifdef HAVE_GTS
    if (rtr)
	freeRouter (rtr);
#endif

    if (vconfig)
	Pobsclose (vconfig);
    if (P) {
	free(P->boxes);
	free(P);
    }
    if (obs) {
	for (i=0; i < npoly; i++)
	    free (obs[i]);
	free (obs);
    }
    return 0;
}

/* splineEdges:
 * Main wrapper code for generating edges.
 * Sets desired separation. 
 * Coalesces equivalent edges (edges * with the same endpoints). 
 * It then calls the edge generating function, and marks the
 * spline phase complete.
 * Returns 0 on success.
 *
 * The edge function is given the graph, the separation to be added
 * around obstacles, and the type of edge. It must guarantee 
 * that all bounding boxes are current; in particular, the bounding box of 
 * g must reflect the addition of the edges.
 */
int
splineEdges(graph_t * g, int (*edgefn) (graph_t *, expand_t*, int),
	    int edgetype)
{
    node_t *n;
    edge_t *e;
    expand_t margin;
    Dt_t *map;

    margin = esepFactor (g);

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    resolvePorts (e);
	}
    }

    /* find equivalent edges */
    map = dtopen(&edgeItemDisc, Dtoset);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    edge_t *leader = equivEdge(map, e);
	    if (leader != e) {
		ED_count(leader)++;
		ED_to_virt(e) = ED_to_virt(leader);
		ED_to_virt(leader) = e;
	    }
	}
    }
    dtclose(map);

    if (edgefn(g, &margin, edgetype))
	return 1;

    State = GVSPLINES;
    return 0;
}

/* spline_edges1:
 * Construct edges using default algorithm and given splines value.
 * Return 0 on success.
 */
int spline_edges1(graph_t * g, int edgetype)
{
    return splineEdges(g, _spline_edges, edgetype);
}

/* spline_edges0:
 * Sets the graph's aspect ratio.
 * Check splines attribute and construct edges using default algorithm.
 * If the splines attribute is defined but equal to "", skip edge routing.
 * 
 * Assumes u.bb for has been computed for g and all clusters
 * (not just top-level clusters), and  that GD_bb(g).LL is at the origin.
 *
 * This last criterion is, I believe, mainly to simplify the code
 * in neato_set_aspect. It would be good to remove this constraint,
 * as this would allow nodes pinned on input to have the same coordinates
 * when output in dot or plain format.
 *
 */
void spline_edges0(graph_t * g)
{
    int et = EDGE_TYPE (g);
    neato_set_aspect(g);
    if (et == ET_NONE) return;
#ifndef ORTHO
    if (et == ET_ORTHO) {
	agerr (AGWARN, "Orthogonal edges not yet supported\n");
	et = ET_PLINE; 
	GD_flags(g->root) &= ~ET_ORTHO;
	GD_flags(g->root) |= ET_PLINE;
    }
#endif
    spline_edges1(g, et);
}

/* shiftClusters:
 */
static void
shiftClusters (graph_t * g, pointf offset)
{
    int i;

    for (i = 1; i <= GD_n_cluster(g); i++) {
	shiftClusters (GD_clust(g)[i], offset);
    }

    GD_bb(g).UR.x -= offset.x;
    GD_bb(g).UR.y -= offset.y;
    GD_bb(g).LL.x -= offset.x;
    GD_bb(g).LL.y -= offset.y;
}

/* spline_edges:
 * Compute bounding box, translate graph to origin,
 * then construct all edges.
 */
void spline_edges(graph_t * g)
{
    node_t *n;
    pointf offset;

    compute_bb(g);
    offset.x = PS2INCH(GD_bb(g).LL.x);
    offset.y = PS2INCH(GD_bb(g).LL.y);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	ND_pos(n)[0] -= offset.x;
	ND_pos(n)[1] -= offset.y;
    }
	
    shiftClusters (g, GD_bb(g).LL);
    spline_edges0(g);
}

/* scaleEdge:
 * Scale edge by given factor.
 * Assume ED_spl != NULL.
 */
static void scaleEdge(edge_t * e, double xf, double yf)
{
    int i, j;
    pointf *pt;
    bezier *bez;
    pointf delh, delt;

    delh.x = POINTS_PER_INCH * (ND_pos(aghead(e))[0] * (xf - 1.0));
    delh.y = POINTS_PER_INCH * (ND_pos(aghead(e))[1] * (yf - 1.0));
    delt.x = POINTS_PER_INCH * (ND_pos(agtail(e))[0] * (xf - 1.0));
    delt.y = POINTS_PER_INCH * (ND_pos(agtail(e))[1] * (yf - 1.0));

    bez = ED_spl(e)->list;
    for (i = 0; i < ED_spl(e)->size; i++) {
	pt = bez->list;
	for (j = 0; j < bez->size; j++) {
	    if ((i == 0) && (j == 0)) {
		pt->x += delt.x;
		pt->y += delt.y;
	    }
	    else if ((i == ED_spl(e)->size-1) && (j == bez->size-1)) {
		pt->x += delh.x;
		pt->y += delh.y;
	    }
	    else {
		pt->x *= xf;
		pt->y *= yf;
	    }
	    pt++;
	}
	if (bez->sflag) {
	    bez->sp.x += delt.x;
	    bez->sp.y += delt.y;
	}
	if (bez->eflag) {
	    bez->ep.x += delh.x;
	    bez->ep.y += delh.y;
	}
	bez++;
    }

    if (ED_label(e) && ED_label(e)->set) {
	ED_label(e)->pos.x *= xf;
	ED_label(e)->pos.y *= yf;
    }
    if (ED_head_label(e) && ED_head_label(e)->set) {
	ED_head_label(e)->pos.x += delh.x;
	ED_head_label(e)->pos.y += delh.y;
    }
    if (ED_tail_label(e) && ED_tail_label(e)->set) {
	ED_tail_label(e)->pos.x += delt.x;
	ED_tail_label(e)->pos.y += delt.y;
    }
}

/* scaleBB:
 * Scale bounding box of clusters of g by given factors.
 */
static void scaleBB(graph_t * g, double xf, double yf)
{
    int i;

    GD_bb(g).UR.x *= xf;
    GD_bb(g).UR.y *= yf;
    GD_bb(g).LL.x *= xf;
    GD_bb(g).LL.y *= yf;

    if (GD_label(g) && GD_label(g)->set) {
	GD_label(g)->pos.x *= xf;
	GD_label(g)->pos.y *= yf;
    }

    for (i = 1; i <= GD_n_cluster(g); i++)
	scaleBB(GD_clust(g)[i], xf, yf);
}

/* _neato_set_aspect;
 * Assume all bounding boxes are correct and
 * that GD_bb(g).LL is at origin.
 * Also assume g is the root graph
 */
static void _neato_set_aspect(graph_t * g)
{
    /* int          i; */
    double xf, yf, actual, desired;
    node_t *n;

    /* compute_bb(g); */
    if (GD_drawing(g)->ratio_kind) {
	/* normalize */
	assert(ROUND(GD_bb(g).LL.x) == 0);
	assert(ROUND(GD_bb(g).LL.y) == 0);
	if (GD_flip(g)) {
	    double t = GD_bb(g).UR.x;
	    GD_bb(g).UR.x = GD_bb(g).UR.y;
	    GD_bb(g).UR.y = t;
	}
	if (GD_drawing(g)->ratio_kind == R_FILL) {
	    /* fill is weird because both X and Y can stretch */
	    if (GD_drawing(g)->size.x <= 0)
		return;
	    xf = (double) GD_drawing(g)->size.x / GD_bb(g).UR.x;
	    yf = (double) GD_drawing(g)->size.y / GD_bb(g).UR.y;
	    /* handle case where one or more dimensions is too big */
	    if ((xf < 1.0) || (yf < 1.0)) {
		if (xf < yf) {
		    yf = yf / xf;
		    xf = 1.0;
		} else {
		    xf = xf / yf;
		    yf = 1.0;
		}
	    }
	} else if (GD_drawing(g)->ratio_kind == R_EXPAND) {
	    if (GD_drawing(g)->size.x <= 0)
		return;
	    xf = (double) GD_drawing(g)->size.x / GD_bb(g).UR.x;
	    yf = (double) GD_drawing(g)->size.y / GD_bb(g).UR.y;
	    if ((xf > 1.0) && (yf > 1.0)) {
		double scale = MIN(xf, yf);
		xf = yf = scale;
	    } else
		return;
	} else if (GD_drawing(g)->ratio_kind == R_VALUE) {
	    desired = GD_drawing(g)->ratio;
	    actual = (GD_bb(g).UR.y) / (GD_bb(g).UR.x);
	    if (actual < desired) {
		yf = desired / actual;
		xf = 1.0;
	    } else {
		xf = actual / desired;
		yf = 1.0;
	    }
	} else
	    return;
	if (GD_flip(g)) {
	    double t = xf;
	    xf = yf;
	    yf = t;
	}

	if (Nop > 1) {
	    edge_t *e;
	    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
		for (e = agfstout(g, n); e; e = agnxtout(g, e))
		    if (ED_spl(e))
			scaleEdge(e, xf, yf);
	    }
	}
	/* Not relying on neato_nlist here allows us not to have to 
	 * allocate it in the root graph and the connected components. 
	 */
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    ND_pos(n)[0] = ND_pos(n)[0] * xf;
	    ND_pos(n)[1] = ND_pos(n)[1] * yf;
	}
	scaleBB(g, xf, yf);
    }
}

/* neato_set_aspect:
 * Sets aspect ratio if necessary; real work done in _neato_set_aspect;
 * This also copies the internal layout coordinates (ND_pos) to the 
 * external ones (ND_coord).
 */
void neato_set_aspect(graph_t * g)
{
    node_t *n;

	/* setting aspect ratio only makes sense on root graph */
    if (g->root == g)
	_neato_set_aspect(g);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	ND_coord(n).x = POINTS_PER_INCH * (ND_pos(n)[0]);
	ND_coord(n).y = POINTS_PER_INCH * (ND_pos(n)[1]);
    }
}

