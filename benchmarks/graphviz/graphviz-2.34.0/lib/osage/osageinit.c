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


/* FIX: handle cluster labels */
/*
 * Written by Emden R. Gansner
 */

#include    "osage.h"
#include    "neatoprocs.h"
#include    "pack.h"

#define CL_CHUNK 10
#define DFLT_SZ  18
#define PARENT(n) ((Agraph_t*)ND_alg(n))

static void
indent (int i)
{
    for (; i > 0; i--)
	fputs ("  ", stderr);
}

typedef struct {
    Agraph_t** cl;
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

static void cluster_init_graph(graph_t * g)
{
    Agnode_t *n;
    Agedge_t *e;

    setEdgeType (g, ET_LINE);
    Ndim = GD_ndim(g)=2;	/* The algorithm only makes sense in 2D */

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	neato_init_node (n);
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
#ifdef WITH_CGRAPH
	    agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);	//edge custom data
#endif /* WITH_CGRAPH */
	    common_init_edge(e);
	}
    }
}

/* layout:
 */
static void
layout (Agraph_t* g, int depth)
{
    int i, j, total, nv;
    int nvs = 0;       /* no. of nodes in subclusters */
    Agnode_t*  n;
    Agraph_t*  subg;
    boxf* gs;
    point* pts;
    boxf bb, rootbb;
    pointf p;
    pack_info pinfo;
    pack_mode pmode;
    double margin;
    void** children;
    Agsym_t* cattr = NULL;
    Agsym_t* vattr = NULL;
    Agraph_t* root = g->root;

    if (Verbose > 1) {
	indent (depth);
	fprintf (stderr, "layout %s\n", agnameof(g));
    }

    /* Lay out subclusters */
    for (i = 1; i <= GD_n_cluster(g); i++) {
        subg = GD_clust(g)[i];
	layout (subg, depth+1);
	nvs += agnnodes (subg);
    }

    nv = agnnodes(g);
    total = (nv - nvs) + GD_n_cluster(g);

    if ((total == 0) && (GD_label(g) == NULL)) {
	GD_bb(g).LL.x = GD_bb(g).LL.y = 0;
	GD_bb(g).UR.x = GD_bb(g).UR.y = DFLT_SZ;
	return;
    }
    
    pmode = getPackInfo(g, l_array, DFLT_MARGIN, &pinfo);
    if (pmode < l_graph) pinfo.mode = l_graph;

        /* add user sort values if necessary */
    if ((pinfo.mode == l_array) && (pinfo.flags & PK_USER_VALS)) {
#ifdef WITH_CGRAPH
	cattr = agattr(root, AGRAPH, "sortv", 0);
	vattr = agattr(root, AGNODE, "sortv", 0);
#else
	cattr = agfindattr(root, "sortv");
	vattr = agfindattr(root->proto->n, "sortv");
#endif
	if (cattr || vattr)
	    pinfo.vals = N_NEW(total, unsigned char);
	else
	    agerr (AGWARN, "Graph %s has array packing with user values but no \"sortv\" attributes are defined.",
		agnameof(g));
    }

    gs = N_NEW(total, boxf);
    children = N_NEW(total, void*);
    j = 0;
    for (i = 1; i <= GD_n_cluster(g); i++) {
	subg = GD_clust(g)[i];
	gs[j] = GD_bb(subg);
	if (pinfo.vals && cattr) {
	    pinfo.vals[j] = late_int (subg, cattr, 0, 0);
	}
	children[j++] = subg;
    }
  
    if (nv-nvs > 0) {
	for (n = agfstnode (g); n; n = agnxtnode (g,n)) {
	    if (ND_alg(n)) continue;
	    ND_alg(n) = g;
	    bb.LL.y = bb.LL.x = 0;
	    bb.UR.x = ND_xsize(n);
	    bb.UR.y = ND_ysize(n);
	    gs[j] = bb;
	    if (pinfo.vals && vattr) {
		pinfo.vals[j] = late_int (n, vattr, 0, 0);
	    }
	    children[j++] = n;
	}
    }

	/* pack rectangles */
    pts = putRects (total, gs, &pinfo);
    if (pinfo.vals) {
	free (pinfo.vals);
    }

    rootbb.LL = pointfof(INT_MAX, INT_MAX);
    rootbb.UR = pointfof(-INT_MAX, -INT_MAX);

    /* reposition children relative to GD_bb(g) */
    for (j = 0; j < total; j++) {
	P2PF(pts[j],p);
	bb = gs[j];
	bb.LL.x += p.x;
	bb.UR.x += p.x;
	bb.LL.y += p.y;
	bb.UR.y += p.y;
	EXPANDBB(rootbb, bb);
	if (j < GD_n_cluster(g)) {
	    subg = (Agraph_t*)children[j];
	    GD_bb(subg) = bb;
	    if (Verbose > 1) {
		indent (depth);
		fprintf (stderr, "%s : %f %f %f %f\n", agnameof(subg), bb.LL.x, bb.LL.y, bb.UR.x, bb.UR.y);
	    }
	}
	else {
	    n = (Agnode_t*)children[j];
	    ND_coord(n) = mid_pointf (bb.LL, bb.UR);
	    if (Verbose > 1) {
		indent (depth);
		fprintf (stderr, "%s : %f %f\n", agnameof(n), ND_coord(n).x, ND_coord(n).y);
	    }
	}
    }

    if (GD_label(g)) {
        pointf p;
        double d;

        p = GD_label(g)->dimen;
	if (total == 0) {
            rootbb.LL.x = 0;
            rootbb.LL.y = 0;
            rootbb.UR.x = p.x;
            rootbb.UR.y = p.y;

	}
        d = p.x - (rootbb.UR.x - rootbb.LL.x);
        if (d > 0) {            /* height of label is added below */
            d /= 2;
            rootbb.LL.x -= d;
            rootbb.UR.x += d;
        }
    }

    if (depth > 0)
	margin = pinfo.margin/2.0;
    else
	margin = 0;
    rootbb.LL.x -= margin;
    rootbb.UR.x += margin;
    rootbb.LL.y -= (margin + GD_border(g)[BOTTOM_IX].y);
    rootbb.UR.y += (margin + GD_border(g)[TOP_IX].y);

    if (Verbose > 1) {
	indent (depth);
	fprintf (stderr, "%s : %f %f %f %f\n", agnameof(g), rootbb.LL.x, rootbb.LL.y, rootbb.UR.x, rootbb.UR.y);
    }

    /* Translate so that rootbb.LL is origin.
     * This makes the repositioning simpler; we just translate the clusters and nodes in g by
     * the final bb.ll of g.
     */
    for (j = 0; j < total; j++) {
	if (j < GD_n_cluster(g)) {
	    subg = (Agraph_t*)children[j];
	    bb = GD_bb(subg);
	    bb.LL = sub_pointf(bb.LL, rootbb.LL);
	    bb.UR = sub_pointf(bb.UR, rootbb.LL);
	    GD_bb(subg) = bb;
	    if (Verbose > 1) {
		indent (depth);
		fprintf (stderr, "%s : %f %f %f %f\n", agnameof(subg), bb.LL.x, bb.LL.y, bb.UR.x, bb.UR.y);
	    }
	}
	else {
	    n = (Agnode_t*)children[j];
	    ND_coord(n) = sub_pointf (ND_coord(n), rootbb.LL);
	    if (Verbose > 1) {
		indent (depth);
		fprintf (stderr, "%s : %f %f\n", agnameof(n), ND_coord(n).x, ND_coord(n).y);
	    }
	}
    }

    rootbb.UR = sub_pointf(rootbb.UR, rootbb.LL);
    rootbb.LL = sub_pointf(rootbb.LL, rootbb.LL);
    GD_bb(g) = rootbb;

    if (Verbose > 1) {
	indent (depth);
	fprintf (stderr, "%s : %f %f %f %f\n", agnameof(g), rootbb.LL.x, rootbb.LL.y, rootbb.UR.x, rootbb.UR.y);
    }

    free (gs);
    free (children);
    free (pts);
}

