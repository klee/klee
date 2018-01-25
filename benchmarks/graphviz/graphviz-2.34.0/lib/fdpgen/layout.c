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


/* layout.c:
 * Written by Emden R. Gansner
 *
 * This module provides the main bookkeeping for the fdp layout.
 * In particular, it handles the recursion and the creation of
 * ports and auxiliary graphs.
 * 
 * TODO : can we use ports to aid in layout of edges? Note that
 * at present, they are deleted.
 *
 *   Can we delay all repositioning of nodes until evalPositions, so
 * finalCC only sets the bounding boxes?
 *
 * Make sure multiple edges have an effect.
 */

/* uses PRIVATE interface */
#define FDP_PRIVATE 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
#ifdef HAVE_VALUES_H
#include <values.h>
#endif
#endif
#include <assert.h>
#include "tlayout.h"
#include "neatoprocs.h"
#include "adjust.h"
#include "comp.h"
#include "pack.h"
#include "clusteredges.h"
#include "dbg.h"
#include <setjmp.h>

static jmp_buf jbuf;

typedef struct {
    graph_t*  rootg;  /* logical root; graph passed in to fdp_layout */
    attrsym_t *G_coord;
    attrsym_t *G_width;
    attrsym_t *G_height;
    int gid;
    pack_info pack;
} layout_info;

typedef struct {
    edge_t *e;
    double alpha;
    double dist2;
} erec;

#define NEW_EDGE(e) (ED_to_virt(e) == 0)

/* finalCC:
 * Set graph bounding box given list of connected
 * components, each with its bounding box set.
 * If c_cnt > 1, then pts != NULL and gives translations for components.
 * Add margin about whole graph unless isRoot is true.
 * Reposition nodes based on final position of
 * node's connected component.
 * Also, entire layout is translated to origin.
 */
static void
finalCC(graph_t * g, int c_cnt, graph_t ** cc, point * pts, graph_t * rg,
	layout_info* infop)
{
    attrsym_t * G_width = infop->G_width;
    attrsym_t * G_height = infop->G_height;
    graph_t *cg;
    box b, bb;
    boxf bbf;
    point pt;
    int margin;
    graph_t **cp = cc;
    point *pp = pts;
    int isRoot = (rg == infop->rootg);
    int isEmpty = 0;

    /* compute graph bounding box in points */
    if (c_cnt) {
	cg = *cp++;
	BF2B(GD_bb(cg), bb);
	if (c_cnt > 1) {
	    pt = *pp++;
	    bb.LL.x += pt.x;
	    bb.LL.y += pt.y;
	    bb.UR.x += pt.x;
	    bb.UR.y += pt.y;
	    while ((cg = *cp++)) {
		BF2B(GD_bb(cg), b);
		pt = *pp++;
		b.LL.x += pt.x;
		b.LL.y += pt.y;
		b.UR.x += pt.x;
		b.UR.y += pt.y;
		bb.LL.x = MIN(bb.LL.x, b.LL.x);
		bb.LL.y = MIN(bb.LL.y, b.LL.y);
		bb.UR.x = MAX(bb.UR.x, b.UR.x);
		bb.UR.y = MAX(bb.UR.y, b.UR.y);
	    }
	}
    } else {			/* empty graph */
	bb.LL.x = 0;
	bb.LL.y = 0;
	bb.UR.x = late_int(rg, G_width, POINTS(DEFAULT_NODEWIDTH), 3);
	bb.UR.y = late_int(rg, G_height, POINTS(DEFAULT_NODEHEIGHT), 3);
	isEmpty = 1;
    }

    if (GD_label(rg)) {
	point p;
	int d;

	isEmpty = 0;
	PF2P(GD_label(rg)->dimen, p);
	d = p.x - (bb.UR.x - bb.LL.x);
	if (d > 0) {		/* height of label added below */
	    d /= 2;
	    bb.LL.x -= d;
	    bb.UR.x += d;
	}
    }

    if (isRoot || isEmpty)
	margin = 0;
    else
	margin = late_int (g, G_margin, CL_OFFSET, 0);
    pt.x = -bb.LL.x + margin;
    pt.y = -bb.LL.y + margin + GD_border(rg)[BOTTOM_IX].y;
    bb.LL.x = 0;
    bb.LL.y = 0;
    bb.UR.x += pt.x + margin;
    bb.UR.y += pt.y + margin + GD_border(rg)[TOP_IX].y;

    /* translate nodes */
    if (c_cnt) {
	cp = cc;
	pp = pts;
	while ((cg = *cp++)) {
	    point p;
	    node_t *n;
	    pointf del;

	    if (pp) {
		p = *pp++;
		p.x += pt.x;
		p.y += pt.y;
	    } else {
		p = pt;
	    }
	    del.x = PS2INCH(p.x);
	    del.y = PS2INCH(p.y);
	    for (n = agfstnode(cg); n; n = agnxtnode(cg, n)) {
		ND_pos(n)[0] += del.x;
		ND_pos(n)[1] += del.y;
	    }
	}
    }

    bbf.LL.x = PS2INCH(bb.LL.x);
    bbf.LL.y = PS2INCH(bb.LL.y);
    bbf.UR.x = PS2INCH(bb.UR.x);
    bbf.UR.y = PS2INCH(bb.UR.y);
    BB(g) = bbf;

}

