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


#include	"dot.h"


static node_t *make_vn_slot(graph_t * g, int r, int pos)
{
    int i;
    node_t **v, *n;

    v = GD_rank(g)[r].v =
	ALLOC(GD_rank(g)[r].n + 2, GD_rank(g)[r].v, node_t *);
    for (i = GD_rank(g)[r].n; i > pos; i--) {
	v[i] = v[i - 1];
	ND_order(v[i])++;
    }
    n = v[pos] = virtual_node(g);
    ND_order(n) = pos;
    ND_rank(n) = r;
    v[++(GD_rank(g)[r].n)] = NULL;
    return v[pos];
}

#define 	HLB 	0	/* hard left bound */
#define		HRB		1	/* hard right bound */
#define		SLB		2	/* soft left bound */
#define		SRB		3	/* soft right bound */

static void findlr(node_t * u, node_t * v, int *lp, int *rp)
{
    int l, r;
    l = ND_order(u);
    r = ND_order(v);
    if (l > r) {
	int t = l;
	l = r;
	r = t;
    }
    *lp = l;
    *rp = r;
}

static void setbounds(node_t * v, int *bounds, int lpos, int rpos)
{
    int i, l, r, ord;
    edge_t *f;

    if (ND_node_type(v) == VIRTUAL) {
	ord = ND_order(v);
	if (ND_in(v).size == 0) {	/* flat */
	    assert(ND_out(v).size == 2);
	    findlr(aghead(ND_out(v).list[0]), aghead(ND_out(v).list[1]), &l,
		   &r);
	    /* the other flat edge could be to the left or right */
	    if (r <= lpos)
		bounds[SLB] = bounds[HLB] = ord;
	    else if (l >= rpos)
		bounds[SRB] = bounds[HRB] = ord;
	    /* could be spanning this one */
	    else if ((l < lpos) && (r > rpos));	/* ignore */
	    /* must have intersecting ranges */
	    else {
		if ((l < lpos) || ((l == lpos) && (r < rpos)))
		    bounds[SLB] = ord;
		if ((r > rpos) || ((r == rpos) && (l > lpos)))
		    bounds[SRB] = ord;
	    }
	} else {		/* forward */
	    boolean onleft, onright;
	    onleft = onright = FALSE;
	    for (i = 0; (f = ND_out(v).list[i]); i++) {
		if (ND_order(aghead(f)) <= lpos) {
		    onleft = TRUE;
		    continue;
		}
		if (ND_order(aghead(f)) >= rpos) {
		    onright = TRUE;
		    continue;
		}
	    }
	    if (onleft && (onright == FALSE))
		bounds[HLB] = ord + 1;
	    if (onright && (onleft == FALSE))
		bounds[HRB] = ord - 1;
	}
    }
}

static int flat_limits(graph_t * g, edge_t * e)
{
    int lnode, rnode, r, bounds[4], lpos, rpos, pos;
    node_t **rank;

    r = ND_rank(agtail(e)) - 1;
    rank = GD_rank(g)[r].v;
    lnode = 0;
    rnode = GD_rank(g)[r].n - 1;
    bounds[HLB] = bounds[SLB] = lnode - 1;
    bounds[HRB] = bounds[SRB] = rnode + 1;
    findlr(agtail(e), aghead(e), &lpos, &rpos);
    while (lnode <= rnode) {
	setbounds(rank[lnode], bounds, lpos, rpos);
	if (lnode != rnode)
	    setbounds(rank[rnode], bounds, lpos, rpos);
	lnode++;
	rnode--;
	if (bounds[HRB] - bounds[HLB] <= 1)
	    break;
    }
    if (bounds[HLB] <= bounds[HRB])
	pos = (bounds[HLB] + bounds[HRB] + 1) / 2;
    else
	pos = (bounds[SLB] + bounds[SRB] + 1) / 2;
    return pos;
}

/* flat_node:
 * Create virtual node representing edge label between
 * actual ends of edge e. 
 * This node is characterized by being virtual and having a non-NULL
 * ND_alg pointing to e.
 */
static void 
flat_node(edge_t * e)
{
    int r, place, ypos, h2;
    graph_t *g;
    node_t *n, *vn;
    edge_t *ve;
    pointf dimen;

    if (ED_label(e) == NULL)
	return;
    g = agraphof(agtail(e));
    r = ND_rank(agtail(e));

    place = flat_limits(g, e);
    /* grab ypos = LL.y of label box before make_vn_slot() */
    if ((n = GD_rank(g)[r - 1].v[0]))
	ypos = ND_coord(n).y - GD_rank(g)[r - 1].ht1;
    else {
	n = GD_rank(g)[r].v[0];
	ypos = ND_coord(n).y + GD_rank(g)[r].ht2 + GD_ranksep(g);
    }
    vn = make_vn_slot(g, r - 1, place);
    dimen = ED_label(e)->dimen;
    if (GD_flip(g)) {
	double f = dimen.x;
	dimen.x = dimen.y;
	dimen.y = f;
    }
    ND_ht(vn) = dimen.y;
    h2 = ND_ht(vn) / 2;
    ND_lw(vn) = ND_rw(vn) = dimen.x / 2;
    ND_label(vn) = ED_label(e);
    ND_coord(vn).y = ypos + h2;
    ve = virtual_edge(vn, agtail(e), e);	/* was NULL? */
    ED_tail_port(ve).p.x = -ND_lw(vn);
    ED_head_port(ve).p.x = ND_rw(agtail(e));
    ED_edge_type(ve) = FLATORDER;
    ve = virtual_edge(vn, aghead(e), e);
    ED_tail_port(ve).p.x = ND_rw(vn);
    ED_head_port(ve).p.x = ND_lw(aghead(e));
    ED_edge_type(ve) = FLATORDER;
    /* another assumed symmetry of ht1/ht2 of a label node */
    if (GD_rank(g)[r - 1].ht1 < h2)
	GD_rank(g)[r - 1].ht1 = h2;
    if (GD_rank(g)[r - 1].ht2 < h2)
	GD_rank(g)[r - 1].ht2 = h2;
    ND_alg(vn) = e;
}

