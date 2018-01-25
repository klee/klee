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
 * Rank the nodes of a directed graph, subject to user-defined
 * sets of nodes to be kept on the same, min, or max rank.
 * The temporary acyclic fast graph is constructed and ranked
 * by a network-simplex technique.  Then ranks are propagated
 * to non-leader nodes and temporary edges are deleted.
 * Leaf nodes and top-level clusters are left collapsed, though.
 * Assigns global minrank and maxrank of graph and all clusters.
 *
 * TODO: safety code.  must not be in two clusters at same level.
 *  must not be in same/min/max/rank and a cluster at the same time.
 *  watch out for interactions between leaves and clusters.
 */

#include	"dot.h"

static void dot1_rank(graph_t * g, aspect_t* asp);
#ifdef WITH_CGRAPH
static void dot2_rank(graph_t * g, aspect_t* asp);
#endif

static void 
renewlist(elist * L)
{
    int i;
    for (i = L->size; i >= 0; i--)
	L->list[i] = NULL;
    L->size = 0;
}

static void 
cleanup1(graph_t * g)
{
    node_t *n;
    edge_t *e, *f;
    int c;

    for (c = 0; c < GD_comp(g).size; c++) {
	GD_nlist(g) = GD_comp(g).list[c];
	for (n = GD_nlist(g); n; n = ND_next(n)) {
	    renewlist(&ND_in(n));
	    renewlist(&ND_out(n));
	    ND_mark(n) = FALSE;
	}
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    f = ED_to_virt(e);
	    /* Null out any other references to f to make sure we don't 
	     * handle it a second time. For example, parallel multiedges 
	     * share a virtual edge.
	     */
	    if (f && (e == ED_to_orig(f))) {
		edge_t *e1, *f1;
#ifndef WITH_CGRAPH
		for (e1 = agfstout(g, n); e1; e1 = agnxtout(g, e1)) {
		    if (e != e1) {
			f1 = ED_to_virt(e1);
			if (f1 && (f == f1)) {
			    ED_to_virt(e1) = NULL;
			}
		    }
		}
#else
		node_t *n1;
		for (n1 = agfstnode(g); n1; n1 = agnxtnode(g, n1)) {
		    for (e1 = agfstout(g, n1); e1; e1 = agnxtout(g, e1)) {
			if (e != e1) {
			    f1 = ED_to_virt(e1);
			    if (f1 && (f == f1)) {
				ED_to_virt(e1) = NULL;
			    }
			}
		    }
		}
		free(f->base.data);
#endif
		free(f);
	    }
	    ED_to_virt(e) = NULL;
	}
    }
    free(GD_comp(g).list);
    GD_comp(g).list = NULL;
    GD_comp(g).size = 0;
}

/* When there are edge labels, extra ranks are reserved here for the virtual
 * nodes of the labels.  This is done by doubling the input edge lengths.
 * The input rank separation is adjusted to compensate.
 */
static void 
edgelabel_ranks(graph_t * g)
{
    node_t *n;
    edge_t *e;

    if (GD_has_labels(g) & EDGE_LABEL) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n))
	    for (e = agfstout(g, n); e; e = agnxtout(g, e))
		ED_minlen(e) *= 2;
	GD_ranksep(g) = (GD_ranksep(g) + 1) / 2;
    }
}

/* Merge the nodes of a min, max, or same rank set. */
static void 
collapse_rankset(graph_t * g, graph_t * subg, int kind)
{
    node_t *u, *v;

    u = v = agfstnode(subg);
    if (u) {
	ND_ranktype(u) = kind;
	while ((v = agnxtnode(subg, v))) {
	    UF_union(u, v);
	    ND_ranktype(v) = ND_ranktype(u);
	}
	switch (kind) {
	case MINRANK:
	case SOURCERANK:
	    if (GD_minset(g) == NULL)
		GD_minset(g) = u;
	    else
		GD_minset(g) = UF_union(GD_minset(g), u);
	    break;
	case MAXRANK:
	case SINKRANK:
	    if (GD_maxset(g) == NULL)
		GD_maxset(g) = u;
	    else
		GD_maxset(g) = UF_union(GD_maxset(g), u);
	    break;
	}
	switch (kind) {
	case SOURCERANK:
	    ND_ranktype(GD_minset(g)) = kind;
	    break;
	case SINKRANK:
	    ND_ranktype(GD_maxset(g)) = kind;
	    break;
	}
    }
}

static int 
rank_set_class(graph_t * g)
{
    static char *name[] = { "same", "min", "source", "max", "sink", NULL };
    static int class[] =
	{ SAMERANK, MINRANK, SOURCERANK, MAXRANK, SINKRANK, 0 };
    int val;

    if (is_cluster(g))
	return CLUSTER;
    val = maptoken(agget(g, "rank"), name, class);
    GD_set_type(g) = val;
    return val;
}