/* mkDeriveNode:
 * Constructor for a node in a derived graph.
 * Allocates dndata.
 */
static node_t *mkDeriveNode(graph_t * dg, char *name)
{
    node_t *dn;

#ifndef WITH_CGRAPH
    dn = agnode(dg, name);
#else /* WITH_CGRAPH */
    dn = agnode(dg, name,1);
    agbindrec(dn, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
    ND_alg(dn) = (void *) NEW(dndata);	/* free in freeDeriveNode */
    ND_pos(dn) = N_GNEW(GD_ndim(dg), double);
    /* fprintf (stderr, "Creating %s\n", dn->name); */
    return dn;
}

static void freeDeriveNode(node_t * n)
{
    free(ND_alg(n));
    free(ND_pos(n));
#ifdef WITH_CGRAPH
    agdelrec(n, "Agnodeinfo_t");
#endif
}

static void freeGData(graph_t * g)
{
    free(GD_alg(g));
}

static void freeDerivedGraph(graph_t * g, graph_t ** cc)
{
    graph_t *cg;
    node_t *dn;
    node_t *dnxt;
    edge_t *e;

    while ((cg = *cc++)) {
	freeGData(cg);
#ifdef WITH_CGRAPH
	agdelrec(cg, "Agraphinfo_t");
#endif
    }
    if (PORTS(g))
	free(PORTS(g));
    freeGData(g);
#ifdef WITH_CGRAPH
    agdelrec(g, "Agraphinfo_t");
#endif
    for (dn = agfstnode(g); dn; dn = dnxt) {
	dnxt = agnxtnode(g, dn);
	for (e = agfstout(g, dn); e; e = agnxtout(g, e)) {
	    free (ED_to_virt(e));
#ifdef WITH_CGRAPH
	    agdelrec(e, "Agedgeinfo_t");
#endif
	}
	freeDeriveNode(dn);
    }
    agclose(g);
}

/* evalPositions:
 * The input is laid out, but node coordinates
 * are relative to smallest containing cluster.
 * Walk through all nodes and clusters, translating
 * the positions to absolute coordinates.
 * Assume that when called, g's bounding box is
 * in absolute coordinates and that box of root graph
 * has LL at origin.
 */
static void evalPositions(graph_t * g, graph_t* rootg)
{
    int i;
    graph_t *subg;
    node_t *n;
    boxf bb;
    boxf sbb;

    bb = BB(g);

    /* translate nodes in g */
    if (g != rootg) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (PARENT(n) != g)
		continue;
	    ND_pos(n)[0] += bb.LL.x;
	    ND_pos(n)[1] += bb.LL.y;
	}
    }

    /* translate top-level clusters and recurse */
    for (i = 1; i <= GD_n_cluster(g); i++) {
	subg = GD_clust(g)[i];
	if (g != rootg) {
	    sbb = BB(subg);
	    sbb.LL.x += bb.LL.x;
	    sbb.LL.y += bb.LL.y;
	    sbb.UR.x += bb.LL.x;
	    sbb.UR.y += bb.LL.y;
	    BB(subg) = sbb;
	}
	evalPositions(subg, rootg);
    }
}

#define CL_CHUNK 10

typedef struct {
    graph_t **cl;
    int sz;
    int cnt;
} clist_t;

static void initCList(clist_t * clist)
{
    clist->cl = 0;
    clist->sz = 0;
    clist->cnt = 0;
}

/* addCluster:
 * Append a new cluster to the list.
 * NOTE: cl[0] is empty. The clusters are in cl[1..cnt].
 * Normally, we increase the array when cnt == sz.
 * The test for cnt > sz is necessary for the first time.
 */
static void addCluster(clist_t * clist, graph_t * subg)
{
    clist->cnt++;
    if (clist->cnt >= clist->sz) {
	clist->sz += CL_CHUNK;
	clist->cl = RALLOC(clist->sz, clist->cl, graph_t *);
    }
    clist->cl[clist->cnt] = subg;
}

#define BSZ 1000

/* portName:
 * Generate a name for a port.
 * We use the name of the subgraph and names of the nodes on the edge,
 * if possible. Otherwise, we use the ids of the nodes.
 * This is for debugging. For production, just use edge id and some
 * id for the graph. Note that all the graphs are subgraphs of the
 * root graph.
 */
