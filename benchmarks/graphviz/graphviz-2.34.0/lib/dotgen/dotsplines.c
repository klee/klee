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
 * set edge splines.
 */

#include "dot.h"

#ifdef ORTHO
#include <ortho.h>
#endif

#define	NSUB	9		/* number of subdivisions, re-aiming splines */
#define	CHUNK	128		/* in building list of edges */

#define MINW 16			/* minimum width of a box in the edge path */
#define HALFMINW 8

#define FWDEDGE    16
#define BWDEDGE    32

#define MAINGRAPH  64
#define AUXGRAPH  128
#define GRAPHTYPEMASK	192	/* the OR of the above */

#ifndef WITH_CGRAPH
#define MAKEFWDEDGE(new, old) { \
	edge_t *newp; \
	newp = new; \
	*newp = *old; \
	newp->tail = old->head; \
	newp->head = old->tail; \
	ED_tail_port(newp) = ED_head_port(old); \
	ED_head_port(newp) = ED_tail_port(old); \
	ED_edge_type(newp) = VIRTUAL; \
	ED_to_orig(newp) = old; \
}
#else /* WITH_CGRAPH */
#define MAKEFWDEDGE(new, old) { \
	edge_t *newp; \
	Agedgeinfo_t *info; \
	newp = new; \
	info = (Agedgeinfo_t*)newp->base.data; \
	*info = *(Agedgeinfo_t*)old->base.data; \
	*newp = *old; \
	newp->base.data = (Agrec_t*)info; \
	AGTAIL(newp) = AGHEAD(old); \
	AGHEAD(newp) = AGTAIL(old); \
	ED_tail_port(newp) = ED_head_port(old); \
	ED_head_port(newp) = ED_tail_port(old); \
	ED_edge_type(newp) = VIRTUAL; \
	ED_to_orig(newp) = old; \
}
#endif /* WITH_CGRAPH */

static boxf boxes[1000];
typedef struct {
    int LeftBound, RightBound, Splinesep, Multisep;
    boxf* Rank_box;
} spline_info_t;

static void adjustregularpath(path *, int, int);
static Agedge_t *bot_bound(Agedge_t *, int);
static boolean pathscross(Agnode_t *, Agnode_t *, Agedge_t *, Agedge_t *);
static Agraph_t *cl_bound(Agnode_t *, Agnode_t *);
static int cl_vninside(Agraph_t *, Agnode_t *);
static void completeregularpath(path *, Agedge_t *, Agedge_t *,
				pathend_t *, pathend_t *, boxf *, int, int);
static int edgecmp(Agedge_t **, Agedge_t **);
static void make_flat_edge(spline_info_t*, path *, Agedge_t **, int, int, int);
static void make_regular_edge(spline_info_t*, path *, Agedge_t **, int, int, int);
static boxf makeregularend(boxf, int, int);
static boxf maximal_bbox(spline_info_t*, Agnode_t *, Agedge_t *, Agedge_t *);
static Agnode_t *neighbor(Agnode_t *, Agedge_t *, Agedge_t *, int);
static void place_vnlabel(Agnode_t *);
static boxf rank_box(spline_info_t* sp, Agraph_t *, int);
static void recover_slack(Agedge_t *, path *);
static void resize_vn(Agnode_t *, int, int, int);
static void setflags(Agedge_t *, int, int, int);
static int straight_len(Agnode_t *);
static Agedge_t *straight_path(Agedge_t *, int, pointf *, int *);
static Agedge_t *top_bound(Agedge_t *, int);

#define GROWEDGES (edges = ALLOC (n_edges + CHUNK, edges, edge_t*))

static edge_t*
getmainedge(edge_t * e)
{
    edge_t *le = e;
    while (ED_to_virt(le))
	le = ED_to_virt(le);
    while (ED_to_orig(le))
	le = ED_to_orig(le);
    return le;
}

static boolean spline_merge(node_t * n)
{
    return ((ND_node_type(n) == VIRTUAL)
	    && ((ND_in(n).size > 1) || (ND_out(n).size > 1)));
}

static boolean swap_ends_p(edge_t * e)
{
    while (ED_to_orig(e))
	e = ED_to_orig(e);
    if (ND_rank(aghead(e)) > ND_rank(agtail(e)))
	return FALSE;
    if (ND_rank(aghead(e)) < ND_rank(agtail(e)))
	return TRUE;
    if (ND_order(aghead(e)) >= ND_order(agtail(e)))
	return FALSE;
    return TRUE;
}

static splineInfo sinfo = { swap_ends_p, spline_merge };

int portcmp(port p0, port p1)
{
    int rv;
    if (p1.defined == FALSE)
	return (p0.defined ? 1 : 0);
    if (p0.defined == FALSE)
	return -1;
    rv = p0.p.x - p1.p.x;
    if (rv == 0)
	rv = p0.p.y - p1.p.y;
    return rv;
}

/* swap_bezier:
 */
static void swap_bezier(bezier * old, bezier * new)
{
    pointf *list;
    pointf *lp;
    pointf *olp;
    int i, sz;

    sz = old->size;
    list = N_GNEW(sz, pointf);
    lp = list;
    olp = old->list + (sz - 1);
    for (i = 0; i < sz; i++) {	/* reverse list of points */
	*lp++ = *olp--;
    }

    new->list = list;
    new->size = sz;
    new->sflag = old->eflag;
    new->eflag = old->sflag;
    new->sp = old->ep;
    new->ep = old->sp;
}

/* swap_spline:
 */
static void swap_spline(splines * s)
{
    bezier *list;
    bezier *lp;
    bezier *olp;
    int i, sz;

    sz = s->size;
    list = N_GNEW(sz, bezier);
    lp = list;
    olp = s->list + (sz - 1);
    for (i = 0; i < sz; i++) {	/* reverse and swap list of beziers */
	swap_bezier(olp--, lp++);
    }

    /* free old structures */
    for (i = 0; i < sz; i++)
	free(s->list[i].list);
    free(s->list);

    s->list = list;
}

/* edge_normalize:
 * Some back edges are reversed during layout and the reversed edge
 * is used to compute the spline. We would like to guarantee that
 * the order of control points always goes from tail to head, so
 * we reverse them if necessary.
 */
static void edge_normalize(graph_t * g)
{
    edge_t *e;
    node_t *n;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (sinfo.swapEnds(e) && ED_spl(e))
		swap_spline(ED_spl(e));
	}
    }
}

/* resetRW:
 * In position, each node has its rw stored in mval and,
 * if a node is part of a loop, rw may be increased to
 * reflect the loops and associated labels. We restore
 * the original value here. 
 */
static void
resetRW (graph_t * g)
{
    node_t* n;

    for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
	if (ND_other(n).list) {
	    double tmp = ND_rw(n);
	    ND_rw(n) = ND_mval(n);
	    ND_mval(n) = tmp;
	}
    }
}

/* setEdgeLabelPos:
 * Set edge label position information for regular and non-adjacent flat edges.
 * Dot has allocated space and position for these labels. This info will be
 * used when routing orthogonal edges.
 */
static void
setEdgeLabelPos (graph_t * g)
{
    node_t* n;
    textlabel_t* l;

    /* place regular edge labels */
    for (n = GD_nlist(g); n; n = ND_next(n)) {
	if (ND_node_type(n) == VIRTUAL) {
	    if (ND_alg(n)) {   // label of non-adjacent flat edge
		edge_t* fe = (edge_t*)ND_alg(n);
		assert ((l = ED_label(fe)));
		l->pos = ND_coord(n);
		l->set = TRUE;
	    }
	    else if ((l = ND_label(n))) {// label of regular edge
		place_vnlabel(n);
	    }
	    if (l) updateBB(g, l);
	}
    }
}

/* _dot_splines:
 * Main spline routing code.
 * The normalize parameter allows this function to be called by the
 * recursive call in make_flat_edge without normalization occurring,
 * so that the edge will only be normalized once in the top level call
 * of dot_splines.
 */
static void _dot_splines(graph_t * g, int normalize)
{
    int i, j, k, n_nodes, n_edges, ind, cnt;
    node_t *n;
#ifndef WITH_CGRAPH
    edge_t fwdedgea, fwdedgeb;
#else
    Agedgeinfo_t fwdedgeai, fwdedgebi;
    Agedgepair_t fwdedgea, fwdedgeb;
#endif
    edge_t *e, *e0, *e1, *ea, *eb, *le0, *le1, **edges;
    path *P;
    spline_info_t sd;
    int et = EDGE_TYPE(g);
#ifdef WITH_CGRAPH
    fwdedgea.out.base.data = (Agrec_t*)&fwdedgeai;
    fwdedgeb.out.base.data = (Agrec_t*)&fwdedgebi;
#endif /* WITH_CGRAPH */

    if (et == ET_NONE) return; 
    if (et == ET_CURVED) {
	resetRW (g);
	if (GD_has_labels(g) & EDGE_LABEL) {
	    agerr (AGWARN, "edge labels with splines=curved not supported in dot - use xlabels\n");
	}
	for (n = agfstnode (g); n; n = agnxtnode(g, n)) {
	    for (e = agfstout(g, n); e; e = agnxtout(g,e)) {
		makeStraightEdge(g, e, et, &sinfo);
	    }
	}
	goto finish;
    } 
#ifdef ORTHO
    if (et == ET_ORTHO) {
	resetRW (g);
	if (GD_has_labels(g) & EDGE_LABEL) {
	    setEdgeLabelPos (g);
	    orthoEdges (g, 1);
	}
	else
	    orthoEdges (g, 0);
	goto finish;
    } 
#endif

    mark_lowclusters(g);
    if (routesplinesinit()) return;
    P = NEW(path);
    /* FlatHeight = 2 * GD_nodesep(g); */
    sd.Splinesep = GD_nodesep(g) / 4;
    sd.Multisep = GD_nodesep(g);
    edges = N_NEW(CHUNK, edge_t *);

    /* compute boundaries and list of splines */
    sd.LeftBound = sd.RightBound = 0;
    n_edges = n_nodes = 0;
    for (i = GD_minrank(g); i <= GD_maxrank(g); i++) {
	n_nodes += GD_rank(g)[i].n;
	if ((n = GD_rank(g)[i].v[0]))
	    sd.LeftBound = MIN(sd.LeftBound, (ND_coord(n).x - ND_lw(n)));
	if (GD_rank(g)[i].n && (n = GD_rank(g)[i].v[GD_rank(g)[i].n - 1]))
	    sd.RightBound = MAX(sd.RightBound, (ND_coord(n).x + ND_rw(n)));
	sd.LeftBound -= MINW;
	sd.RightBound += MINW;

	for (j = 0; j < GD_rank(g)[i].n; j++) {
	    n = GD_rank(g)[i].v[j];
		/* if n is the label of a flat edge, copy its position to
		 * the label.
		 */
	    if (ND_alg(n)) {
		edge_t* fe = (edge_t*)ND_alg(n);
		assert (ED_label(fe));
		ED_label(fe)->pos = ND_coord(n);
		ED_label(fe)->set = TRUE;
	    }
	    if ((ND_node_type(n) != NORMAL) &&
		(sinfo.splineMerge(n) == FALSE))
		continue;
	    for (k = 0; (e = ND_out(n).list[k]); k++) {
		if ((ED_edge_type(e) == FLATORDER)
		    || (ED_edge_type(e) == IGNORED))
		    continue;
		setflags(e, REGULAREDGE, FWDEDGE, MAINGRAPH);
		edges[n_edges++] = e;
		if (n_edges % CHUNK == 0)
		    GROWEDGES;
	    }
	    if (ND_flat_out(n).list)
		for (k = 0; (e = ND_flat_out(n).list[k]); k++) {
		    setflags(e, FLATEDGE, 0, AUXGRAPH);
		    edges[n_edges++] = e;
		    if (n_edges % CHUNK == 0)
			GROWEDGES;
		}
	    if (ND_other(n).list) {
		/* In position, each node has its rw stored in mval and,
                 * if a node is part of a loop, rw may be increased to
                 * reflect the loops and associated labels. We restore
                 * the original value here. 
                 */
		if (ND_node_type(n) == NORMAL) {
		    double tmp = ND_rw(n);
		    ND_rw(n) = ND_mval(n);
		    ND_mval(n) = tmp;
		}
		for (k = 0; (e = ND_other(n).list[k]); k++) {
		    setflags(e, 0, 0, AUXGRAPH);
		    edges[n_edges++] = e;
		    if (n_edges % CHUNK == 0)
			GROWEDGES;
		}
	    }
	}
    }

    /* Sort so that equivalent edges are contiguous. 
     * Equivalence should basically mean that 2 edges have the
     * same set {(tailnode,tailport),(headnode,headport)}, or
     * alternatively, the edges would be routed identically if
     * routed separately.
     */
    qsort((char *) &edges[0], n_edges, sizeof(edges[0]),
	  (qsort_cmpf) edgecmp);

    /* FIXME: just how many boxes can there be? */
    P->boxes = N_NEW(n_nodes + 20 * 2 * NSUB, boxf);
    sd.Rank_box = N_NEW(i, boxf);

    if (et == ET_LINE) {
    /* place regular edge labels */
	for (n = GD_nlist(g); n; n = ND_next(n)) {
	    if ((ND_node_type(n) == VIRTUAL) && (ND_label(n))) {
		place_vnlabel(n);
	    }
	}
    }

    for (i = 0; i < n_edges;) {
	ind = i;
	le0 = getmainedge((e0 = edges[i++]));
	ea = (ED_tail_port(e0).defined
	      || ED_head_port(e0).defined) ? e0 : le0;
	if (ED_tree_index(ea) & BWDEDGE) {
#ifndef WITH_CGRAPH
	    MAKEFWDEDGE(&fwdedgea, ea);
	    ea = &fwdedgea;
#else
	    MAKEFWDEDGE(&fwdedgea.out, ea);
	    ea = &fwdedgea.out;
#endif
	}
	for (cnt = 1; i < n_edges; cnt++, i++) {
	    if (le0 != (le1 = getmainedge((e1 = edges[i]))))
		break;
	    if (ED_adjacent(e0)) continue; /* all flat adjacent edges at once */
	    eb = (ED_tail_port(e1).defined
		  || ED_head_port(e1).defined) ? e1 : le1;
	    if (ED_tree_index(eb) & BWDEDGE) {
#ifndef WITH_CGRAPH
		MAKEFWDEDGE(&fwdedgeb, eb);
		eb = &fwdedgeb;
#else
		MAKEFWDEDGE(&fwdedgeb.out, eb);
		eb = &fwdedgeb.out;
#endif
	    }
	    if (portcmp(ED_tail_port(ea), ED_tail_port(eb)))
		break;
	    if (portcmp(ED_head_port(ea), ED_head_port(eb)))
		break;
	    if ((ED_tree_index(e0) & EDGETYPEMASK) == FLATEDGE
		&& ED_label(e0) != ED_label(e1))
		break;
	    if (ED_tree_index(edges[i]) & MAINGRAPH)	/* Aha! -C is on */
		break;
	}

	if (agtail(e0) == aghead(e0)) {
	    int b, sizey, r;
	    n = agtail(e0);
	    r = ND_rank(n);
	    if (r == GD_maxrank(g)) {
		if (r > 0)
		    sizey = ND_coord(GD_rank(g)[r-1].v[0]).y - ND_coord(n).y;
		else
		    sizey = ND_ht(n);
	    }
	    else if (r == GD_minrank(g)) {
		sizey = ND_coord(n).y - ND_coord(GD_rank(g)[r+1].v[0]).y;
	    }
	    else {
		int upy = ND_coord(GD_rank(g)[r-1].v[0]).y - ND_coord(n).y;
		int dwny = ND_coord(n).y - ND_coord(GD_rank(g)[r+1].v[0]).y;
		sizey = MIN(upy, dwny);
	    }
	    makeSelfEdge(P, edges, ind, cnt, sd.Multisep, sizey/2, &sinfo);
	    for (b = 0; b < cnt; b++) {
		e = edges[ind+b];
		if (ED_label(e))
		    updateBB(g, ED_label(e));
	    }
	}
	else if (ND_rank(agtail(e0)) == ND_rank(aghead(e0))) {
	    make_flat_edge(&sd, P, edges, ind, cnt, et);
	}
	else
	    make_regular_edge(&sd, P, edges, ind, cnt, et);
    }

    /* place regular edge labels */
    for (n = GD_nlist(g); n; n = ND_next(n)) {
	if ((ND_node_type(n) == VIRTUAL) && (ND_label(n))) {
	    place_vnlabel(n);
	    updateBB(g, ND_label(n));
	}
    }

    /* normalize splines so they always go from tail to head */
    /* place_portlabel relies on this being done first */
    if (normalize)
	edge_normalize(g);

finish :
    /* vladimir: place port labels */
    /* FIX: head and tail labels are not part of cluster bbox */
    if ((E_headlabel || E_taillabel) && (E_labelangle || E_labeldistance)) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (E_headlabel) {
		for (e = agfstin(g, n); e; e = agnxtin(g, e))
#ifndef WITH_CGRAPH
		    if (ED_head_label(e)) {
			if (place_portlabel(e, TRUE))
			    updateBB(g, ED_head_label(e));
		    }
#else
		    if (ED_head_label(AGMKOUT(e))) {
			place_portlabel(AGMKOUT(e), TRUE);
			updateBB(g, ED_head_label(AGMKOUT(e)));
		    }
#endif

	    }
	    if (E_taillabel) {
		for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
		    if (ED_tail_label(e)) {
			if (place_portlabel(e, FALSE))
			    updateBB(g, ED_tail_label(e));
		    }
		}
	    }
	}
    }
    /* end vladimir */

