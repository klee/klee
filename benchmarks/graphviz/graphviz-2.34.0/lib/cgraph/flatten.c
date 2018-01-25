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

#include "cghdr.h"

static void agflatten_elist(Dict_t * d, Dtlink_t ** lptr, int flag)
{
    dtrestore(d, *lptr);
    dtmethod(d, flag? Dtlist : Dtoset);
    *lptr = dtextract(d);
}

void agflatten_edges(Agraph_t * g, Agnode_t * n, int flag)
{
    Agsubnode_t *sn;
    Dtlink_t **tmp;

	sn = agsubrep(g,n);
    tmp = &(sn->out_seq); /* avoiding - "dereferencing type-punned pointer will break strict-aliasing rules" */
    agflatten_elist(g->e_seq, tmp, flag);
    tmp = &(sn->in_seq);
    agflatten_elist(g->e_seq, tmp, flag);
}

void agflatten(Agraph_t * g, int flag)
{
    Agnode_t *n;

    if (flag) {
	if (g->desc.flatlock == FALSE) {
	    dtmethod(g->n_seq,Dtlist);
	    for (n = agfstnode(g); n; n = agnxtnode(g,n))
		agflatten_edges(g, n, flag);
	    g->desc.flatlock = TRUE;
	}
    } else {
	if (g->desc.flatlock) {
	    dtmethod(g->n_seq,Dtoset);
	    for (n = agfstnode(g); n; n = agnxtnode(g,n))
		agflatten_edges(g, n, flag);
	    g->desc.flatlock = FALSE;
	}
    }
}

void agnotflat(Agraph_t * g)
{
    if (g->desc.flatlock)
	agerr(AGERR, "flat lock broken");
}