static char *portName(graph_t * g, bport_t * p)
{
    edge_t *e = p->e;
    node_t *h = aghead(e);
    node_t *t = agtail(e);
    static char buf[BSZ + 1];
    int len = 8;

    len += strlen(agnameof(g)) + strlen(agnameof(h)) + strlen(agnameof(t));
    if (len >= BSZ)
	sprintf(buf, "_port_%s_%s_%s_%ld", agnameof(g), agnameof(t), agnameof(h),
		(unsigned long)AGSEQ(e));
    else
	sprintf(buf, "_port_%s_(%d)_(%d)_%ld",agnameof(g), ND_id(t), ND_id(h),
		(unsigned long)AGSEQ(e));
    return buf;
}

/* chkPos:
 * If cluster has coords attribute, use to supply initial position
 * of derived node.
 * Only called if G_coord is defined.
 * We also look at the parent graph's G_coord attribute. If this
 * is identical to the child graph, we have to assume the child
 * inherited it.
 */
static void chkPos(graph_t* g, node_t* n, layout_info* infop, boxf* bbp)
{
    char *p;
    char *pp;
    boxf bb;
    char c;
    graph_t *parent;
    attrsym_t *G_coord = infop->G_coord;

#ifndef WITH_CGRAPH
    p = agxget(g, G_coord->index);
#else /* WITH_CGRAPH */
    p = agxget(g, G_coord);
#endif /* WITH_CGRAPH */
    if (p[0]) {
	if (g != infop->rootg) {
#ifndef WITH_CGRAPH
	    parent = agusergraph((agfstin(g->meta_node->graph, g->meta_node))->tail);
	    pp = agxget(parent, G_coord->index);
#else /* WITH_CGRAPH */
	    parent =agparent(g);
	    pp = agxget(parent, G_coord);
#endif /* WITH_CGRAPH */
	    if ((pp == p) || !strcmp(p, pp))
		return;
	}
	c = '\0';
	if (sscanf(p, "%lf,%lf,%lf,%lf%c",
		   &bb.LL.x, &bb.LL.y, &bb.UR.x, &bb.UR.y, &c) >= 4) {
	    if (PSinputscale > 0.0) {
		bb.LL.x /= PSinputscale;
		bb.LL.y /= PSinputscale;
		bb.UR.x /= PSinputscale;
		bb.UR.y /= PSinputscale;
	    }
	    if (c == '!')
		ND_pinned(n) = P_PIN;
	    else if (c == '?')
		ND_pinned(n) = P_FIX;
	    else
		ND_pinned(n) = P_SET;
	    *bbp = bb;
	} else
	    agerr(AGWARN, "graph %s, coord %s, expected four doubles\n",
		  agnameof(g), p);
    }
}

/* addEdge:
 * Add real edge e to its image de in the derived graph.
 * We use the to_virt and count fields to store the list.
 */
static void addEdge(edge_t * de, edge_t * e)
{
    short cnt = ED_count(de);
    edge_t **el;

    el = (edge_t **) (ED_to_virt(de));
    el = ALLOC(cnt + 1, el, edge_t *);
    el[cnt] = e;
    ED_to_virt(de) = (edge_t *) el;
    ED_count(de)++;
}

/* copyAttr:
 * Copy given attribute from g to dg.
 */