static int 
make_new_cluster(graph_t * g, graph_t * subg)
{
    int cno;
    cno = ++(GD_n_cluster(g));
    GD_clust(g) = ZALLOC(cno + 1, GD_clust(g), graph_t *, GD_n_cluster(g));
    GD_clust(g)[cno] = subg;
    do_graph_label(subg);
    return cno;
}

static void 
node_induce(graph_t * par, graph_t * g)
{
    node_t *n, *nn;
    edge_t *e;
    int i;

    /* enforce that a node is in at most one cluster at this level */
    for (n = agfstnode(g); n; n = nn) {
	nn = agnxtnode(g, n);
	if (ND_ranktype(n)) {
	    agdelete(g, n);
	    continue;
	}
	for (i = 1; i < GD_n_cluster(par); i++)
	    if (agcontains(GD_clust(par)[i], n))
		break;
	if (i < GD_n_cluster(par))
	    agdelete(g, n);
	ND_clust(n) = NULL;
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(agroot(g), n); e; e = agnxtout(agroot(g), e)) {
	    if (agcontains(g, aghead(e)))
#ifndef WITH_CGRAPH
		aginsert(g, e);
#else /* WITH_CGRAPH */
		agsubedge(g,e,1);
#endif /* WITH_CGRAPH */
	}
    }
}

void 
dot_scan_ranks(graph_t * g)
{
    node_t *n, *leader = NULL;
    GD_minrank(g) = MAXSHORT;
    GD_maxrank(g) = -1;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (GD_maxrank(g) < ND_rank(n))
	    GD_maxrank(g) = ND_rank(n);
	if (GD_minrank(g) > ND_rank(n))
	    GD_minrank(g) = ND_rank(n);
	if (leader == NULL)
	    leader = n;
	else {
	    if (ND_rank(n) < ND_rank(leader))
		leader = n;
	}
    }
    GD_leader(g) = leader;
}

static void
cluster_leader(graph_t * clust)
{
    node_t *leader, *n;
    int maxrank = 0;

    /* find number of ranks and select a leader */
    leader = NULL;
    for (n = GD_nlist(clust); n; n = ND_next(n)) {
	if ((ND_rank(n) == 0) && (ND_node_type(n) == NORMAL))
	    leader = n;
	if (maxrank < ND_rank(n))
	    maxrank = ND_rank(n);
    }
    assert(leader != NULL);
    GD_leader(clust) = leader;

    for (n = agfstnode(clust); n; n = agnxtnode(clust, n)) {
	assert((ND_UF_size(n) <= 1) || (n == leader));
	UF_union(n, leader);
	ND_ranktype(n) = CLUSTER;
    }
}

/*
 * A cluster is collapsed in three steps.
 * 1) The nodes of the cluster are ranked locally.
 * 2) The cluster is collapsed into one node on the least rank.
 * 3) In class1(), any inter-cluster edges are converted using
 *    the "virtual node + 2 edges" trick.
 */
static void 
collapse_cluster(graph_t * g, graph_t * subg)
{
    if (GD_parent(subg)) {
#ifndef WITH_CGRAPH
	agerr(AGWARN, "Cluster %s is multiply defined in %s and %s - this may cause problems.\n", subg->name,
		g->name, GD_parent(subg)->name);
#endif
	return;
    }
    GD_parent(subg) = g;
    node_induce(g, subg);
    if (agfstnode(subg) == NULL)
	return;
    make_new_cluster(g, subg);
    if (CL_type == LOCAL) {
	dot1_rank(subg, 0);
	cluster_leader(subg);
    } else
	dot_scan_ranks(subg);
}

/* Execute union commands for "same rank" subgraphs and clusters. */
static void 
collapse_sets(graph_t *rg, graph_t *g)
{
    int c;
    graph_t  *subg;
#ifdef OBSOLETE
    node_t *n;
#endif

#ifndef WITH_CGRAPH
    graph_t *mg;
    node_t *mn;
    edge_t *me;
    mg = g->meta_node->graph;
    for (me = agfstout(mg, g->meta_node); me; me = agnxtout(mg, me)) {
	mn = aghead(me);
	subg = agusergraph(mn);
#else /* WITH_CGRAPH */
    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
#endif /* WITH_CGRAPH */
	c = rank_set_class(subg);
	if (c) {
	    if ((c == CLUSTER) && CL_type == LOCAL)
		collapse_cluster(rg, subg);
	    else
		collapse_rankset(rg, subg, c);
	}
	else collapse_sets(rg, subg);

#ifdef OBSOLETE
 Collapsing leaves is currently obsolete

	/* mark nodes with ordered edges so their leaves are not collapsed */
	if (agget(subg, "ordering"))
	    for (n = agfstnode(subg); n; n = agnxtnode(subg, n))
		ND_order(n) = 1;
#endif
    }
}

