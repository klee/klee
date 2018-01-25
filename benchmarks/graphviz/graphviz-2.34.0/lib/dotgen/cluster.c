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


#include "dot.h"

static node_t*
map_interclust_node(node_t * n)
{
    node_t *rv;

#ifndef WITH_CGRAPH
    if ((ND_clust(n) == NULL) || (ND_clust(n)->u.expanded))
#else /* WITH_CGRAPH */
    if ((ND_clust(n) == NULL) || (  GD_expanded(ND_clust(n))) )
#endif /* WITH_CGRAPH */
	rv = n;
    else
#ifndef WITH_CGRAPH
	rv = ND_clust(n)->u.rankleader[ND_rank(n)];
#else /* WITH_CGRAPH */
	rv = GD_rankleader(ND_clust(n))[ND_rank(n)];
#endif /* WITH_CGRAPH */
    return rv;
}

/* make d slots starting at position pos (where 1 already exists) */
static void 
make_slots(graph_t * root, int r, int pos, int d)
{
    int i;
    node_t *v, **vlist;
#ifndef WITH_CGRAPH
    vlist = ND_rank(root)[r].v;
#else /* WITH_CGRAPH */
    vlist = GD_rank(root)[r].v;
#endif /* WITH_CGRAPH */
    if (d <= 0) {
#ifndef WITH_CGRAPH
	for (i = pos - d + 1; i < ND_rank(root)[r].n; i++) {
#else /* WITH_CGRAPH */
	for (i = pos - d + 1; i < GD_rank(root)[r].n; i++) {
#endif /* WITH_CGRAPH */
	    v = vlist[i];
	    ND_order(v) = i + d - 1;
	    vlist[ND_order(v)] = v;
	}
#ifndef WITH_CGRAPH
	for (i = ND_rank(root)[r].n + d - 1; i < ND_rank(root)[r].n; i++)
#else /* WITH_CGRAPH */
	for (i = GD_rank(root)[r].n + d - 1; i < GD_rank(root)[r].n; i++)
#endif /* WITH_CGRAPH */
	    vlist[i] = NULL;
    } else {
/*assert(ND_rank(root)[r].n + d - 1 <= ND_rank(root)[r].an);*/
#ifndef WITH_CGRAPH
	for (i = ND_rank(root)[r].n - 1; i > pos; i--) {
#else /* WITH_CGRAPH */
	for (i = GD_rank(root)[r].n - 1; i > pos; i--) {
#endif /* WITH_CGRAPH */
	    v = vlist[i];
	    ND_order(v) = i + d - 1;
	    vlist[ND_order(v)] = v;
	}
	for (i = pos + 1; i < pos + d; i++)
	    vlist[i] = NULL;
    }
#ifndef WITH_CGRAPH
    ND_rank(root)[r].n += d - 1;
#else /* WITH_CGRAPH */
    GD_rank(root)[r].n += d - 1;
#endif /* WITH_CGRAPH */
}

static node_t* 
clone_vn(graph_t * g, node_t * vn)
{
    node_t *rv;
    int r;

    r = ND_rank(vn);
    make_slots(g, r, ND_order(vn), 2);
    rv = virtual_node(g);
    ND_lw(rv) = ND_lw(vn);
    ND_rw(rv) = ND_rw(vn);
    ND_rank(rv) = ND_rank(vn);
    ND_order(rv) = ND_order(vn) + 1;
    GD_rank(g)[r].v[ND_order(rv)] = rv;
    return rv;
}

static void 
map_path(node_t * from, node_t * to, edge_t * orig, edge_t * ve, int type)
{
    int r;
    node_t *u, *v;
    edge_t *e;

    assert(ND_rank(from) < ND_rank(to));

    if ((agtail(ve) == from) && (aghead(ve) == to))
	return;

    if (ED_count(ve) > 1) {
	ED_to_virt(orig) = NULL;
	if (ND_rank(to) - ND_rank(from) == 1) {
	    if ((e = find_fast_edge(from, to)) && (ports_eq(orig, e))) {
		merge_oneway(orig, e);
		if ((ND_node_type(from) == NORMAL)
		    && (ND_node_type(to) == NORMAL))
		    other_edge(orig);
		return;
	    }
	}
	u = from;
	for (r = ND_rank(from); r < ND_rank(to); r++) {
	    if (r < ND_rank(to) - 1)
		v = clone_vn(agraphof(from), aghead(ve));
	    else
		v = to;
	    e = virtual_edge(u, v, orig);
	    ED_edge_type(e) = type;
	    u = v;
	    ED_count(ve)--;
	    ve = ND_out(aghead(ve)).list[0];
	}
    } else {
	if (ND_rank(to) - ND_rank(from) == 1) {
	    if ((ve = find_fast_edge(from, to)) && (ports_eq(orig, ve))) {
		/*ED_to_orig(ve) = orig; */
		ED_to_virt(orig) = ve;
		ED_edge_type(ve) = type;
		ED_count(ve)++;
		if ((ND_node_type(from) == NORMAL)
		    && (ND_node_type(to) == NORMAL))
		    other_edge(orig);
	    } else {
		ED_to_virt(orig) = NULL;
		ve = virtual_edge(from, to, orig);
		ED_edge_type(ve) = type;
	    }
	}
	if (ND_rank(to) - ND_rank(from) > 1) {
	    e = ve;
	    if (agtail(ve) != from) {
		ED_to_virt(orig) = NULL;
		e = ED_to_virt(orig) = virtual_edge(from, aghead(ve), orig);
		delete_fast_edge(ve);
	    } else
		e = ve;
	    while (ND_rank(aghead(e)) != ND_rank(to))
		e = ND_out(aghead(e)).list[0];
	    if (aghead(e) != to) {
		ve = e;
		e = virtual_edge(agtail(e), to, orig);
		ED_edge_type(e) = type;
		delete_fast_edge(ve);
	    }
	}
    }
}

static void 
make_interclust_chain(graph_t * g, node_t * from, node_t * to, edge_t * orig)
{
    int newtype;
    node_t *u, *v;

    u = map_interclust_node(from);
    v = map_interclust_node(to);
    if ((u == from) && (v == to))
	newtype = VIRTUAL;
    else
	newtype = CLUSTER_EDGE;
    map_path(u, v, orig, ED_to_virt(orig), newtype);
}

/* 
 * attach and install edges between clusters.
 * essentially, class2() for interclust edges.
 */
void interclexp(graph_t * subg)
{
    graph_t *g;
    node_t *n;
    edge_t *e, *prev, *next;

    g = agroot(subg);
    for (n = agfstnode(subg); n; n = agnxtnode(subg, n)) {

	/* N.B. n may be in a sub-cluster of subg */
	prev = NULL;
	for (e = agfstedge(agroot(subg), n); e; e = next) {
	    next = agnxtedge(agroot(subg), e, n);
	    if (agcontains(subg, e))
		continue;

#ifdef WITH_CGRAPH
		/* canonicalize edge */
	    e = AGMKOUT(e);
#endif
	    /* short/flat multi edges */
	    if (mergeable(prev, e)) {
		if (ND_rank(agtail(e)) == ND_rank(aghead(e)))
		    ED_to_virt(e) = prev;
		else
		    ED_to_virt(e) = NULL;
		if (ED_to_virt(prev) == NULL)
		    continue;	/* internal edge */
		merge_chain(subg, e, ED_to_virt(prev), FALSE);
		safe_other_edge(e);
		continue;
	    }

	    /* flat edges */
	    if (ND_rank(agtail(e)) == ND_rank(aghead(e))) {
		edge_t* fe;
		if ((fe = find_flat_edge(agtail(e), aghead(e))) == NULL) {
		    flat_edge(g, e);
		    prev = e;
		} else if (e != fe) {
		    safe_other_edge(e);
		    if (!ED_to_virt(e)) merge_oneway(e, fe);
		}
		continue;
	    }

/* This assertion is still valid if the new ranking is not used */
#ifndef WITH_CGRAPH
	    assert(ED_to_virt(e) != NULL);
#endif

	    /* forward edges */
	    if (ND_rank(aghead(e)) > ND_rank(agtail(e))) {
		make_interclust_chain(g, agtail(e), aghead(e), e);
		prev = e;
		continue;
	    }

	    /* backward edges */
	    else {
/*
I think that make_interclust_chain should create call other_edge(e) anyway 
				if (agcontains(subg,agtail(e))
					&& agfindedge(subg->root,aghead(e),agtail(e))) other_edge(e);
*/
		make_interclust_chain(g, aghead(e), agtail(e), e);
		prev = e;
	    }
	}
    }
}

static void 
merge_ranks(graph_t * subg)
{
    int i, d, r, pos, ipos;
    node_t *v;
    graph_t *root;

    root = agroot(subg);
    if (GD_minrank(subg) > 0)
#ifndef WITH_CGRAPH
	ND_rank(root)[GD_minrank(subg) - 1].valid = FALSE;
#else /* WITH_CGRAPH */
	GD_rank(root)[GD_minrank(subg) - 1].valid = FALSE;
#endif /* WITH_CGRAPH */
    for (r = GD_minrank(subg); r <= GD_maxrank(subg); r++) {
	d = GD_rank(subg)[r].n;
#ifndef WITH_CGRAPH
	ipos = pos = GD_rankleader(subg)[r]->u.order;
#else /* WITH_CGRAPH */
	ipos = pos = ND_order(GD_rankleader(subg)[r]);
#endif /* WITH_CGRAPH */
	make_slots(root, r, pos, d);
	for (i = 0; i < GD_rank(subg)[r].n; i++) {
#ifndef WITH_CGRAPH
	    v = ND_rank(root)[r].v[pos] = GD_rank(subg)[r].v[i];
#else /* WITH_CGRAPH */
	    v = GD_rank(root)[r].v[pos] = GD_rank(subg)[r].v[i];
#endif /* WITH_CGRAPH */
	    ND_order(v) = pos++;
#ifndef WITH_CGRAPH
	    v->graph = subg->root;
#else /* WITH_CGRAPH */
	/* real nodes automatically have v->root = root graph */
	    if (ND_node_type(v) == VIRTUAL)
		v->root = root;
#endif /* WITH_CGRAPH */
	    delete_fast_node(subg, v);
	    fast_node(agroot(subg), v);
	    GD_n_nodes(agroot(subg))++;
	}
#ifndef WITH_CGRAPH
	GD_rank(subg)[r].v = ND_rank(root)[r].v + ipos;
	ND_rank(root)[r].valid = FALSE;
#else /* WITH_CGRAPH */
	GD_rank(subg)[r].v = GD_rank(root)[r].v + ipos;
	GD_rank(root)[r].valid = FALSE;
#endif /* WITH_CGRAPH */
    }
    if (r < GD_maxrank(root))
	GD_rank(root)[r].valid = FALSE;
    GD_expanded(subg) = TRUE;
}

static void 
remove_rankleaders(graph_t * g)
{
    int r;
    node_t *v;
    edge_t *e;

    for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	v = GD_rankleader(g)[r];

	/* remove the entire chain */
	while ((e = ND_out(v).list[0]))
	    delete_fast_edge(e);
	while ((e = ND_in(v).list[0]))
	    delete_fast_edge(e);
	delete_fast_node(agroot(g), v);
	GD_rankleader(g)[r] = NULL;
    }
}

/* delete virtual nodes of a cluster, and install real nodes or sub-clusters */
void expand_cluster(graph_t * subg)
{
    /* build internal structure of the cluster */
    class2(subg);
    GD_comp(subg).size = 1;
    GD_comp(subg).list[0] = GD_nlist(subg);
    allocate_ranks(subg);
    build_ranks(subg, 0);
    merge_ranks(subg);

    /* build external structure of the cluster */
    interclexp(subg);
    remove_rankleaders(subg);
}

/* this function marks every node in <g> with its top-level cluster under <g> */
void mark_clusters(graph_t * g)
{
    int c;
    node_t *n, *nn, *vn;
    edge_t *orig, *e;
    graph_t *clust;

    /* remove sub-clusters below this level */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (ND_ranktype(n) == CLUSTER)
	    UF_singleton(n);
	ND_clust(n) = NULL;
    }

    for (c = 1; c <= GD_n_cluster(g); c++) {
	clust = GD_clust(g)[c];
	for (n = agfstnode(clust); n; n = nn) {
		nn = agnxtnode(clust,n);
	    if (ND_ranktype(n) != NORMAL) {
		agerr(AGWARN,
		      "%s was already in a rankset, deleted from cluster %s\n",
		      agnameof(n), agnameof(g));
		agdelete(clust,n);
		continue;
	    }
	    UF_setname(n, GD_leader(clust));
	    ND_clust(n) = clust;
	    ND_ranktype(n) = CLUSTER;

	    /* here we mark the vnodes of edges in the cluster */
	    for (orig = agfstout(clust, n); orig;
		 orig = agnxtout(clust, orig)) {
		if ((e = ED_to_virt(orig))) {
#ifndef WITH_CGRAPH
		    while (e && (vn = e->head)->u.node_type == VIRTUAL) {
#else /* WITH_CGRAPH */
		    while (e && ND_node_type(vn =aghead(e)) == VIRTUAL) {
#endif /* WITH_CGRAPH */
			ND_clust(vn) = clust;
			e = ND_out(aghead(e)).list[0];
			/* trouble if concentrators and clusters are mixed */
		    }
		}
	    }
	}
    }
}

void build_skeleton(graph_t * g, graph_t * subg)
{
    int r;
    node_t *v, *prev, *rl;
    edge_t *e;

    prev = NULL;
    GD_rankleader(subg) = N_NEW(GD_maxrank(subg) + 2, node_t *);
    for (r = GD_minrank(subg); r <= GD_maxrank(subg); r++) {
	v = GD_rankleader(subg)[r] = virtual_node(g);
	ND_rank(v) = r;
	ND_ranktype(v) = CLUSTER;
	ND_clust(v) = subg;
	if (prev) {
	    e = virtual_edge(prev, v, NULL);
	    ED_xpenalty(e) *= CL_CROSS;
	}
	prev = v;
    }

    /* set the counts on virtual edges of the cluster skeleton */
    for (v = agfstnode(subg); v; v = agnxtnode(subg, v)) {
	rl = GD_rankleader(subg)[ND_rank(v)];
	ND_UF_size(rl)++;
	for (e = agfstout(subg, v); e; e = agnxtout(subg, e)) {
	    for (r = ND_rank(agtail(e)); r < ND_rank(aghead(e)); r++) {
		ED_count(ND_out(rl).list[0])++;
	    }
	}
    }
    for (r = GD_minrank(subg); r <= GD_maxrank(subg); r++) {
	rl = GD_rankleader(subg)[r];
	if (ND_UF_size(rl) > 1)
	    ND_UF_size(rl)--;
    }
}

void install_cluster(graph_t * g, node_t * n, int pass, nodequeue * q)
{
    int r;
    graph_t *clust;

    clust = ND_clust(n);
    if (GD_installed(clust) != pass + 1) {
	for (r = GD_minrank(clust); r <= GD_maxrank(clust); r++)
	    install_in_rank(g, GD_rankleader(clust)[r]);
	for (r = GD_minrank(clust); r <= GD_maxrank(clust); r++)
	    enqueue_neighbors(q, GD_rankleader(clust)[r], pass);
	GD_installed(clust) = pass + 1;
    }
}

static void mark_lowcluster_basic(Agraph_t * g);
void mark_lowclusters(Agraph_t * root)
{
    Agnode_t *n, *vn;
    Agedge_t *orig, *e;

    /* first, zap any previous cluster labelings */
    for (n = agfstnode(root); n; n = agnxtnode(root, n)) {
	ND_clust(n) = NULL;
	for (orig = agfstout(root, n); orig; orig = agnxtout(root, orig)) {
	    if ((e = ED_to_virt(orig))) {
#ifndef WITH_CGRAPH
		while (e && (vn = e->head)->u.node_type == VIRTUAL) {
#else /* WITH_CGRAPH */
		while (e && (ND_node_type(vn = aghead(e))) == VIRTUAL) {
#endif /* WITH_CGRAPH */
		    ND_clust(vn) = NULL;
		    e = ND_out(aghead(e)).list[0];
		}
	    }
	}
    }

    /* do the recursion */
    mark_lowcluster_basic(root);
}

static void mark_lowcluster_basic(Agraph_t * g)
{
    Agraph_t *clust;
    Agnode_t *n, *vn;
    Agedge_t *orig, *e;
    int c;

    for (c = 1; c <= GD_n_cluster(g); c++) {
	clust = GD_clust(g)[c];
	mark_lowcluster_basic(clust);
    }
    /* see what belongs to this graph that wasn't already marked */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (ND_clust(n) == NULL)
	    ND_clust(n) = g;
	for (orig = agfstout(g, n); orig; orig = agnxtout(g, orig)) {
	    if ((e = ED_to_virt(orig))) {
#ifndef WITH_CGRAPH
		while (e && (vn = e->head)->u.node_type == VIRTUAL) {
#else /* WITH_CGRAPH */
		while (e && (ND_node_type(vn = aghead(e))) == VIRTUAL) {
#endif /* WITH_CGRAPH */
		    if (ND_clust(vn) == NULL)
			ND_clust(vn) = g;
		    e = ND_out(aghead(e)).list[0];
		}
	    }
	}
    }
}