static void
copyAttr (graph_t* g, graph_t* dg, char* attr)
{
    char*     ov_val;
    Agsym_t*  ov;

#ifndef WITH_CGRAPH
    if ((ov = agfindattr(g, attr))) {
	ov_val = agxget(g,ov->index);
	ov = agfindattr(dg, attr);
#else /* WITH_CGRAPH */
    if ((ov = agattr(g,AGRAPH, attr, NULL))) {
	ov_val = agxget(g,ov);
	ov = agattr(dg,AGRAPH, attr, NULL);
#endif /* WITH_CGRAPH */
	if (ov)
#ifndef WITH_CGRAPH
	    agxset (dg, ov->index, ov_val);
#else /* WITH_CGRAPH */
	    agxset (dg, ov, ov_val);
#endif /* WITH_CGRAPH */
	else
#ifndef WITH_CGRAPH
	    agraphattr(dg, attr, ov_val);
#else /* WITH_CGRAPH */
	    agattr(dg, AGRAPH, attr, ov_val);
#endif /* WITH_CGRAPH */
    }
}

/* deriveGraph:
 * Create derived graph of g by collapsing clusters into
 * nodes. An edge is created between nodes if there is
 * an edge between two nodes in the clusters of the base graph.
 * Such edges record all corresponding real edges.
 * In addition, we add a node and edge for each port.
 */
static graph_t *deriveGraph(graph_t * g, layout_info * infop)
{
    graph_t *dg;
    node_t *dn;
    graph_t *subg;
    char name[100];
    bport_t *pp;
    node_t *n;
    edge_t *de;
    int i, id = 0;

    sprintf(name, "_dg_%d", infop->gid++);
    if (Verbose >= 2)
#ifndef WITH_CGRAPH
	fprintf(stderr, "derive graph %s of %s\n", name, g->name);
#else /* WITH_CGRAPH */
	fprintf(stderr, "derive graph %s of %s\n", name, agnameof(g));
#endif

#ifndef WITH_CGRAPH
    dg = agopen(name, AGRAPHSTRICT);
#else /* WITH_CGRAPH */
    dg = agopen("derived", Agstrictdirected,NIL(Agdisc_t *));
    agbindrec(dg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);
#endif /* WITH_CGRAPH */
    GD_alg(dg) = (void *) NEW(gdata);	/* freed in freeDeriveGraph */
#ifdef DEBUG
    GORIG(dg) = g;
#endif
    GD_ndim(dg) = GD_ndim(g);

    /* Copy attributes from g.
     */
    copyAttr(g,dg,"overlap");
    copyAttr(g,dg,"sep");
    copyAttr(g,dg,"K");

    /* create derived nodes from clusters */
    for (i = 1; i <= GD_n_cluster(g); i++) {
	boxf fix_bb = {{ MAXDOUBLE, MAXDOUBLE },{ -MAXDOUBLE, -MAXDOUBLE }};
	subg = GD_clust(g)[i];

	do_graph_label(subg);
	dn = mkDeriveNode(dg, agnameof(subg));
	ND_clust(dn) = subg;
	ND_id(dn) = id++;
	if (infop->G_coord)
		chkPos(subg, dn, infop, &fix_bb);
	for (n = agfstnode(subg); n; n = agnxtnode(subg, n)) {
	    DNODE(n) = dn;
#ifdef UNIMPLEMENTED
/* This code starts the implementation of supporting pinned nodes
 * within clusters. This needs more work. In particular, we may need
 * a separate notion of pinning related to contained nodes, which will
 * allow the cluster itself to wiggle.
 */
	    if (ND_pinned(n)) {
		fix_bb.LL.x = MIN(fix_bb.LL.x, ND_pos(n)[0]);
		fix_bb.LL.y = MIN(fix_bb.LL.y, ND_pos(n)[1]);
		fix_bb.UR.x = MAX(fix_bb.UR.x, ND_pos(n)[0]);
		fix_bb.UR.y = MAX(fix_bb.UR.y, ND_pos(n)[1]);
		ND_pinned(dn) = MAX(ND_pinned(dn), ND_pinned(n));
	    }
#endif
	}
	if (ND_pinned(dn)) {
	    ND_pos(dn)[0] = (fix_bb.LL.x + fix_bb.UR.x) / 2;
	    ND_pos(dn)[1] = (fix_bb.LL.y + fix_bb.UR.y) / 2;
	}
    }

    /* create derived nodes from remaining nodes */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (!DNODE(n)) {
	    if (PARENT(n) && (PARENT(n) != GPARENT(g))) {
		agerr (AGERR, "node \"%s\" is contained in two non-comparable clusters \"%s\" and \"%s\"\n", agnameof(n), agnameof(g), agnameof(PARENT(n)));
		longjmp (jbuf, 1);
	    }
	    PARENT(n) = g;
	    if (IS_CLUST_NODE(n))
		continue;
	    dn = mkDeriveNode(dg, agnameof(n));
	    DNODE(n) = dn;
	    ND_id(dn) = id++;
	    ND_width(dn) = ND_width(n);
	    ND_height(dn) = ND_height(n);
	    ND_lw(dn) = ND_lw(n);
	    ND_rw(dn) = ND_rw(n);
	    ND_ht(dn) = ND_ht(n);
	    ND_shape(dn) = ND_shape(n);
	    ND_shape_info(dn) = ND_shape_info(n);
	    if (ND_pinned(n)) {
		ND_pos(dn)[0] = ND_pos(n)[0];
		ND_pos(dn)[1] = ND_pos(n)[1];
		ND_pinned(dn) = ND_pinned(n);
	    }
	    ANODE(dn) = n;
	}
    }

    /* add edges */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	edge_t *e;
	node_t *hd;
	node_t *tl = DNODE(n);
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    hd = DNODE(aghead(e));
	    if (hd == tl)
		continue;
	    if (hd > tl)
#ifndef WITH_CGRAPH
		de = agedge(dg, tl, hd);
#else /* WITH_CGRAPH */
		de = agedge(dg, tl, hd, NULL,1);
#endif /* WITH_CGRAPH */
	    else
#ifndef WITH_CGRAPH
		de = agedge(dg, hd, tl);
#else /* WITH_CGRAPH */
		de = agedge(dg, hd, tl, NULL,1);
	    agbindrec(de, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);
#endif /* WITH_CGRAPH */
	    ED_dist(de) = ED_dist(e);
	    ED_factor(de) = ED_factor(e);
	    /* fprintf (stderr, "edge %s -- %s\n", tl->name, hd->name); */
	    WDEG(hd)++;
	    WDEG(tl)++;
	    if (NEW_EDGE(de)) {
		DEG(hd)++;
		DEG(tl)++;
	    }
	    addEdge(de, e);
	}
    }

    /* transform ports */
    if ((pp = PORTS(g))) {
	bport_t *pq;
	node_t *m;
	int sz = NPORTS(g);

	/* freed in freeDeriveGraph */
	PORTS(dg) = pq = N_NEW(sz + 1, bport_t);
	sz = 0;
	while (pp->e) {
	    m = DNODE(pp->n);
	    /* Create port in derived graph only if hooks to internal node */
	    if (m) {
		dn = mkDeriveNode(dg, portName(g, pp));
		sz++;
		ND_id(dn) = id++;
		if (dn > m)
#ifndef WITH_CGRAPH
		    de = agedge(dg, m, dn);
#else /* WITH_CGRAPH */
		    de = agedge(dg, m, dn, NULL,1);
#endif /* WITH_CGRAPH */
		else
#ifndef WITH_CGRAPH
		    de = agedge(dg, dn, m);
#else /* WITH_CGRAPH */
		    de = agedge(dg, dn, m, NULL,1);
		agbindrec(de, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);
#endif /* WITH_CGRAPH */
		ED_dist(de) = ED_dist(pp->e);
		ED_factor(de) = ED_factor(pp->e);
		addEdge(de, pp->e);
		WDEG(dn)++;
		WDEG(m)++;
		DEG(dn)++;	/* ports are unique, so this will be the first and */
		DEG(m)++;	/* only time the edge is touched. */
		pq->n = dn;
		pq->alpha = pp->alpha;
		pq->e = de;
		pq++;
	    }
	    pp++;
	}
	NPORTS(dg) = sz;
    }

    return dg;
}

