/* $Id$Revision: */
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

#include "dot2.h"
#include "utils.h"
#include "arith.h"

#define	BACKWARD_PENALTY	1000
#define STRONG_CLUSTER_WEIGHT   1000
#define	NORANK		6

static int is_a_strong_cluster(graph_t * g);
static void compile_samerank(graph_t * ug, graph_t * parent_clust);
static int is_internal_to_cluster(edge_t * e);
static void compile_nodes(graph_t * g, graph_t * Xg);
static void strong(graph_t * g, node_t * t, node_t * h, edge_t * orig);
static void weak(graph_t * g, node_t * t, node_t * h, edge_t * orig);
static void compile_edges(graph_t * ug, graph_t * Xg);
static void compile_clusters(graph_t * g, graph_t * Xg);
static void dfs(graph_t * g, node_t * n);
static void break_cycles(graph_t * g);
static void readout_levels(graph_t * g, graph_t * Xg);
static void connect_components(graph_t * g);

static void my_init_node(Agraph_t *g, Agobj_t *node, void *arg)
{ int *sz = arg; agbindrec(node,"level node rec",sz[1],TRUE); }
static void my_init_edge(Agraph_t *g, Agobj_t *edge, void *arg)
{ int *sz = arg; agbindrec(edge,"level edge rec",sz[2],TRUE); }
static void my_init_graph(Agraph_t *g, Agobj_t *graph, void *arg)
{ int *sz = arg; agbindrec(graph,"level graph rec",sz[0],TRUE); }
static Agcbdisc_t mydisc = { {my_init_graph,0,0}, {my_init_node,0,0}, {my_init_edge,0,0} };

static void agautoinit(Agraph_t *g, int graphinfo_size, int nodeinfo_size, int edgeinfo_size) 
{
    int *s;

    s = N_NEW(3,int);	/* until we fix something, this is a storage leak */
    s[0] = graphinfo_size; s[1] = nodeinfo_size; s[2] = edgeinfo_size;
    agpushdisc(g,&mydisc,s);
}

void dot2_levels(graph_t * g)
{
    int ssize;
    char* s;
    graph_t *Xg =
	agopen("level assignment constraints", Agstrictdirected, 0);

    agautoinit(Xg, sizeof(Agraphinfo_t), sizeof(Agnodeinfo_t),
	       sizeof(Agedgeinfo_t));
    compile_samerank(g, 0);
    compile_nodes(g, Xg);
    compile_edges(g, Xg);
    compile_clusters(g, Xg);
    break_cycles(Xg);
    connect_components(Xg);
    if ((s = agget(g,"searchsize")))
	ssize = atoi (s);
    else
	ssize = -1;
    rank2(Xg, 1, INT_MAX, ssize);
    readout_levels(g, Xg);
}

static int is_empty(graph_t * g)
{
    return (agfstnode(g) == NILnode);
}