static void 
find_clusters(graph_t * g)
{
    graph_t *subg;
#ifndef WITH_CGRAPH
    graph_t *mg;
    node_t *mn;
    edge_t *me;

    mg = g->meta_node->graph;
    for (me = agfstout(mg, g->meta_node); me; me = agnxtout(mg, me)) {
	mn = me->head;
	subg = agusergraph(mn);
#else /* WITH_CGRAPH */
    for (subg = agfstsubg(agroot(g)); subg; subg = agnxtsubg(subg)) {
#endif /* WITH_CGRAPH */
	if (GD_set_type(subg) == CLUSTER)
	    collapse_cluster(g, subg);
    }
}

static void 
set_minmax(graph_t * g)
{
    int c;

    GD_minrank(g) += ND_rank(GD_leader(g));
    GD_maxrank(g) += ND_rank(GD_leader(g));
    for (c = 1; c <= GD_n_cluster(g); c++)
	set_minmax(GD_clust(g)[c]);
}

/* To ensure that min and max rank nodes always have the intended rank
 * assignment, reverse any incompatible edges.
 */
static point 
minmax_edges(graph_t * g)
{
    node_t *n;
    edge_t *e;
    point  slen;

    slen.x = slen.y = 0;
    if ((GD_maxset(g) == NULL) && (GD_minset(g) == NULL))
	return slen;
    if (GD_minset(g) != NULL)
	GD_minset(g) = UF_find(GD_minset(g));
    if (GD_maxset(g) != NULL)
	GD_maxset(g) = UF_find(GD_maxset(g));

    if ((n = GD_maxset(g))) {
	slen.y = (ND_ranktype(GD_maxset(g)) == SINKRANK);
	while ((e = ND_out(n).list[0])) {
	    assert(aghead(e) == UF_find(aghead(e)));
	    reverse_edge(e);
	}
    }
    if ((n = GD_minset(g))) {
	slen.x = (ND_ranktype(GD_minset(g)) == SOURCERANK);
	while ((e = ND_in(n).list[0])) {
	    assert(agtail(e) == UF_find(agtail(e)));
	    reverse_edge(e);
	}
    }
    return slen;
}
    
static int 
minmax_edges2(graph_t * g, point slen)
{
    node_t *n;
    edge_t *e = 0;

    if ((GD_maxset(g)) || (GD_minset(g))) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (n != UF_find(n))
		continue;
	    if ((ND_out(n).size == 0) && GD_maxset(g) && (n != GD_maxset(g))) {
		e = virtual_edge(n, GD_maxset(g), NULL);
		ED_minlen(e) = slen.y;
		ED_weight(e) = 0;
	    }
	    if ((ND_in(n).size == 0) && GD_minset(g) && (n != GD_minset(g))) {
		e = virtual_edge(GD_minset(g), n, NULL);
		ED_minlen(e) = slen.x;
		ED_weight(e) = 0;
	    }
	}
    }
    return (e != 0);
}

/* Run the network simplex algorithm on each component. */
void rank1(graph_t * g)
{
    int maxiter = INT_MAX;
    int c;
    char *s;

    if ((s = agget(g, "nslimit1")))
	maxiter = atof(s) * agnnodes(g);
    for (c = 0; c < GD_comp(g).size; c++) {
	GD_nlist(g) = GD_comp(g).list[c];
	rank(g, (GD_n_cluster(g) == 0 ? 1 : 0), maxiter);	/* TB balance */
    }
}

/* 
 * Assigns ranks of non-leader nodes.
 * Expands same, min, max rank sets.
 * Leaf sets and clusters remain merged.
 * Sets minrank and maxrank appropriately.
 */
static void expand_ranksets(graph_t * g, aspect_t* asp)
{
    int c;
    node_t *n, *leader;

    if ((n = agfstnode(g))) {
	GD_minrank(g) = MAXSHORT;
	GD_maxrank(g) = -1;
	while (n) {
	    leader = UF_find(n);
	    /* The following works because ND_rank(n) == 0 if n is not in a
	     * cluster, and ND_rank(n) = the local rank offset if n is in
	     * a cluster. */
	    if ((leader != n) && (!asp || (ND_rank(n) == 0)))
		ND_rank(n) += ND_rank(leader);

	    if (GD_maxrank(g) < ND_rank(n))
		GD_maxrank(g) = ND_rank(n);
	    if (GD_minrank(g) > ND_rank(n))
		GD_minrank(g) = ND_rank(n);

	    if (ND_ranktype(n) && (ND_ranktype(n) != LEAFSET))
		UF_singleton(n);
	    n = agnxtnode(g, n);
	}
	if (g == agroot(g)) {
	    if (CL_type == LOCAL) {
		for (c = 1; c <= GD_n_cluster(g); c++)
		    set_minmax(GD_clust(g)[c]);
	    } else {
		find_clusters(g);
	    }
	}
    } else {
	GD_minrank(g) = GD_maxrank(g) = 0;
    }
}