/* ecmp:
 * Sort edges by angle, then distance.
 */
static int ecmp(const void *v1, const void *v2)
{
    erec *e1 = (erec *) v1;
    erec *e2 = (erec *) v2;
    if (e1->alpha > e2->alpha)
	return 1;
    else if (e1->alpha < e2->alpha)
	return -1;
    else if (e1->dist2 > e2->dist2)
	return 1;
    else if (e1->dist2 < e2->dist2)
	return -1;
    else
	return 0;
}

#define ANG (M_PI/90)		/* Maximum angular change: 2 degrees */

/* getEdgeList:
 * Generate list of edges in derived graph g using
 * node n. The list is in counterclockwise order.
 * This, of course, assumes we have an initial layout for g.
 */
static erec *getEdgeList(node_t * n, graph_t * g)
{
    erec *erecs;
    int deg = DEG(n);
    int i;
    double dx, dy;
    edge_t *e;
    node_t *m;

    /* freed in expandCluster */
    erecs = N_NEW(deg + 1, erec);
    i = 0;
    for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
	if (aghead(e) == n)
	    m = agtail(e);
	else
	    m = aghead(e);
	dx = ND_pos(m)[0] - ND_pos(n)[0];
	dy = ND_pos(m)[1] - ND_pos(n)[1];
	erecs[i].e = e;
	erecs[i].alpha = atan2(dy, dx);
	erecs[i].dist2 = dx * dx + dy * dy;
	i++;
    }
    assert(i == deg);
    qsort(erecs, deg, sizeof(erec), ecmp);

    /* ensure no two angles are equal */
    if (deg >= 2) {
	int j;
	double a, inc, delta, bnd;

	i = 0;
	while (i < deg - 1) {
	    a = erecs[i].alpha;
	    j = i + 1;
	    while ((j < deg) && (erecs[j].alpha == a))
		j++;
	    if (j == i + 1)
		i = j;
	    else {
		if (j == deg)
		    bnd = M_PI;	/* all values equal up to end */
		else
		    bnd = erecs[j].alpha;
		delta = (bnd - a) / (j - i);
		if (delta > ANG)
		    delta = ANG;
		inc = 0;
		for (; i < j; i++) {
		    erecs[i].alpha += inc;
		    inc += delta;
		}
	    }
	}
    }

    return erecs;
}

/* genPorts:
 * Given list of edges with node n in derived graph, add corresponding
 * ports to port list pp, starting at index idx. Return next index.
 * If an edge in the derived graph corresponds to multiple real edges,
 * add them in order if address of n is smaller than other node address.
 * Otherwise, reverse order.
 * Attach angles. The value bnd gives next angle after er->alpha.
 */
static int
genPorts(node_t * n, erec * er, bport_t * pp, int idx, double bnd)
{
    node_t *other;
    int cnt;
    edge_t *e = er->e;
    edge_t *el;
    edge_t **ep;
    double angle, delta;
    int i, j, inc;

    cnt = ED_count(e);

    if (aghead(e) == n)
	other = agtail(e);
    else
	other = aghead(e);

    delta = (bnd - er->alpha) / cnt;
    angle = er->alpha;
    if (delta > ANG)
	delta = ANG;

    if (n < other) {
	i = idx;
	inc = 1;
    } else {
	i = idx + cnt - 1;
	inc = -1;
	angle += delta * (cnt - 1);
	delta = -delta;
    }

    ep = (edge_t **) (el = ED_to_virt(e));
    for (j = 0; j < ED_count(e); j++, ep++) {
	el = *ep;
	pp[i].e = el;
	pp[i].n = (DNODE(agtail(el)) == n ? agtail(el) : aghead(el));
	pp[i].alpha = angle;
	i += inc;
	angle += delta;
    }
    return (idx + cnt);
}

