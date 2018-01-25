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

#include <limits.h>

#include "libgraph.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#if 0
/* graphs_of_e() is never used - suppress compiler warnings. */
static void graphs_of_e(Agedge_t * e, Dict_t * g_e, Agraph_t * g)
{
    Agedge_t *sub;

    if (dtsearch(g->inedges, e) == NULL)
	return;
    if (dtsearch(g_e, g->meta_node))
	return;
    dtinsert(g_e, g->meta_node);
    for (sub = agfstin(g->meta_node->graph, g->meta_node); sub;
	 sub = agnxtin(g->meta_node->graph, sub))
	graphs_of_e(e, g_e, agusergraph(sub->head));
}
#endif

static Agedge_t *esearch(Agraph_t * g, Agnode_t * tail, Agnode_t * head,
			 char *usrkey)
{
    Agedge_t key, *e;
    char *attr[KEYX + 1];

    attr[KEYX] = usrkey;
    key.tail = tail;
    key.head = head;
    key.attr = (usrkey ? attr : 0);
    if (usrkey)
	e = (Agedge_t *) dtsearch(g->inedges, &key);
    else {
	e = (Agedge_t *) dtnext(g->inedges, &key);
	if (e && ((e->tail != tail) || (e->head != head)))
	    e = NULL;
    }
    return e;
}

Agedge_t *agfindedge(Agraph_t * g, Agnode_t * tail, Agnode_t * head)
{
    Agedge_t *e;

    e = esearch(g, tail, head, NULL);
    if ((e == NULL) && !(AG_IS_DIRECTED(g)))
	e = esearch(g, head, tail, NULL);
    return e;
}

static void install_edge(Agraph_t * g, Agedge_t * e)
{
    Agraph_t *meta;
    Agedge_t *f;

    if (dtsearch(g->inedges, e))
	return;
    agINSnode(g, e->tail);
    agINSnode(g, e->head);
    dtinsert(g->outedges, e);
    dtinsert(g->inedges, e);
    f = (Agedge_t *) dtprev(g->outedges, e);
    if (f && (f->tail == e->tail) && (f->head == e->head)
	&& (e->printkey == NOPRINT))
	e->printkey = MULTIPLE;
    if (AG_IS_METAGRAPH(g) == FALSE) {
	meta = g->meta_node->graph;
	for (f = agfstin(meta, g->meta_node); f; f = agnxtin(meta, f)) {
	    install_edge(agusergraph(f->tail), e);
	}
    }
}

void agINSedge(Agraph_t * g, Agedge_t * e)
{
    if (e->printkey == MULTIPLE)
	e->printkey = MUSTPRINT;
    install_edge(g, e);
}


static int printedge(Dict_t * d, void *p, void *ignored)
{
    Agedge_t *e = (Agedge_t *) p;
    agerr(AGPREV, "\t%p %s,%s\n", e, e->tail->name, e->head->name);
    return 0;
}

Agedge_t *agfstedge(Agraph_t * g, Agnode_t * n)
{
    Agedge_t *e;

    e = NULL;
    if ((g != NULL) && (n != NULL)) {
	e = agfstout(g, n);
	if (e == NULL)
	    e = agfstin(g, n);
    }
    return e;
}

Agedge_t *agnxtedge(Agraph_t * g, Agedge_t * e, Agnode_t * n)
{
    Agedge_t *f;

    f = NULL;
    if ((g != NULL) && (e != NULL) && (n != NULL)) {
	if (e->tail == n) {
	    f = (Agedge_t *) dtnext(g->outedges, e);
	    if ((f != NULL) && (f->tail == n))
		return f;
	    f = agfstin(g, n);
	    while (f && (f->head == f->tail) && (f->head == n))
		f = (Agedge_t *) dtnext(g->inedges, f);
	} else {
	    if (e->head != n)
		return NULL;
	    else
		f = (Agedge_t *) dtnext(g->inedges, e);
	}
	while (f && (f->head == f->tail) && (f->head == n))
	    f = (Agedge_t *) dtnext(g->inedges, f);
	if (f && (f->head != n))
	    f = NULL;
    }
    return f;
}

Agedge_t *agfstout(Agraph_t * g, Agnode_t * n)
{
    Agedge_t *f, key;

    f = NULL;
    if ((g != NULL) && (n != NULL)) {
	key.tail = n;
	key.head = NULL;
	key.attr = NULL;
	f = (Agedge_t *) dtnext(g->outedges, &key);
	if (f && (f->tail != n))
	    f = NULL;
    }
    return f;
}