#ifdef ALLOW_LEVELS
void
setRanks (graph_t* g, attrsym_t* lsym)
{
    node_t* n;
    char*   s;
    char*   ep;
    long    v;

    for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
	s = agxget (n, lsym->index);
	v = strtol (s, &ep, 10);
	if (ep == s)
	    agerr(AGWARN, "no level attribute for node \"%s\"\n", n->name);
	ND_rank(n) = v;
    }
}
#endif

#ifdef UNUSED
static node_t **virtualEdgeHeadList = NULL;
static node_t **virtualEdgeTailList = NULL;
static int nVirtualEdges = 0;

static void
saveVirtualEdges(graph_t *g)
{
    edge_t *e;
    node_t *n;
    int cnt = 0;
    int lc;
    
    if (virtualEdgeHeadList != NULL) {
	free(virtualEdgeHeadList);
    }
    if (virtualEdgeTailList != NULL) {
	free(virtualEdgeTailList);
    }

  /* allocate memory */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (lc = 0; lc < ND_in(n).size; lc++) {
	    e = ND_in(n).list[lc];
	    if (ED_edge_type(e) == VIRTUAL) cnt++;
	}
    }

    nVirtualEdges = cnt;
    virtualEdgeHeadList = N_GNEW(cnt, node_t*);
    virtualEdgeTailList = N_GNEW(cnt, node_t*);

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (lc = 0, cnt = 0; lc < ND_in(n).size; lc++) {
	    e = ND_in(n).list[lc];
	    if (ED_edge_type(e) == VIRTUAL) {
		virtualEdgeHeadList[cnt] = e->head;
		virtualEdgeTailList[cnt] = e->tail;
		if (Verbose)
		    printf("saved virtual edge: %s->%s\n", 
			virtualEdgeTailList[cnt]->name, 
			virtualEdgeHeadList[cnt]->name);	    
		cnt++;
	    }
	}
    }
}

static void
restoreVirtualEdges(graph_t *g)
{
    int i;
    edge_t e;

    for (i = 0; i < nVirtualEdges; i++) {
	if (virtualEdgeTailList[i] && virtualEdgeHeadList[i]) {
	    if (Verbose)
		printf("restoring virtual edge: %s->%s\n", 
		    virtualEdgeTailList[i]->name, virtualEdgeHeadList[i]->name);
	    virtual_edge(virtualEdgeTailList[i], virtualEdgeHeadList[i], NULL);
	}
    }
    if (Verbose)
	printf("restored %d virt edges\n", nVirtualEdges);
}
#endif

/* dot1_rank:
 * asp != NULL => g is root
 */
static void dot1_rank(graph_t * g, aspect_t* asp)
{
    point p;
#ifdef ALLOW_LEVELS
    attrsym_t* N_level;
#endif
    edgelabel_ranks(g);

    if (asp) {
	init_UF_size(g);
	initEdgeTypes(g);
    }

    collapse_sets(g,g);
    /*collapse_leaves(g); */
    class1(g);
    p = minmax_edges(g);
    decompose(g, 0);
    if (asp && ((GD_comp(g).size > 1)||(GD_n_cluster(g) > 0))) {
	asp->badGraph = 1;
	asp = NULL;
    }
    acyclic(g);
    if (minmax_edges2(g, p))
	decompose(g, 0);
#ifdef ALLOW_LEVELS
    if ((N_level = agfindattr(g->proto->n, "level")))
	setRanks(g, N_level);
    else
#endif

    if (asp)
	rank3(g, asp);
    else
	rank1(g);

    expand_ranksets(g, asp);
    cleanup1(g);
}

void dot_rank(graph_t * g, aspect_t* asp)
{
#ifdef WITH_CGRAPH
    if (agget (g, "newrank")) {
	GD_flags(g) |= NEW_RANK;
	dot2_rank (g, asp);
    }
    else
#endif
	dot1_rank (g, asp);
    if (Verbose)
	fprintf (stderr, "Maxrank = %d, minrank = %d\n", GD_maxrank(g), GD_minrank(g));
}

int is_cluster(graph_t * g)
{
    return (strncmp(agnameof(g), "cluster", 7) == 0);
}

#ifdef OBSOLETE
static node_t*
merge_leaves(graph_t * g, node_t * cur, node_t * new)
{
    node_t *rv;

    if (cur == NULL)
	rv = new;
    else {
	rv = UF_union(cur, new);
	ND_ht(rv) = MAX(ND_ht(cur), ND_ht(new));
	ND_lw(rv) = ND_lw(cur) + ND_lw(new) + GD_nodesep(g) / 2;
	ND_rw(rv) = ND_rw(cur) + ND_rw(new) + GD_nodesep(g) / 2;
    }
    return rv;
}

