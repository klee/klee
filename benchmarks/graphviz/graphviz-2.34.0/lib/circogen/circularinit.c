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
 * Circular layout. Biconnected components are put on circles.
 * block-cutnode tree is done recursively, with children placed
 * about parent block.
 * Based on:
 *   Six and Tollis, "A Framework for Circular Drawings of
 * Networks", GD '99, LNCS 1731, pp. 107-116;
 *   Six and Tollis, "Circular Drawings of Biconnected Graphs",
 * Proc. ALENEX '99, pp 57-73.
 *   Kaufmann and Wiese, "Maintaining the Mental Map for Circular
 * Drawings", GD '02, LNCS 2528, pp. 12-22.
 */

#include    "circular.h"
#include    "adjust.h"
#include    "pack.h"
#include    "neatoprocs.h"
#include    <string.h>

static void circular_init_edge(edge_t * e)
{
#ifdef WITH_CGRAPH
    agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
    common_init_edge(e);

    ED_factor(e) = late_double(e, E_weight, 1.0, 0.0);
}


static void circular_init_node_edge(graph_t * g)
{
    node_t *n;
    edge_t *e;
    int i = 0;
    ndata* alg = N_NEW(agnnodes(g), ndata);

    GD_neato_nlist(g) = N_NEW(agnnodes(g) + 1, node_t *);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	neato_init_node(n);
	ND_alg(n) = alg + i;
	GD_neato_nlist(g)[i++] = n;
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    circular_init_edge(e);
	}
    }
}


void circo_init_graph(graph_t * g)
{
    setEdgeType (g, ET_LINE);
    /* GD_ndim(g) = late_int(g,agfindattr(g,"dim"),2,2); */
    Ndim = GD_ndim(g) = 2;	/* The algorithm only makes sense in 2D */
    circular_init_node_edge(g);
}

/* makeDerivedNode:
 * Make a node in the derived graph, with the given name.
 * orig points to what it represents, either a real node or
 * a cluster. Copy size info from original node; needed for
 * adjustNodes and packSubgraphs.
 */
static node_t *makeDerivedNode(graph_t * dg, char *name, int isNode,
			       void *orig)
{
#ifndef WITH_CGRAPH
    node_t *n = agnode(dg, name);
#else /* WITH_CGRAPH */
    node_t *n = agnode(dg, name,1);
    agbindrec(n, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
    ND_alg(n) = (void *) NEW(cdata);
    if (isNode) {
	ND_pos(n) = N_NEW(Ndim, double);
	ND_lw(n) = ND_lw((node_t *) orig);
	ND_rw(n) = ND_rw((node_t *) orig);
	ND_ht(n) = ND_ht((node_t *) orig);
	ORIGN(n) = (node_t *) orig;
    } else
	ORIGG(n) = (graph_t *) orig;
    return n;
}

/* circomps:
 * Construct a strict, undirected graph with no loops from g.
 * Construct the connected components with the provision that all
 * nodes in a block subgraph are considered connected.
 * Return array of components with number of components in cnt.
 * Each component has its blocks as subgraphs.
 * FIX: Check that blocks are disjoint.
 */
Agraph_t **circomps(Agraph_t * g, int *cnt)
{
    int c_cnt;
    Agraph_t **ccs;
    Agraph_t *dg;
    Agnode_t *n, *v, *dt, *dh;
    Agedge_t *e;
    Agraph_t *sg;
    int i;
    Agedge_t *ep;
    Agnode_t *p;

#ifndef WITH_CGRAPH
    dg = agopen("derived", AGFLAG_STRICT);
#else /* WITH_CGRAPH */
    dg = agopen("derived", Agstrictundirected,NIL(Agdisc_t *));
    agbindrec (dg, "info", sizeof(Agraphinfo_t), TRUE);
#endif /* WITH_CGRAPH */
    GD_alg(g) = dg;  /* store derived graph for closing later */

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if (DNODE(v))
	    continue;
	n = makeDerivedNode(dg, agnameof(v), 1, v);
	DNODE(v) = n;
    }

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	for (e = agfstout(g, v); e; e = agnxtout(g, e)) {
	    dt = DNODE(agtail(e));
	    dh = DNODE(aghead(e));
	    if (dt != dh) {
#ifndef WITH_CGRAPH
		agedge(dg, dt, dh);
#else /* WITH_CGRAPH */
		agbindrec(agedge(dg, dt, dh, NULL, 1), "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
	    }
	}
    }

    ccs = ccomps(dg, &c_cnt, 0);

    /* replace block nodes with block contents */
    for (i = 0; i < c_cnt; i++) {
	sg = ccs[i];

	/* add edges: since sg is a union of components, all edges
	 * of any node should be added, except loops.
	 */
	for (n = agfstnode(sg); n; n = agnxtnode(sg, n)) {
	    p = ORIGN(n);
	    for (e = agfstout(g, p); e; e = agnxtout(g, e)) {
		/* n = DNODE(agtail(e)); by construction since agtail(e) == p */
		dh = DNODE(aghead(e));
		if (n != dh) {
#ifndef WITH_CGRAPH
		    ep = agedge(dg, n, dh);
		    aginsert(sg, ep);
#else /* WITH_CGRAPH */
		    ep = agedge(dg, n, dh, NULL, 1);
		    agbindrec(ep, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);	//node custom data
		    agsubedge(sg,ep,1);
#endif /* WITH_CGRAPH */
		}
	    }
	}
    }

    /* Finally, add edge data to edges */
    for (n = agfstnode(dg); n; n = agnxtnode(dg, n)) {
	for (e = agfstout(dg, n); e; e = agnxtout(dg, e)) {
	    ED_alg(e) = NEW(edata);
	}
    }

    *cnt = c_cnt;
    return ccs;
}