Agedge_t *agnxtout(Agraph_t * g, Agedge_t * e)
{
    Agedge_t *f;
    f = (Agedge_t *) dtnext(g->outedges, e);
    if (f && (f->tail != e->tail))
	f = NULL;
    return f;
}

Agedge_t *agfstin(Agraph_t * g, Agnode_t * n)
{
    Agedge_t *f, key;

    f = NULL;
    if ((g != NULL) && (n != NULL)) {
	key.head = n;
	key.tail = NULL;
	key.attr = NULL;
	f = (Agedge_t *) dtnext(g->inedges, &key);
	if (f && (f->head != n))
	    f = NULL;
    }
    return f;
}

Agedge_t *agnxtin(Agraph_t * g, Agedge_t * e)
{
    Agedge_t *f;

    f = (Agedge_t *) dtnext(g->inedges, e);
    if (f && (f->head != e->head))
	f = NULL;
    return f;
}

Agedge_t *agNEWedge(Agraph_t * subg, Agnode_t * tail, Agnode_t * head,
		    Agedge_t * proto)
{
    int i, nobj;
    Agedge_t *e;

    e = (Agedge_t *) calloc(1, AG.edge_nbytes);
    e->tag = TAG_EDGE;
    e->tail = tail;
    e->head = head;
    e->id = subg->univ->max_edge_id++;

    nobj = dtsize(subg->univ->edgeattr->dict);
    if (nobj) {
		e->attr = N_NEW(nobj, char *);
		e->didset = N_NEW((nobj + CHAR_BIT - 1) / CHAR_BIT, char);
	}
    else {
		e->attr = NULL;
		e->didset = NULL;
	}
    for (i = 0; i < nobj; i++)
		e->attr[i] =
			agstrdup(proto ? proto->attr[i] : subg->univ->edgeattr->
				 list[i]->value);
    return e;
}

Agedge_t *agedge(Agraph_t * g, Agnode_t * tail, Agnode_t * head)
{
    Agedge_t *e;
    char *keystr, key[SMALLBUF], printkey = NOPRINT;
    static int ctr;

    keystr = g->proto->e->attr[KEYX];	/* temporarily set aside */
    e = NULL;
    g->proto->e->head = head;
    g->proto->e->tail = tail;
    if (AG_IS_STRICT(g)) {
	e = esearch(g, tail, head, NULL);
	if (!e && !AG_IS_DIRECTED(g))
	    e = esearch(g, head, tail, NULL);
	if (e)
	    install_edge(g, e);
    } else {
	if (keystr[0]) {
	    e = esearch(g, tail, head, keystr);
	    if (!e && !AG_IS_DIRECTED(g))
		e = esearch(g, head, tail, keystr);
	    if (e)
		agINSedge(g, e);
	    else
		printkey = MUSTPRINT;
	} else {
	    sprintf(key, "%d", ctr++);
	    g->proto->e->attr[KEYX] = key;
	}
    }
    if (e == NULL) {
	e = agNEWedge(g, tail, head, g->proto->e);
	install_edge(g, e);
	g->proto->e->head = g->proto->e->tail = g->proto->n;
	e->printkey = printkey;
    }
    g->proto->e->attr[KEYX] = keystr;
    return e;
}

void agFREEedge(Agedge_t * e)
{
    int i, nobj;
    Agdict_t *dict = agdictof(e);

    dict = dict;
    TAG_OF(e) = -1;
    nobj = dtsize(e->tail->graph->univ->edgeattr->dict);
    for (i = 0; i < nobj; i++)
	agstrfree(e->attr[i]);
    free(e->attr);
	free(e->didset);
    free(e);
}

void agDELedge(Agraph_t * g, Agedge_t * e)
{
    Agraph_t *meta;
    Agraph_t *g0;
    Agedge_t *f;

    if (dtsearch(g->inedges, e) == NULL) {
	agerr(AGERR, "Edge %p was not found\n", e);
	dtwalk(g->inedges, printedge, NIL(void *));
	return;
    }
    if (AG_IS_METAGRAPH(g) == FALSE) {
	meta = g->meta_node->graph;
	for (f = agfstout(meta, g->meta_node); f; f = agnxtout(meta, f)) {
	    g0 = agusergraph(f->head);
	    if (dtsearch(g0->inedges, e))
		agDELedge(g0, e);
	}
    }
    dtdelete(g->inedges, e);
    dtdelete(g->outedges, e);
    if (g == g->root)
	agFREEedge(e);
}