static void 
potential_leaf(graph_t * g, edge_t * e, node_t * leaf)
{
    node_t *par;

    if ((ED_tail_port(e).p.x) || (ED_head_port(e).p.x))
	return;
    if ((ED_minlen(e) != 1) || (ND_order(agtail(e)) > 0))
	return;
    par = ((leaf != aghead(e)) ? aghead(e) : agtail(e));
    ND_ranktype(leaf) = LEAFSET;
    if (par == agtail(e))
	GD_outleaf(par) = merge_leaves(g, GD_outleaf(par), leaf);
    else
	GD_inleaf(par) = merge_leaves(g, GD_inleaf(par), leaf);
}

static void 
collapse_leaves(graph_t * g)
{
    node_t *n;
    edge_t *e;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {

	/* consider n as a potential leaf of some other node. */
	if ((ND_ranktype(n) != NOCMD) || (ND_order(n)))
	    continue;
	if (agfstout(g, n) == NULL) {
	    if ((e = agfstin(g, n)) && (agnxtin(g, e) == NULL)) {
#ifndef WITH_CGRAPH
		potential_leaf(g, e, n);
#else
		potential_leaf(g, AGMKOUT(e), n);
#endif
		continue;
	    }
	}
	if (agfstin(g, n) == NULL) {
	    if ((e = agfstout(g, n)) && (agnxtout(g, e) == NULL)) {
		potential_leaf(g, e, n);
		continue;
	    }
	}
    }
}
#endif

#ifdef WITH_CGRAPH
/* new ranking code:
 * Allows more constraints
 * Copy of level.c in dotgen2
 * Many of the utility functions are simpler or gone with
 * cgraph library.
 */
#define	BACKWARD_PENALTY	1000
#define STRONG_CLUSTER_WEIGHT   1000
#define	NORANK		6
#define ROOT    "\177root"
#define TOPNODE     "\177top"
#define BOTNODE     "\177bot"

/* hops is not used in dot, so we overload it to 
 * contain the index of the connected component
 */
#define ND_comp(n)  ND_hops(n)   

extern int rank2(Agraph_t *, int, int, int);

static void set_parent(graph_t* g, graph_t* p) 
{
    GD_parent(g) = p;
    make_new_cluster(p, g);
    node_induce(p, g);
}

static int is_empty(graph_t * g)
{
    return (!agfstnode(g));
}

static int is_a_strong_cluster(graph_t * g)
{
    int rv;
    char *str = agget(g, "compact");
    /* rv = mapBool((str), TRUE); */
    rv = mapBool((str), FALSE);
    return rv;
}

static int rankset_kind(graph_t * g)
{
    char *str = agget(g, "rank");

    if (str && str[0]) {
	if (!strcmp(str, "min"))
	    return MINRANK;
	if (!strcmp(str, "source"))
	    return SOURCERANK;
	if (!strcmp(str, "max"))
	    return MAXRANK;
	if (!strcmp(str, "sink"))
	    return SINKRANK;
	if (!strcmp(str, "same"))
	    return SAMERANK;
    }
    return NORANK;
}

static int is_nonconstraint(edge_t * e)
{
    char *constr;

    if (E_constr && (constr = agxget(e, E_constr))) {
	if (constr[0] && mapbool(constr) == FALSE)
	    return TRUE;
    }
    return FALSE;
}

static node_t *find(node_t * n)
{
    node_t *set;
    if ((set = ND_set(n))) {
	if (set != n)
	    set = ND_set(n) = find(set);
    } else
	set = ND_set(n) = n;
    return set;
}

static node_t *union_one(node_t * leader, node_t * n)
{
    if (n)
	return (ND_set(find(n)) = find(leader));
    else
	return leader;
}

static node_t *union_all(graph_t * g)
{
    node_t *n, *leader;

    n = agfstnode(g);
    if (!n)
	return n;
    leader = find(n);
    while ((n = agnxtnode(g, n)))
	union_one(leader, n);
    return leader;
}

