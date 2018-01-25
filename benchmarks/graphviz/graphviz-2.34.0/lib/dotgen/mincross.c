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
 * dot_mincross(g) takes a ranked graphs, and finds an ordering
 * that avoids edge crossings.  clusters are expanded.
 * N.B. the rank structure is global (not allocated per cluster)
 * because mincross may compare nodes in different clusters.
 */

#include "dot.h"

/* #define DEBUG */
#define MARK(v)		(ND_mark(v))
#define saveorder(v)	(ND_coord(v)).x
#define flatindex(v)	ND_low(v)

static int gd_minrank(Agraph_t *g) {return GD_minrank(g);}
static int gd_maxrank(Agraph_t *g) {return GD_maxrank(g);}
static rank_t *gd_rank(Agraph_t *g, int r) {return &GD_rank(g)[r];}
static int nd_order(Agnode_t *v) { return ND_order(v); }

	/* forward declarations */
static boolean medians(graph_t * g, int r0, int r1);
static int nodeposcmpf(node_t ** n0, node_t ** n1);
static int edgeidcmpf(edge_t ** e0, edge_t ** e1);
static void flat_breakcycles(graph_t * g);
static void flat_reorder(graph_t * g);
static void flat_search(graph_t * g, node_t * v);
static void init_mincross(graph_t * g);
static void merge2(graph_t * g);
static void init_mccomp(graph_t * g, int c);
static void cleanup2(graph_t * g, int nc);
static int mincross_clust(graph_t * par, graph_t * g, int);
static int mincross(graph_t * g, int startpass, int endpass, int);
static void mincross_step(graph_t * g, int pass);
static void mincross_options(graph_t * g);
static void save_best(graph_t * g);
static void restore_best(graph_t * g);
static adjmatrix_t *new_matrix(int i, int j);
static void free_matrix(adjmatrix_t * p);
#ifdef DEBUG
void check_rs(graph_t * g, int null_ok);
void check_order(void);
void check_vlists(graph_t * g);
void node_in_root_vlist(node_t * n);
#endif


	/* mincross parameters */
static int MinQuit;
static double Convergence;

static graph_t *Root;
static int GlobalMinRank, GlobalMaxRank;
static edge_t **TE_list;
static int *TI_list;
static boolean ReMincross;

/* dot_mincross:
 * Minimize edge crossings
 * Note that nodes are not placed into GD_rank(g) until mincross()
 * is called.
 */
void dot_mincross(graph_t * g, int doBalance)
{
    int c, nc;
    char *s;

    init_mincross(g);

    for (nc = c = 0; c < GD_comp(g).size; c++) {
	init_mccomp(g, c);
	nc += mincross(g, 0, 2, doBalance);
    }

    merge2(g);

    /* run mincross on contents of each cluster */
    for (c = 1; c <= GD_n_cluster(g); c++) {
	nc += mincross_clust(g, GD_clust(g)[c], doBalance);
#ifdef DEBUG
	check_vlists(GD_clust(g)[c]);
	check_order();
#endif
    }

    if ((GD_n_cluster(g) > 0)
	&& (!(s = agget(g, "remincross")) || (mapbool(s)))) {
	mark_lowclusters(g);
	ReMincross = TRUE;
	nc = mincross(g, 2, 2, doBalance);
#ifdef DEBUG
	for (c = 1; c <= GD_n_cluster(g); c++)
	    check_vlists(GD_clust(g)[c]);
#endif
    }
    cleanup2(g, nc);
}

static adjmatrix_t *new_matrix(int i, int j)
{
    adjmatrix_t *rv = NEW(adjmatrix_t);
    rv->nrows = i;
    rv->ncols = j;
    rv->data = N_NEW(i * j, char);
    return rv;
}

static void free_matrix(adjmatrix_t * p)
{
    if (p) {
	free(p->data);
	free(p);
    }
}

#define ELT(M,i,j)		(M->data[((i)*M->ncols)+(j)])

static void init_mccomp(graph_t * g, int c)
{
    int r;

    GD_nlist(g) = GD_comp(g).list[c];
    if (c > 0) {
	for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	    GD_rank(g)[r].v = GD_rank(g)[r].v + GD_rank(g)[r].n;
	    GD_rank(g)[r].n = 0;
	}
    }
}

static int betweenclust(edge_t * e)
{
    while (ED_to_orig(e))
	e = ED_to_orig(e);
    return (ND_clust(agtail(e)) != ND_clust(aghead(e)));
}

static void do_ordering_node (graph_t * g, node_t* n, int outflag)
{
    int i, ne;
    node_t *u, *v;
    edge_t *e, *f, *fe;
    edge_t **sortlist = TE_list;

    if (ND_clust(n))
	return;
    if (outflag) {
	for (i = ne = 0; (e = ND_out(n).list[i]); i++)
	    if (!betweenclust(e))
		sortlist[ne++] = e;
    } else {
	for (i = ne = 0; (e = ND_in(n).list[i]); i++)
	    if (!betweenclust(e))
		sortlist[ne++] = e;
    }
    if (ne <= 1)
	return;
    /* write null terminator at end of list.
       requires +1 in TE_list alloccation */
    sortlist[ne] = 0;
    qsort(sortlist, ne, sizeof(sortlist[0]), (qsort_cmpf) edgeidcmpf);
    for (ne = 1; (f = sortlist[ne]); ne++) {
	e = sortlist[ne - 1];
	if (outflag) {
	    u = aghead(e);
	    v = aghead(f);
	} else {
	    u = agtail(e);
	    v = agtail(f);
	}
	if (find_flat_edge(u, v))
	    return;
	fe = new_virtual_edge(u, v, NULL);
	ED_edge_type(fe) = FLATORDER;
	flat_edge(g, fe);
    }
}

static void do_ordering(graph_t * g, int outflag)
{
    /* Order all nodes in graph */
    node_t *n;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	do_ordering_node (g, n, outflag);
    }
}

static void do_ordering_for_nodes(graph_t * g)
{
    /* Order nodes which have the "ordered" attribute */
    node_t *n;
    const char *ordering;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if ((ordering = late_string(n, N_ordering, NULL))) {
	    if (streq(ordering, "out"))
		do_ordering_node(g, n, TRUE);
	    else if (streq(ordering, "in"))
		do_ordering_node(g, n, FALSE);
	    else if (ordering[0])
		agerr(AGERR, "ordering '%s' not recognized for node '%s'.\n", ordering, agnameof(n));
	}
    }
}

/* ordered_edges:
 * handle case where graph specifies edge ordering
 * If the graph does not have an ordering attribute, we then
 * check for nodes having the attribute.
 * Note that, in this implementation, the value of G_ordering
 * dominates the value of N_ordering.
 */