static int is_a_strong_cluster(graph_t * g)
{
    char *str = agget(g, "compact");
    return mapBool((str), TRUE);
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

int is_nonconstraint(edge_t * e)
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

    if (is_a_cluster(ug)) {
	clust = ug;
	GD_parent(ug) = parent_clust;
	if (parent_clust)
	    GD_level(ug) = GD_level(parent_clust) + 1;
	else
	    GD_level(ug) = 0;
    } else
	clust = parent_clust;
    if (is_empty(ug))
	return;

    /* process subgraphs of this subgraph */
    for (s = agfstsubg(ug); s; s = agnxtsubg(s))
	compile_samerank(s, clust);

    /* process this subgraph as a cluster */
    if (is_a_cluster(ug)) {
	for (n = agfstnode(ug); n; n = agnxtnode(ug, n)) {
	    if (ND_clust(n) == NILgraph)
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
	agerr (AGWARN, "%s has unrecognized rank=%s", agnameof(ug), agget(ug, "rank"));
    }

    /* a cluster may become degenerate */
    if (is_a_cluster(ug) && GD_minrep(ug)) {
	if (GD_minrep(ug) == GD_maxrep(ug)) {
	    GD_minrep(ug) = GD_maxrep(ug) = union_all(ug);
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
    if (par == agroot(par))
	return FALSE;
    if ((par == ct) || (par == ch))
	return TRUE;
    return FALSE;
}

static void compile_nodes(graph_t * g, graph_t * Xg)
{
    /* build variables */
    node_t *n;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (find(n) == n)
	    ND_rep(n) = agnode(Xg, agnameof(n), TRUE);
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (ND_rep(n) == NILnode)
	    ND_rep(n) = ND_rep(find(n));
    }
}

static void merge(edge_t * e, int minlen, int weight)
{
    ED_minlen(e) = MAX(ED_minlen(e), minlen);
    ED_weight(e) += weight;
}

/*Agedge_t* agfindedge(Agraph_t *g, Agnode_t *t, Agnode_t *h)
{
	return agedge(g,t,h,0,0);
}*/

static void strong(graph_t * g, node_t * t, node_t * h, edge_t * orig)
{
    edge_t *e;
    if ((e = agfindedge(g, t, h)) ||
	(e = agfindedge(g, h, t)) ||
	(e = agedge(g, t, h, (char *) 0, TRUE)))
	merge(e, ED_minlen(orig), ED_weight(orig));
    else
	abort();
}

static void weak(graph_t * g, node_t * t, node_t * h, edge_t * orig)
{
    node_t *v;
    edge_t *e, *f;

    for (e = agfstin(g, t); e; e = agnxtin(g, e)) {
	/* merge with existing weak edge (e,f) */
	v = agtail(e);
	if ((f = agfstout(g, v)) && (aghead(f) == h)) {
	    return;
	}
    }
    if (!e) {
	v = agnode(g, 0, TRUE);
	e = agedge(g, v, t, (char *) 0, TRUE);
	f = agedge(g, v, h, (char *) 0, TRUE);
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
		    || (find(aghead(e)) ==
			GD_minrep(ND_clust(aghead(e))))) {
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

static char *synthname(Agraph_t * g, char *prefix, char *suffix)
{
    char *rv = agalloc(g, strlen(prefix) + strlen(suffix) + 1);
    strcpy(rv, prefix);
    strcat(rv, suffix);
    return rv;
}

static void compile_clusters(graph_t * g, graph_t * Xg)
{
    node_t *n;
    node_t *rep, *top = 0, *bot = 0;
    edge_t *e;
    graph_t *sub;
    char *sname;

    if (is_a_cluster(g) && is_a_strong_cluster(g)) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (agfstin(g, n) == NILedge) {
		rep = ND_rep(find(n));
		if (!top) {
		    sname = synthname(g, agnameof(g), "_top\177");
		    top = agnode(Xg, sname, TRUE);
		    agedge(Xg, top, rep, (char *) 0, TRUE);
		    agfree(g, sname);
		}
	    }
	    if (agfstout(g, n) == NILedge) {
		rep = ND_rep(find(n));
		if (!bot) {
		    sname = synthname(g, agnameof(g), "_bottom\177");
		    bot = agnode(Xg, sname, TRUE);
		    agedge(Xg, rep, bot, (char *) 0, TRUE);
		    agfree(g, sname);
		}
	    }
	}
	if (top && bot) {
	    e = agedge(Xg, top, bot, (char *) 0, TRUE);
	    merge(e, 0, STRONG_CLUSTER_WEIGHT);
	}
    }
    for (sub = agfstsubg(g); sub; sub = agnxtsubg(sub))
	compile_clusters(sub, Xg);
}

static void reverse_edge(graph_t * g, edge_t * e)
{
    edge_t *rev;

    rev = agfindedge(g, aghead(e), agtail(e));
    if (!rev)
	rev = agedge(g, aghead(e), agtail(e), (char *) 0, TRUE);
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
	    reverse_edge(g, e);
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

static void readout_levels(graph_t * g, graph_t * Xg)
{
    node_t *n;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	ND_rank(n) = ND_rank(ND_rep(find(n)));
    }
}

static void dfscc(graph_t * g, node_t * n, int cc)
{
    edge_t *e;
    if (ND_mark(n) == 0) {
	ND_mark(n) = cc;
	for (e = agfstout(g, n); e; e = agnxtout(g, e))
	    dfscc(g, aghead(e), cc);
	for (e = agfstin(g, n); e; e = agnxtin(g, e))
	    dfscc(g, agtail(e), cc);
    }
}

static void connect_components(graph_t * g)
{
    int cc = 1;
    node_t *n;
    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	ND_mark(n) = 0;
    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	if (ND_mark(n) == 0)
	    dfscc(g, n, cc++);
    if (cc > 1) {
	node_t *root = agnode(g, "\177root", TRUE);
	int ncc = 1;
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (ND_mark(n) == ncc) {
		(void) agedge(g, root, n, (char *) 0, TRUE);
		ncc++;
	    }
	}
    }
}

static void aaa(graph_t * g)
{
    node_t *n;
    edge_t *e;
    Agsym_t *weight, *minlen, *label;
    char buf[64];

    weight = agattr(g, AGEDGE, "weight", "");
    minlen = agattr(g, AGEDGE, "minlen", "");
    label = agattr(g, AGEDGE, "label", "");
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    sprintf(buf, "%f", ED_weight(e));
	    agxset(e, weight, buf);
	    sprintf(buf, "%d", ED_minlen(e));
	    agxset(e, minlen, buf);
	    sprintf(buf, "%.02f,%d", ED_weight(e), ED_minlen(e));
	    agxset(e, label, buf);
	}
    }
}
static void printgraph(Agraph_t * g)
{
    aaa(g);
    agwrite(g, stderr);
}

static int nd_rank(Agnode_t * n)
{
    return ND_rank(n);
}