/* expandCluster;
 * Given positioned derived graph cg with node n which corresponds
 * to a cluster, generate a graph containing the interior of the
 * cluster, plus port information induced by the layout of cg.
 * Basically, we use the cluster subgraph to which n corresponds,
 * attached with port information.
 */
static graph_t *expandCluster(node_t * n, graph_t * cg)
{
    erec *es;
    erec *ep;
    erec *next;
    graph_t *sg = ND_clust(n);
    bport_t *pp;
    int sz = WDEG(n);
    int idx = 0;
    double bnd;

    if (sz != 0) {
	/* freed in cleanup_subgs */
	pp = N_NEW(sz + 1, bport_t);

	/* create sorted list of edges of n */
	es = ep = getEdgeList(n, cg);

	/* generate ports from edges */
	while (ep->e) {
	    next = ep + 1;
	    if (next->e)
		bnd = next->alpha;
	    else
		bnd = 2 * M_PI + es->alpha;
	    idx = genPorts(n, ep, pp, idx, bnd);
	    ep = next;
	}
	assert(idx == sz);

	PORTS(sg) = pp;
	NPORTS(sg) = sz;
	free(es);
    }
    return sg;
}

/* setClustNodes:
 * At present, cluster nodes are not assigned a position during layout,
 * but positioned in the center of its associated cluster. Because the
 * dummy edge associated with a cluster node may not occur at a sufficient
 * level of cluster, the edge may not be used during layout and we cannot
 * therefore rely find these nodes via ports.
 *
 * In this implementation, we just do a linear pass over all nodes in the
 * root graph. At some point, we may use a better method, like having each
 * cluster contain its list of cluster nodes, or have the graph keep a list.
 * 
 * As nodes, we need to assign cluster nodes the coordinates in the
 * coordinates of its cluster p. Note that p's bbox is in its parent's
 * coordinates. 
 * 
 * If routing, we may decide to place on cluster boundary,
 * and use polyline.
 */
static void 
setClustNodes(graph_t* root)
{
    boxf bb;
    graph_t* p;
    pointf ctr;
    node_t *n;
    double w, h, h_pts;
    double h2, w2;
    pointf *vertices;

    for (n = agfstnode(root); n; n = agnxtnode(root, n)) {
	if (!IS_CLUST_NODE(n)) continue;

	p = PARENT(n);
	bb = BB(p);  /* bbox in parent cluster's coordinates */
	w = bb.UR.x - bb.LL.x;
	h = bb.UR.y - bb.LL.y;
	ctr.x = w / 2.0;
	ctr.y = h / 2.0;
	w2 = INCH2PS(w / 2.0);
	h2 = INCH2PS(h / 2.0);
	h_pts = INCH2PS(h);
	ND_pos(n)[0] = ctr.x;
	ND_pos(n)[1] = ctr.y;
	ND_width(n) = w;
	ND_height(n) = h;
	/* ND_xsize(n) = POINTS(w); */
	ND_lw(n) = ND_rw(n) = w2;
	ND_ht(n) = h_pts;

	vertices = ((polygon_t *) ND_shape_info(n))->vertices;
	vertices[0].x = ND_rw(n);
	vertices[0].y = h2;
	vertices[1].x = -ND_lw(n);
	vertices[1].y = h2;
	vertices[2].x = -ND_lw(n);
	vertices[2].y = -h2;
	vertices[3].x = ND_rw(n);
	vertices[3].y = -h2;
    }
}

/* layout:
 * Given g with ports:
 *  Derive g' from g by reducing clusters to points (deriveGraph)
 *  Compute connected components of g' (findCComp)
 *  For each cc of g': 
 *    Layout cc (tLayout)
 *    For each node n in cc of g' <-> cluster c in g:
 *      Add ports based on layout of cc to get c' (expandCluster)
 *      Layout c' (recursion)
 *    Remove ports from cc
 *    Expand nodes of cc to reflect size of c'  (xLayout)
 *  Pack connected components to get layout of g (putGraphs)
 *  Translate layout so that bounding box of layout + margin 
 *  has the origin as LL corner. 
 *  Set position of top level clusters and real nodes.
 *  Set bounding box of graph
 * 
 * TODO:
 * 
 * Possibly should modify so that only do connected components
 * on top-level derived graph. Unconnected parts of a cluster
 * could just rattle within cluster boundaries. This may mix
 * up components but give a tighter packing.
 * 
 * Add edges per components to get better packing, rather than
 * wait until the end.
 */