#ifdef ORTHO
    if ((et != ET_ORTHO) && (et != ET_CURVED))  {
#else
    if (et != ET_CURVED) {
#endif
	free(edges);
	free(P->boxes);
	free(P);
	free(sd.Rank_box);
	routesplinesterm();
    } 
    State = GVSPLINES;
    EdgeLabelsDone = 1;
}

/* dot_splines:
 * If the splines attribute is defined but equal to "", skip edge routing.
 */
void dot_splines(graph_t * g)
{
    _dot_splines (g, 1);
}

/* place_vnlabel:
 * assign position of an edge label from its virtual node
 * This is for regular edges only.
 */
static void 
place_vnlabel(node_t * n)
{
    pointf dimen;
    double width;
    edge_t *e;
    if (ND_in(n).size == 0)
	return;			/* skip flat edge labels here */
    for (e = ND_out(n).list[0]; ED_edge_type(e) != NORMAL;
	 e = ED_to_orig(e));
    dimen = ED_label(e)->dimen;
    width = GD_flip(agraphof(n)) ? dimen.y : dimen.x;
    ED_label(e)->pos.x = ND_coord(n).x + width / 2.0;
    ED_label(e)->pos.y = ND_coord(n).y;
    ED_label(e)->set = TRUE;
}

static void 
setflags(edge_t *e, int hint1, int hint2, int f3)
{
    int f1, f2;
    if (hint1 != 0)
	f1 = hint1;
    else {
	if (agtail(e) == aghead(e))
	    if (ED_tail_port(e).defined || ED_head_port(e).defined)
		f1 = SELFWPEDGE;
	    else
		f1 = SELFNPEDGE;
	else if (ND_rank(agtail(e)) == ND_rank(aghead(e)))
	    f1 = FLATEDGE;
	else
	    f1 = REGULAREDGE;
    }
    if (hint2 != 0)
	f2 = hint2;
    else {
	if (f1 == REGULAREDGE)
	    f2 = (ND_rank(agtail(e)) < ND_rank(aghead(e))) ? FWDEDGE : BWDEDGE;
	else if (f1 == FLATEDGE)
	    f2 = (ND_order(agtail(e)) < ND_order(aghead(e))) ?  FWDEDGE : BWDEDGE;
	else			/* f1 == SELF*EDGE */
	    f2 = FWDEDGE;
    }
    ED_tree_index(e) = (f1 | f2 | f3);
}

/* edgecmp:
 * lexicographically order edges by
 *  - edge type
 *  - |rank difference of nodes|
 *  - |x difference of nodes|
 *  - id of witness edge for equivalence class
 *  - port comparison
 *  - graph type
 *  - labels if flat edges
 *  - edge id
 */
static int edgecmp(edge_t** ptr0, edge_t** ptr1)
{
#ifndef WITH_CGRAPH
    edge_t fwdedgea, fwdedgeb, *e0, *e1, *ea, *eb, *le0, *le1;
#else
    Agedgeinfo_t fwdedgeai, fwdedgebi;
    Agedgepair_t fwdedgea, fwdedgeb;
    edge_t *e0, *e1, *ea, *eb, *le0, *le1;
#endif
    int et0, et1, v0, v1, rv;
    double t0, t1;

#ifdef WITH_CGRAPH
    fwdedgea.out.base.data = (Agrec_t*)&fwdedgeai;
    fwdedgeb.out.base.data = (Agrec_t*)&fwdedgebi;
#endif
    e0 = (edge_t *) * ptr0;
    e1 = (edge_t *) * ptr1;
    et0 = ED_tree_index(e0) & EDGETYPEMASK;
    et1 = ED_tree_index(e1) & EDGETYPEMASK;
    if (et0 != et1)
	return (et1 - et0);

    le0 = getmainedge(e0);
    le1 = getmainedge(e1);

    t0 = ND_rank(agtail(le0)) - ND_rank(aghead(le0));
    t1 = ND_rank(agtail(le1)) - ND_rank(aghead(le1));
    v0 = ABS((int)t0);   /* ugly, but explicit as to how we avoid equality tests on fp numbers */
    v1 = ABS((int)t1);
    if (v0 != v1)
	return (v0 - v1);

    t0 = ND_coord(agtail(le0)).x - ND_coord(aghead(le0)).x;
    t1 = ND_coord(agtail(le1)).x - ND_coord(aghead(le1)).x;
    v0 = ABS((int)t0);
    v1 = ABS((int)t1);
    if (v0 != v1)
	return (v0 - v1);

    /* This provides a cheap test for edges having the same set of endpoints.  */
    if (AGSEQ(le0) != AGSEQ(le1))
	return (AGSEQ(le0) - AGSEQ(le1));

    ea = (ED_tail_port(e0).defined || ED_head_port(e0).defined) ? e0 : le0;
    if (ED_tree_index(ea) & BWDEDGE) {
#ifndef WITH_CGRAPH
	MAKEFWDEDGE(&fwdedgea, ea);
	ea = &fwdedgea;
#else
	MAKEFWDEDGE(&fwdedgea.out, ea);
	ea = &fwdedgea.out;
#endif
    }
    eb = (ED_tail_port(e1).defined || ED_head_port(e1).defined) ? e1 : le1;
    if (ED_tree_index(eb) & BWDEDGE) {
#ifndef WITH_CGRAPH
	MAKEFWDEDGE(&fwdedgeb, eb);
	eb = &fwdedgeb;
#else
	MAKEFWDEDGE(&fwdedgeb.out, eb);
	eb = &fwdedgeb.out;
#endif
    }
    if ((rv = portcmp(ED_tail_port(ea), ED_tail_port(eb))))
	return rv;
    if ((rv = portcmp(ED_head_port(ea), ED_head_port(eb))))
	return rv;

    et0 = ED_tree_index(e0) & GRAPHTYPEMASK;
    et1 = ED_tree_index(e1) & GRAPHTYPEMASK;
    if (et0 != et1)
	return (et0 - et1);

    if (et0 == FLATEDGE && ED_label(e0) != ED_label(e1))
	return (int) (ED_label(e0) - ED_label(e1));

    return (AGSEQ(e0) - AGSEQ(e1));
}

/* cloneGraph:
 */
typedef struct {
    attrsym_t* E_constr;
    attrsym_t* E_samehead;
    attrsym_t* E_sametail;
    attrsym_t* E_weight;
    attrsym_t* E_minlen;
    attrsym_t* E_fontcolor;
    attrsym_t* E_fontname;
    attrsym_t* E_fontsize;
    attrsym_t* E_headclip;
    attrsym_t* E_headlabel;
    attrsym_t* E_label;
    attrsym_t* E_label_float;
    attrsym_t* E_labelfontcolor;
    attrsym_t* E_labelfontname;
    attrsym_t* E_labelfontsize;
    attrsym_t* E_tailclip;
    attrsym_t* E_taillabel;
    attrsym_t* E_xlabel;

    attrsym_t* N_height;
    attrsym_t* N_width;
    attrsym_t* N_shape;
    attrsym_t* N_style;
    attrsym_t* N_fontsize;
    attrsym_t* N_fontname;
    attrsym_t* N_fontcolor;
    attrsym_t* N_label;
    attrsym_t* N_xlabel;
    attrsym_t* N_showboxes;
    attrsym_t* N_ordering;
    attrsym_t* N_sides;
    attrsym_t* N_peripheries;
    attrsym_t* N_skew;
    attrsym_t* N_orientation;
    attrsym_t* N_distortion;
    attrsym_t* N_fixed;
    attrsym_t* N_nojustify;
    attrsym_t* N_group;

    attrsym_t* G_ordering;
    int        State;
} attr_state_t;

