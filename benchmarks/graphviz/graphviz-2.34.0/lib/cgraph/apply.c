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

#include	<cghdr.h>

/* The following functions take a graph and a template (node/edge/graph)
 * and return the object representing the template within the local graph.
 */
static Agobj_t *subnode_search(Agraph_t * sub, Agobj_t * n)
{
    if (agraphof(n) == sub)
	return n;
    return (Agobj_t *) agsubnode(sub, (Agnode_t *) n, FALSE);
}

static Agobj_t *subedge_search(Agraph_t * sub, Agobj_t * e)
{
    if (agraphof(e) == sub)
	return e;
    return (Agobj_t *) agsubedge(sub, (Agedge_t *) e, FALSE);
}

static Agobj_t *subgraph_search(Agraph_t * sub, Agobj_t * g)
{
    NOTUSED(g);
    return (Agobj_t *) sub;
}

/* recursively apply objfn within the hierarchy of a graph.
 * if obj is a node or edge, it and its images in every subg are visited.
 * if obj is a graph, then it and its subgs are visited.
 */
static void rec_apply(Agraph_t * g, Agobj_t * obj, agobjfn_t fn, void *arg,
		      agobjsearchfn_t objsearch, int preorder)
{
    Agraph_t *sub;
    Agobj_t *subobj;

    if (preorder)
	fn(g, obj, arg);
    for (sub = agfstsubg(g); sub; sub = agnxtsubg(sub)) {
	if ((subobj = objsearch(sub, obj)))
	    rec_apply(sub, subobj, fn, arg, objsearch, preorder);
    }
    if (NOT(preorder))
	fn(g, obj, arg);
}

/* external entry point (this seems to be one of those ineffective
 * comments censured in books on programming style) */
int agapply(Agraph_t * g, Agobj_t * obj, agobjfn_t fn, void *arg,
	    int preorder)
{
    Agobj_t *subobj;

    agobjsearchfn_t objsearch;
    switch (AGTYPE(obj)) {
    case AGRAPH:
	objsearch = subgraph_search;
	break;
    case AGNODE:
	objsearch = subnode_search;
	break;
    case AGOUTEDGE:
    case AGINEDGE:
	objsearch = subedge_search;
	break;
    default:
	agerr(AGERR, "agapply: unknown object type %d\n", AGTYPE(obj));
	return FAILURE;
	break;
    }
    if ((subobj = objsearch(g, obj))) {
	rec_apply(g, subobj, fn, arg, objsearch, preorder);
	return SUCCESS;
    } else
	return FAILURE;
}