static void ordered_edges(graph_t * g)
{
    char *ordering;

    if (!G_ordering && !N_ordering)
	return;
    if ((ordering = late_string(g, G_ordering, NULL))) {
	if (streq(ordering, "out"))
	    do_ordering(g, TRUE);
	else if (streq(ordering, "in"))
	    do_ordering(g, FALSE);
	else if (ordering[0])
	    agerr(AGERR, "ordering '%s' not recognized.\n", ordering);
    }
    else
    {
#ifndef WITH_CGRAPH
	/* search meta-graph to find subgraphs that may be ordered */
	graph_t *mg, *subg;
	node_t *mm, *mn;
	edge_t *me;

	mm = g->meta_node;
	mg = mm->graph;
	for (me = agfstout(mg, mm); me; me = agnxtout(mg, me)) {
	    mn = me->head;
	    subg = agusergraph(mn);
#else /* WITH_CGRAPH */
	graph_t *subg;

	for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
#endif /* WITH_CGRAPH */
	    /* clusters are processed by separate calls to ordered_edges */
	    if (!is_cluster(subg))
		ordered_edges(subg);
	}
	if (N_ordering) do_ordering_for_nodes (g);
    }
}

static int mincross_clust(graph_t * par, graph_t * g, int doBalance)
{
    int c, nc;

    expand_cluster(g);
    ordered_edges(g);
    flat_breakcycles(g);
    flat_reorder(g);
    nc = mincross(g, 2, 2, doBalance);

    for (c = 1; c <= GD_n_cluster(g); c++)
	nc += mincross_clust(g, GD_clust(g)[c], doBalance);

    save_vlist(g);
    return nc;
}

static int left2right(graph_t * g, node_t * v, node_t * w)
{
    adjmatrix_t *M;
    int rv;

    /* CLUSTER indicates orig nodes of clusters, and vnodes of skeletons */
    if (ReMincross == FALSE) {
	if ((ND_clust(v) != ND_clust(w)) && (ND_clust(v)) && (ND_clust(w))) {
	    /* the following allows cluster skeletons to be swapped */
	    if ((ND_ranktype(v) == CLUSTER)
		&& (ND_node_type(v) == VIRTUAL))
		return FALSE;
	    if ((ND_ranktype(w) == CLUSTER)
		&& (ND_node_type(w) == VIRTUAL))
		return FALSE;
	    return TRUE;
	    /*return ((ND_ranktype(v) != CLUSTER) && (ND_ranktype(w) != CLUSTER)); */
	}
    } else {
	if ((ND_clust(v)) != (ND_clust(w)))
	    return TRUE;
    }
    M = GD_rank(g)[ND_rank(v)].flat;
    if (M == NULL)
	rv = FALSE;
    else {
	if (GD_flip(g)) {
	    node_t *t = v;
	    v = w;
	    w = t;
	}
	rv = ELT(M, flatindex(v), flatindex(w));
    }
    return rv;
}

static int in_cross(node_t * v, node_t * w)
{
    register edge_t **e1, **e2;
    register int inv, cross = 0, t;

    for (e2 = ND_in(w).list; *e2; e2++) {
	register int cnt = ED_xpenalty(*e2);		
		
	inv = ND_order((agtail(*e2)));

	for (e1 = ND_in(v).list; *e1; e1++) {
	    t = ND_order(agtail(*e1)) - inv;
	    if ((t > 0)
		|| ((t == 0)
		    && (  ED_tail_port(*e1).p.x > ED_tail_port(*e2).p.x)))
		cross += ED_xpenalty(*e1) * cnt;
	}
    }
    return cross;
}

static int out_cross(node_t * v, node_t * w)
{
    register edge_t **e1, **e2;
    register int inv, cross = 0, t;

    for (e2 = ND_out(w).list; *e2; e2++) {
	register int cnt = ED_xpenalty(*e2);
	inv = ND_order(aghead(*e2));

	for (e1 = ND_out(v).list; *e1; e1++) {
	    t = ND_order(aghead(*e1)) - inv;
	    if ((t > 0)
		|| ((t == 0)
		    && ((ED_head_port(*e1)).p.x > (ED_head_port(*e2)).p.x)))
		cross += ((ED_xpenalty(*e1)) * cnt);
	}
    }
    return cross;

}

static void exchange(node_t * v, node_t * w)
{
    int vi, wi, r;

    r = ND_rank(v);
    vi = ND_order(v);
    wi = ND_order(w);
    ND_order(v) = wi;
    GD_rank(Root)[r].v[wi] = v;
    ND_order(w) = vi;
    GD_rank(Root)[r].v[vi] = w;
}

static void balanceNodes(graph_t * g, int r, node_t * v, node_t * w)
{
    node_t *s;			/* separator node */
    int sepIndex;
    int nullType;		/* type of null nodes */
    int cntDummy = 0, cntOri = 0;
    int k = 0, m = 0, k1 = 0, m1 = 0, i = 0;

    /* we only consider v and w of different types */
    if (ND_node_type(v) == ND_node_type(w))
	return;

    /* count the number of dummy and original nodes */
    for (i = 0; i < GD_rank(g)[r].n; i++) {
	if (ND_node_type(GD_rank(g)[r].v[i]) == NORMAL)
	    cntOri++;
	else
	    cntDummy++;
    }

    if (cntOri < cntDummy) {
	if (ND_node_type(v) == NORMAL)
	    s = v;
	else
	    s = w;
    } else {
	if (ND_node_type(v) == NORMAL)
	    s = w;
	else
	    s = v;
    }

    /* get the separator node index */
    for (i = 0; i < GD_rank(g)[r].n; i++) {
	if (GD_rank(g)[r].v[i] == s)
	    sepIndex = i;
    }

    nullType = (ND_node_type(s) == NORMAL) ? VIRTUAL : NORMAL;

    /* count the number of null nodes to the left and 
     * right of the separator node 
     */
    for (i = sepIndex - 1; i >= 0; i--) {
	if (ND_node_type(GD_rank(g)[r].v[i]) == nullType)
	    k++;
	else
	    break;
    }

    for (i = sepIndex + 1; i < GD_rank(g)[r].n; i++) {
	if (ND_node_type(GD_rank(g)[r].v[i]) == nullType)
	    m++;
	else
	    break;
    }

    /* now exchange v,w and calculate the same counts */

    exchange(v, w);

    /* get the separator node index */
    for (i = 0; i < GD_rank(g)[r].n; i++) {
	if (GD_rank(g)[r].v[i] == s)
	    sepIndex = i;
    }

    /* count the number of null nodes to the left and 
     * right of the separator node 
     */
    for (i = sepIndex - 1; i >= 0; i--) {
	if (ND_node_type(GD_rank(g)[r].v[i]) == nullType)
	    k1++;
	else
	    break;
    }

    for (i = sepIndex + 1; i < GD_rank(g)[r].n; i++) {
	if (ND_node_type(GD_rank(g)[r].v[i]) == nullType)
	    m1++;
	else
	    break;
    }

    if (abs(k1 - m1) > abs(k - m)) {
	exchange(v, w);		//revert to the original ordering
    }
}