static void
setState (graph_t* auxg, attr_state_t* attr_state)
{
    /* save state */
    attr_state->E_constr = E_constr;
    attr_state->E_samehead = E_samehead;
    attr_state->E_sametail = E_sametail;
    attr_state->E_weight = E_weight;
    attr_state->E_minlen = E_minlen;
    attr_state->E_fontcolor = E_fontcolor;
    attr_state->E_fontname = E_fontname;
    attr_state->E_fontsize = E_fontsize;
    attr_state->E_headclip = E_headclip;
    attr_state->E_headlabel = E_headlabel;
    attr_state->E_label = E_label;
    attr_state->E_label_float = E_label_float;
    attr_state->E_labelfontcolor = E_labelfontcolor;
    attr_state->E_labelfontname = E_labelfontname;
    attr_state->E_labelfontsize = E_labelfontsize;
    attr_state->E_tailclip = E_tailclip;
    attr_state->E_taillabel = E_taillabel;
    attr_state->E_xlabel = E_xlabel;
    attr_state->N_height = N_height;
    attr_state->N_width = N_width;
    attr_state->N_shape = N_shape;
    attr_state->N_style = N_style;
    attr_state->N_fontsize = N_fontsize;
    attr_state->N_fontname = N_fontname;
    attr_state->N_fontcolor = N_fontcolor;
    attr_state->N_label = N_label;
    attr_state->N_xlabel = N_xlabel;
    attr_state->N_showboxes = N_showboxes;
    attr_state->N_ordering = N_ordering;
    attr_state->N_sides = N_sides;
    attr_state->N_peripheries = N_peripheries;
    attr_state->N_skew = N_skew;
    attr_state->N_orientation = N_orientation;
    attr_state->N_distortion = N_distortion;
    attr_state->N_fixed = N_fixed;
    attr_state->N_nojustify = N_nojustify;
    attr_state->N_group = N_group;
    attr_state->State = State;
    attr_state->G_ordering = G_ordering;

    E_constr = NULL;
#ifndef WITH_CGRAPH
    E_samehead = agfindattr(auxg->proto->e, "samehead");
    E_sametail = agfindattr(auxg->proto->e, "sametail");
    E_weight = agfindattr(auxg->proto->e, "weight");
#else /* WITH_CGRAPH */
    E_samehead = agattr(auxg,AGEDGE, "samehead", NULL);
    E_sametail = agattr(auxg,AGEDGE, "sametail", NULL);
    E_weight = agattr(auxg,AGEDGE, "weight", NULL);
#endif /* WITH_CGRAPH */
    if (!E_weight)
#ifndef WITH_CGRAPH
	E_weight = agedgeattr (auxg, "weight", "");
#else /* WITH_CGRAPH */
	E_weight = agattr (auxg,AGEDGE,"weight", "");
#endif /* WITH_CGRAPH */
    E_minlen = NULL;
    E_fontcolor = NULL;
    E_fontname = agfindedgeattr(auxg, "fontname");
    E_fontsize = agfindedgeattr(auxg, "fontsize");
    E_headclip = agfindedgeattr(auxg, "headclip");
    E_headlabel = NULL;
    E_label = agfindedgeattr(auxg, "label");
    E_label_float = agfindedgeattr(auxg, "label_float");
    E_labelfontcolor = NULL;
    E_labelfontname = agfindedgeattr(auxg, "labelfontname");
    E_labelfontsize = agfindedgeattr(auxg, "labelfontsize");
    E_tailclip = agfindedgeattr(auxg, "tailclip");
    E_taillabel = NULL;
    E_xlabel = NULL;
    N_height = agfindnodeattr(auxg, "height");
    N_width = agfindnodeattr(auxg, "width");
    N_shape = agfindnodeattr(auxg, "shape");
    N_style = NULL;
    N_fontsize = agfindnodeattr(auxg, "fontsize");
    N_fontname = agfindnodeattr(auxg, "fontname");
    N_fontcolor = NULL;
    N_label = agfindnodeattr(auxg, "label");
    N_xlabel = NULL;
    N_showboxes = NULL;
    N_ordering = agfindnodeattr(auxg, "ordering");
    N_sides = agfindnodeattr(auxg, "sides");
    N_peripheries = agfindnodeattr(auxg, "peripheries");
    N_skew = agfindnodeattr(auxg, "skew");
    N_orientation = agfindnodeattr(auxg, "orientation");
    N_distortion = agfindnodeattr(auxg, "distortion");
    N_fixed = agfindnodeattr(auxg, "fixed");
    N_nojustify = NULL;
    N_group = NULL;
    G_ordering = agfindgraphattr (auxg, "ordering");
}

/* cloneGraph:
 * Create clone graph. It stores the global Agsyms, to be
 * restored in cleanupCloneGraph. The graph uses the main
 * graph's settings for certain geometry parameters, and
 * declares all node and edge attributes used in the original
 * graph.
 */
