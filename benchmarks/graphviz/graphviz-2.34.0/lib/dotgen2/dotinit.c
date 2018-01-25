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
#define DEBUG
#ifdef DEBUG
static void attach_attributes(graph_t *g)
{
    node_t	*n;
    Agsym_t	*rank, *order;
    char	buf[64];

    rank = agattr(g,AGNODE,"rank","");
    order = agattr(g,AGNODE,"order","");
    for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
	sprintf(buf,"%d",ND_rank(n));
	agxset(n,rank,buf);
	sprintf(buf,"%d",ND_order(n));
	agxset(n,order,buf);
    }
}
#endif

static void dot_init_node(node_t * n)
{
    common_init_node(n);
    gv_nodesize(n, GD_flip(agraphof(n)));
/*
    alloc_elist(4, ND_in(n));
    alloc_elist(4, ND_out(n));
    alloc_elist(2, ND_flat_in(n));
    alloc_elist(2, ND_flat_out(n));
    alloc_elist(2, ND_other(n));
    ND_UF_size(n) = 1;
*/
}

static void dot_init_edge(edge_t * e)
{
    char *tailgroup, *headgroup;
    common_init_edge(e);

    ED_weight(e) = late_double(e, E_weight, 1.0, 0.0);
    tailgroup = late_string(agtail(e), N_group, "");
    headgroup = late_string(aghead(e), N_group, "");
    ED_count(e) = ED_xpenalty(e) = 1;
    /* FIX: does new ranking and mincross handle these? */
    if (tailgroup[0] && (tailgroup == headgroup)) {
	ED_xpenalty(e) = CL_CROSS;
	ED_weight(e) *= 100;
    }
    if (is_nonconstraint(e)) {
	ED_xpenalty(e) = 0;
	ED_weight(e) = 0;
    }

    ED_showboxes(e) = late_int(e, E_showboxes, 0, 0);
    ED_minlen(e) = late_int(e, E_minlen, 1, 0);
}

static void initGraph(graph_t * g)
{
    aginit(g, AGRAPH, "Agraphinfo_t", -sizeof(Agraphinfo_t), TRUE);
    aginit(g, AGNODE, "Agnodeinfo_t", -sizeof(Agnodeinfo_t), TRUE);
    aginit(g, AGEDGE, "Agedgeinfo_t", -sizeof(Agedgeinfo_t), TRUE);
}

static void dot_init_node_edge(graph_t * g)
{
    node_t *n;
    edge_t *e;

    initGraph (g);
    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	dot_init_node(n);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e))
	    dot_init_edge(e);
    }
}

/* dot2_layout:
 * FIX:
 *  connected components
 *  multiedges ?
 *  concentrator ?
 *  aspect ?
 */
void dot2_layout(Agraph_t * g)
{
    Agraph_t *tempG;

    fprintf(stderr, "dot_layout\n");
    dot_init_node_edge(g);

    dot2_levels(g);
    /* agwrite(g, stdout); */
    tempG = dot2_mincross(g);
#ifdef DEBUG
    attach_attributes(tempG);
#endif
//    agwrite(tempG,stdout);

    return;
}

void dot2_cleanup(graph_t * g)
{
    /* FIX */
    printf("dot_cleanup\n");
    return;
}
