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
 * Decompose finds the connected components of a graph.
 * It searches the temporary edges and ignores non-root nodes.
 * The roots of the search are the real nodes of the graph,
 * but any virtual nodes discovered are also included in the
 * component.
 */

#include "dot.h"


static graph_t *G;
static node_t *Last_node;
static char Cmark;

static void 
begin_component(void)
{
    Last_node = GD_nlist(G) = NULL;
}

static void 
add_to_component(node_t * n)
{
    GD_n_nodes(G)++;
    ND_mark(n) = Cmark;
    if (Last_node) {
	ND_prev(n) = Last_node;
	ND_next(Last_node) = n;
    } else {
	ND_prev(n) = NULL;
	GD_nlist(G) = n;
    }
    Last_node = n;
    ND_next(n) = NULL;
}

static void 
end_component(void)
{
    int i;

    i = GD_comp(G).size++;
    GD_comp(G).list = ALLOC(GD_comp(G).size, GD_comp(G).list, node_t *);
    GD_comp(G).list[i] = GD_nlist(G);
}

static void
search_component(graph_t * g, node_t * n)
{
    int c, i;
    elist vec[4];
    node_t *other;
    edge_t *e;

    add_to_component(n);
    vec[0] = ND_out(n);
    vec[1] = ND_in(n);
    vec[2] = ND_flat_out(n);
    vec[3] = ND_flat_in(n);

    for (c = 0; c <= 3; c++) {
	if (vec[c].list)
	    for (i = 0; (e = vec[c].list[i]); i++) {
		if ((other = aghead(e)) == n)
		    other = agtail(e);
		if ((ND_mark(other) != Cmark) && (other == UF_find(other)))
		    search_component(g, other);
	    }
    }
}

void decompose(graph_t * g, int pass)
{
    graph_t *subg;
    node_t *n, *v;

    G = g;
    if (++Cmark == 0)
	Cmark = 1;
    GD_n_nodes(g) = GD_comp(g).size = 0;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	v = n;
	if ((pass > 0) && (subg = ND_clust(v)))
	    v = GD_rankleader(subg)[ND_rank(v)];
	else if (v != UF_find(v))
	    continue;
	if (ND_mark(v) != Cmark) {
	    begin_component();
	    search_component(g, v);
	    end_component();
	}
    }
}