static int balance(graph_t * g)
{
    int i, c0, c1, rv;
    node_t *v, *w;
    int r;

    rv = 0;

    for (r = GD_maxrank(g); r >= GD_minrank(g); r--) {

	GD_rank(g)[r].candidate = FALSE;
	for (i = 0; i < GD_rank(g)[r].n - 1; i++) {
	    v = GD_rank(g)[r].v[i];
	    w = GD_rank(g)[r].v[i + 1];
	    assert(ND_order(v) < ND_order(w));
	    if (left2right(g, v, w))
		continue;
	    c0 = c1 = 0;
	    if (r > 0) {
		c0 += in_cross(v, w);
		c1 += in_cross(w, v);
	    }

	    if (GD_rank(g)[r + 1].n > 0) {
		c0 += out_cross(v, w);
		c1 += out_cross(w, v);
	    }
#if 0
	    if ((c1 < c0) || ((c0 > 0) && reverse && (c1 == c0))) {
		exchange(v, w);
		rv += (c0 - c1);
		GD_rank(Root)[r].valid = FALSE;
		GD_rank(g)[r].candidate = TRUE;

		if (r > GD_minrank(g)) {
		    GD_rank(Root)[r - 1].valid = FALSE;
		    GD_rank(g)[r - 1].candidate = TRUE;
		}
		if (r < GD_maxrank(g)) {
		    GD_rank(Root)[r + 1].valid = FALSE;
		    GD_rank(g)[r + 1].candidate = TRUE;
		}
	    }
#endif

	    if (c1 <= c0) {
		balanceNodes(g, r, v, w);
	    }
	}
    }
    return rv;
}

static int transpose_step(graph_t * g, int r, int reverse)
{
    int i, c0, c1, rv;
    node_t *v, *w;

    rv = 0;
    GD_rank(g)[r].candidate = FALSE;
    for (i = 0; i < GD_rank(g)[r].n - 1; i++) {
	v = GD_rank(g)[r].v[i];
	w = GD_rank(g)[r].v[i + 1];
	assert(ND_order(v) < ND_order(w));
	if (left2right(g, v, w))
	    continue;
	c0 = c1 = 0;
	if (r > 0) {
	    c0 += in_cross(v, w);
	    c1 += in_cross(w, v);
	}
	if (GD_rank(g)[r + 1].n > 0) {
	    c0 += out_cross(v, w);
	    c1 += out_cross(w, v);
	}
	if ((c1 < c0) || ((c0 > 0) && reverse && (c1 == c0))) {
	    exchange(v, w);
	    rv += (c0 - c1);
	    GD_rank(Root)[r].valid = FALSE;
	    GD_rank(g)[r].candidate = TRUE;

	    if (r > GD_minrank(g)) {
		GD_rank(Root)[r - 1].valid = FALSE;
		GD_rank(g)[r - 1].candidate = TRUE;
	    }
	    if (r < GD_maxrank(g)) {
		GD_rank(Root)[r + 1].valid = FALSE;
		GD_rank(g)[r + 1].candidate = TRUE;
	    }
	}
    }
    return rv;
}

static void transpose(graph_t * g, int reverse)
{
    int r, delta;

    for (r = GD_minrank(g); r <= GD_maxrank(g); r++)
	GD_rank(g)[r].candidate = TRUE;
    do {
	delta = 0;
#ifdef NOTDEF
	/* don't run both the upward and downward passes- they cancel. 
	   i tried making it depend on whether an odd or even pass, 
	   but that didn't help. */
	for (r = GD_maxrank(g); r >= GD_minrank(g); r--)
	    if (GD_rank(g)[r].candidate)
		delta += transpose_step(g, r, reverse);
#endif
	for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	    if (GD_rank(g)[r].candidate) {
		delta += transpose_step(g, r, reverse);
	    }
	}
	/*} while (delta > ncross(g)*(1.0 - Convergence)); */
    } while (delta >= 1);
}

static int mincross(graph_t * g, int startpass, int endpass, int doBalance)
{
    int maxthispass, iter, trying, pass;
    int cur_cross, best_cross;

    if (startpass > 1) {
	cur_cross = best_cross = ncross(g);
	save_best(g);
    } else
	cur_cross = best_cross = INT_MAX;
    for (pass = startpass; pass <= endpass; pass++) {
	if (pass <= 1) {
	    maxthispass = MIN(4, MaxIter);
	    if (g == agroot(g))
		build_ranks(g, pass);
	    if (pass == 0)
		flat_breakcycles(g);
	    flat_reorder(g);

	    if ((cur_cross = ncross(g)) <= best_cross) {
		save_best(g);
		best_cross = cur_cross;
	    }
	    trying = 0;
	} else {
	    maxthispass = MaxIter;
	    if (cur_cross > best_cross)
		restore_best(g);
	    cur_cross = best_cross;
	}
	trying = 0;
	for (iter = 0; iter < maxthispass; iter++) {
	    if (Verbose)
		fprintf(stderr,
			"mincross: pass %d iter %d trying %d cur_cross %d best_cross %d\n",
			pass, iter, trying, cur_cross, best_cross);
	    if (trying++ >= MinQuit)
		break;
	    if (cur_cross == 0)
		break;
	    mincross_step(g, iter);
	    if ((cur_cross = ncross(g)) <= best_cross) {
		save_best(g);
		if (cur_cross < Convergence * best_cross)
		    trying = 0;
		best_cross = cur_cross;
	    }
	}
	if (cur_cross == 0)
	    break;
    }
    if (cur_cross > best_cross)
	restore_best(g);
    if (best_cross > 0) {
	transpose(g, FALSE);
	best_cross = ncross(g);
    }
    if (doBalance) {
	for (iter = 0; iter < maxthispass; iter++)
	    balance(g);
    }

    return best_cross;
}

static void restore_best(graph_t * g)
{
    node_t *n;
    int r;

    for (n = GD_nlist(g); n; n = ND_next(n))
	ND_order(n) = saveorder(n);
    for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	GD_rank(Root)[r].valid = FALSE;
	qsort(GD_rank(g)[r].v, GD_rank(g)[r].n, sizeof(GD_rank(g)[0].v[0]),
	      (qsort_cmpf) nodeposcmpf);
    }
}

static void save_best(graph_t * g)
{
    node_t *n;
    for (n = GD_nlist(g); n; n = ND_next(n))
	saveorder(n) = ND_order(n);
}

/* merges the connected components of g */
static void merge_components(graph_t * g)
{
    int c;
    node_t *u, *v;

    if (GD_comp(g).size <= 1)
	return;
    u = NULL;
    for (c = 0; c < GD_comp(g).size; c++) {
	v = GD_comp(g).list[c];
	if (u)
	    ND_next(u) = v;
	ND_prev(v) = u;
	while (ND_next(v)) {
	    v = ND_next(v);
	}
	u = v;
    }
    GD_comp(g).size = 1;
    GD_nlist(g) = GD_comp(g).list[0];
    GD_minrank(g) = GlobalMinRank;
    GD_maxrank(g) = GlobalMaxRank;
}