static void compile_samerank(graph_t * ug, graph_t * parent_clust)
{
    graph_t *s;			/* subgraph being scanned */
    graph_t *clust;		/* cluster that contains the rankset */
    node_t *n, *leader;

    if (is_empty(ug))
	return;
    if (is_a_cluster(ug)) {
	clust = ug;
	if (parent_clust) {
	    GD_level(ug) = GD_level(parent_clust) + 1;
	    set_parent(ug, parent_clust);
	}
	else
	    GD_level(ug) = 0;
    } else
	clust = parent_clust;

    /* process subgraphs of this subgraph */
    for (s = agfstsubg(ug); s; s = agnxtsubg(s))
	compile_samerank(s, clust);

    /* process this subgraph as a cluster */
    if (is_a_cluster(ug)) {
	for (n = agfstnode(ug); n; n = agnxtnode(ug, n)) {
	    if (ND_clust(n) == 0)
		ND_clust(n) = ug;
#ifdef DEBUG
	    fprintf(stderr, "(%s) %s  %p\n", agnameof(ug), agnameof(n),
		    ND_clust(n));
#endif
	}
    }

    /* process this subgraph as a rankset */
    switch (rankset_kind(ug)) {
    case SOURCERANK:
	GD_has_sourcerank(clust) = TRUE;	/* fall through */
    case MINRANK:
	leader = union_all(ug);
	GD_minrep(clust) = union_one(leader, GD_minrep(clust));
	break;
    case SINKRANK:
	GD_has_sinkrank(clust) = TRUE;	/* fall through */
    case MAXRANK:
	leader = union_all(ug);
	GD_maxrep(clust) = union_one(leader, GD_maxrep(clust));
	break;
    case SAMERANK:
	leader = union_all(ug);
	/* do we need to record these ranksets? */
	break;
    case NORANK:
	break;
    default:			/* unrecognized - warn and do nothing */
	agerr(AGWARN, "%s has unrecognized rank=%s", agnameof(ug),
	      agget(ug, "rank"));
    }

    /* a cluster may become degenerate */
    if (is_a_cluster(ug) && GD_minrep(ug)) {
	if (GD_minrep(ug) == GD_maxrep(ug)) {
	    node_t *up = union_all(ug);
	    GD_minrep(ug) = up;
	    GD_maxrep(ug) = up;
	}
    }
}

static graph_t *dot_lca(graph_t * c0, graph_t * c1)
{
    while (c0 != c1) {
	if (GD_level(c0) >= GD_level(c1))
	    c0 = GD_parent(c0);
	else
	    c1 = GD_parent(c1);
    }
    return c0;
}

static int is_internal_to_cluster(edge_t * e)
{
    graph_t *par, *ct, *ch;
    ct = ND_clust(agtail(e));
    ch = ND_clust(aghead(e));
    if (ct == ch)
	return TRUE;
    par = dot_lca(ct, ch);
    /* if (par == agroot(par)) */
	/* return FALSE; */
    if ((par == ct) || (par == ch))
	return TRUE;
    return FALSE;
}

static node_t* Last_node;
static node_t* makeXnode (graph_t* G, char* name)
{
    node_t *n = agnode(G, name, 1);
    alloc_elist(4, ND_in(n));
    alloc_elist(4, ND_out(n));
    if (Last_node) {
	ND_prev(n) = Last_node;
	ND_next(Last_node) = n;
    } else {
	ND_prev(n) = NULL;
	GD_nlist(G) = n;
    }
    Last_node = n;
    ND_next(n) = NULL;
    
    return n;
}

static void compile_nodes(graph_t * g, graph_t * Xg)
{
    /* build variables */
    node_t *n;

    Last_node = NULL;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (find(n) == n)
	    ND_rep(n) = makeXnode (Xg, agnameof(n));
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (ND_rep(n) == 0)
	    ND_rep(n) = ND_rep(find(n));
    }
}

static void merge(edge_t * e, int minlen, int weight)
{
    ED_minlen(e) = MAX(ED_minlen(e), minlen);
    ED_weight(e) += weight;
}

static void strong(graph_t * g, node_t * t, node_t * h, edge_t * orig)
{
    edge_t *e;
    if ((e = agfindedge(g, t, h)) ||
	(e = agfindedge(g, h, t)) || (e = agedge(g, t, h, 0, 1)))
	merge(e, ED_minlen(orig), ED_weight(orig));
    else
	agerr(AGERR, "ranking: failure to create strong constraint edge between nodes %s and %s\n", 
	    agnameof(t), agnameof(h));
}

static void weak(graph_t * g, node_t * t, node_t * h, edge_t * orig)
{
    node_t *v;
    edge_t *e, *f;
    static int id;
    char buf[100];

    for (e = agfstin(g, t); e; e = agnxtin(g, e)) {
	/* merge with existing weak edge (e,f) */
	v = agtail(e);
	if ((f = agfstout(g, v)) && (aghead(f) == h)) {
	    return;
	}
    }
    if (!e) {
	sprintf (buf, "_weak_%d", id++);
	v = makeXnode(g, buf);
	e = agedge(g, v, t, 0, 1);
	f = agedge(g, v, h, 0, 1);
    }
    ED_minlen(e) = MAX(ED_minlen(e), 0);	/* effectively a nop */
    ED_weight(e) += ED_weight(orig) * BACKWARD_PENALTY;
    ED_minlen(f) = MAX(ED_minlen(f), ED_minlen(orig));
    ED_weight(f) += ED_weight(orig);
}

