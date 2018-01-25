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
 * Classify edges for rank assignment phase to
 * create temporary edges.
 */

#include "dot.h"


int nonconstraint_edge(edge_t * e)
{
    char *constr;

#ifndef WITH_CGRAPH
    if (E_constr && (constr = agxget(e, E_constr->index))) {
#else /* WITH_CGRAPH */
    if (E_constr && (constr = agxget(e, E_constr))) {
#endif /* WITH_CGRAPH */
	if (constr[0] && mapbool(constr) == FALSE)
	    return TRUE;
    }
    return FALSE;
}

static void 
interclust1(graph_t * g, node_t * t, node_t * h, edge_t * e)
{
    node_t *v, *t0, *h0;
    int offset, t_len, h_len, t_rank, h_rank;
    edge_t *rt, *rh;

    if (ND_clust(agtail(e)))
	t_rank = ND_rank(agtail(e)) - ND_rank(GD_leader(ND_clust(agtail(e))));
    else
	t_rank = 0;
    if (ND_clust(aghead(e)))
	h_rank = ND_rank(aghead(e)) - ND_rank(GD_leader(ND_clust(aghead(e))));
    else
	h_rank = 0;
    offset = ED_minlen(e) + t_rank - h_rank;
    if (offset > 0) {
	t_len = 0;
	h_len = offset;
    } else {
	t_len = -offset;
	h_len = 0;
    }

    v = virtual_node(g);
    ND_node_type(v) = SLACKNODE;
    t0 = UF_find(t);
    h0 = UF_find(h);
    rt = make_aux_edge(v, t0, t_len, CL_BACK * ED_weight(e));
    rh = make_aux_edge(v, h0, h_len, ED_weight(e));
    ED_to_orig(rt) = ED_to_orig(rh) = e;
}
void class1(graph_t * g)
{
    node_t *n, *t, *h;
    edge_t *e, *rep;

    mark_clusters(g);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {

	    /* skip edges already processed */
	    if (ED_to_virt(e))
		continue;

	    /* skip edges that we want to ignore in this phase */
	    if (nonconstraint_edge(e))
		continue;

	    t = UF_find(agtail(e));
	    h = UF_find(aghead(e));

	    /* skip self, flat, and intra-cluster edges */
	    if (t == h)
		continue;


	    /* inter-cluster edges require special treatment */
	    if (ND_clust(t) || ND_clust(h)) {
		interclust1(g, agtail(e), aghead(e), e);
		continue;
	    }

	    if ((rep = find_fast_edge(t, h)))
		merge_oneway(e, rep);
	    else
		virtual_edge(t, h, e);

#ifdef NOTDEF
	    if ((t == agtail(e)) && (h == aghead(e))) {
		if (rep = find_fast_edge(t, h))
		    merge_oneway(e, rep);
		else
		    virtual_edge(t, h, e);
	    } else {
		f = agfindedge(g, t, h);
		if (f && (ED_to_virt(f) == NULL))
		    rep = virtual_edge(t, h, f);
		else
		    rep = find_fast_edge(t, h);
		if (rep)
		    merge_oneway(e, rep);
		else
		    virtual_edge(t, h, e);
	    }
#endif
	}
    }
}