/* merge connected components, create globally consistent rank lists */
static void merge2(graph_t * g)
{
    int i, r;
    node_t *v;

    /* merge the components and rank limits */
    merge_components(g);

    /* install complete ranks */
    for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	GD_rank(g)[r].n = GD_rank(g)[r].an;
	GD_rank(g)[r].v = GD_rank(g)[r].av;
	for (i = 0; i < GD_rank(g)[r].n; i++) {
	    v = GD_rank(g)[r].v[i];
	    if (v == NULL) {
		if (Verbose)
		    fprintf(stderr,
			    "merge2: graph %s, rank %d has only %d < %d nodes\n",
			    agnameof(g), r, i, GD_rank(g)[r].n);
		GD_rank(g)[r].n = i;
		break;
	    }
	    ND_order(v) = i;
	}
    }
}

static void cleanup2(graph_t * g, int nc)
{
    int i, j, r, c;
    node_t *v;
    edge_t *e;

    if (TI_list) {
	free(TI_list);
	TI_list = NULL;
    }
    if (TE_list) {
	free(TE_list);
	TE_list = NULL;
    }
    /* fix vlists of clusters */
    for (c = 1; c <= GD_n_cluster(g); c++)
	rec_reset_vlists(GD_clust(g)[c]);

    /* remove node temporary edges for ordering nodes */
    for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	for (i = 0; i < GD_rank(g)[r].n; i++) {
	    v = GD_rank(g)[r].v[i];
	    ND_order(v) = i;
	    if (ND_flat_out(v).list) {
		for (j = 0; (e = ND_flat_out(v).list[j]); j++)
		    if (ED_edge_type(e) == FLATORDER) {
			delete_flat_edge(e);
#ifdef WITH_CGRAPH
			free(e->base.data);
#endif
			free(e);
			j--;
		    }
	    }
	}
	free_matrix(GD_rank(g)[r].flat);
    }
    if (Verbose)
	fprintf(stderr, "mincross %s: %d crossings, %.2f secs.\n",
		agnameof(g), nc, elapsed_sec());
}

static node_t *neighbor(node_t * v, int dir)
{
    node_t *rv;

    rv = NULL;
assert(v);
    if (dir < 0) {
	if (ND_order(v) > 0)
	    rv = GD_rank(Root)[ND_rank(v)].v[ND_order(v) - 1];
    } else
	rv = GD_rank(Root)[ND_rank(v)].v[ND_order(v) + 1];
assert((rv == 0) || (ND_order(rv)-ND_order(v))*dir > 0);
    return rv;
}

static int is_a_normal_node_of(graph_t * g, node_t * v)
{
    return ((ND_node_type(v) == NORMAL) && agcontains(g, v));
}

static int is_a_vnode_of_an_edge_of(graph_t * g, node_t * v)
{
    if ((ND_node_type(v) == VIRTUAL)
	&& (ND_in(v).size == 1) && (ND_out(v).size == 1)) {
	edge_t *e = ND_out(v).list[0];
	while (ED_edge_type(e) != NORMAL)
	    e = ED_to_orig(e);
	if (agcontains(g, e))
	    return TRUE;
    }
    return FALSE;
}

static int inside_cluster(graph_t * g, node_t * v)
{
    return (is_a_normal_node_of(g, v) | is_a_vnode_of_an_edge_of(g, v));
}

static node_t *furthestnode(graph_t * g, node_t * v, int dir)
{
    node_t *u, *rv;

    rv = u = v;
    while ((u = neighbor(u, dir))) {
	if (is_a_normal_node_of(g, u))
	    rv = u;
	else if (is_a_vnode_of_an_edge_of(g, u))
	    rv = u;
    }
    return rv;
}

void save_vlist(graph_t * g)
{
    int r;

    if (GD_rankleader(g))
	for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	    GD_rankleader(g)[r] = GD_rank(g)[r].v[0];
	}
}

void rec_save_vlists(graph_t * g)
{
    int c;

    save_vlist(g);
    for (c = 1; c <= GD_n_cluster(g); c++)
	rec_save_vlists(GD_clust(g)[c]);
}


void rec_reset_vlists(graph_t * g)
{
    int r, c;
    node_t *u, *v, *w;

    /* fix vlists of sub-clusters */
    for (c = 1; c <= GD_n_cluster(g); c++)
	rec_reset_vlists(GD_clust(g)[c]);

    if (GD_rankleader(g))
	for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	    v = GD_rankleader(g)[r];
#ifdef DEBUG
	    node_in_root_vlist(v);
#endif
	    u = furthestnode(g, v, -1);
	    w = furthestnode(g, v, 1);
	    GD_rankleader(g)[r] = u;
#ifdef DEBUG
	    assert(GD_rank(g->root)[r].v[ND_order(u)] == u);
#endif
#ifndef WITH_CGRAPH
	    GD_rank(g)[r].v = ND_rank(g->root)[r].v + ND_order(u);
#else /* WITH_CGRAPH */
	    GD_rank(g)[r].v = GD_rank(agroot(g))[r].v + ND_order(u);
#endif /* WITH_CGRAPH */
	    GD_rank(g)[r].n = ND_order(w) - ND_order(u) + 1;
	}
}

#ifdef WITH_CGRAPH
/* realFillRanks:
 * The structures in crossing minimization and positioning require
 * that clusters have some node on each rank. This function recursively
 * guarantees this property. It takes into account nodes and edges in
 * a cluster, the latter causing dummy nodes for intervening ranks.
 * For any rank without node, we create a real node of small size. This
 * is stored in the subgraph sg, for easy removal later.
 *
 * I believe it is not necessary to do this for the root graph, as these
 * are laid out one component at a time and these will necessarily have a
 * node on each rank from source to sink levels.
 */
static Agraph_t*
realFillRanks (Agraph_t* g, int rnks[], int rnks_sz, Agraph_t* sg)
{
    int i, c;
    Agedge_t* e;
    Agnode_t* n;

    for (c = 1; c <= GD_n_cluster(g); c++)
	sg = realFillRanks (GD_clust(g)[c], rnks, rnks_sz, sg);

    if (agroot(g) == g)
	return sg;
    memset (rnks, 0, sizeof(int)*rnks_sz);
    for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
	rnks[ND_rank(n)] = 1;
	for (e = agfstout(g,n); e; e = agnxtout(g,e)) {
	    for (i = ND_rank(n)+1; i <= ND_rank(aghead(e)); i++) 
		rnks[i] = 1;
	}
    }
    for (i = GD_minrank(g); i <= GD_maxrank(g); i++) {
	if (rnks[i] == 0) {
	    if (!sg) {
		sg = agsubg (agroot(g), "_new_rank", 1);
	    }
	    n = agnode (sg, NULL, 1);
	    agbindrec(n, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);
	    ND_rank(n) = i;
	    ND_lw(n) = ND_rw(n) = 0.5;
	    ND_ht(n) = 1;
	    ND_UF_size(n) = 1;
	    alloc_elist(4, ND_in(n));
	    alloc_elist(4, ND_out(n));
	    agsubnode (g, n, 1);
	}
    }
    return sg;
}

static void
fillRanks (Agraph_t* g)
{
    Agraph_t* sg;
    int rnks_sz = GD_maxrank(g) + 2;
    int* rnks = N_NEW(rnks_sz, int);
    sg = realFillRanks (g, rnks, rnks_sz, NULL);
    free (rnks);
}
#endif