static void
reposition (Agraph_t* g, int depth)
{
    boxf sbb, bb = GD_bb(g);
    Agnode_t* n;
    Agraph_t* subg;
    int i;

    if (Verbose > 1) {
	indent (depth);
	fprintf (stderr, "reposition %s\n", agnameof(g));
    }

    /* translate nodes in g but not in a subcluster */
    if (depth) {
        for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
            if (PARENT(n) != g)
                continue;
            ND_coord(n).x += bb.LL.x;
            ND_coord(n).y += bb.LL.y;
	    if (Verbose > 1) {
		indent (depth);
		fprintf (stderr, "%s : %f %f\n", agnameof(n), ND_coord(n).x, ND_coord(n).y);
	    }
        }
    }

    /* translate top-level clusters and recurse */
    for (i = 1; i <= GD_n_cluster(g); i++) {
        subg = GD_clust(g)[i];
        if (depth) {
            sbb = GD_bb(subg);
            sbb.LL.x += bb.LL.x;
            sbb.LL.y += bb.LL.y;
            sbb.UR.x += bb.LL.x;
            sbb.UR.y += bb.LL.y;
	    if (Verbose > 1) {
		indent (depth);
		fprintf (stderr, "%s : %f %f %f %f\n", agnameof(subg), sbb.LL.x, sbb.LL.y, sbb.UR.x, sbb.UR.y);
	    }
            GD_bb(subg) = sbb;
        }
        reposition (subg, depth+1);
    }

}

