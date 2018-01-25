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

#include    "patchwork.h"
#include    "adjust.h"
#include    "pack.h"
#include    "neatoprocs.h"

/* the following code shamelessly copied from lib/fdpgen/layout.c
and should be extracted and made into a common function */

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
    graph_t* subg;
    clist_t  list;
    clist_t* clist;
#ifndef WITH_CGRAPH
    node_t*  mn;
    edge_t*  me;
    graph_t* mg;
#endif

    if (pclist == NULL) {
        clist = &list;
        initCList(clist);
    }
    else
        clist = pclist;

#ifndef WITH_CGRAPH
    mg = g->meta_node->graph;
    for (me = agfstout(mg, g->meta_node); me; me = agnxtout(mg, me)) {
        mn = aghead(me);
        subg = agusergraph(mn);
#else /* WITH_CGRAPH */
    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
#endif /* WITH_CGRAPH */
        if (!strncmp(agnameof(subg), "cluster", 7)) {
#ifdef WITH_CGRAPH
	    agbindrec(subg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);
#endif
#ifdef FDP_GEN
            GD_alg(subg) = (void *) NEW(gdata); /* freed in cleanup_subgs */
            GD_ndim(subg) = GD_ndim(parent);
            LEVEL(subg) = LEVEL(parent) + 1;
            GPARENT(subg) = parent;
#endif
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

static void patchwork_init_node(node_t * n)
{
    agset(n,"shape","box");
    /* common_init_node_opt(n,FALSE); */
}

static void patchwork_init_edge(edge_t * e)
{
    agbindrec(e, "Agedgeinfo_t", sizeof(Agnodeinfo_t), TRUE);  // edge custom data
    /* common_init_edge(e); */
}

static void patchwork_init_node_edge(graph_t * g)
{
    node_t *n;
    edge_t *e;
    int i = 0;
    rdata* alg = N_NEW(agnnodes(g), rdata);

    GD_neato_nlist(g) = N_NEW(agnnodes(g) + 1, node_t *);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
#ifdef WITH_CGRAPH
	agbindrec(n, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);  // node custom data
#endif /* WITH_CGRAPH */
	ND_alg(n) = alg + i;
	GD_neato_nlist(g)[i++] = n;
	patchwork_init_node(n);

	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    patchwork_init_edge(e);
	}
    }
}

static void patchwork_init_graph(graph_t * g)
{
#ifndef WITH_CGRAPH
    N_shape = agnodeattr(g, "shape", "box");
#else
    N_shape = agattr(g, AGNODE, "shape","box");
#endif
    setEdgeType (g, ET_LINE);
    /* GD_ndim(g) = late_int(g,agfindattr(g,"dim"),2,2); */
    Ndim = GD_ndim(g) = 2;	/* The algorithm only makes sense in 2D */
    mkClusters(g, NULL, g);
    patchwork_init_node_edge(g);
}

/* patchwork_layout:
 * The current version makes no use of edges, neither for a notion of connectivity
 * nor during drawing.
 */
void patchwork_layout(Agraph_t *g)
{
    patchwork_init_graph(g);

    if ((agnnodes(g) == 0) && (GD_n_cluster(g) == 0)) return;

    patchworkLayout (g);

    dotneato_postprocess(g);
}

static void patchwork_cleanup_graph(graph_t * g)
{
    free(GD_neato_nlist(g));
    if (g != agroot(g))
#ifndef WITH_CGRAPH
        memset(&(g->u), 0, sizeof(Agraphinfo_t));
#else /* WITH_CGRAPH */
        agclean(g, AGRAPH , "Agraphinfo_t");
#endif /* WITH_CGRAPH */
}

void patchwork_cleanup(graph_t * g)
{
    node_t *n;
    edge_t *e;

    n = agfstnode(g);
    if (!n) return;
    free (ND_alg(n));
    for (; n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    gv_cleanup_edge(e);
	}
	gv_cleanup_node(n);
    }
    patchwork_cleanup_graph(g);
}