static void init_mincross(graph_t * g)
{
    int size;

    if (Verbose)
	start_timer();

    ReMincross = FALSE;
    Root = g;
    /* alloc +1 for the null terminator usage in do_ordering() */
    /* also, the +1 avoids attempts to alloc 0 sizes, something
       that efence complains about */
    size = agnedges(agroot(g)) + 1;
    TE_list = N_NEW(size, edge_t *);
    TI_list = N_NEW(size, int);
    mincross_options(g);
#ifdef WITH_CGRAPH
    if (GD_flags(g) & NEW_RANK)
	fillRanks (g);
#endif
    class2(g);
    decompose(g, 1);
    allocate_ranks(g);
    ordered_edges(g);
    GlobalMinRank = GD_minrank(g);
    GlobalMaxRank = GD_maxrank(g);
}

void flat_rev(Agraph_t * g, Agedge_t * e)
{
    int j;
    Agedge_t *rev;

    if (!ND_flat_out(aghead(e)).list)
	rev = NULL;
    else
	for (j = 0; (rev = ND_flat_out(aghead(e)).list[j]); j++)
	    if (aghead(rev) == agtail(e))
		break;
    if (rev) {
	merge_oneway(e, rev);
	if (ED_to_virt(e) == 0)
	    ED_to_virt(e) = rev;
	if ((ED_edge_type(rev) == FLATORDER)
	    && (ED_to_orig(rev) == 0))
	    ED_to_orig(rev) = e;
	elist_append(e, ND_other(agtail(e)));
    } else {
	rev = new_virtual_edge(aghead(e), agtail(e), e);
	if (ED_edge_type(e) == FLATORDER)
	    ED_edge_type(rev) = FLATORDER;
	else
	    ED_edge_type(rev) = REVERSED;
	ED_label(rev) = ED_label(e);
	flat_edge(g, rev);
    }
}

static void flat_search(graph_t * g, node_t * v)
{
    int i;
    boolean hascl;
    edge_t *e;
    adjmatrix_t *M = GD_rank(g)[ND_rank(v)].flat;

    ND_mark(v) = TRUE;
    ND_onstack(v) = TRUE;
#ifndef WITH_CGRAPH
    hascl = (ND_n_cluster(g->root) > 0);
#else /* WITH_CGRAPH */
    hascl = (GD_n_cluster(agroot(g)) > 0);
#endif /* WITH_CGRAPH */
    if (ND_flat_out(v).list)
	for (i = 0; (e = ND_flat_out(v).list[i]); i++) {
	    if (hascl
		&& NOT(agcontains(g, agtail(e)) && agcontains(g, aghead(e))))
		continue;
	    if (ED_weight(e) == 0)
		continue;
	    if (ND_onstack(aghead(e)) == TRUE) {
		assert(flatindex(aghead(e)) < M->nrows);
		assert(flatindex(agtail(e)) < M->ncols);
		ELT(M, flatindex(aghead(e)), flatindex(agtail(e))) = 1;
		delete_flat_edge(e);
		i--;
		if (ED_edge_type(e) == FLATORDER)
		    continue;
		flat_rev(g, e);
	    } else {
		assert(flatindex(aghead(e)) < M->nrows);
		assert(flatindex(agtail(e)) < M->ncols);
		ELT(M, flatindex(agtail(e)), flatindex(aghead(e))) = 1;
		if (ND_mark(aghead(e)) == FALSE)
		    flat_search(g, aghead(e));
	    }
	}
    ND_onstack(v) = FALSE;
}

static void flat_breakcycles(graph_t * g)
{
    int i, r, flat;
    node_t *v;

    for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	flat = 0;
	for (i = 0; i < GD_rank(g)[r].n; i++) {
	    v = GD_rank(g)[r].v[i];
	    ND_mark(v) = ND_onstack(v) = FALSE;
	    flatindex(v) = i;
	    if ((ND_flat_out(v).size > 0) && (flat == 0)) {
		GD_rank(g)[r].flat =
		    new_matrix(GD_rank(g)[r].n, GD_rank(g)[r].n);
		flat = 1;
	    }
	}
	if (flat) {
	    for (i = 0; i < GD_rank(g)[r].n; i++) {
		v = GD_rank(g)[r].v[i];
		if (ND_mark(v) == FALSE)
		    flat_search(g, v);
	    }
	}
    }
}

/* allocate_ranks:
 * Allocate rank structure, determining number of nodes per rank.
 * Note that no nodes are put into the structure yet.
 */
void allocate_ranks(graph_t * g)
{
    int r, low, high, *cn;
    node_t *n;
    edge_t *e;

    cn = N_NEW(GD_maxrank(g) + 2, int);	/* must be 0 based, not GD_minrank */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	cn[ND_rank(n)]++;
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    low = ND_rank(agtail(e));
	    high = ND_rank(aghead(e));
	    if (low > high) {
		int t = low;
		low = high;
		high = t;
	    }
	    for (r = low + 1; r < high; r++)
		cn[r]++;
	}
    }
    GD_rank(g) = N_NEW(GD_maxrank(g) + 2, rank_t);
    for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	GD_rank(g)[r].an = GD_rank(g)[r].n = cn[r];
	GD_rank(g)[r].av = GD_rank(g)[r].v = N_NEW(cn[r] + 1, node_t *);
    }
    free(cn);
}

/* install a node at the current right end of its rank */
void install_in_rank(graph_t * g, node_t * n)
{
    int i, r;

    r = ND_rank(n);
    i = GD_rank(g)[r].n;
    if (GD_rank(g)[r].an <= 0) {
	agerr(AGERR, "install_in_rank, line %d: %s %s rank %d i = %d an = 0\n",
	      __LINE__, agnameof(g), agnameof(n), r, i);
	return;
    }

    GD_rank(g)[r].v[i] = n;
    ND_order(n) = i;
    GD_rank(g)[r].n++;
    assert(GD_rank(g)[r].n <= GD_rank(g)[r].an);
#ifdef DEBUG
    {
	node_t *v;

	for (v = GD_nlist(g); v; v = ND_next(v))
	    if (v == n)
		break;
	assert(v != NULL);
    }
#endif
    if (ND_order(n) > GD_rank(Root)[r].an) {
	agerr(AGERR, "install_in_rank, line %d: ND_order(%s) [%d] > GD_rank(Root)[%d].an [%d]\n",
	      __LINE__, agnameof(n), ND_order(n), r, GD_rank(Root)[r].an);
	return;
    }
    if ((r < GD_minrank(g)) || (r > GD_maxrank(g))) {
	agerr(AGERR, "install_in_rank, line %d: rank %d not in rank range [%d,%d]\n",
	      __LINE__, r, GD_minrank(g), GD_maxrank(g));
	return;
    }
    if (GD_rank(g)[r].v + ND_order(n) >
	GD_rank(g)[r].av + GD_rank(Root)[r].an) {
	agerr(AGERR, "install_in_rank, line %d: GD_rank(g)[%d].v + ND_order(%s) [%d] > GD_rank(g)[%d].av + GD_rank(Root)[%d].an [%d]\n",
	      __LINE__, r, agnameof(n),GD_rank(g)[r].v + ND_order(n), r, r, GD_rank(g)[r].av+GD_rank(Root)[r].an);
	return;
    }
}