static void abomination(graph_t * g)
{
    int r;
    rank_t *rptr;

    assert(GD_minrank(g) == 0);
    /* 3 = one for new rank, one for sentinel, one for off-by-one */
    r = GD_maxrank(g) + 3;
    rptr = ALLOC(r, GD_rank(g), rank_t);
    GD_rank(g) = rptr + 1;
    for (r = GD_maxrank(g); r >= 0; r--)
	GD_rank(g)[r] = GD_rank(g)[r - 1];
    GD_rank(g)[r].n = GD_rank(g)[r].an = 0;
    GD_rank(g)[r].v = GD_rank(g)[r].av = N_NEW(2, node_t *);
    GD_rank(g)[r].flat = NULL;
    GD_rank(g)[r].ht1 = GD_rank(g)[r].ht2 = 1;
    GD_rank(g)[r].pht1 = GD_rank(g)[r].pht2 = 1;
    GD_minrank(g)--;
}

/* checkFlatAdjacent:
 * Check if tn and hn are adjacent. 
 * If so, set adjacent bit on all related edges.
 * Assume e is flat.
 */
static void
checkFlatAdjacent (edge_t* e)
{
    node_t* tn = agtail(e);
    node_t* hn = aghead(e);
    int i, lo, hi;
    node_t* n;
    rank_t *rank;

    if (ND_order(tn) < ND_order(hn)) {
	lo = ND_order(tn);
	hi = ND_order(hn);
    }
    else {
	lo = ND_order(hn);
	hi = ND_order(tn);
    }
    rank = &(GD_rank(agraphof(tn))[ND_rank(tn)]);
    for (i = lo + 1; i < hi; i++) {
	n = rank->v[i];
	if ((ND_node_type(n) == VIRTUAL && ND_label(n)) || 
             ND_node_type(n) == NORMAL)
	    break;
    }
    if (i == hi) {  /* adjacent edge */
	do {
	    ED_adjacent(e) = 1;
	    e = ED_to_virt(e);
	} while (e); 
    }
}
 
/* flat_edges:
 * Process flat edges.
 * First, mark flat edges as having adjacent endpoints or not.
 *
 * Second, if there are edge labels, nodes are placed on ranks 0,2,4,...
 * If we have a labeled flat edge on rank 0, add a rank -1.
 *
 * Finally, create label information. Add a virtual label node in the 
 * previous rank for each labeled, non-adjacent flat edge. If this is 
 * done for any edge, return true, so that main code will reset y coords.
 * For labeled adjacent flat edges, store label width in representative edge.
 * FIX: We should take into account any extra height needed for the latter
 * labels.
 * 
 * We leave equivalent flat edges in ND_other. Their ED_virt field should
 * still point to the class representative.
 */
int 
flat_edges(graph_t * g)
{
    int i, j, reset = FALSE;
    node_t *n;
    edge_t *e;
    int found = FALSE;

    for (n = GD_nlist(g); n; n = ND_next(n)) {
	if (ND_flat_out(n).list) {
	    for (j = 0; (e = ND_flat_out(n).list[j]); j++) {
		checkFlatAdjacent (e);
	    }
	}
	for (j = 0; j < ND_other(n).size; j++) {
	    e = ND_other(n).list[j];
	    if (ND_rank(aghead(e)) == ND_rank(agtail(e)))
		checkFlatAdjacent (e);
	}
    }

    if ((GD_rank(g)[0].flat) || (GD_n_cluster(g) > 0)) {
	for (i = 0; (n = GD_rank(g)[0].v[i]); i++) {
	    for (j = 0; (e = ND_flat_in(n).list[j]); j++) {
		if ((ED_label(e)) && !ED_adjacent(e)) {
		    abomination(g);
		    found = TRUE;
		    break;
		}
	    }
	    if (found)
		break;
	}
    }

    rec_save_vlists(g);
    for (n = GD_nlist(g); n; n = ND_next(n)) {
          /* if n is the tail of any flat edge, one will be in flat_out */
	if (ND_flat_out(n).list) {
	    for (i = 0; (e = ND_flat_out(n).list[i]); i++) {
		if (ED_label(e)) {
		    if (ED_adjacent(e)) {
			if (GD_flip(g)) ED_dist(e) = ED_label(e)->dimen.y;
			else ED_dist(e) = ED_label(e)->dimen.x; 
		    }
		    else {
			reset = TRUE;
			flat_node(e);
		    }
		}
	    }
		/* look for other flat edges with labels */
	    for (j = 0; j < ND_other(n).size; j++) {
		edge_t* le;
		e = ND_other(n).list[j];
		if (ND_rank(agtail(e)) != ND_rank(aghead(e))) continue;
		if (agtail(e) == aghead(e)) continue; /* skip loops */
		le = e;
		while (ED_to_virt(le)) le = ED_to_virt(le);
		ED_adjacent(e) = ED_adjacent(le); 
		if (ED_label(e)) {
		    if (ED_adjacent(e)) {
			double lw;
			if (GD_flip(g)) lw = ED_label(e)->dimen.y;
			else lw = ED_label(e)->dimen.x; 
			ED_dist(le) = MAX(lw,ED_dist(le));
		    }
		    else {
			reset = TRUE;
			flat_node(e);
		    }
		}
	    }
	}
    }
    if (reset)
	rec_reset_vlists(g);
    return reset;
}