static 
void layout(graph_t * g, layout_info * infop)
{
    point *pts = NULL;
    graph_t *dg;
    node_t *dn;
    node_t *n;
    graph_t *cg;
    graph_t *sg;
    graph_t **cc;
    graph_t **pg;
    int c_cnt;
    int pinned;
    xparams xpms;

#ifdef DEBUG
    incInd();
#endif
    if (Verbose) {
#ifdef DEBUG
	prIndent();
#endif
	fprintf (stderr, "layout %s\n", agnameof(g));
    }
    /* initialize derived node pointers */
    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	DNODE(n) = 0;

    dg = deriveGraph(g, infop);
    cc = pg = findCComp(dg, &c_cnt, &pinned);

    while ((cg = *pg++)) {
	fdp_tLayout(cg, &xpms);
	for (n = agfstnode(cg); n; n = agnxtnode(cg, n)) {
	    if (ND_clust(n)) {
		pointf pt;
		sg = expandCluster(n, cg);	/* attach ports to sg */
		layout(sg, infop);
		/* bb.LL == origin */
		ND_width(n) = BB(sg).UR.x;
		ND_height(n) = BB(sg).UR.y;
		pt.x = POINTS_PER_INCH * BB(sg).UR.x;
		pt.y = POINTS_PER_INCH * BB(sg).UR.y;
		ND_rw(n) = ND_lw(n) = pt.x/2;
		ND_ht(n) = pt.y;
	    } else if (IS_PORT(n))
		agdelete(cg, n);	/* remove ports from component */
	}

	/* Remove overlaps */
	if (agnnodes(cg) >= 2) {
	    if (g == infop->rootg)
		normalize (cg);
	    fdp_xLayout(cg, &xpms);
	}
	/* set bounding box but don't use ports */
	/* setBB (cg); */
    }

    /* At this point, each connected component has its nodes correctly
     * positioned. If we have multiple components, we pack them together.
     * All nodes will be moved to their new positions.
     * NOTE: packGraphs uses nodes in components, so if port nodes are
     * not removed, it won't work.
     */
    /* Handle special cases well: no ports to real internal nodes
     *   Place cluster edges separately, after layout.
     * How to combine parts, especially with disparate components?
     */
    if (c_cnt > 1) {
	boolean *bp;
	if (pinned) {
	    bp = N_NEW(c_cnt, boolean);
	    bp[0] = TRUE;
	} else
	    bp = 0;
	infop->pack.fixed = bp;
	pts = putGraphs(c_cnt, cc, NULL, &infop->pack);
	if (bp)
	    free(bp);
    } else {
	pts = NULL;
	if (c_cnt == 1)
	    compute_bb(cc[0]);
    }

    /* set bounding box of dg and reposition nodes */
    finalCC(dg, c_cnt, cc, pts, g, infop);
    free (pts);

    /* record positions from derived graph to input graph */
    /* At present, this does not record port node info */
    /* In fact, as noted above, we have removed port nodes */
    for (dn = agfstnode(dg); dn; dn = agnxtnode(dg, dn)) {
	if ((sg = ND_clust(dn))) {
	    BB(sg).LL.x = ND_pos(dn)[0] - ND_width(dn) / 2;
	    BB(sg).LL.y = ND_pos(dn)[1] - ND_height(dn) / 2;
	    BB(sg).UR.x = BB(sg).LL.x + ND_width(dn);
	    BB(sg).UR.y = BB(sg).LL.y + ND_height(dn);
	} else if ((n = ANODE(dn))) {
	    ND_pos(n)[0] = ND_pos(dn)[0];
	    ND_pos(n)[1] = ND_pos(dn)[1];
	}
    }
    BB(g) = BB(dg);
#ifdef DEBUG
    if (g == infop->rootg)
	dump(g, 1, 0);
#endif

    /* clean up temp graphs */
    freeDerivedGraph(dg, cc);
    free(cc);
    if (Verbose) {
#ifdef DEBUG
	prIndent ();
#endif
	fprintf (stderr, "end %s\n", agnameof(g));
    }
#ifdef DEBUG
    decInd();
#endif
}

/* setBB;
 * Set point box g->bb from inch box BB(g).
 */
static void setBB(graph_t * g)
{
    int i;
    boxf bb;

    bb.LL.x = POINTS_PER_INCH * BB(g).LL.x;
    bb.LL.y = POINTS_PER_INCH * BB(g).LL.y;
    bb.UR.x = POINTS_PER_INCH * BB(g).UR.x;
    bb.UR.y = POINTS_PER_INCH * BB(g).UR.y;
    GD_bb(g) = bb;
    for (i = 1; i <= GD_n_cluster(g); i++) {
	setBB(GD_clust(g)[i]);
    }
}

/* init_info:
 * Initialize graph-dependent information and
 * state variable.s
 */