/*	install nodes in ranks. the initial ordering ensure that series-parallel
 *	graphs such as trees are drawn with no crossings.  it tries searching
 *	in- and out-edges and takes the better of the two initial orderings.
 */
void build_ranks(graph_t * g, int pass)
{
    int i, j;
    node_t *n, *n0;
    edge_t **otheredges;
    nodequeue *q;

    q = new_queue(GD_n_nodes(g));
    for (n = GD_nlist(g); n; n = ND_next(n))
	MARK(n) = FALSE;

#ifdef DEBUG
    {
	edge_t *e;
	for (n = GD_nlist(g); n; n = ND_next(n)) {
	    for (i = 0; (e = ND_out(n).list[i]); i++)
		assert(MARK(aghead(e)) == FALSE);
	    for (i = 0; (e = ND_in(n).list[i]); i++)
		assert(MARK(agtail(e)) == FALSE);
	}
    }
#endif

    for (i = GD_minrank(g); i <= GD_maxrank(g); i++)
	GD_rank(g)[i].n = 0;

    for (n = GD_nlist(g); n; n = ND_next(n)) {
	otheredges = ((pass == 0) ? ND_in(n).list : ND_out(n).list);
	if (otheredges[0] != NULL)
	    continue;
	if (MARK(n) == FALSE) {
	    MARK(n) = TRUE;
	    enqueue(q, n);
	    while ((n0 = dequeue(q))) {
		if (ND_ranktype(n0) != CLUSTER) {
		    install_in_rank(g, n0);
		    enqueue_neighbors(q, n0, pass);
		} else {
		    install_cluster(g, n0, pass, q);
		}
	    }
	}
    }
    if (dequeue(q))
	agerr(AGERR, "surprise\n");
    for (i = GD_minrank(g); i <= GD_maxrank(g); i++) {
	GD_rank(Root)[i].valid = FALSE;
	if (GD_flip(g) && (GD_rank(g)[i].n > 0)) {
	    int n, ndiv2;
	    node_t **vlist = GD_rank(g)[i].v;
	    n = GD_rank(g)[i].n - 1;
	    ndiv2 = n / 2;
	    for (j = 0; j <= ndiv2; j++)
		exchange(vlist[j], vlist[n - j]);
	}
    }

    if ((g == agroot(g)) && ncross(g) > 0)
	transpose(g, FALSE);
    free_queue(q);
}

void enqueue_neighbors(nodequeue * q, node_t * n0, int pass)
{
    int i;
    edge_t *e;

    if (pass == 0) {
	for (i = 0; i < ND_out(n0).size; i++) {
	    e = ND_out(n0).list[i];
	    if ((MARK(aghead(e))) == FALSE) {
		MARK(aghead(e)) = TRUE;
		enqueue(q, aghead(e));
	    }
	}
    } else {
	for (i = 0; i < ND_in(n0).size; i++) {
	    e = ND_in(n0).list[i];
	    if ((MARK(agtail(e))) == FALSE) {
		MARK(agtail(e)) = TRUE;
		enqueue(q, agtail(e));
	    }
	}
    }
}

static int constraining_flat_edge(Agraph_t *g, Agnode_t *v, Agedge_t *e)
{
	if (ED_weight(e) == 0) return FALSE;
	if (!inside_cluster(g,agtail(e))) return FALSE;
	if (!inside_cluster(g,aghead(e))) return FALSE;
	return TRUE;
}


/* construct nodes reachable from 'here' in post-order.
* This is the same as doing a topological sort in reverse order.
*/
static int postorder(graph_t * g, node_t * v, node_t ** list, int r)
{
    edge_t *e;
    int i, cnt = 0;

    MARK(v) = TRUE;
    if (ND_flat_out(v).size > 0) {
	for (i = 0; (e = ND_flat_out(v).list[i]); i++) {
	    if (!constraining_flat_edge(g,v,e)) continue;
	    if (MARK(aghead(e)) == FALSE)
		cnt += postorder(g, aghead(e), list + cnt, r);
	}
    }
    assert(ND_rank(v) == r);
    list[cnt++] = v;
    return cnt;
}

static void flat_reorder(graph_t * g)
{
    int i, j, r, pos, n_search, local_in_cnt, local_out_cnt, base_order;
    node_t *v, **left, **right, *t;
    node_t **temprank = NULL;
    edge_t *flat_e, *e;

    if (GD_has_flat_edges(g) == FALSE)
	return;
    for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	if (GD_rank(g)[r].n == 0) continue;
	base_order = ND_order(GD_rank(g)[r].v[0]);
	for (i = 0; i < GD_rank(g)[r].n; i++)
	    MARK(GD_rank(g)[r].v[i]) = FALSE;
	temprank = ALLOC(i + 1, temprank, node_t *);
	pos = 0;

	/* construct reverse topological sort order in temprank */
	for (i = 0; i < GD_rank(g)[r].n; i++) {
	    if (GD_flip(g)) v = GD_rank(g)[r].v[i];
	    else v = GD_rank(g)[r].v[GD_rank(g)[r].n - i - 1];

	    local_in_cnt = local_out_cnt = 0;
	    for (j = 0; j < ND_flat_in(v).size; j++) {
		flat_e = ND_flat_in(v).list[j];
		if (constraining_flat_edge(g,v,flat_e)) local_in_cnt++;
	    }
	    for (j = 0; j < ND_flat_out(v).size; j++) {
		flat_e = ND_flat_out(v).list[j];
		if (constraining_flat_edge(g,v,flat_e)) local_out_cnt++;
	    }
	    if ((local_in_cnt == 0) && (local_out_cnt == 0))
		temprank[pos++] = v;
	    else {
		if ((MARK(v) == FALSE) && (local_in_cnt == 0)) {
		    left = temprank + pos;
		    n_search = postorder(g, v, left, r);
		    pos += n_search;
		}
	    }
	}

	if (pos) {
	    if (GD_flip(g) == FALSE) {
		left = temprank;
		right = temprank + pos - 1;
		while (left < right) {
		    t = *left;
		    *left = *right;
		    *right = t;
		    left++;
		    right--;
		}
	    }
	    for (i = 0; i < GD_rank(g)[r].n; i++) {
		v = GD_rank(g)[r].v[i] = temprank[i];
		ND_order(v) = i + base_order;
	    }

	    /* nonconstraint flat edges must be made LR */
	    for (i = 0; i < GD_rank(g)[r].n; i++) {
		v = GD_rank(g)[r].v[i];
		if (ND_flat_out(v).list) {
		    for (j = 0; (e = ND_flat_out(v).list[j]); j++) {
			if ( ((GD_flip(g) == FALSE) && (ND_order(aghead(e)) < ND_order(agtail(e)))) ||
				 ( (GD_flip(g)) && (ND_order(aghead(e)) > ND_order(agtail(e)) ))) {
			    assert(constraining_flat_edge(g,v,e) == FALSE);
			    delete_flat_edge(e);
			    j--;
			    flat_rev(g, e);
			}
		    }
		}
	    }
	    /* postprocess to restore intended order */
	}
	/* else do no harm! */
	GD_rank(Root)[r].valid = FALSE;
    }
    if (temprank)
	free(temprank);
}