/* closeDerivedGraph:
 */
static void closeDerivedGraph(graph_t * g)
{
    node_t *n;
    edge_t *e;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    free(ED_alg(e));
	}
	free(ND_alg(n));
	free(ND_pos(n));
    }
    agclose(g);
}

/* copyPosns:
 * Copy position of nodes in given subgraph of derived graph
 * to corresponding node in original graph.
 * FIX: consider assigning from n directly to ORIG(n).
 */
static void copyPosns(graph_t * g)
{
    node_t *n;
    node_t *v;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	v = ORIGN(n);
	ND_pos(v)[0] = ND_pos(n)[0];
	ND_pos(v)[1] = ND_pos(n)[1];
    }
}

/* circoLayout:
 */
void circoLayout(Agraph_t * g)
{
    Agraph_t **ccs;
    Agraph_t *sg;
    int ncc;
    int i;

    if (agnnodes(g)) {
	ccs = circomps(g, &ncc);

	if (ncc == 1) {
	    circularLayout(ccs[0], g);
	    copyPosns(ccs[0]);
	    adjustNodes(g);
	} else {
	    Agraph_t *dg = ccs[0]->root;
	    pack_info pinfo;
	    getPackInfo(g, l_node, CL_OFFSET, &pinfo);

	    for (i = 0; i < ncc; i++) {
		sg = ccs[i];
		circularLayout(sg, g);
		adjustNodes(sg);
	    }
	    /* FIX: splines have not been calculated for dg
	     * To use, either do splines in dg and copy to g, or
	     * construct components of g from ccs and use that in packing.
	     */
	    packSubgraphs(ncc, ccs, dg, &pinfo);
	    for (i = 0; i < ncc; i++)
		copyPosns(ccs[i]);
	}
	free(ccs);
    }
}

/* circo_layout:
 */
void circo_layout(Agraph_t * g)
{
    if (agnnodes(g) == 0) return;
    circo_init_graph(g);
    circoLayout(g);
	/* Release ND_alg as we need to reuse it during edge routing */
    free(ND_alg(agfstnode(g)));
    spline_edges(g);
    dotneato_postprocess(g);
}

/* circo_cleanup:
 * ND_alg is freed in circo_layout
 */
void circo_cleanup(graph_t * g)
{
    node_t *n;
    edge_t *e;

    n = agfstnode(g);
    if (n == NULL)
	return;			/* g is empty */

    closeDerivedGraph((graph_t*)GD_alg(g));	/* delete derived graph */

    /* free (ND_alg(n)); */
    for (; n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    gv_cleanup_edge(e);
	}
	gv_cleanup_node(n);
    }
    free(GD_neato_nlist(g));
    if (g != agroot(g)) 
#ifndef WITH_CGRAPH
	memset(&(g->u), 0, sizeof(Agraphinfo_t));
#else /* WITH_CGRAPH */
	agclean (g,AGRAPH,"Agraphinfo_t");
#endif /* WITH_CGRAPH */
}
