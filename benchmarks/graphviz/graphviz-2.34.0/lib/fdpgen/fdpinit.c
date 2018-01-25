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


/* fdpinit.c:
 * Written by Emden R. Gansner
 *
 * Mostly boilerplate initialization and cleanup code.
 */

/* uses PRIVATE interface */
#define FDP_PRIVATE 1

#include    "tlayout.h"
#include    "neatoprocs.h"
#include    "agxbuf.h"

static void initialPositions(graph_t * g)
{
    int i;
    node_t *np;
    attrsym_t *possym;
    attrsym_t *pinsym;
    double *pvec;
    char *p;
    char c;

#ifndef WITH_CGRAPH
    possym = agfindattr(g->proto->n, "pos");
#else /* WITH_CGRAPH */
    possym = agattr(g,AGNODE, "pos", NULL);
#endif /* WITH_CGRAPH */
    if (!possym)
	return;
#ifndef WITH_CGRAPH
    pinsym = agfindattr(g->proto->n, "pin");
#else /* WITH_CGRAPH */
    pinsym = agattr(g,AGNODE, "pin", NULL);
#endif /* WITH_CGRAPH */
    for (i = 0; (np = GD_neato_nlist(g)[i]); i++) {
#ifndef WITH_CGRAPH
	p = agxget(np, possym->index);
#else /* WITH_CGRAPH */
	p = agxget(np, possym);
#endif /* WITH_CGRAPH */
	if (p[0]) {
	    pvec = ND_pos(np);
	    c = '\0';
	    if (sscanf(p, "%lf,%lf%c", pvec, pvec + 1, &c) >= 2) {
		if (PSinputscale > 0.0) {
		    int i;
		    for (i = 0; i < NDIM; i++)
			pvec[i] = pvec[i] / PSinputscale;
		}
		ND_pinned(np) = P_SET;
		if ((c == '!')
#ifndef WITH_CGRAPH
		    || (pinsym && mapbool(agxget(np, pinsym->index))))
#else /* WITH_CGRAPH */
		    || (pinsym && mapbool(agxget(np, pinsym))))
#endif /* WITH_CGRAPH */
		    ND_pinned(np) = P_PIN;
	    } else
		fprintf(stderr,
			"Warning: node %s, position %s, expected two floats\n",
			agnameof(np), p);
	}
    }
}

/* init_edge:
 */
static void init_edge(edge_t * e, attrsym_t * E_len)
{
#ifdef WITH_CGRAPH
    agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
    ED_factor(e) = late_double(e, E_weight, 1.0, 0.0);
    ED_dist(e) = late_double(e, E_len, fdp_parms->K, 0.0);

    common_init_edge(e);
}

#ifdef WITH_CGRAPH
static void init_node(node_t * n)
{
    common_init_node(n);
    ND_pos(n) = N_NEW(GD_ndim(agraphof(n)), double);
    gv_nodesize(n, GD_flip(agraphof(n)));
}
#endif

void fdp_init_node_edge(graph_t * g)
{
    attrsym_t *E_len;
    node_t *n;
    edge_t *e;
    int nn;
    int i;
    /* ndata* alg; */

#ifdef WITH_CGRAPH
    aginit(g, AGNODE, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);
#endif
    processClusterEdges(g);

    /* Get node count after processClusterEdges(), as this function may
     * add new nodes.
     */
    nn = agnnodes(g);
    /* alg = N_NEW(nn, ndata); */
    GD_neato_nlist(g) = N_NEW(nn + 1, node_t *);

    for (i = 0, n = agfstnode(g); n; n = agnxtnode(g, n)) {
#ifdef WITH_CGRAPH
	init_node (n);
#else
	neato_init_node (n);
#endif
	/* ND_alg(n) = alg + i; */
	GD_neato_nlist(g)[i] = n;
	ND_id(n) = i++;
    }

#ifndef WITH_CGRAPH
    E_len = agfindattr(g->proto->e, "len");
#else /* WITH_CGRAPH */
    E_len = agattr(g,AGEDGE, "len", NULL);
#endif /* WITH_CGRAPH */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    init_edge(e, E_len);
	}
    }
    initialPositions(g);

}

static void cleanup_subgs(graph_t * g)
{
    graph_t *subg;
    int i;

    for (i = 1; i <= GD_n_cluster(g); i++) {
	subg = GD_clust(g)[i];
	free_label(GD_label(subg));
	if (GD_alg(subg)) {
	    free(PORTS(subg));
	    free(GD_alg(subg));
	}
	cleanup_subgs(subg);
    }
    free (GD_clust(g));
    if (g != agroot(g))
#ifndef WITH_CGRAPH
	memset(&(g->u), 0, sizeof(Agraphinfo_t));
#else /* WITH_CGRAPH */
	agdelrec(g, "Agraphinfo_t");				
#endif /* WITH_CGRAPH */
}

static void fdp_cleanup_graph(graph_t * g)
{
    cleanup_subgs(g);
    free(GD_neato_nlist(g));
    free(GD_alg(g));
}

void fdp_cleanup(graph_t * g)
{
    node_t *n;
    edge_t *e;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    gv_cleanup_edge(e);
	}
	gv_cleanup_node(n);
    }
    fdp_cleanup_graph(g);
}