static void compile_edges(graph_t * ug, graph_t * Xg)
{
    node_t *n;
    edge_t *e;
    node_t *Xt, *Xh;
    graph_t *tc, *hc;

    /* build edge constraints */
    for (n = agfstnode(ug); n; n = agnxtnode(ug, n)) {
	Xt = ND_rep(n);
	for (e = agfstout(ug, n); e; e = agnxtout(ug, e)) {
	    if (is_nonconstraint(e))
		continue;
	    Xh = ND_rep(find(aghead(e)));
	    if (Xt == Xh)
		continue;

	    tc = ND_clust(agtail(e));
	    hc = ND_clust(aghead(e));

	    if (is_internal_to_cluster(e)) {
		/* determine if graph requires reversed edge */
		if ((find(agtail(e)) == GD_maxrep(ND_clust(agtail(e))))
		    || (find(aghead(e)) == GD_minrep(ND_clust(aghead(e))))) {
		    node_t *temp = Xt;
		    Xt = Xh;
		    Xh = temp;
		}
		strong(Xg, Xt, Xh, e);
	    } else {
		if (is_a_strong_cluster(tc) || is_a_strong_cluster(hc))
		    weak(Xg, Xt, Xh, e);
		else
		    strong(Xg, Xt, Xh, e);
	    }
	}
    }
}

static void compile_clusters(graph_t* g, graph_t* Xg, node_t* top, node_t* bot)
{
    node_t *n;
    node_t *rep;
    edge_t *e;
    graph_t *sub;

    if (is_a_cluster(g) && is_a_strong_cluster(g)) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (agfstin(g, n) == 0) {
		rep = ND_rep(find(n));
		if (!top) top = makeXnode(Xg,TOPNODE);
		agedge(Xg, top, rep, 0, 1);
	    }
	    if (agfstout(g, n) == 0) {
		rep = ND_rep(find(n));
		if (!bot)  bot = makeXnode(Xg,BOTNODE);
		agedge(Xg, rep, bot, 0, 1);
	    }
	}
	if (top && bot) {
	    e = agedge(Xg, top, bot, 0, 1);
	    merge(e, 0, STRONG_CLUSTER_WEIGHT);
	}
    }
    for (sub = agfstsubg(g); sub; sub = agnxtsubg(sub))
	compile_clusters(sub, Xg, top, bot);
}

static void reverse_edge2(graph_t * g, edge_t * e)
{
    edge_t *rev;

    rev = agfindedge(g, aghead(e), agtail(e));
    if (!rev)
	rev = agedge(g, aghead(e), agtail(e), 0, 1);
    merge(rev, ED_minlen(e), ED_weight(e));
    agdelete(g, e);
}

static void dfs(graph_t * g, node_t * v)
{
    edge_t *e, *f;
    node_t *w;

    if (ND_mark(v))
	return;
    ND_mark(v) = TRUE;
    ND_onstack(v) = TRUE;
    for (e = agfstout(g, v); e; e = f) {
	f = agnxtout(g, e);
	w = aghead(e);
	if (ND_onstack(w))
	    reverse_edge2(g, e);
	else {
	    if (ND_mark(w) == FALSE)
		dfs(g, w);
	}
    }
    ND_onstack(v) = FALSE;
}

static void break_cycles(graph_t * g)
{
    node_t *n;

    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	ND_mark(n) = ND_onstack(n) = FALSE;
    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	dfs(g, n);
}

static void setMinMax (graph_t* g, int doRoot)
{
    int c, v;
    node_t *n;
    node_t* leader;

      /* Do child clusters */
    for (c = 1; c <= GD_n_cluster(g); c++)
	    setMinMax(GD_clust(g)[c], 0);

    if (!GD_parent(g) && !doRoot) // root graph
	return;

    GD_minrank(g) = MAXSHORT;
    GD_maxrank(g) = -1;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	v = ND_rank(n);
	if (GD_maxrank(g) < v)
	    GD_maxrank(g) = v;
	if (GD_minrank(g) > v) {
	    GD_minrank(g) = v;
	    leader = n;
	}
    }
    GD_leader(g) = leader;
}

/* readout_levels:
 * Store node rank information in original graph.
 * Set rank bounds in graph and clusters
 * Free added data structures.
 *
 * rank2 is called with balance=1, which ensures that minrank=0
 */
