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
 * Break cycles in a directed graph by depth-first search.
 */

#include "dot.h"

void reverse_edge(edge_t * e)
{
    edge_t *f;

    delete_fast_edge(e);
    if ((f = find_fast_edge(aghead(e), agtail(e))))
	merge_oneway(e, f);
    else
	virtual_edge(aghead(e), agtail(e), e);
}

static void 
dfs(node_t * n)
{
    int i;
    edge_t *e;
    node_t *w;

    if (ND_mark(n))
	return;
    ND_mark(n) = TRUE;
    ND_onstack(n) = TRUE;
    for (i = 0; (e = ND_out(n).list[i]); i++) {
	w = aghead(e);
	if (ND_onstack(w)) {
	    reverse_edge(e);
	    i--;
	} else {
	    if (ND_mark(w) == FALSE)
		dfs(w);
	}
    }
    ND_onstack(n) = FALSE;
}


void acyclic(graph_t * g)
{
    int c;
    node_t *n;

    for (c = 0; c < GD_comp(g).size; c++) {
	GD_nlist(g) = GD_comp(g).list[c];
	for (n = GD_nlist(g); n; n = ND_next(n))
	    ND_mark(n) = FALSE;
	for (n = GD_nlist(g); n; n = ND_next(n))
	    dfs(n);
    }
}