static graph_t*
cloneGraph (graph_t* g, attr_state_t* attr_state)
{
    Agsym_t* sym;
    graph_t* auxg;
#ifndef WITH_CGRAPH
    Agsym_t **list;

    auxg = agopen ("auxg", AG_IS_DIRECTED(g)?AGDIGRAPH:AGRAPH);
    agraphattr(auxg, "rank", "");
#else /* WITH_CGRAPH */
    if (agisdirected(g))
	auxg = agopen ("auxg",Agdirected, NIL(Agdisc_t *));
    else
	auxg = agopen ("auxg",Agundirected, NIL(Agdisc_t *));
    agbindrec(auxg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);
    agattr(auxg, AGRAPH, "rank", "");
#endif /* WITH_CGRAPH */
    GD_drawing(auxg) = NEW(layout_t);
    GD_drawing(auxg)->quantum = GD_drawing(g)->quantum; 
    GD_drawing(auxg)->dpi = GD_drawing(g)->dpi;

    GD_charset(auxg) = GD_charset (g);
    if (GD_flip(g))
	SET_RANKDIR(auxg, RANKDIR_TB);
    else
	SET_RANKDIR(auxg, RANKDIR_LR);
    GD_nodesep(auxg) = GD_nodesep(g);
    GD_ranksep(auxg) = GD_ranksep(g);

#ifndef WITH_CGRAPH
    list = g->root->univ->nodeattr->list;
    while ((sym = *list++)) {
	agnodeattr (auxg, sym->name, sym->value);
#else /* WITH_CGRAPH */
	//copy node attrs to auxg
//	list = g->root->univ->nodeattr->list;
	sym=agnxtattr(agroot(g),AGNODE,NULL); //get the first attr.
	while ((sym = agnxtattr(agroot(g),AGNODE,sym))) {
		agattr (auxg, AGNODE,sym->name, sym->defval	);
#endif /* WITH_CGRAPH */
    }

#ifndef WITH_CGRAPH
    list = g->root->univ->edgeattr->list;
    while ((sym = *list++)) {
	agedgeattr (auxg, sym->name, sym->value);
#else /* WITH_CGRAPH */
	//copy edge attributes
	sym=agnxtattr(agroot(g),AGEDGE,NULL); //get the first attr.
	while ((sym = agnxtattr(agroot(g),AGEDGE,sym))) {
	agattr (auxg, AGEDGE,sym->name, sym->defval);
#endif /* WITH_CGRAPH */
    }

#ifndef WITH_CGRAPH
    if (!agfindattr(auxg->proto->e, "headport"))
	agedgeattr (auxg, "headport", "");
    if (!agfindattr(auxg->proto->e, "tailport"))
	agedgeattr (auxg, "tailport", "");
#else /* WITH_CGRAPH */
    if (!agattr(auxg,AGEDGE, "headport", NULL))
	agattr(auxg,AGEDGE, "headport", "");
    if (!agattr(auxg,AGEDGE, "tailport", NULL))
	agattr(auxg,AGEDGE, "tailport", "");
#endif /* WITH_CGRAPH */

    setState (auxg, attr_state);

    return auxg;
}

/* cleanupCloneGraph:
 */
static void
cleanupCloneGraph (graph_t* g, attr_state_t* attr_state)
{
    /* restore main graph syms */
    E_constr = attr_state->E_constr;
    E_samehead = attr_state->E_samehead;
    E_sametail = attr_state->E_sametail;
    E_weight = attr_state->E_weight;
    E_minlen = attr_state->E_minlen;
    E_fontcolor = attr_state->E_fontcolor;
    E_fontname = attr_state->E_fontname;
    E_fontsize = attr_state->E_fontsize;
    E_headclip = attr_state->E_headclip;
    E_headlabel = attr_state->E_headlabel;
    E_label = attr_state->E_label;
    E_label_float = attr_state->E_label_float;
    E_labelfontcolor = attr_state->E_labelfontcolor;
    E_labelfontname = attr_state->E_labelfontname;
    E_labelfontsize = attr_state->E_labelfontsize;
    E_tailclip = attr_state->E_tailclip;
    E_taillabel = attr_state->E_taillabel;
    E_xlabel = attr_state->E_xlabel;
    N_height = attr_state->N_height;
    N_width = attr_state->N_width;
    N_shape = attr_state->N_shape;
    N_style = attr_state->N_style;
    N_fontsize = attr_state->N_fontsize;
    N_fontname = attr_state->N_fontname;
    N_fontcolor = attr_state->N_fontcolor;
    N_label = attr_state->N_label;
    N_xlabel = attr_state->N_xlabel;
    N_showboxes = attr_state->N_showboxes;
    N_ordering = attr_state->N_ordering;
    N_sides = attr_state->N_sides;
    N_peripheries = attr_state->N_peripheries;
    N_skew = attr_state->N_skew;
    N_orientation = attr_state->N_orientation;
    N_distortion = attr_state->N_distortion;
    N_fixed = attr_state->N_fixed;
    N_nojustify = attr_state->N_nojustify;
    N_group = attr_state->N_group;
    G_ordering = attr_state->G_ordering;
    State = attr_state->State;

    free (attr_state);
    dot_cleanup(g);
    agclose(g);
}

/* cloneNode:
 * If flipped is true, original graph has rankdir=LR or RL.
 * In this case, records change shape, so we wrap a record node's
 * label in "{...}" to prevent this.
 */
static node_t*
cloneNode (graph_t* g, node_t* orign, int flipped)
{
#ifndef WITH_CGRAPH
    node_t* n = agnode(g, orign->name);
#else /* WITH_CGRAPH */
    node_t* n = agnode(g, agnameof(orign),1);
    agbindrec(n, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);
#endif /* WITH_CGRAPH */
    agcopyattr (orign, n);
    if (shapeOf(orign) == SH_RECORD) {
	int lbllen = strlen(ND_label(orign)->text);
        char* buf = N_GNEW(lbllen+3,char);
        sprintf (buf, "{%s}", ND_label(orign)->text);
	agset (n, "label", buf);
    }

    return n;
}

/* cloneEdge:
 */
static edge_t*
cloneEdge (graph_t* g, node_t* tn, node_t* hn, edge_t* orig)
{
#ifndef WITH_CGRAPH
    edge_t* e = agedge(g, tn, hn);
#else /* WITH_CGRAPH */
    edge_t* e = agedge(g, tn, hn,NULL,1);
#endif /* WITH_CGRAPH */
    /* for (; ED_edge_type(orig) != NORMAL; orig = ED_to_orig(orig)); */
    agcopyattr (orig, e);
/*
    if (orig->tail != ND_alg(tn)) {
	char* hdport = agget (orig, HEAD_ID);
	char* tlport = agget (orig, TAIL_ID);
	agset (e, TAIL_ID, (hdport ? hdport : ""));
	agset (e, HEAD_ID, (tlport ? tlport : ""));
    }
*/

    return e;
}

/* transformf:
 * Rotate, if necessary, then translate points.
 */
static pointf
transformf (pointf p, pointf del, int flip)
{
    if (flip) {
	double i = p.x;
	p.x = p.y;
	p.y = -i;
    }
    return add_pointf(p, del);
}

/* edgelblcmpfn:
 * lexicographically order edges by
 *  - has label
 *  - label is wider
 *  - label is higher
 */
static int edgelblcmpfn(edge_t** ptr0, edge_t** ptr1)
{
    edge_t *e0, *e1;
    pointf sz0, sz1;

    e0 = (edge_t *) * ptr0;
    e1 = (edge_t *) * ptr1;

    if (ED_label(e0)) {
	if (ED_label(e1)) {
	    sz0 = ED_label(e0)->dimen;
	    sz1 = ED_label(e1)->dimen;
	    if (sz0.x > sz1.x) return -1;
	    else if (sz0.x < sz1.x) return 1;
	    else if (sz0.y > sz1.y) return -1;
	    else if (sz0.y < sz1.y) return 1;
	    else return 0;
	}
	else
	    return -1;
    }
    else if (ED_label(e1)) {
 	return 1;
    }
    else
 	return 0;
}

#define LBL_SPACE  6  /* space between labels, in points */

/* makeSimpleFlatLabels:
 * This handles the second simplest case for flat edges between
 * two adjacent nodes. We still invoke a dot on a rotated problem
 * to handle edges with ports. This usually works, but fails for
 * records because of their weird nature.
 */
static void
makeSimpleFlatLabels (node_t* tn, node_t* hn, edge_t** edges, int ind, int cnt, int et, int n_lbls)
{
    pointf *ps;
    Ppoly_t poly;
    int pn;
    edge_t* e = edges[ind];
    pointf points[10], tp, hp;
    int i, pointn;
    double leftend, rightend, ctrx, ctry, miny, maxy;
    double uminx, umaxx;
    double lminx, lmaxx;

    edge_t** earray = N_NEW(cnt, edge_t*);

    for (i = 0; i < cnt; i++) {
	earray[i] = edges[ind + i];
    }

    qsort (earray, cnt, sizeof(edge_t*), (qsort_cmpf) edgelblcmpfn);

    tp = add_pointf(ND_coord(tn), ED_tail_port(e).p);
    hp = add_pointf(ND_coord(hn), ED_head_port(e).p);

    leftend = tp.x+ND_rw(tn);
    rightend = hp.x-ND_lw(hn);
    ctrx = (leftend + rightend)/2.0;
    
    /* do first edge */
    e = earray[0];
    pointn = 0;
    points[pointn++] = tp;
    points[pointn++] = tp;
    points[pointn++] = hp;
    points[pointn++] = hp;
#ifndef WITH_CGRAPH
    clip_and_install(e, e->head, points, pointn, &sinfo);
#else /* WITH_CGRAPH */
    clip_and_install(e, aghead(e), points, pointn, &sinfo);
#endif /* WITH_CGRAPH */
    ED_label(e)->pos.x = ctrx;
    ED_label(e)->pos.y = tp.y + (ED_label(e)->dimen.y+LBL_SPACE)/2.0;
    ED_label(e)->set = TRUE;

    miny = tp.y + LBL_SPACE/2.0;
    maxy = miny + ED_label(e)->dimen.y;
    uminx = ctrx - (ED_label(e)->dimen.x)/2.0;
    umaxx = ctrx + (ED_label(e)->dimen.x)/2.0;

    for (i = 1; i < n_lbls; i++) {
	e = earray[i];
	if (i%2) {  /* down */
	    if (i == 1) {
		lminx = ctrx - (ED_label(e)->dimen.x)/2.0;
		lmaxx = ctrx + (ED_label(e)->dimen.x)/2.0;
	    }
	    miny -= LBL_SPACE + ED_label(e)->dimen.y;
	    points[0] = tp;
	    points[1].x = tp.x;
	    points[1].y = miny - LBL_SPACE;
	    points[2].x = hp.x;
	    points[2].y = points[1].y;
	    points[3] = hp;
	    points[4].x = lmaxx;
	    points[4].y = hp.y;
	    points[5].x = lmaxx;
	    points[5].y = miny;
	    points[6].x = lminx;
	    points[6].y = miny;
	    points[7].x = lminx;
	    points[7].y = tp.y;
	    ctry = miny + (ED_label(e)->dimen.y)/2.0;
	}
	else {   /* up */
	    points[0] = tp;
	    points[1].x = uminx;
	    points[1].y = tp.y;
	    points[2].x = uminx;
	    points[2].y = maxy;
	    points[3].x = umaxx;
	    points[3].y = maxy;
	    points[4].x = umaxx;
	    points[4].y = hp.y;
	    points[5].x = hp.x;
	    points[5].y = hp.y;
	    points[6].x = hp.x;
	    points[6].y = maxy + LBL_SPACE;
	    points[7].x = tp.x;
	    points[7].y = maxy + LBL_SPACE;
	    ctry =  maxy + (ED_label(e)->dimen.y)/2.0 + LBL_SPACE;
	    maxy += ED_label(e)->dimen.y + LBL_SPACE;
	}
	poly.pn = 8;
	poly.ps = (Ppoint_t*)points;
	ps = simpleSplineRoute (tp, hp, poly, &pn, et == ET_PLINE);
	if (pn == 0) return;
	ED_label(e)->pos.x = ctrx;
	ED_label(e)->pos.y = ctry;
	ED_label(e)->set = TRUE;
#ifndef WITH_CGRAPH
	clip_and_install(e, e->head, ps, pn, &sinfo);
#else /* WITH_CGRAPH */
	clip_and_install(e, aghead(e), ps, pn, &sinfo);
#endif /* WITH_CGRAPH */
    }

    /* edges with no labels */
    for (; i < cnt; i++) {
	e = earray[i];
	if (i%2) {  /* down */
	    if (i == 1) {
		lminx = (2*leftend + rightend)/3.0;
		lmaxx = (leftend + 2*rightend)/3.0;
	    }
	    miny -= LBL_SPACE;
	    points[0] = tp;
	    points[1].x = tp.x;
	    points[1].y = miny - LBL_SPACE;
	    points[2].x = hp.x;
	    points[2].y = points[1].y;
	    points[3] = hp;
	    points[4].x = lmaxx;
	    points[4].y = hp.y;
	    points[5].x = lmaxx;
	    points[5].y = miny;
	    points[6].x = lminx;
	    points[6].y = miny;
	    points[7].x = lminx;
	    points[7].y = tp.y;
	}
	else {   /* up */
	    points[0] = tp;
	    points[1].x = uminx;
	    points[1].y = tp.y;
	    points[2].x = uminx;
	    points[2].y = maxy;
	    points[3].x = umaxx;
	    points[3].y = maxy;
	    points[4].x = umaxx;
	    points[4].y = hp.y;
	    points[5].x = hp.x;
	    points[5].y = hp.y;
	    points[6].x = hp.x;
	    points[6].y = maxy + LBL_SPACE;
	    points[7].x = tp.x;
	    points[7].y = maxy + LBL_SPACE;
	    maxy += + LBL_SPACE;
	}
	poly.pn = 8;
	poly.ps = (Ppoint_t*)points;
	ps = simpleSplineRoute (tp, hp, poly, &pn, et == ET_PLINE);
	if (pn == 0) return;
#ifndef WITH_CGRAPH
	clip_and_install(e, e->head, ps, pn, &sinfo);
#else /* WITH_CGRAPH */
	clip_and_install(e, aghead(e), ps, pn, &sinfo);
#endif /* WITH_CGRAPH */
    }
   
    free (earray);
}

/* makeSimpleFlat:
 */
static void
makeSimpleFlat (node_t* tn, node_t* hn, edge_t** edges, int ind, int cnt, int et)
{
    edge_t* e = edges[ind];
    pointf points[10], tp, hp;
    int i, pointn;
    double stepy, dy;

    tp = add_pointf(ND_coord(tn), ED_tail_port(e).p);
    hp = add_pointf(ND_coord(hn), ED_head_port(e).p);

    stepy = (cnt > 1) ? ND_ht(tn) / (double)(cnt - 1) : 0.;
    dy = tp.y - ((cnt > 1) ? ND_ht(tn) / 2. : 0.);

    for (i = 0; i < cnt; i++) {
	e = edges[ind + i];
	pointn = 0;
	if ((et == ET_SPLINE) || (et == ET_LINE)) {
	    points[pointn++] = tp;
	    points[pointn++] = pointfof((2 * tp.x + hp.x) / 3, dy);
	    points[pointn++] = pointfof((2 * hp.x + tp.x) / 3, dy);
	    points[pointn++] = hp;
	}
	else {   /* ET_PLINE */
	    points[pointn++] = tp;
	    points[pointn++] = tp;
	    points[pointn++] = pointfof((2 * tp.x + hp.x) / 3, dy);
	    points[pointn++] = pointfof((2 * tp.x + hp.x) / 3, dy);
	    points[pointn++] = pointfof((2 * tp.x + hp.x) / 3, dy);
	    points[pointn++] = pointfof((2 * hp.x + tp.x) / 3, dy);
	    points[pointn++] = pointfof((2 * hp.x + tp.x) / 3, dy);
	    points[pointn++] = pointfof((2 * hp.x + tp.x) / 3, dy);
	    points[pointn++] = hp;
	    points[pointn++] = hp;
	}
	dy += stepy;
#ifndef WITH_CGRAPH
	clip_and_install(e, e->head, points, pointn, &sinfo);
#else /* WITH_CGRAPH */
	clip_and_install(e, aghead(e), points, pointn, &sinfo);
#endif /* WITH_CGRAPH */
    }
}

/* make_flat_adj_edges:
 * In the simple case, with no labels or ports, this creates a simple
 * spindle of splines.
 * If there are only labels, cobble something together.
 * Otherwise, we run dot recursively on the 2 nodes and the edges, 
 * essentially using rankdir=LR, to get the needed spline info.
 * This is probably to cute and fragile, and should be rewritten in a 
 * more straightforward and laborious fashion. 
 */
static void
make_flat_adj_edges(path* P, edge_t** edges, int ind, int cnt, edge_t* e0,
                    int et)
{
    node_t* n;
    node_t *tn, *hn;
    edge_t* e;
    int labels = 0, ports = 0;
    graph_t* g;
    graph_t* auxg;
    graph_t* subg;
    node_t *auxt, *auxh;
    edge_t* auxe;
    int     i, j, midx, midy, leftx, rightx;
    pointf   del;
    edge_t* hvye = NULL;
    attr_state_t* attrs;

    g = agraphof(agtail(e0));
    tn = agtail(e0), hn = aghead(e0);
    for (i = 0; i < cnt; i++) {
	e = edges[ind + i];
	if (ED_label(e)) labels++;
	if (ED_tail_port(e).defined || ED_head_port(e).defined) ports = 1;
    }

    if (ports == 0) {
	/* flat edges without ports and labels can go straight left to right */
	if (labels == 0) {
	    makeSimpleFlat (tn, hn, edges, ind, cnt, et);
	}
	/* flat edges without ports but with labels take more work */
	else {
	    makeSimpleFlatLabels (tn, hn, edges, ind, cnt, et, labels);
	}
	return;
    }

    attrs = NEW(attr_state_t);
    auxg = cloneGraph (g, attrs);
#ifndef WITH_CGRAPH
    subg = agsubg (auxg, "xxx");
#else /* WITH_CGRAPH */
    subg = agsubg (auxg, "xxx",1);
    agbindrec(subg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);
#endif /* WITH_CGRAPH */
    agset (subg, "rank", "source");
    rightx = ND_coord(hn).x;
    leftx = ND_coord(tn).x;
    if (GD_flip(g)) {
        node_t* n;
        n = tn;
        tn = hn;
        hn = n;
    }
    auxt = cloneNode(subg, tn, GD_flip(g)); 
    auxh = cloneNode(auxg, hn, GD_flip(g)); 
    for (i = 0; i < cnt; i++) {
	e = edges[ind + i];
	for (; ED_edge_type(e) != NORMAL; e = ED_to_orig(e));
	if (agtail(e) == tn)
	    auxe = cloneEdge (auxg, auxt, auxh, e);
	else
	    auxe = cloneEdge (auxg, auxh, auxt, e);
	ED_alg(e) = auxe;
	if (!hvye && !ED_tail_port(e).defined && !ED_head_port(e).defined) {
	    hvye = auxe;
	    ED_alg(hvye) = e;
	}
    }
    if (!hvye) {
#ifndef WITH_CGRAPH
	hvye = agedge (auxg, auxt, auxh);
#else /* WITH_CGRAPH */
	hvye = agedge (auxg, auxt, auxh,NULL,1);
#endif /* WITH_CGRAPH */
    }
#ifndef WITH_CGRAPH
    agxset (hvye, E_weight->index, "10000");
#else /* WITH_CGRAPH */
    agxset (hvye, E_weight, "10000");
#endif /* WITH_CGRAPH */
    GD_gvc(auxg) = GD_gvc(g);
    setEdgeType (auxg, et);
    dot_init_node_edge(auxg);

    dot_rank(auxg, 0);
    dot_mincross(auxg, 0);
    dot_position(auxg, 0);
    
    /* reposition */
    midx = (ND_coord(tn).x - ND_rw(tn) + ND_coord(hn).x + ND_lw(hn))/2;
    midy = (ND_coord(auxt).x + ND_coord(auxh).x)/2;
    for (n = GD_nlist(auxg); n; n = ND_next(n)) {
	if (n == auxt) {
	    ND_coord(n).y = rightx;
	    ND_coord(n).x = midy;
	}
	else if (n == auxh) {
	    ND_coord(n).y = leftx;
	    ND_coord(n).x = midy;
	}
	else ND_coord(n).y = midx;
    }
    dot_sameports(auxg);
    _dot_splines(auxg, 0);
    dotneato_postprocess(auxg);

       /* copy splines */
    if (GD_flip(g)) {
	del.x = ND_coord(tn).x - ND_coord(auxt).y;
	del.y = ND_coord(tn).y + ND_coord(auxt).x;
    }
    else {
	del.x = ND_coord(tn).x - ND_coord(auxt).x;
	del.y = ND_coord(tn).y - ND_coord(auxt).y;
    }
    for (i = 0; i < cnt; i++) {
	bezier* auxbz;
	bezier* bz;

	e = edges[ind + i];
	for (; ED_edge_type(e) != NORMAL; e = ED_to_orig(e));
	auxe = (edge_t*)ED_alg(e);
	if ((auxe == hvye) & !ED_alg(auxe)) continue; /* pseudo-edge */
	auxbz = ED_spl(auxe)->list;
	bz = new_spline(e, auxbz->size);
	bz->sflag = auxbz->sflag;
	bz->sp = transformf(auxbz->sp, del, GD_flip(g));
	bz->eflag = auxbz->eflag;
	bz->ep = transformf(auxbz->ep, del, GD_flip(g));
	for (j = 0; j <  auxbz->size; ) {
	    pointf cp[4];
	    cp[0] = bz->list[j] = transformf(auxbz->list[j], del, GD_flip(g));
	    j++;
	    if ( j >= auxbz->size ) 
		break;
	    cp[1] = bz->list[j] = transformf(auxbz->list[j], del, GD_flip(g));
	    j++;
	    cp[2] = bz->list[j] = transformf(auxbz->list[j], del, GD_flip(g));
	    j++;
	    cp[3] = transformf(auxbz->list[j], del, GD_flip(g));
	    update_bb_bz(&GD_bb(g), cp);
        }
	if (ED_label(e)) {
	    ED_label(e)->pos = transformf(ED_label(auxe)->pos, del, GD_flip(g));
	    ED_label(e)->set = TRUE;
	    updateBB(g, ED_label(e));
	}
    }

    cleanupCloneGraph (auxg, attrs);
}

/* makeFlatEnd;
 */
static void
makeFlatEnd (spline_info_t* sp, path* P, node_t* n, edge_t* e, pathend_t* endp,
             boolean isBegin)
{
    boxf b;
    graph_t* g = agraphof(n);

    b = endp->nb = maximal_bbox(sp, n, NULL, e);
    endp->sidemask = TOP;
    if (isBegin) beginpath(P, e, FLATEDGE, endp, FALSE);
    else endpath(P, e, FLATEDGE, endp, FALSE);
    b.UR.y = endp->boxes[endp->boxn - 1].UR.y;
    b.LL.y = endp->boxes[endp->boxn - 1].LL.y;
    b = makeregularend(b, TOP, ND_coord(n).y + GD_rank(g)[ND_rank(n)].ht2);
    if (b.LL.x < b.UR.x && b.LL.y < b.UR.y)
	endp->boxes[endp->boxn++] = b;
}
/* makeBottomFlatEnd;
 */
static void
makeBottomFlatEnd (spline_info_t* sp, path* P, node_t* n, edge_t* e, 
	pathend_t* endp, boolean isBegin)
{
    boxf b;
    graph_t* g = agraphof(n);

    b = endp->nb = maximal_bbox(sp, n, NULL, e);
    endp->sidemask = BOTTOM;
    if (isBegin) beginpath(P, e, FLATEDGE, endp, FALSE);
    else endpath(P, e, FLATEDGE, endp, FALSE);
    b.UR.y = endp->boxes[endp->boxn - 1].UR.y;
    b.LL.y = endp->boxes[endp->boxn - 1].LL.y;
    b = makeregularend(b, BOTTOM, ND_coord(n).y - GD_rank(g)[ND_rank(n)].ht2);
    if (b.LL.x < b.UR.x && b.LL.y < b.UR.y)
	endp->boxes[endp->boxn++] = b;
}


/* make_flat_labeled_edge:
 */
static void
make_flat_labeled_edge(spline_info_t* sp, path* P, edge_t* e, int et)
{
    graph_t *g;
    node_t *tn, *hn, *ln;
    pointf *ps;
    pathend_t tend, hend;
    boxf lb;
    int boxn, i, pn, ydelta;
    edge_t *f;
    pointf points[7];

    tn = agtail(e);
    hn = aghead(e);
    g = agraphof(tn);

    for (f = ED_to_virt(e); ED_to_virt(f); f = ED_to_virt(f));
    ln = agtail(f);
    ED_label(e)->pos = ND_coord(ln);
    ED_label(e)->set = TRUE;

    if (et == ET_LINE) {
	pointf startp, endp, lp;

	startp = add_pointf(ND_coord(tn), ED_tail_port(e).p);
	endp = add_pointf(ND_coord(hn), ED_head_port(e).p);

        lp = ED_label(e)->pos;
	lp.y -= (ED_label(e)->dimen.y)/2.0;
	points[1] = points[0] = startp;
	points[2] = points[3] = points[4] = lp;
	points[5] = points[6] = endp;
	ps = points;
	pn = 7;
    }
    else {
	lb.LL.x = ND_coord(ln).x - ND_lw(ln);
	lb.UR.x = ND_coord(ln).x + ND_rw(ln);
	lb.UR.y = ND_coord(ln).y + ND_ht(ln)/2;
	ydelta = ND_coord(ln).y - GD_rank(g)[ND_rank(tn)].ht1 -
		ND_coord(tn).y + GD_rank(g)[ND_rank(tn)].ht2;
	ydelta /= 6.;
	lb.LL.y = lb.UR.y - MAX(5.,ydelta); 

	boxn = 0;
	makeFlatEnd (sp, P, tn, e, &tend, TRUE);
	makeFlatEnd (sp, P, hn, e, &hend, FALSE);

	boxes[boxn].LL.x = tend.boxes[tend.boxn - 1].LL.x; 
	boxes[boxn].LL.y = tend.boxes[tend.boxn - 1].UR.y; 
	boxes[boxn].UR.x = lb.LL.x;
	boxes[boxn].UR.y = lb.LL.y;
	boxn++;
	boxes[boxn].LL.x = tend.boxes[tend.boxn - 1].LL.x; 
	boxes[boxn].LL.y = lb.LL.y;
	boxes[boxn].UR.x = hend.boxes[hend.boxn - 1].UR.x;
	boxes[boxn].UR.y = lb.UR.y;
	boxn++;
	boxes[boxn].LL.x = lb.UR.x;
	boxes[boxn].UR.y = lb.LL.y;
	boxes[boxn].LL.y = hend.boxes[hend.boxn - 1].UR.y; 
	boxes[boxn].UR.x = hend.boxes[hend.boxn - 1].UR.x;
	boxn++;

	for (i = 0; i < tend.boxn; i++) add_box(P, tend.boxes[i]);
	for (i = 0; i < boxn; i++) add_box(P, boxes[i]);
	for (i = hend.boxn - 1; i >= 0; i--) add_box(P, hend.boxes[i]);

	if (et == ET_SPLINE) ps = routesplines(P, &pn);
	else ps = routepolylines(P, &pn);
	if (pn == 0) return;
    }
#ifndef WITH_CGRAPH
    clip_and_install(e, e->head, ps, pn, &sinfo);
#else /* WITH_CGRAPH */
    clip_and_install(e, aghead(e), ps, pn, &sinfo);
#endif /* WITH_CGRAPH */
}

/* make_flat_bottom_edges:
 */
static void
make_flat_bottom_edges(spline_info_t* sp, path * P, edge_t ** edges, int 
	ind, int cnt, edge_t* e, int splines)
{
    node_t *tn, *hn;
    int j, i, r;
    double stepx, stepy, vspace;
    rank_t* nextr;
    int pn;
    pointf *ps;
    pathend_t tend, hend;
    graph_t* g;

    tn = agtail(e);
    hn = aghead(e);
    g = agraphof(tn);
    r = ND_rank(tn);
    if (r < GD_maxrank(g)) {
	nextr = GD_rank(g) + (r+1);
	vspace = ND_coord(tn).y - GD_rank(g)[r].pht1 -
		(ND_coord(nextr->v[0]).y + nextr->pht2);
    }
    else {
	vspace = GD_ranksep(g);
    }
    stepx = ((double)(sp->Multisep)) / (cnt+1); 
    stepy = vspace / (cnt+1);

    makeBottomFlatEnd (sp, P, tn, e, &tend, TRUE);
    makeBottomFlatEnd (sp, P, hn, e, &hend, FALSE);

    for (i = 0; i < cnt; i++) {
	int boxn;
	boxf b;
	e = edges[ind + i];
	boxn = 0;

	b = tend.boxes[tend.boxn - 1];
 	boxes[boxn].LL.x = b.LL.x; 
	boxes[boxn].UR.y = b.LL.y; 
	boxes[boxn].UR.x = b.UR.x + (i + 1) * stepx;
	boxes[boxn].LL.y = b.LL.y - (i + 1) * stepy;
	boxn++;
	boxes[boxn].LL.x = tend.boxes[tend.boxn - 1].LL.x; 
	boxes[boxn].UR.y = boxes[boxn-1].LL.y;
	boxes[boxn].UR.x = hend.boxes[hend.boxn - 1].UR.x;
	boxes[boxn].LL.y = boxes[boxn].UR.y - stepy;
	boxn++;
	b = hend.boxes[hend.boxn - 1];
	boxes[boxn].UR.x = b.UR.x;
	boxes[boxn].UR.y = b.LL.y;
	boxes[boxn].LL.x = b.LL.x - (i + 1) * stepx;
	boxes[boxn].LL.y = boxes[boxn-1].UR.y;
	boxn++;

	for (j = 0; j < tend.boxn; j++) add_box(P, tend.boxes[j]);
	for (j = 0; j < boxn; j++) add_box(P, boxes[j]);
	for (j = hend.boxn - 1; j >= 0; j--) add_box(P, hend.boxes[j]);

	if (splines) ps = routesplines(P, &pn);
	else ps = routepolylines(P, &pn);
	if (pn == 0)
	    return;
	clip_and_install(e, aghead(e), ps, pn, &sinfo);
	P->nbox = 0;
    }
}

/* make_flat_edge:
 * Construct flat edges edges[ind...ind+cnt-1]
 * There are 4 main cases:
 *  - all edges between a and b where a and b are adjacent 
 *  - one labeled edge
 *  - all non-labeled edges with identical ports between non-adjacent a and b 
 *     = connecting bottom to bottom/left/right - route along bottom
 *     = the rest - route along top
 */
static void
make_flat_edge(spline_info_t* sp, path * P, edge_t ** edges, int ind, int cnt, int et)
{
    node_t *tn, *hn;
#ifndef WITH_CGRAPH
    edge_t fwdedge, *e;
#else
    Agedgeinfo_t fwdedgei;
    Agedgepair_t fwdedge;
    edge_t *e;
#endif
    int j, i, r, isAdjacent;
    double stepx, stepy, vspace;
    int tside, hside, pn;
    pointf *ps;
    pathend_t tend, hend;
    graph_t* g;

#ifdef WITH_CGRAPH
    fwdedge.out.base.data = (Agrec_t*)&fwdedgei;
#endif

    /* Get sample edge; normalize to go from left to right */
    e = edges[ind];
    isAdjacent = ED_adjacent(e);
    if (ED_tree_index(e) & BWDEDGE) {
#ifndef WITH_CGRAPH
	MAKEFWDEDGE(&fwdedge, e);
	e = &fwdedge;
#else
	MAKEFWDEDGE(&fwdedge.out, e);
	e = &fwdedge.out;
#endif
    }
    for (i = 1; i < cnt; i++) {
	if (ED_adjacent(edges[ind+i])) {
	    isAdjacent = 1;
	    break;
	}
    }
    /* The lead edge edges[ind] might not have been marked earlier as adjacent,
     * so check them all.
     */
    if (isAdjacent) {
	make_flat_adj_edges (P, edges, ind, cnt, e, et);
	return;
    }
    if (ED_label(e)) {  /* edges with labels aren't multi-edges */
	make_flat_labeled_edge (sp, P, e, et);
	return;
    }

    if (et == ET_LINE) {
	makeSimpleFlat (agtail(e), aghead(e), edges, ind, cnt, et);
	return;
    }

    tside = ED_tail_port(e).side;
    hside = ED_head_port(e).side;
    if (((tside == BOTTOM) && (hside != TOP)) ||
        ((hside == BOTTOM) && (tside != TOP))) {
	make_flat_bottom_edges (sp, P, edges, ind, cnt, e, et == ET_SPLINE);
	return;
    }

    tn = agtail(e);
    hn = aghead(e);
    g = agraphof(tn);
    r = ND_rank(tn);
    if (r > 0) {
	rank_t* prevr;
	if (GD_has_labels(g) & EDGE_LABEL)
	    prevr = GD_rank(g) + (r-2);
	else
	    prevr = GD_rank(g) + (r-1);
	vspace = ND_coord(prevr->v[0]).y - prevr->ht1 - ND_coord(tn).y - GD_rank(g)[r].ht2;
    }
    else {
	vspace = GD_ranksep(g);
    }
    stepx = ((double)sp->Multisep) / (cnt+1); 
    stepy = vspace / (cnt+1);

    makeFlatEnd (sp, P, tn, e, &tend, TRUE);
    makeFlatEnd (sp, P, hn, e, &hend, FALSE);

    for (i = 0; i < cnt; i++) {
	int boxn;
	boxf b;
	e = edges[ind + i];
	boxn = 0;

	b = tend.boxes[tend.boxn - 1];
 	boxes[boxn].LL.x = b.LL.x; 
	boxes[boxn].LL.y = b.UR.y; 
	boxes[boxn].UR.x = b.UR.x + (i + 1) * stepx;
	boxes[boxn].UR.y = b.UR.y + (i + 1) * stepy;
	boxn++;
	boxes[boxn].LL.x = tend.boxes[tend.boxn - 1].LL.x; 
	boxes[boxn].LL.y = boxes[boxn-1].UR.y;
	boxes[boxn].UR.x = hend.boxes[hend.boxn - 1].UR.x;
	boxes[boxn].UR.y = boxes[boxn].LL.y + stepy;
	boxn++;
	b = hend.boxes[hend.boxn - 1];
	boxes[boxn].UR.x = b.UR.x;
	boxes[boxn].LL.y = b.UR.y;
	boxes[boxn].LL.x = b.LL.x - (i + 1) * stepx;
	boxes[boxn].UR.y = boxes[boxn-1].LL.y;
	boxn++;

	for (j = 0; j < tend.boxn; j++) add_box(P, tend.boxes[j]);
	for (j = 0; j < boxn; j++) add_box(P, boxes[j]);
	for (j = hend.boxn - 1; j >= 0; j--) add_box(P, hend.boxes[j]);

	if (et == ET_SPLINE) ps = routesplines(P, &pn);
	else ps = routepolylines(P, &pn);
	if (pn == 0)
	    return;
	clip_and_install(e, aghead(e), ps, pn, &sinfo);
	P->nbox = 0;
    }
}

/* ccw:
 * Return true if p3 is to left of ray p1->p2
 */
static int
leftOf (pointf p1, pointf p2, pointf p3)
{
    int d;

    d = ((p1.y - p2.y) * (p3.x - p2.x)) -
        ((p3.y - p2.y) * (p1.x - p2.x));
    return (d > 0);
}

/* makeLineEdge:
 * Create an edge as line segment. We guarantee that the points
 * are always drawn downwards. This means that for flipped edges,
 * we draw from the head to the tail. The routine returns the
 * end node of the edge in *hp. The points are stored in the
 * given array of points, and the number of points is returned.
 *
 * If the edge has a label, the edge is draw as two segments, with
 * the bend near the label.
 *
 * If the endpoints are on adjacent ranks, revert to usual code by
 * returning 0.
 * This is done because the usual code handles the interaction of
 * multiple edges better.
 */
static int 
makeLineEdge(edge_t* fe, pointf* points, node_t** hp)
{
    int delr, pn;
    node_t* hn;
    node_t* tn;
    edge_t* e = fe;
    pointf startp, endp, lp;
    pointf dimen;
    double width, height;

    while (ED_edge_type(e) != NORMAL)
	e = ED_to_orig(e);
    hn = aghead(e);
    tn = agtail(e);
    delr = ABS(ND_rank(hn)-ND_rank(tn));
    if ((delr == 1) || ((delr == 2) && (GD_has_labels(agraphof(hn)) & EDGE_LABEL)))
	return 0;
    if (agtail(fe) == agtail(e)) {
	*hp = hn;
	startp = add_pointf(ND_coord(tn), ED_tail_port(e).p);
	endp = add_pointf(ND_coord(hn), ED_head_port(e).p);
    }
    else {
 	*hp = tn; 
	startp = add_pointf(ND_coord(hn), ED_head_port(e).p);
	endp = add_pointf(ND_coord(tn), ED_tail_port(e).p);
    }

    if (ED_label(e)) {
	dimen = ED_label(e)->dimen;
	if (GD_flip(agraphof(hn))) {
	    width = dimen.y;
	    height = dimen.x;
	}
	else {
	    width = dimen.x;
	    height = dimen.y;
	}

	lp = ED_label(e)->pos, lp;
	if (leftOf (endp,startp,lp)) {
	    lp.x += width/2.0;
	    lp.y -= height/2.0;
	}    
	else {
	    lp.x -= width/2.0;
	    lp.y += height/2.0;
	}

	points[1] = points[0] = startp;
	points[2] = points[3] = points[4] = lp;
	points[5] = points[6] = endp;
	pn = 7;
    }
    else {
	points[1] = points[0] = startp;
	points[3] = points[2] = endp;
	pn = 4;
    }

    return pn;
}

#define NUMPTS 2000

/* make_regular_edge:
 */
static void
make_regular_edge(spline_info_t* sp, path * P, edge_t ** edges, int ind, int cnt, int et)
{
    graph_t *g;
    node_t *tn, *hn;
#ifndef WITH_CGRAPH
    edge_t fwdedgea, fwdedgeb, fwdedge, *e, *fe, *le, *segfirst;
#else
    Agedgeinfo_t fwdedgeai, fwdedgebi, fwdedgei;
    Agedgepair_t fwdedgea, fwdedgeb, fwdedge;
    edge_t *e, *fe, *le, *segfirst;
#endif /* WITH_CGRAPH */
    pointf *ps;
    pathend_t tend, hend;
    boxf b;
    int boxn, sl, si, smode, i, j, dx, pn, hackflag, longedge;
    static pointf* pointfs;
    static pointf* pointfs2;
    static int numpts;
    static int numpts2;
    int pointn;

#ifdef WITH_CGRAPH
    fwdedgea.out.base.data = (Agrec_t*)&fwdedgeai;
    fwdedgeb.out.base.data = (Agrec_t*)&fwdedgebi;
    fwdedge.out.base.data = (Agrec_t*)&fwdedgei;
#endif /* WITH_CGRAPH */

    if (!pointfs) {
	pointfs = N_GNEW(NUMPTS, pointf);
   	pointfs2 = N_GNEW(NUMPTS, pointf);
	numpts = NUMPTS;
	numpts2 = NUMPTS;
    }
    sl = 0;
    e = edges[ind];
    g = agraphof(agtail(e));
    hackflag = FALSE;
    if (ABS(ND_rank(agtail(e)) - ND_rank(aghead(e))) > 1) {
#ifndef WITH_CGRAPH
	fwdedgea = *e;
#else /* WITH_CGRAPH */
	fwdedgeai = *(Agedgeinfo_t*)e->base.data;
	fwdedgea.out = *e;
	fwdedgea.out.base.data = (Agrec_t*)&fwdedgeai;
#endif /* WITH_CGRAPH */
	if (ED_tree_index(e) & BWDEDGE) {
#ifndef WITH_CGRAPH
	    MAKEFWDEDGE(&fwdedgeb, e);
	    fwdedgea.tail = aghead(e);
	    fwdedgea.u.tail_port = ED_head_port(e);
#else /* WITH_CGRAPH */
	    MAKEFWDEDGE(&fwdedgeb.out, e);
	    agtail(&fwdedgea.out) = aghead(e);
	    ED_tail_port(&fwdedgea.out) = ED_head_port(e);
#endif /* WITH_CGRAPH */
	} else {
#ifndef WITH_CGRAPH
	    fwdedgeb = *e;
	    fwdedgea.tail = e->tail;
#else /* WITH_CGRAPH */
	    fwdedgebi = *(Agedgeinfo_t*)e->base.data;
	    fwdedgeb.out = *e;
	    fwdedgeb.out.base.data = (Agrec_t*)&fwdedgebi;
	    agtail(&fwdedgea.out) = agtail(e);
#endif /* WITH_CGRAPH */
	}
	le = getmainedge(e);
	while (ED_to_virt(le))
	    le = ED_to_virt(le);
#ifndef WITH_CGRAPH
	fwdedgea.head = aghead(le);
	fwdedgea.u.head_port.defined = FALSE;
	fwdedgea.u.edge_type = VIRTUAL;
	fwdedgea.u.head_port.p.x = fwdedgea.u.head_port.p.y = 0;
	fwdedgea.u.to_orig = e;
	e = &fwdedgea;
#else /* WITH_CGRAPH */
	aghead(&fwdedgea.out) = aghead(le);
	ED_head_port(&fwdedgea.out).defined = FALSE;
	ED_edge_type(&fwdedgea.out) = VIRTUAL;
	ED_head_port(&fwdedgea.out).p.x = ED_head_port(&fwdedgea.out).p.y = 0;
	ED_to_orig(&fwdedgea.out) = e;
	e = &fwdedgea.out;
#endif /* WITH_CGRAPH */
	hackflag = TRUE;
    } else {
	if (ED_tree_index(e) & BWDEDGE) {
#ifndef WITH_CGRAPH
	    MAKEFWDEDGE(&fwdedgea, e);
	    e = &fwdedgea;
#else
	    MAKEFWDEDGE(&fwdedgea.out, e);
	    e = &fwdedgea.out;
#endif
	}
    }
    fe = e;

    /* compute the spline points for the edge */

    if ((et == ET_LINE) && (pointn = makeLineEdge (fe, pointfs, &hn))) {
    }
    else {
	int splines = et == ET_SPLINE;
	boxn = 0;
	pointn = 0;
	segfirst = e;
	tn = agtail(e);
	hn = aghead(e);
	b = tend.nb = maximal_bbox(sp, tn, NULL, e);
	beginpath(P, e, REGULAREDGE, &tend, spline_merge(tn));
	b.UR.y = tend.boxes[tend.boxn - 1].UR.y;
	b.LL.y = tend.boxes[tend.boxn - 1].LL.y;
	b = makeregularend(b, BOTTOM,
	    	   ND_coord(tn).y - GD_rank(agraphof(tn))[ND_rank(tn)].ht1);
	if (b.LL.x < b.UR.x && b.LL.y < b.UR.y)
	    tend.boxes[tend.boxn++] = b;
	longedge = 0;
	smode = FALSE, si = -1;
	while (ND_node_type(hn) == VIRTUAL && !sinfo.splineMerge(hn)) {
	    longedge = 1;
	    boxes[boxn++] = rank_box(sp, g, ND_rank(tn));
	    if (!smode
	        && ((sl = straight_len(hn)) >=
	    	((GD_has_labels(g) & EDGE_LABEL) ? 4 + 1 : 2 + 1))) {
	        smode = TRUE;
	        si = 1, sl -= 2;
	    }
	    if (!smode || si > 0) {
	        si--;
	        boxes[boxn++] = maximal_bbox(sp, hn, e, ND_out(hn).list[0]);
	        e = ND_out(hn).list[0];
	        tn = agtail(e);
	        hn = aghead(e);
	        continue;
	    }
	    hend.nb = maximal_bbox(sp, hn, e, ND_out(hn).list[0]);
	    endpath(P, e, REGULAREDGE, &hend, spline_merge(aghead(e)));
	    b = makeregularend(hend.boxes[hend.boxn - 1], TOP,
	    	       ND_coord(hn).y + GD_rank(agraphof(hn))[ND_rank(hn)].ht2);
	    if (b.LL.x < b.UR.x && b.LL.y < b.UR.y)
	        hend.boxes[hend.boxn++] = b;
	    P->end.theta = M_PI / 2, P->end.constrained = TRUE;
	    completeregularpath(P, segfirst, e, &tend, &hend, boxes, boxn, 1);
	    if (splines) ps = routesplines(P, &pn);
	    else {
		ps = routepolylines (P, &pn);
		if ((et == ET_LINE) && (pn > 4)) {
		    ps[1] = ps[0];
		    ps[3] = ps[2] = ps[pn-1];
		    pn = 4;
		}
	    }
	    if (pn == 0)
	        return;
	
	    if (pointn + pn > numpts) {
                /* This should be enough to include 3 extra points added by
                 * straight_path below.
                 */
		numpts = 2*(pointn+pn); 
		pointfs = RALLOC(numpts, pointfs, pointf);
	    }
	    for (i = 0; i < pn; i++) {
		pointfs[pointn++] = ps[i];
	    }
	    e = straight_path(ND_out(hn).list[0], sl, pointfs, &pointn);
	    recover_slack(segfirst, P);
	    segfirst = e;
	    tn = agtail(e);
	    hn = aghead(e);
	    boxn = 0;
	    tend.nb = maximal_bbox(sp, tn, ND_in(tn).list[0], e);
	    beginpath(P, e, REGULAREDGE, &tend, spline_merge(tn));
	    b = makeregularend(tend.boxes[tend.boxn - 1], BOTTOM,
	    	       ND_coord(tn).y - GD_rank(agraphof(tn))[ND_rank(tn)].ht1);
	    if (b.LL.x < b.UR.x && b.LL.y < b.UR.y)
	        tend.boxes[tend.boxn++] = b;
	    P->start.theta = -M_PI / 2, P->start.constrained = TRUE;
	    smode = FALSE;
	}
	boxes[boxn++] = rank_box(sp, g, ND_rank(tn));
	b = hend.nb = maximal_bbox(sp, hn, e, NULL);
#ifndef WITH_CGRAPH
	endpath(P, hackflag ? &fwdedgeb : e, REGULAREDGE, &hend, spline_merge(aghead(e)));
#else
	endpath(P, hackflag ? &fwdedgeb.out : e, REGULAREDGE, &hend, spline_merge(aghead(e)));
#endif
	b.UR.y = hend.boxes[hend.boxn - 1].UR.y;
	b.LL.y = hend.boxes[hend.boxn - 1].LL.y;
	b = makeregularend(b, TOP,
	    	   ND_coord(hn).y + GD_rank(agraphof(hn))[ND_rank(hn)].ht2);
	if (b.LL.x < b.UR.x && b.LL.y < b.UR.y)
	    hend.boxes[hend.boxn++] = b;
	completeregularpath(P, segfirst, e, &tend, &hend, boxes, boxn,
	    		longedge);
	if (splines) ps = routesplines(P, &pn);
	else ps = routepolylines (P, &pn);
	if ((et == ET_LINE) && (pn > 4)) {
	    /* Here we have used the polyline case to handle
	     * an edge between two nodes on adjacent ranks. If the
	     * results really is a polyline, straighten it.
	     */
	    ps[1] = ps[0];
	    ps[3] = ps[2] = ps[pn-1];
	    pn = 4;
        }
	if (pn == 0)
	    return;
	if (pointn + pn > numpts) {
	    numpts = 2*(pointn+pn); 
	    pointfs = RALLOC(numpts, pointfs, pointf);
	}
	for (i = 0; i < pn; i++) {
	    pointfs[pointn++] = ps[i];
	}
	recover_slack(segfirst, P);
#ifndef WITH_CGRAPH
	hn = hackflag ? aghead(&fwdedgeb) : aghead(e);
#else
	hn = hackflag ? aghead(&fwdedgeb.out) : aghead(e);
#endif
    }

    /* make copies of the spline points, one per multi-edge */

    if (cnt == 1) {
	clip_and_install(fe, hn, pointfs, pointn, &sinfo);
	return;
    }
    dx = sp->Multisep * (cnt - 1) / 2;
    for (i = 1; i < pointn - 1; i++)
	pointfs[i].x -= dx;

    if (numpts > numpts2) {
	numpts2 = numpts; 
	pointfs2 = RALLOC(numpts2, pointfs2, pointf);
    }
    for (i = 0; i < pointn; i++)
	pointfs2[i] = pointfs[i];
    clip_and_install(fe, hn, pointfs2, pointn, &sinfo);
    for (j = 1; j < cnt; j++) {
	e = edges[ind + j];
	if (ED_tree_index(e) & BWDEDGE) {
#ifndef WITH_CGRAPH
	    MAKEFWDEDGE(&fwdedge, e);
	    e = &fwdedge;
#else
	    MAKEFWDEDGE(&fwdedge.out, e);
	    e = &fwdedge.out;
#endif
	}
	for (i = 1; i < pointn - 1; i++)
	    pointfs[i].x += sp->Multisep;
	for (i = 0; i < pointn; i++)
	    pointfs2[i] = pointfs[i];
	clip_and_install(e, aghead(e), pointfs2, pointn, &sinfo);
    }
}

/* regular edges */

#define DONT_WANT_ANY_ENDPOINT_PATH_REFINEMENT
#ifdef DONT_WANT_ANY_ENDPOINT_PATH_REFINEMENT
static void
completeregularpath(path * P, edge_t * first, edge_t * last,
		    pathend_t * tendp, pathend_t * hendp, boxf * boxes,
		    int boxn, int flag)
{
    edge_t *uleft, *uright, *lleft, *lright;
    int i, fb, lb;
    splines *spl;
    pointf *pp;
    int pn;

    fb = lb = -1;
    uleft = uright = NULL;
    uleft = top_bound(first, -1), uright = top_bound(first, 1);
    if (uleft) {
	if (!(spl = getsplinepoints(uleft))) return;
	pp = spl->list[0].list;
       	pn = spl->list[0].size;
    }
    if (uright) {
	if (!(spl = getsplinepoints(uright))) return;
	pp = spl->list[0].list;
       	pn = spl->list[0].size;
    }
    lleft = lright = NULL;
    lleft = bot_bound(last, -1), lright = bot_bound(last, 1);
    if (lleft) {
	if (!(spl = getsplinepoints(lleft))) return;
	pp = spl->list[spl->size - 1].list;
       	pn = spl->list[spl->size - 1].size;
    }
    if (lright) {
	if (!(spl = getsplinepoints(lright))) return;
	pp = spl->list[spl->size - 1].list;
       	pn = spl->list[spl->size - 1].size;
    }
    for (i = 0; i < tendp->boxn; i++)
	add_box(P, tendp->boxes[i]);
    fb = P->nbox + 1;
    lb = fb + boxn - 3;
    for (i = 0; i < boxn; i++)
	add_box(P, boxes[i]);
    for (i = hendp->boxn - 1; i >= 0; i--)
	add_box(P, hendp->boxes[i]);
    adjustregularpath(P, fb, lb);
}
#else
void refineregularends(edge_t * left, edge_t * right, pathend_t * endp,
		       int dir, boxf b, boxf * boxes, int *boxnp);

/* box subdivision is obsolete, I think... ek */
static void
completeregularpath(path * P, edge_t * first, edge_t * last,
		    pathend_t * tendp, pathend_t * hendp, boxf * boxes,
		    int boxn, int flag)
{
    edge_t *uleft, *uright, *lleft, *lright;
    boxf uboxes[NSUB], lboxes[NSUB];
    boxf b;
    int uboxn, lboxn, i, y, fb, lb;

    fb = lb = -1;
    uleft = uright = NULL;
    if (flag || ND_rank(agtail(first)) + 1 != ND_rank(aghead(last)))
	uleft = top_bound(first, -1), uright = top_bound(first, 1);
    refineregularends(uleft, uright, tendp, 1, boxes[0], uboxes, &uboxn);
    lleft = lright = NULL;
    if (flag || ND_rank(agtail(first)) + 1 != ND_rank(aghead(last)))
	lleft = bot_bound(last, -1), lright = bot_bound(last, 1);
    refineregularends(lleft, lright, hendp, -1, boxes[boxn - 1], lboxes,
		      &lboxn);
    for (i = 0; i < tendp->boxn; i++)
	add_box(P, tendp->boxes[i]);
    if (ND_rank(agtail(first)) + 1 == ND_rank(aghead(last))) {
	if ((!uleft && !uright) && (lleft || lright)) {
	    b = boxes[0];
	    y = b.UR.y - b.LL.y;
	    for (i = 0; i < NSUB; i++) {
		uboxes[i] = b;
		uboxes[i].UR.y = b.UR.y - y * i / NSUB;
		uboxes[i].LL.y = b.UR.y - y * (i + 1) / NSUB;
	    }
	    uboxn = NSUB;
	} else if ((uleft || uright) && (!lleft && !lright)) {
	    b = boxes[boxn - 1];
	    y = b.UR.y - b.LL.y;
	    for (i = 0; i < NSUB; i++) {
		lboxes[i] = b;
		lboxes[i].UR.y = b.UR.y - y * i / NSUB;
		lboxes[i].LL.y = b.UR.y - y * (i + 1) / NSUB;
	    }
	    lboxn = NSUB;
	}
	for (i = 0; i < uboxn; i++) {
	    uboxes[i].LL.x = MAX(uboxes[i].LL.x, lboxes[i].LL.x);
	    uboxes[i].UR.x = MIN(uboxes[i].UR.x, lboxes[i].UR.x);
	}
	for (i = 0; i < uboxn; i++)
	    add_box(P, uboxes[i]);
    } else {
	for (i = 0; i < uboxn; i++)
	    add_box(P, uboxes[i]);
	fb = P->nbox;
	lb = fb + boxn - 3;
	for (i = 1; i < boxn - 1; i++)
	    add_box(P, boxes[i]);
	for (i = 0; i < lboxn; i++)
	    add_box(P, lboxes[i]);
    }
    for (i = hendp->boxn - 1; i >= 0; i--)
	add_box(P, hendp->boxes[i]);
    adjustregularpath(P, fb, lb);
}
#endif

/* makeregularend:
 * Add box to fill between node and interrank space. Needed because
 * nodes in a given rank can differ in height.
 * for now, regular edges always go from top to bottom 
 */
static boxf makeregularend(boxf b, int side, int y)
{
    boxf newb;
    switch (side) {
    case BOTTOM:
	newb = boxfof(b.LL.x, y, b.UR.x, b.LL.y);
	break;
    case TOP:
	newb = boxfof(b.LL.x, b.UR.y, b.UR.x, y);
	break;
    }
    return newb;
}

#ifndef DONT_WANT_ANY_ENDPOINT_PATH_REFINEMENT
void refineregularends(left, right, endp, dir, b, boxes, boxnp)
edge_t *left, *right;
pathend_t *endp;
int dir;
box b;
box *boxes;
int *boxnp;
{
    splines *lspls, *rspls;
    point pp, cp;
    box eb;
    box *bp;
    int y, i, j, k;
    int nsub;

    y = b.UR.y - b.LL.y;
    if ((y == 1) || (!left && !right)) {
	boxes[0] = b;
	*boxnp = 1;
	return;
    }
    nsub = MIN(NSUB, y);
    for (i = 0; i < nsub; i++) {
	boxes[i] = b;
	boxes[i].UR.y = b.UR.y - y * i / nsub;
	boxes[i].LL.y = b.UR.y - y * (i + 1) / nsub;
	if (boxes[i].UR.y == boxes[i].LL.y)
	    abort();
    }
    *boxnp = nsub;
    /* only break big boxes */
    for (j = 0; j < endp->boxn; j++) {
	eb = endp->boxes[j];
	y = eb.UR.y - eb.LL.y;
#ifdef STEVE_AND_LEFTY_GRASPING_AT_STRAWS
	if (y < 15)
	    continue;
#else
	if (y < nsub)
	    continue;
#endif
	for (k = endp->boxn - 1; k > j; k--)
	    endp->boxes[k + (nsub - 1)] = endp->boxes[k];
	for (i = 0; i < nsub; i++) {
	    bp = &endp->boxes[j + ((dir == 1) ? i : (nsub - i - 1))];
	    *bp = eb;
	    bp->UR.y = eb.UR.y - y * i / nsub;
	    bp->LL.y = eb.UR.y - y * (i + 1) / nsub;
	    if (bp->UR.y == bp->LL.y)
		abort();
	}
	endp->boxn += (nsub - 1);
	j += nsub - 1;
    }
    if (left) {
	if (!(lspls = getsplinepoints(left))) return;
	pp = spline_at_y(lspls, boxes[0].UR.y);
	for (i = 0; i < nsub; i++) {
	    cp = spline_at_y(lspls, boxes[i].LL.y);
	    /*boxes[i].LL.x = AVG (pp.x, cp.x); */
	    boxes[i].LL.x = MAX(pp.x, cp.x);
	    pp = cp;
	}
	pp = spline_at_y(lspls, (dir == 1) ?
			 endp->boxes[1].UR.y : endp->boxes[1].LL.y);
	for (i = 1; i < endp->boxn; i++) {
	    cp = spline_at_y(lspls, (dir == 1) ?
			     endp->boxes[i].LL.y : endp->boxes[i].UR.y);
	    endp->boxes[i].LL.x = MIN(endp->nb.UR.x, MAX(pp.x, cp.x));
	    pp = cp;
	}
	i = (dir == 1) ? 0 : *boxnp - 1;
	if (boxes[i].LL.x > endp->boxes[endp->boxn - 1].UR.x - MINW)
	    boxes[i].LL.x = endp->boxes[endp->boxn - 1].UR.x - MINW;
    }
    if (right) {
	if (!(rspls = getsplinepoints(right))) return;
	pp = spline_at_y(rspls, boxes[0].UR.y);
	for (i = 0; i < nsub; i++) {
	    cp = spline_at_y(rspls, boxes[i].LL.y);
	    /*boxes[i].UR.x = AVG (pp.x, cp.x); */
	    boxes[i].UR.x = AVG(pp.x, cp.x);
	    pp = cp;
	}
	pp = spline_at_y(rspls, (dir == 1) ?
			 endp->boxes[1].UR.y : endp->boxes[1].LL.y);
	for (i = 1; i < endp->boxn; i++) {
	    cp = spline_at_y(rspls, (dir == 1) ?
			     endp->boxes[i].LL.y : endp->boxes[i].UR.y);
	    endp->boxes[i].UR.x = MAX(endp->nb.LL.x, AVG(pp.x, cp.x));
	    pp = cp;
	}
	i = (dir == 1) ? 0 : *boxnp - 1;
	if (boxes[i].UR.x < endp->boxes[endp->boxn - 1].LL.x + MINW)
	    boxes[i].UR.x = endp->boxes[endp->boxn - 1].LL.x + MINW;
    }
}
#endif

/* adjustregularpath:
 * make sure the path is wide enough.
 * the % 2 was so that in rank boxes would only be grown if
 * they were == 0 while inter-rank boxes could be stretched to a min
 * width.
 * The list of boxes has three parts: tail boxes, path boxes, and head
 * boxes. (Note that because of back edges, the tail boxes might actually
 * belong to the head node, and vice versa.) fb is the index of the
 * first interrank path box and lb is the last interrank path box.
 * If fb > lb, there are none.
 *
 * The second for loop was added by ek long ago, and apparently is intended
 * to guarantee an overlap between adjacent boxes of at least MINW.
 * It doesn't do this, and the ifdef'ed part has the potential of moving 
 * a box within a node for more complex paths.
 */
static void adjustregularpath(path * P, int fb, int lb)
{
    boxf *bp1, *bp2;
    int i, x;

    for (i = fb-1; i < lb+1; i++) {
	bp1 = &P->boxes[i];
	if ((i - fb) % 2 == 0) {
	    if (bp1->LL.x >= bp1->UR.x) {
		x = (bp1->LL.x + bp1->UR.x) / 2;
		bp1->LL.x = x - HALFMINW, bp1->UR.x = x + HALFMINW;
	    }
	} else {
	    if (bp1->LL.x + MINW > bp1->UR.x) {
		x = (bp1->LL.x + bp1->UR.x) / 2;
		bp1->LL.x = x - HALFMINW, bp1->UR.x = x + HALFMINW;
	    }
	}
    }
    for (i = 0; i < P->nbox - 1; i++) {
	bp1 = &P->boxes[i], bp2 = &P->boxes[i + 1];
	if (i >= fb && i <= lb && (i - fb) % 2 == 0) {
	    if (bp1->LL.x + MINW > bp2->UR.x)
		bp2->UR.x = bp1->LL.x + MINW;
	    if (bp1->UR.x - MINW < bp2->LL.x)
		bp2->LL.x = bp1->UR.x - MINW;
	} else if (i + 1 >= fb && i < lb && (i + 1 - fb) % 2 == 0) {
	    if (bp1->LL.x + MINW > bp2->UR.x)
		bp1->LL.x = bp2->UR.x - MINW;
	    if (bp1->UR.x - MINW < bp2->LL.x)
		bp1->UR.x = bp2->LL.x + MINW;
	} 
    }
}

static boxf rank_box(spline_info_t* sp, graph_t * g, int r)
{
    boxf b;
    node_t /* *right0, *right1, */  * left0, *left1;

    b = sp->Rank_box[r];
    if (b.LL.x == b.UR.x) {
	left0 = GD_rank(g)[r].v[0];
	/* right0 = GD_rank(g)[r].v[GD_rank(g)[r].n - 1]; */
	left1 = GD_rank(g)[r + 1].v[0];
	/* right1 = GD_rank(g)[r + 1].v[GD_rank(g)[r + 1].n - 1]; */
	b.LL.x = sp->LeftBound;
	b.LL.y = ND_coord(left1).y + GD_rank(g)[r + 1].ht2;
	b.UR.x = sp->RightBound;
	b.UR.y = ND_coord(left0).y - GD_rank(g)[r].ht1;
	sp->Rank_box[r] = b;
    }
    return b;
}

/* returns count of vertically aligned edges starting at n */
static int straight_len(node_t * n)
{
    int cnt = 0;
    node_t *v;

    v = n;
    while (1) {
	v = aghead(ND_out(v).list[0]);
	if (ND_node_type(v) != VIRTUAL)
	    break;
	if ((ND_out(v).size != 1) || (ND_in(v).size != 1))
	    break;
	if (ND_coord(v).x != ND_coord(n).x)
	    break;
	cnt++;
    }
    return cnt;
}

static edge_t *straight_path(edge_t * e, int cnt, pointf * plist, int *np)
{
    int n = *np;
    edge_t *f = e;

    while (cnt--)
	f = ND_out(aghead(f)).list[0];
    plist[(*np)++] = plist[n - 1];
    plist[(*np)++] = plist[n - 1];
    plist[(*np)] = ND_coord(agtail(f));  /* will be overwritten by next spline */

    return f;
}

static void recover_slack(edge_t * e, path * p)
{
    int b;
    node_t *vn;

    b = 0;			/* skip first rank box */
    for (vn = aghead(e);
	 ND_node_type(vn) == VIRTUAL && !sinfo.splineMerge(vn);
	 vn = aghead(ND_out(vn).list[0])) {
	while ((b < p->nbox) && (p->boxes[b].LL.y > ND_coord(vn).y))
	    b++;
	if (b >= p->nbox)
	    break;
	if (p->boxes[b].UR.y < ND_coord(vn).y)
	    continue;
	if (ND_label(vn))
	    resize_vn(vn, p->boxes[b].LL.x, p->boxes[b].UR.x,
		      p->boxes[b].UR.x + ND_rw(vn));
	else
	    resize_vn(vn, p->boxes[b].LL.x, (p->boxes[b].LL.x +
					     p->boxes[b].UR.x) / 2,
		      p->boxes[b].UR.x);
    }
}

static void resize_vn(vn, lx, cx, rx)
node_t *vn;
int lx, cx, rx;
{
    ND_coord(vn).x = cx;
    ND_lw(vn) = cx - lx, ND_rw(vn) = rx - cx;
}

/* side > 0 means right. side < 0 means left */
static edge_t *top_bound(edge_t * e, int side)
{
    edge_t *f, *ans = NULL;
    int i;

    for (i = 0; (f = ND_out(agtail(e)).list[i]); i++) {
#if 0				/* were we out of our minds? */
	if (ED_tail_port(e).p.x != ED_tail_port(f).p.x)
	    continue;
#endif
	if (side * (ND_order(aghead(f)) - ND_order(aghead(e))) <= 0)
	    continue;
	if ((ED_spl(f) == NULL)
	    && ((ED_to_orig(f) == NULL) || (ED_spl(ED_to_orig(f)) == NULL)))
	    continue;
	if ((ans == NULL)
	    || (side * (ND_order(aghead(ans)) - ND_order(aghead(f))) > 0))
	    ans = f;
    }
    return ans;
}

static edge_t *bot_bound(edge_t * e, int side)
{
    edge_t *f, *ans = NULL;
    int i;

    for (i = 0; (f = ND_in(aghead(e)).list[i]); i++) {
#if 0				/* same here */
	if (ED_head_port(e).p.x != ED_head_port(f).p.x)
	    continue;
#endif
	if (side * (ND_order(agtail(f)) - ND_order(agtail(e))) <= 0)
	    continue;
	if ((ED_spl(f) == NULL)
	    && ((ED_to_orig(f) == NULL) || (ED_spl(ED_to_orig(f)) == NULL)))
	    continue;
	if ((ans == NULL)
	    || (side * (ND_order(agtail(ans)) - ND_order(agtail(f))) > 0))
	    ans = f;
    }
    return ans;
}

/* common routines */

static int cl_vninside(graph_t * cl, node_t * n)
{
    return (BETWEEN(GD_bb(cl).LL.x, (double)(ND_coord(n).x), GD_bb(cl).UR.x) &&
	    BETWEEN(GD_bb(cl).LL.y, (double)(ND_coord(n).y), GD_bb(cl).UR.y));
}

/* All nodes belong to some cluster, which may be the root graph.
 * For the following, we only want a cluster if it is a real cluster
 * It is not clear this will handle all potential problems. It seems one
 * could have hcl and tcl contained in cl, which would also cause problems.
 */
#define REAL_CLUSTER(n) (ND_clust(n)==agraphof(n)?NULL:ND_clust(n))

/* returns the cluster of (adj) that interferes with n,
 */
static Agraph_t *cl_bound(n, adj)
node_t *n, *adj;
{
    graph_t *rv, *cl, *tcl, *hcl;
    edge_t *orig;

    rv = NULL;
    if (ND_node_type(n) == NORMAL)
	tcl = hcl = ND_clust(n);
    else {
	orig = ED_to_orig(ND_out(n).list[0]);
	tcl = ND_clust(agtail(orig));
	hcl = ND_clust(aghead(orig));
    }
    if (ND_node_type(adj) == NORMAL) {
	cl = REAL_CLUSTER(adj);
	if (cl && (cl != tcl) && (cl != hcl))
	    rv = cl;
    } else {
	orig = ED_to_orig(ND_out(adj).list[0]);
	cl = REAL_CLUSTER(agtail(orig));
	if (cl && (cl != tcl) && (cl != hcl) && cl_vninside(cl, adj))
	    rv = cl;
	else {
	    cl = REAL_CLUSTER(aghead(orig));
	    if (cl && (cl != tcl) && (cl != hcl) && cl_vninside(cl, adj))
		rv = cl;
	}
    }
    return rv;
}

/* maximal_bbox:
 * Return an initial bounding box to be used for building the
 * beginning or ending of the path of boxes.
 * Height reflects height of tallest node on rank.
 * The extra space provided by FUDGE allows begin/endpath to create a box
 * FUDGE-2 away from the node, so the routing can avoid the node and the
 * box is at least 2 wide.
 */
#define FUDGE 4

static boxf maximal_bbox(spline_info_t* sp, node_t* vn, edge_t* ie, edge_t* oe)
{
    double b, nb;
    graph_t *g = agraphof(vn), *left_cl, *right_cl;
    node_t *left, *right;
    boxf rv;

    left_cl = right_cl = NULL;

    /* give this node all the available space up to its neighbors */
    b = (double)(ND_coord(vn).x - ND_lw(vn) - FUDGE);
    if ((left = neighbor(vn, ie, oe, -1))) {
	if ((left_cl = cl_bound(vn, left)))
	    nb = GD_bb(left_cl).UR.x + (double)(sp->Splinesep);
	else {
	    nb = (double)(ND_coord(left).x + ND_mval(left));
	    if (ND_node_type(left) == NORMAL)
		nb += GD_nodesep(g) / 2.;
	    else
		nb += (double)(sp->Splinesep);
	}
	if (nb < b)
	    b = nb;
	rv.LL.x = ROUND(b);
    } else
	rv.LL.x = MIN(ROUND(b), sp->LeftBound);

    /* we have to leave room for our own label! */
    if ((ND_node_type(vn) == VIRTUAL) && (ND_label(vn)))
	b = (double)(ND_coord(vn).x + 10);
    else
	b = (double)(ND_coord(vn).x + ND_rw(vn) + FUDGE);
    if ((right = neighbor(vn, ie, oe, 1))) {
	if ((right_cl = cl_bound(vn, right)))
	    nb = GD_bb(right_cl).LL.x - (double)(sp->Splinesep);
	else {
	    nb = ND_coord(right).x - ND_lw(right);
	    if (ND_node_type(right) == NORMAL)
		nb -= GD_nodesep(g) / 2.;
	    else
		nb -= (double)(sp->Splinesep);
	}
	if (nb > b)
	    b = nb;
	rv.UR.x = ROUND(b);
    } else
	rv.UR.x = MAX(ROUND(b), sp->RightBound);

    if ((ND_node_type(vn) == VIRTUAL) && (ND_label(vn))) {
	rv.UR.x -= ND_rw(vn);
	if (rv.UR.x < rv.LL.x) rv.UR.x = ND_coord(vn).x;
    }

    rv.LL.y = ND_coord(vn).y - GD_rank(g)[ND_rank(vn)].ht1;
    rv.UR.y = ND_coord(vn).y + GD_rank(g)[ND_rank(vn)].ht2;
    return rv;
}

static node_t *neighbor(vn, ie, oe, dir)
node_t *vn;
edge_t *ie, *oe;
int dir;
{
    int i;
    node_t *n, *rv = NULL;
    rank_t *rank = &(GD_rank(agraphof(vn))[ND_rank(vn)]);

    for (i = ND_order(vn) + dir; ((i >= 0) && (i < rank->n)); i += dir) {
	n = rank->v[i];
	if ((ND_node_type(n) == VIRTUAL) && (ND_label(n))) {
	    rv = n;
	    break;
	}
	if (ND_node_type(n) == NORMAL) {
	    rv = n;
	    break;
	}
	if (pathscross(n, vn, ie, oe) == FALSE) {
	    rv = n;
	    break;
	}
    }
    return rv;
}

static boolean pathscross(n0, n1, ie1, oe1)
node_t *n0, *n1;
edge_t *ie1, *oe1;
{
    edge_t *e0, *e1;
    node_t *na, *nb;
    int order, cnt;

    order = (ND_order(n0) > ND_order(n1));
    if ((ND_out(n0).size != 1) && (ND_out(n0).size != 1))
	return FALSE;
    e1 = oe1;
    if (ND_out(n0).size == 1 && e1) {
	e0 = ND_out(n0).list[0];
	for (cnt = 0; cnt < 2; cnt++) {
	    if ((na = aghead(e0)) == (nb = aghead(e1)))
		break;
	    if (order != (ND_order(na) > ND_order(nb)))
		return TRUE;
	    if ((ND_out(na).size != 1) || (ND_node_type(na) == NORMAL))
		break;
	    e0 = ND_out(na).list[0];
	    if ((ND_out(nb).size != 1) || (ND_node_type(nb) == NORMAL))
		break;
	    e1 = ND_out(nb).list[0];
	}
    }
    e1 = ie1;
    if (ND_in(n0).size == 1 && e1) {
	e0 = ND_in(n0).list[0];
	for (cnt = 0; cnt < 2; cnt++) {
	    if ((na = agtail(e0)) == (nb = agtail(e1)))
		break;
	    if (order != (ND_order(na) > ND_order(nb)))
		return TRUE;
	    if ((ND_in(na).size != 1) || (ND_node_type(na) == NORMAL))
		break;
	    e0 = ND_in(na).list[0];
	    if ((ND_in(nb).size != 1) || (ND_node_type(nb) == NORMAL))
		break;
	    e1 = ND_in(nb).list[0];
	}
    }
    return FALSE;
}

#ifdef DEBUG
void showpath(path * p)
{
    int i;
    pointf LL, UR;

    fprintf(stderr, "%%!PS\n");
    for (i = 0; i < p->nbox; i++) {
	LL = p->boxes[i].LL;
	UR = p->boxes[i].UR;
	fprintf(stderr,
		"newpath %.04f %.04f moveto %.04f %.04f lineto %.04f %.04f lineto %.04f %.04f lineto closepath stroke\n",
		LL.x, LL.y, UR.x, LL.y, UR.x, UR.y, LL.x, UR.y);
    }
    fprintf(stderr, "showpage\n");
}
#endif