static void readout_levels(graph_t * g, graph_t * Xg, int ncc)
{
    node_t *n;
    node_t *xn;
    int* minrk = NULL;
    int doRoot = 0;

    GD_minrank(g) = MAXSHORT;
    GD_maxrank(g) = -1;
    if (ncc > 1) {
	int i;
	minrk = N_NEW(ncc+1,int);
	for (i = 1; i <= ncc; i++)
	    minrk[i] = MAXSHORT;
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	xn = ND_rep(find(n));
	ND_rank(n) = ND_rank(xn);
	if (GD_maxrank(g) < ND_rank(n))
	    GD_maxrank(g) = ND_rank(n);
	if (GD_minrank(g) > ND_rank(n))
	    GD_minrank(g) = ND_rank(n);
	if (minrk) {
	    ND_comp(n) = ND_comp(xn);
	    minrk[ND_comp(n)] = MIN(minrk[ND_comp(n)],ND_rank(n));
	}
    }
    if (minrk) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n))
	    ND_rank(n) -= minrk[ND_comp(n)];
	/* Non-uniform shifting, so recompute maxrank/minrank of root graph */
	doRoot = 1;
    }
    else if (GD_minrank(g) > 0) {  /* should never happen */
	int delta = GD_minrank(g);
	for (n = agfstnode(g); n; n = agnxtnode(g, n))
	    ND_rank(n) -= delta;
	GD_minrank(g) -= delta;
	GD_maxrank(g) -= delta;
    }

    setMinMax(g, doRoot);

    /* release fastgraph memory from Xg */
    for (n = agfstnode(Xg); n; n = agnxtnode(Xg, n)) {
	free_list(ND_in(n));
	free_list(ND_out(n));
    }

    free(ND_alg(agfstnode(g)));
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	ND_alg(n) = NULL;
    }
    if (minrk)
	free (minrk);
}

static void dfscc(graph_t * g, node_t * n, int cc)
{
    edge_t *e;
    if (ND_comp(n) == 0) {
	ND_comp(n) = cc;
	for (e = agfstout(g, n); e; e = agnxtout(g, e))
	    dfscc(g, aghead(e), cc);
	for (e = agfstin(g, n); e; e = agnxtin(g, e))
	    dfscc(g, agtail(e), cc);
    }
}

static int connect_components(graph_t * g)
{
    int cc = 0;
    node_t *n;

    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	ND_comp(n) = 0;
    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	if (ND_comp(n) == 0)
	    dfscc(g, n, ++cc);
    if (cc > 1) {
	node_t *root = makeXnode(g, ROOT);
	int ncc = 1;
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (ND_comp(n) == ncc) {
		(void) agedge(g, root, n, 0, 1);
		ncc++;
	    }
	}
    }
    return (cc);
}

static void add_fast_edges (graph_t * g)
{
    node_t *n;
    edge_t *e;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    elist_append(e, ND_out(n));
	    elist_append(e, ND_in(aghead(e)));
	}
    }
}

static void my_init_graph(Agraph_t *g, Agobj_t *graph, void *arg)
{ int *sz = arg; agbindrec(graph,"level graph rec",sz[0],TRUE); }
static void my_init_node(Agraph_t *g, Agobj_t *node, void *arg)
{ int *sz = arg; agbindrec(node,"level node rec",sz[1],TRUE); }
static void my_init_edge(Agraph_t *g, Agobj_t *edge, void *arg)
{ int *sz = arg; agbindrec(edge,"level edge rec",sz[2],TRUE); }
static Agcbdisc_t mydisc = { {my_init_graph,0,0}, {my_init_node,0,0}, {my_init_edge,0,0} };

int infosizes[] = {
    sizeof(Agraphinfo_t), 
    sizeof(Agnodeinfo_t),
    sizeof(Agedgeinfo_t)
};

void dot2_rank(graph_t * g, aspect_t* asp)
{
    int ssize;
    int ncc, maxiter = INT_MAX;
    char *s;
    graph_t *Xg;
#ifdef ALLOW_LEVELS
    attrsym_t* N_level;
#endif
    Last_node = NULL;
    Xg = agopen("level assignment constraints", Agstrictdirected, 0);
    agbindrec(Xg,"level graph rec",sizeof(Agraphinfo_t),TRUE);
    agpushdisc(Xg,&mydisc,infosizes);

    edgelabel_ranks(g);

    if ((s = agget(g, "nslimit1")))
	maxiter = atof(s) * agnnodes(g);
    else
	maxiter = INT_MAX;

    compile_samerank(g, 0);
    compile_nodes(g, Xg);
    compile_edges(g, Xg);
    compile_clusters(g, Xg, 0, 0);
    break_cycles(Xg);
    ncc = connect_components(Xg);
    add_fast_edges (Xg);

    if (asp) {
	init_UF_size(Xg);
	initEdgeTypes(Xg);
    }

    if ((s = agget(g, "searchsize")))
	ssize = atoi(s);
    else
	ssize = -1;
    rank2(Xg, 1, maxiter, ssize);
/* fastgr(Xg); */
    readout_levels(g, Xg, ncc);
#ifdef DEBUG
    fprintf (stderr, "Xg %d nodes %d edges\n", agnnodes(Xg), agnedges(Xg));
#endif
    agclose(Xg);
}

#endif /* WITH_CGRAPH */
/* end of new ranking code
 */