void init_info(graph_t * g, layout_info * infop)
{
#ifndef WITH_CGRAPH
    infop->G_coord = agfindattr(g, "coords");
    infop->G_width = agfindattr(g, "width");
    infop->G_height = agfindattr(g, "height");
#else /* WITH_CGRAPH */
    infop->G_coord = agattr(g,AGRAPH, "coords", NULL);
    infop->G_width = agattr(g,AGRAPH, "width", NULL);
    infop->G_height = agattr(g, AGRAPH,"height", NULL);
#endif /* WITH_CGRAPH */
    infop->rootg = g;
    infop->gid = 0;
    infop->pack.mode = getPackInfo(g, l_node, CL_OFFSET / 2, &(infop->pack));
}

/* mkClusters:
 * Attach list of immediate child clusters.
 * NB: By convention, the indexing starts at 1.
 * If pclist is NULL, the graph is the root graph or a cluster
 * If pclist is non-NULL, we are recursively scanning a non-cluster
 * subgraph for cluster children.
 */
static void
mkClusters (graph_t * g, clist_t* pclist, graph_t* parent)
{
#ifndef WITH_CGRAPH
    node_t*  mn;
    edge_t*  me;
    graph_t* mg;
#endif /* WITH_CGRAPH */
    graph_t* subg;
    clist_t  list;
    clist_t* clist;

    if (pclist == NULL) {
	clist = &list;
	initCList(clist);
    }
    else
	clist = pclist;
#ifndef WITH_CGRAPH
    mg = g->meta_node->graph;
    for (me = agfstout(mg, g->meta_node); me; me = agnxtout(mg, me)) {
	mn = me->head;
	subg = agusergraph(mn);
	if (!strncmp(subg->name, "cluster", 7)) {
#else /* WITH_CGRAPH */

    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg))
	{
	if (!strncmp(agnameof(subg), "cluster", 7)) {
	    agbindrec(subg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);
#endif /* WITH_CGRAPH */
	    GD_alg(subg) = (void *) NEW(gdata);	/* freed in cleanup_subgs */
	    GD_ndim(subg) = GD_ndim(parent);
	    LEVEL(subg) = LEVEL(parent) + 1;
	    GPARENT(subg) = parent;
	    addCluster(clist, subg);
	    mkClusters(subg, NULL, subg);
	}
	else {
	    mkClusters(subg, clist, parent);
	}
    }
    if (pclist == NULL) {
	GD_n_cluster(g) = list.cnt;
	if (list.cnt)
	    GD_clust(g) = RALLOC(list.cnt + 1, list.cl, graph_t*);
    }
}

void fdp_init_graph(Agraph_t * g)
{
    setEdgeType (g, ET_LINE);
    GD_alg(g) = (void *) NEW(gdata);	/* freed in cleanup_graph */
#ifndef WITH_CGRAPH
    g->u.ndim = late_int(g, agfindattr(g, "dim"), 2, 2);
    Ndim = g->u.ndim = MIN(g->u.ndim, MAXDIM);
#else /* WITH_CGRAPH */
    GD_ndim(g) = late_int(g, agattr(g,AGRAPH, "dim", NULL), 2, 2);
    Ndim = GD_ndim(g) = MIN(GD_ndim(g), MAXDIM);
#endif /* WITH_CGRAPH */

    mkClusters (g, NULL, g);
    fdp_initParams(g);
    fdp_init_node_edge(g);
}

void fdpLayout(graph_t * g)
{
    layout_info info;

    init_info(g, &info);
    layout(g, &info);
    setClustNodes(g);
    evalPositions(g,g);

    /* Set bbox info for g and all clusters. This is needed for
     * spline drawing. We already know the graph bbox is at the origin.
     * (We pass the origin to spline_edges0. This really isn't true,
     * as the algorithm has done many translations.)
     * On return from spline drawing, all bounding boxes should be correct.
     */
    setBB(g);
}

static void
fdpSplines (graph_t * g)
{
    int trySplines = 0;
    int et = EDGE_TYPE(g);

    if (et != ET_LINE) {
	if (et == ET_COMPOUND) {
	    trySplines = splineEdges(g, compoundEdges, ET_SPLINE);
	    /* When doing the edges again, accept edges done by compoundEdges */
	    if (trySplines)
		Nop = 2;
	}
	if (trySplines || (et != ET_COMPOUND)) {
	    if (HAS_CLUST_EDGE(g)) {
		agerr(AGWARN,
		      "splines and cluster edges not supported - using line segments\n");
	    } else {
		spline_edges1(g, et);
	    }
	}
	Nop = 0;
    }
    if (State < GVSPLINES)
	spline_edges1(g, ET_LINE);
}

void fdp_layout(graph_t * g)
{
    /* Agnode_t* n; */

    fdp_init_graph(g);
    if (setjmp(jbuf)) {
	return;
    }
    fdpLayout(g);
#if 0
    /* free ND_alg field so it can be used in spline routing */
    if ((n = agfstnode(g)))
	free(ND_alg(n));
#endif
    neato_set_aspect(g);

    if (EDGE_TYPE(g) != ET_NONE) fdpSplines (g); 

    dotneato_postprocess(g);
}