static void reorder(graph_t * g, int r, int reverse, int hasfixed)
{
    int changed = 0, nelt;
    boolean muststay, sawclust;
    node_t **vlist = GD_rank(g)[r].v;
    node_t **lp, **rp, **ep = vlist + GD_rank(g)[r].n;

    for (nelt = GD_rank(g)[r].n - 1; nelt >= 0; nelt--) {
	lp = vlist;
	while (lp < ep) {
	    /* find leftmost node that can be compared */
	    while ((lp < ep) && (ND_mval(*lp) < 0))
		lp++;
	    if (lp >= ep)
		break;
	    /* find the node that can be compared */
	    sawclust = muststay = FALSE;
	    for (rp = lp + 1; rp < ep; rp++) {
		if (sawclust && ND_clust(*rp))
		    continue;	/* ### */
		if (left2right(g, *lp, *rp)) {
		    muststay = TRUE;
		    break;
		}
		if (ND_mval(*rp) >= 0)
		    break;
		if (ND_clust(*rp))
		    sawclust = TRUE;	/* ### */
	    }
	    if (rp >= ep)
		break;
	    if (muststay == FALSE) {
		register int p1 = (ND_mval(*lp));
		register int p2 = (ND_mval(*rp));
		if ((p1 > p2) || ((p1 == p2) && (reverse))) {
		    exchange(*lp, *rp);
		    changed++;
		}
	    }
	    lp = rp;
	}
	if ((hasfixed == FALSE) && (reverse == FALSE))
	    ep--;
    }

    if (changed) {
	GD_rank(Root)[r].valid = FALSE;
	if (r > 0)
	    GD_rank(Root)[r - 1].valid = FALSE;
    }
}

static void mincross_step(graph_t * g, int pass)
{
    int r, other, first, last, dir;
    int hasfixed, reverse;

    if ((pass % 4) < 2)
	reverse = TRUE;
    else
	reverse = FALSE;
    if (pass % 2) {
	r = GD_maxrank(g) - 1;
	dir = -1;
    } /* up pass */
    else {
	r = 1;
	dir = 1;
    }				/* down pass */

    if (pass % 2 == 0) {	/* down pass */
	first = GD_minrank(g) + 1;
	if (GD_minrank(g) > GD_minrank(Root))
	    first--;
	last = GD_maxrank(g);
	dir = 1;
    } else {			/* up pass */
	first = GD_maxrank(g) - 1;
	last = GD_minrank(g);
	if (GD_maxrank(g) < GD_maxrank(Root))
	    first++;
	dir = -1;
    }

    for (r = first; r != last + dir; r += dir) {
	other = r - dir;
	hasfixed = medians(g, r, other);
	reorder(g, r, reverse, hasfixed);
    }
    transpose(g, NOT(reverse));
}

static int local_cross(elist l, int dir)
{
    int i, j, is_out;
    int cross = 0;
    edge_t *e, *f;
    if (dir > 0)
	is_out = TRUE;
    else
	is_out = FALSE;
    for (i = 0; (e = l.list[i]); i++) {
	if (is_out)
	    for (j = i + 1; (f = l.list[j]); j++) {
		if ((ND_order(aghead(f)) - ND_order(aghead(e)))
			 * (ED_tail_port(f).p.x - ED_tail_port(e).p.x) < 0)
		    cross += ED_xpenalty(e) * ED_xpenalty(f);
	} else
	    for (j = i + 1; (f = l.list[j]); j++) {
		if ((ND_order(agtail(f)) - ND_order(agtail(e)))
			* (ED_head_port(f).p.x - ED_head_port(e).p.x) < 0)
		    cross += ED_xpenalty(e) * ED_xpenalty(f);
	    }
    }
    return cross;
}

static int rcross(graph_t * g, int r)
{
    static int *Count, C;
    int top, bot, cross, max, i, k;
    node_t **rtop, *v;

    cross = 0;
    max = 0;
    rtop = GD_rank(g)[r].v;

    if (C <= GD_rank(Root)[r + 1].n) {
	C = GD_rank(Root)[r + 1].n + 1;
	Count = ALLOC(C, Count, int);
    }

    for (i = 0; i < GD_rank(g)[r + 1].n; i++)
	Count[i] = 0;

    for (top = 0; top < GD_rank(g)[r].n; top++) {
	register edge_t *e;
	if (max > 0) {
	    for (i = 0; (e = ND_out(rtop[top]).list[i]); i++) {
		for (k = ND_order(aghead(e)) + 1; k <= max; k++)
		    cross += Count[k] * ED_xpenalty(e);
	    }
	}
	for (i = 0; (e = ND_out(rtop[top]).list[i]); i++) {
	    register int inv = ND_order(aghead(e));
	    if (inv > max)
		max = inv;
	    Count[inv] += ED_xpenalty(e);
	}
    }
    for (top = 0; top < GD_rank(g)[r].n; top++) {
	v = GD_rank(g)[r].v[top];
	if (ND_has_port(v))
	    cross += local_cross(ND_out(v), 1);
    }
    for (bot = 0; bot < GD_rank(g)[r + 1].n; bot++) {
	v = GD_rank(g)[r + 1].v[bot];
	if (ND_has_port(v))
	    cross += local_cross(ND_in(v), -1);
    }
    return cross;
}

int ncross(graph_t * g)
{
    int r, count, nc;

    g = Root;
    count = 0;
    for (r = GD_minrank(g); r < GD_maxrank(g); r++) {
	if (GD_rank(g)[r].valid)
	    count += GD_rank(g)[r].cache_nc;
	else {
	    nc = GD_rank(g)[r].cache_nc = rcross(g, r);
	    count += nc;
	    GD_rank(g)[r].valid = TRUE;
	}
    }
    return count;
}

static int ordercmpf(int *i0, int *i1)
{
    return (*i0) - (*i1);
}

/* flat_mval:
 * Calculate a mval for nodes with no in or out non-flat edges.
 * Assume (ND_out(n).size == 0) && (ND_in(n).size == 0)
 * Find flat edge a->n where a has the largest order and set
 * n.mval = a.mval+1, assuming a.mval is defined (>=0).
 * If there are no flat in edges, find flat edge n->a where a 
 * has the smallest order and set * n.mval = a.mval-1, assuming 
 * a.mval is > 0.
 * Return true if n.mval is left -1, indicating a fixed node for sorting.
 */