static void
mkClusters (Agraph_t* g, clist_t* pclist, Agraph_t* parent)
{
#ifndef WITH_CGRAPH
    node_t*  mn;
    edge_t*  me;
    graph_t* mg;
#endif
    graph_t* subg;
    clist_t  list;
    clist_t* clist;

    if (pclist == NULL) {
        clist = &list;
        initCList(clist);
    }
    else
        clist = pclist;

#ifdef WITH_CGRAPH
    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
        if (!strncmp(agnameof(subg), "cluster", 7)) {
	    agbindrec(subg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);
	    do_graph_label (subg);
            addCluster(clist, subg);
            mkClusters(subg, NULL, subg);
        }
        else {
            mkClusters(subg, clist, parent);
        }
    }
#else
    mg = g->meta_node->graph;
    for (me = agfstout(mg, g->meta_node); me; me = agnxtout(mg, me)) {
        mn = me->head;
        subg = agusergraph(mn);
        if (!strncmp(agnameof(subg), "cluster", 7)) {
	    do_graph_label (subg);
            addCluster(clist, subg);
            mkClusters(subg, NULL, subg);
        }
        else {
            mkClusters(subg, clist, parent);
        }
    }
#endif
    if (pclist == NULL) {
        GD_n_cluster(g) = list.cnt;
        if (list.cnt)
            GD_clust(g) = RALLOC(list.cnt + 1, list.cl, graph_t*);
    }
}

void osage_layout(Agraph_t *g)
{
    cluster_init_graph(g);
    mkClusters(g, NULL, g);
    layout(g, 0);
    reposition (g, 0);

    if (GD_drawing(g)->ratio_kind) {
	Agnode_t* n;
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    ND_pos(n)[0] = PS2INCH(ND_coord(n).x);
	    ND_pos(n)[1] = PS2INCH(ND_coord(n).y);
	}
	spline_edges0(g);
    }
    else {
	int et = EDGE_TYPE (g);
	if (et != ET_NONE) spline_edges1(g, et);
    }
    dotneato_postprocess(g);
}

static void cleanup_graphs (Agraph_t *g)
{
    graph_t *subg;
    int i;
    
    for (i = 1; i <= GD_n_cluster(g); i++) {
        subg = GD_clust(g)[i];
	free_label(GD_label(subg));
        cleanup_graphs (subg);
    }
    free (GD_clust(g));
}

void osage_cleanup(Agraph_t *g)
{
    node_t *n;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
        gv_cleanup_node(n);
    }
    cleanup_graphs(g);
}
