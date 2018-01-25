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

Dtdisc_t agNamedisc = {
    offsetof(Agnode_t, name),
    -1,
    -1,				/* link offset */
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    NIL(Dtcompar_f),		/* use strcmp */
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

Dtdisc_t agNodedisc = {
    offsetof(Agnode_t, id),
    sizeof(int),
    -1,				/* link offset */
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    (Dtcompar_f) agcmpid,
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

Dtdisc_t agIndisc = {
    0,				/* pass whole object as key */
    0,
    -1,				/* link offset */
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    (Dtcompar_f) agcmpin,
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

Dtdisc_t agOutdisc = {
    0,				/* pass whole object as key */
    0,
    -1,				/* link offset */
    (Dtmake_f) 0,
    (Dtfree_f) 0,
    (Dtcompar_f) agcmpout,
    (Dthash_f) 0,
    (Dtmemory_f) 0,
    (Dtevent_f) 0
};

int agcmpid(Dt_t * dict, int *id0, int *id1, Dtdisc_t * disc)
{
    return (*id0) - *(id1);
}

#ifdef DEBUG
static int myinedgecmp(e0, e1)
Agedge_t *e0, *e1;
{
    int rv = myinedgecmp(e0, e1);
    printf("compare (%s,%s:%s),(%s,%s:%s) = %d\n",
	   e0->head ? e0->head->name : "nil",
	   e0->tail ? e0->tail->name : "nil",
	   e0->attr && e0->attr[KEYX]? e0->attr[KEYX] : "nil",
	   e1->head ? e1->head->name : "nil",
	   e1->tail ? e1->tail->name : "nil",
	   e1->attr && e1->attr[KEYX]? e1->attr[KEYX] : "nil",
	   rv);
    return rv;
}
#endif

static int keycmp(Agedge_t * e0, Agedge_t * e1)
{
    char *key0, *key1;
    key0 = e0->attr ? e0->attr[KEYX] : NULL;
    key1 = e1->attr ? e1->attr[KEYX] : NULL;
    if (key0 == NULL)
	return (key1 ? -1 : 0);
    if (key1 == NULL)
	return 1;
    return strcmp(key0, key1);
}

int agcmpin(Dict_t * d, Agedge_t * e0, Agedge_t * e1, Dtdisc_t * disc)
{
    int e0tailid, e0headid, e1tailid, e1headid;

    e0tailid = e0->tail ? e0->tail->id : -1;
    e0headid = e0->head ? e0->head->id : -1;
    e1tailid = e1->tail ? e1->tail->id : -1;
    e1headid = e1->head ? e1->head->id : -1;

    if (e0headid != e1headid)
	return e0headid - e1headid;
    if (e0tailid != e1tailid)
	return e0tailid - e1tailid;
    return keycmp(e0, e1);
}

int agcmpout(Dict_t * d, Agedge_t * e0, Agedge_t * e1, Dtdisc_t * disc)
{
    int e0tailid, e0headid, e1tailid, e1headid;

    e0tailid = e0->tail ? e0->tail->id : -1;
    e0headid = e0->head ? e0->head->id : -1;
    e1tailid = e1->tail ? e1->tail->id : -1;
    e1headid = e1->head ? e1->head->id : -1;

    if (e0tailid != e1tailid)
	return e0tailid - e1tailid;
    if (e0headid != e1headid)
	return e0headid - e1headid;
    return keycmp(e0, e1);
}

static Agdata_t *agnewdata(void)
{
    Agdata_t *rv;

    rv = NEW(Agdata_t);
    rv->node_dict = dtopen(&agNamedisc, Dttree);
    rv->globattr = agNEWdict("graph");
    rv->nodeattr = agNEWdict("node");
    rv->edgeattr = agNEWdict("edge");
    if (AG.proto_g) {
	agcopydict(rv->globattr, AG.proto_g->univ->globattr);
	agcopydict(rv->nodeattr, AG.proto_g->univ->nodeattr);
	agcopydict(rv->edgeattr, AG.proto_g->univ->edgeattr);
    }
    return rv;
}

static void agfreedata(Agraph_t * g)
{
    agFREEdict(g, g->univ->globattr);
    agFREEdict(g, g->univ->nodeattr);
    agFREEdict(g, g->univ->edgeattr);
    dtclose(g->univ->node_dict);
    free(g->univ);
}

static void dup_proto(Agraph_t * g, Agproto_t * proto)
{
    Agnode_t *n = NULL;
    Agedge_t *e = NULL;
    Agproto_t *s = NEW(Agproto_t);

    s->prev = g->proto;
    if (proto) {
	n = proto->n;
	e = proto->e;
    }
    s->n = agNEWnode(g, "\001proto", n);
    s->e = agNEWedge(g, s->n, s->n, e);
    g->proto = s;
}

void agpushproto(Agraph_t * g)
{
    dup_proto(g, g->proto);
}

void agpopproto(Agraph_t * g)
{
    Agproto_t *s = g->proto;
    if (s != NULL) {
	g->proto = s->prev;
	s->e->tail = s->e->head = s->n;
	agFREEedge(s->e);
	agFREEnode(s->n);
	free(s);
    }
}

static Agraph_t *agNEWgraph(char *name, Agraph_t * parent, int kind)
{
    int i, nobj;
    Agraph_t *g;

    if (AG.init_called == FALSE) {
	agerr(AGERR, "libag error -- aginit() was not called\n");
	return 0;
    }
    g = (Agraph_t *) calloc(1, AG.graph_nbytes);
    g->tag = TAG_GRAPH;
    g->kind = kind;
    g->nodes = dtopen(&agNodedisc, Dttree);
    g->inedges = dtopen(&agIndisc, Dttree);
    g->outedges = dtopen(&agOutdisc, Dttree);

    if (parent == NULL) {
	g->univ = agnewdata();
	g->root = g;
	nobj = dtsize(g->univ->globattr->dict);
	if (nobj) {
	    g->attr = N_NEW(nobj, char *);
		g->didset = N_NEW((nobj + CHAR_BIT - 1) / CHAR_BIT, char);
	}
	else {
	    g->attr = NULL;
		g->didset = NULL;
	}
	for (i = 0; i < nobj; i++)
	    g->attr[i] = agstrdup(AG.proto_g->attr[i]);
	} else {
	g->univ = parent->univ;
	g->root = parent->root;
	nobj = dtsize(parent->univ->globattr->dict);
	if (nobj) {
	    g->attr = N_NEW(nobj, char *);
		g->didset = N_NEW((nobj + CHAR_BIT - 1) / CHAR_BIT, char);
	}
	else {
	    g->attr = NULL;
		g->didset = NULL;
	}
	for (i = 0; i < nobj; i++)
	    g->attr[i] = agstrdup(parent->attr[i]);

    }

    g->meta_node = NULL;
    g->name = agstrdup(name);
    g->proto = NULL;

    if (parent)
	dup_proto(g, parent->proto);
    else
	agpushproto(g);
    return g;
}

static int reach0(Dict_t * m, Agnode_t * from, Agnode_t * to)
{
    Agedge_t *e;

    if (from == to)
	return TRUE;
    if (agfindedge(from->graph->root, from, to))
	return TRUE;
    dtinsert(m, from);
    for (e = agfstout(from->graph, from); e; e = agnxtout(from->graph, e))
	if ((dtsearch(m, e->head) == NULL) && reach0(m, e->head, to))
	    return TRUE;
    return FALSE;
}

static int reach(Agnode_t * from, Agnode_t * to)
{
    Dict_t *m;
    int rv;

    m = dtopen(&agNodedisc, Dttree);
    rv = reach0(m, from, to);
    dtclose(m);
    return rv;
}

Agraph_t *agusergraph(Agnode_t * n)
{
    return (n->graph->meta_node ? NULL : (Agraph_t *) (n->attr[0]));
}




Agraph_t *agopen(char *name, int kind)
{
    Agraph_t *g, *meta;

    g = agNEWgraph(name, NULL, kind);
    meta = agNEWgraph(name, NULL, AGMETAGRAPH);
    if (!g || !meta)
	return 0;
    agnodeattr(meta, "agusergraph", NULL);
    g->meta_node = agnode(meta, name);
    g->meta_node->attr[0] = (char *) g;
    return g;
}

Agraph_t *agsubg(Agraph_t * g, char *name)
{
    Agraph_t *subg, *meta;
    Agnode_t *n;

    meta = g->meta_node->graph;
    n = agfindnode(meta, name);
    if (n)
	subg = agusergraph(n);
    else {
	subg = agNEWgraph(name, g, g->kind);
	if (!subg)
	    return 0;
	n = agnode(meta, name);
	subg->meta_node = n;
	n->attr[0] = (char *) subg;
    }
    agINSgraph(g, subg);
    return subg;
}

Agraph_t *agfindsubg(Agraph_t * g, char *name)
{
    Agnode_t *n;

    if (g->meta_node) {
	n = agfindnode(g->meta_node->graph, name);
	if (n)
	    return agusergraph(n);
    }
    return NULL;
}

void agINSgraph(Agraph_t * g, Agraph_t * subg)
{
    Agnode_t *h, *t;
    t = g->meta_node;
    h = subg->meta_node;
    if (t && h && (reach(h, t) == FALSE))
	agedge(t->graph, t, h);
}

void agclose(Agraph_t * g)
{
    Agedge_t *e, *f;
    Agnode_t *n, *nn;
    Agraph_t *meta = NULL;
    int i, nobj, flag, is_meta;

    if ((g == NULL) || (TAG_OF(g) != TAG_GRAPH))
	return;
    is_meta = AG_IS_METAGRAPH(g);
    if (is_meta == FALSE) {
	meta = g->meta_node->graph;
	/* recursively remove its subgraphs */
	do {			/* better semantics would be to find strong component */
	    flag = FALSE;
	    for (e = agfstout(meta, g->meta_node); e; e = f) {
		f = agnxtout(meta, e);
		if (agnxtin(meta, agfstin(meta, e->head)) == NULL) {
		    agclose(agusergraph(e->head));
		    flag = TRUE;
		}
	    }
	} while (flag);
    }
    while (g->proto)
	agpopproto(g);
    if (is_meta == FALSE) {
	nobj = dtsize(g->univ->globattr->dict);
	for (i = 0; i < nobj; i++)
	    agstrfree(g->attr[i]);
    }
    if (g->attr)
	free(g->attr);
	if (g->didset)
		free(g->didset);
    if (g == g->root) {
	for (n = agfstnode(g); n; n = nn) {
	    nn = agnxtnode(g, n);
	    agDELnode(g, n);
	}
	if (is_meta == FALSE)
	    agclose(g->meta_node->graph);
	agfreedata(g);
    } else {
	if (is_meta == FALSE)
	    agdelete(meta, g->meta_node);
    }
    dtclose(g->nodes);
    dtclose(g->inedges);
    dtclose(g->outedges);
    agstrfree(g->name);
    TAG_OF(g) = -1;
    free(g);
}

int agcontains(Agraph_t * g, void *obj)
{
    switch (TAG_OF(obj)) {
    case TAG_NODE:
	return (agidnode(g, ((Agnode_t *) obj)->id) != NULL);
    case TAG_EDGE:
	return (dtsearch(g->inedges, (Agedge_t *) obj) != NULL);
    case TAG_GRAPH:
	return (reach(g->meta_node, ((Agraph_t *) obj)->meta_node));
    }
    return FALSE;
}

void aginsert(Agraph_t * g, void *obj)
{
    switch (TAG_OF(obj)) {
    case TAG_NODE:
	agINSnode(g, (Agnode_t*)obj);
	break;
    case TAG_EDGE:
	agINSedge(g, (Agedge_t*)obj);
	break;
    case TAG_GRAPH:
	agINSgraph(g, (Agraph_t*)obj);
	break;
    }
}

void agdelete(Agraph_t * g, void *obj)
{
    switch (TAG_OF(obj)) {
    case TAG_NODE:
	agDELnode(g, (Agnode_t*)obj);
	break;
    case TAG_EDGE:
	agDELedge(g, (Agedge_t*)obj);
	break;
    case TAG_GRAPH:
	agclose((Agraph_t*)obj);
	break;
    }
}

int agnnodes(Agraph_t * g)
{
    return dtsize(g->nodes);
}

int agnedges(Agraph_t * g)
{
    return dtsize(g->outedges);
}