static int flat_mval(node_t * n)
{
    int i;
    edge_t *e, **fl;
    node_t *nn;

    if (ND_flat_in(n).size > 0) {
	fl = ND_flat_in(n).list;
	nn = agtail(fl[0]);
	for (i = 1; (e = fl[i]); i++)
	    if (ND_order(agtail(e)) > ND_order(nn))
		nn = agtail(e);
	if (ND_mval(nn) >= 0) {
	    ND_mval(n) = ND_mval(nn) + 1;
	    return FALSE;
	}
    } else if (ND_flat_out(n).size > 0) {
	fl = ND_flat_out(n).list;
	nn = aghead(fl[0]);
	for (i = 1; (e = fl[i]); i++)
	    if (ND_order(aghead(e)) < ND_order(nn))
		nn = aghead(e);
	if (ND_mval(nn) > 0) {
	    ND_mval(n) = ND_mval(nn) - 1;
	    return FALSE;
	}
    }
    return TRUE;
}

#define VAL(node,port) (MC_SCALE * ND_order(node) + (port).order)

static boolean medians(graph_t * g, int r0, int r1)
{
    int i, j, j0, lm, rm, lspan, rspan, *list;
    node_t *n, **v;
    edge_t *e;
    boolean hasfixed = FALSE;

    list = TI_list;
    v = GD_rank(g)[r0].v;
    for (i = 0; i < GD_rank(g)[r0].n; i++) {
	n = v[i];
	j = 0;
	if (r1 > r0)
	    for (j0 = 0; (e = ND_out(n).list[j0]); j0++) {
		if (ED_xpenalty(e) > 0)
		    list[j++] = VAL(aghead(e), ED_head_port(e));
	} else
	    for (j0 = 0; (e = ND_in(n).list[j0]); j0++) {
		if (ED_xpenalty(e) > 0)
		    list[j++] = VAL(agtail(e), ED_tail_port(e));
	    }
	switch (j) {
	case 0:
	    ND_mval(n) = -1;
	    break;
	case 1:
	    ND_mval(n) = list[0];
	    break;
	case 2:
	    ND_mval(n) = (list[0] + list[1]) / 2;
	    break;
	default:
	    qsort(list, j, sizeof(int), (qsort_cmpf) ordercmpf);
	    if (j % 2)
		ND_mval(n) = list[j / 2];
	    else {
		/* weighted median */
		rm = j / 2;
		lm = rm - 1;
		rspan = list[j - 1] - list[rm];
		lspan = list[lm] - list[0];
		if (lspan == rspan)
		    ND_mval(n) = (list[lm] + list[rm]) / 2;
		else {
		    int w = list[lm] * rspan + list[rm] * lspan;
		    ND_mval(n) = w / (lspan + rspan);
		}
	    }
	}
    }
    for (i = 0; i < GD_rank(g)[r0].n; i++) {
	n = v[i];
	if ((ND_out(n).size == 0) && (ND_in(n).size == 0))
	    hasfixed |= flat_mval(n);
    }
    return hasfixed;
}

static int nodeposcmpf(node_t ** n0, node_t ** n1)
{
    return (ND_order(*n0) - ND_order(*n1));
}

static int edgeidcmpf(edge_t ** e0, edge_t ** e1)
{
    return (AGSEQ(*e0) - AGSEQ(*e1));
}

/* following code deals with weights of edges of "virtual" nodes */
#define ORDINARY	0
#define SINGLETON	1
#define	VIRTUALNODE	2
#define NTYPES		3

#define C_EE		1
#define C_VS		2
#define C_SS		2
#define C_VV		4

static int table[NTYPES][NTYPES] = {
    /* ordinary */ {C_EE, C_EE, C_EE},
    /* singleton */ {C_EE, C_SS, C_VS},
    /* virtual */ {C_EE, C_VS, C_VV}
};

static int endpoint_class(node_t * n)
{
    if (ND_node_type(n) == VIRTUAL)
	return VIRTUALNODE;
    if (ND_weight_class(n) <= 1)
	return SINGLETON;
    return ORDINARY;
}

void virtual_weight(edge_t * e)
{
    int t;
    t = table[endpoint_class(agtail(e))][endpoint_class(aghead(e))];
    ED_weight(e) *= t;
}

#ifdef DEBUG
void check_rs(graph_t * g, int null_ok)
{
    int i, r;
    node_t *v, *prev;

    fprintf(stderr, "\n\n%s:\n", agnameof(g));
    for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	fprintf(stderr, "%d: ", r);
	prev = NULL;
	for (i = 0; i < GD_rank(g)[r].n; i++) {
	    v = GD_rank(g)[r].v[i];
	    if (v == NULL) {
		fprintf(stderr, "NULL\t");
		if (null_ok == FALSE)
		    abort();
	    } else {
		fprintf(stderr, "%s(%d)\t", agnameof(v), ND_mval(v));
		assert(ND_rank(v) == r);
		assert(v != prev);
		prev = v;
	    }
	}
	fprintf(stderr, "\n");
    }
}

void check_order(void)
{
    int i, r;
    node_t *v;
    graph_t *g = Root;

    for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	assert(GD_rank(g)[r].v[GD_rank(g)[r].n] == NULL);
	for (i = 0; (v = GD_rank(g)[r].v[i]); i++) {
	    assert(ND_rank(v) == r);
	    assert(ND_order(v) == i);
	}
    }
}
#endif

static void mincross_options(graph_t * g)
{
    char *p;
    double f;

    /* set default values */
    MinQuit = 8;
    MaxIter = 24;
    Convergence = .995;

    p = agget(g, "mclimit");
    if (p && ((f = atof(p)) > 0.0)) {
	MinQuit = MAX(1, MinQuit * f);
	MaxIter = MAX(1, MaxIter * f);
    }
}

#ifdef DEBUG
void check_exchange(node_t * v, node_t * w)
{
    int i, r;
    node_t *u;

    if ((ND_clust(v) == NULL) && (ND_clust(w) == NULL))
	return;
    assert((ND_clust(v) == NULL) || (ND_clust(w) == NULL));
    assert(ND_rank(v) == ND_rank(w));
    assert(ND_order(v) < ND_order(w));
    r = ND_rank(v);

    for (i = ND_order(v) + 1; i < ND_order(w); i++) {
	u = GD_rank(agraphof(v))[r].v[i];
	if (ND_clust(u))
	    abort();
    }
}

void check_vlists(graph_t * g)
{
    int c, i, j, r;
    node_t *u;

    for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	for (i = 0; i < GD_rank(g)[r].n; i++) {
	    u = GD_rank(g)[r].v[i];
	    j = ND_order(u);
	    assert(GD_rank(Root)[r].v[j] == u);
	}
	if (GD_rankleader(g)) {
	    u = GD_rankleader(g)[r];
	    j = ND_order(u);
	    assert(GD_rank(Root)[r].v[j] == u);
	}
    }
    for (c = 1; c <= GD_n_cluster(g); c++)
	check_vlists(GD_clust(g)[c]);
}

void node_in_root_vlist(node_t * n)
{
    node_t **vptr;

    for (vptr = GD_rank(Root)[ND_rank(n)].v; *vptr; vptr++)
	if (*vptr == n)
	    break;
    if (*vptr == 0)
	abort();
}
#endif				/* DEBUG code */
