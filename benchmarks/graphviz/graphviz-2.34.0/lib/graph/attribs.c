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

#define EXTERN
#include "libgraph.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

Agdict_t *agdictof(void *obj)
{
    Agdict_t *d = NULL;

    switch (TAG_OF(obj)) {
    case TAG_GRAPH:
	d = ((Agraph_t *) obj)->univ->globattr;
	break;
    case TAG_NODE:
	d = ((Agnode_t *) obj)->graph->univ->nodeattr;
	break;
    case TAG_EDGE:
	d = ((Agedge_t *) obj)->tail->graph->univ->edgeattr;
	break;
    }
    return d;
}

Agsym_t *agNEWsym(Agdict_t * dict, char *name, char *value)
{
    Agsym_t *a;
    int i;

    a = NEW(Agsym_t);
    a->name = agstrdup(name);
    a->value = agstrdup(value);
    a->printed = TRUE;
    i = a->index = dtsize(dict->dict);
    dict->list = ALLOC(i + 2, dict->list, Agsym_t *);
    dict->list[i++] = a;
    dict->list[i++] = NULL;
    dtinsert(dict->dict, a);
    return a;
}

static void obj_init_attr(void *obj, Agsym_t * attr, int isnew)
{
    int i;
    Agraph_t *gobj;		/* generic object */

    gobj = (Agraph_t *) obj;
    i = attr->index;
	if (isnew) {
		gobj->attr = ALLOC(i + 1, gobj->attr, char *);
		gobj->attr[i] = agstrdup(attr->value);
		if (i % CHAR_BIT == 0) {
			/* allocate in chunks of CHAR_BIT bits */
			gobj->didset = ALLOC(i / CHAR_BIT + 1, gobj->didset, char);
			gobj->didset[i / CHAR_BIT] = 0;
		}
	}
	else if ((gobj->didset[i / CHAR_BIT] & (1 << (i % CHAR_BIT))) == 0) {
		/* the i-th attr was not set by agxset, so we can replace it */
		agstrfree(gobj->attr[i]);
		gobj->attr[i] = agstrdup(attr->value);
	}
}

static void add_graph_attr(Agraph_t * g, Agsym_t * attr, int isnew)
{
    Agnode_t *n;

    if (g->meta_node) {
	for (n = agfstnode(g->meta_node->graph); n;
	     n = agnxtnode(g->meta_node->graph, n))
	    obj_init_attr(agusergraph(n), attr, isnew);
    } else
	obj_init_attr(g, attr, isnew);
}

static void add_node_attr(Agraph_t * g, Agsym_t * attr, int isnew)
{
    Agnode_t *n;
    Agproto_t *proto;

    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	obj_init_attr(n, attr, isnew);
    if (g->meta_node) {
	for (n = agfstnode(g->meta_node->graph); n;
	     n = agnxtnode(g->meta_node->graph, n))
	    for (proto = agusergraph(n)->proto; proto; proto = proto->prev)
		obj_init_attr(proto->n, attr, isnew);
    } else
	for (proto = g->proto; proto; proto = proto->prev)
	    obj_init_attr(proto->n, attr, isnew);
}

static void add_edge_attr(Agraph_t * g, Agsym_t * attr, int isnew)
{
    Agnode_t *n;
    Agedge_t *e;
    Agproto_t *proto;

    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	for (e = agfstout(g, n); e; e = agnxtout(g, e))
	    obj_init_attr(e, attr, isnew);
    if (g->meta_node) {
	for (n = agfstnode(g->meta_node->graph); n;
	     n = agnxtnode(g->meta_node->graph, n))
	    for (proto = agusergraph(n)->proto; proto; proto = proto->prev)
		obj_init_attr(proto->e, attr, isnew);
    } else
	for (proto = g->proto; proto; proto = proto->prev)
	    obj_init_attr(proto->e, attr, isnew);
}

Agsym_t *agattr(void *obj, char *name, char *value)
{
    Agsym_t *rv;
    int isnew = 1;

    rv = agfindattr(obj, name);
    if (rv) {
	if (strcmp(rv->value, value)) {
	    agstrfree(rv->value);
	    rv->value = agstrdup(value);
	    isnew = 0;
	}
	else
	    return rv;
    }
    else
	rv = agNEWsym(agdictof(obj), name, value);
    if (rv) {
	switch (TAG_OF(obj)) {
	case TAG_GRAPH:
	    add_graph_attr((Agraph_t *) obj, rv, isnew);
	    break;
	case TAG_NODE:
	    add_node_attr(((Agnode_t *) obj)->graph, rv, isnew);
	    break;
	case TAG_EDGE:
	    add_edge_attr(((Agedge_t *) obj)->head->graph, rv, isnew);
	    break;
	}
    }
    return rv;
}

Agraph_t *agprotograph()
{
    return AG.proto_g;
}

Agnode_t *agprotonode(Agraph_t *g)
{
	return g->proto->n;
}


Agedge_t *agprotoedge(Agraph_t *g)
{
	return g->proto->e;
}


static int initproto(void)
{
    Agsym_t *a;
    Agraph_t *g;
    g = AG.proto_g = agopen("ProtoGraph", AGRAPH);
    a = agattr(g->proto->e, KEY_ID, "");
    if (a->index != KEYX)
	return 1;
    a = agattr(g->proto->e, TAIL_ID, "");
    if (a->index != TAILX)
	return 1;
    a->printed = FALSE;
    a = agattr(g->proto->e, HEAD_ID, "");
    if (a->index != HEADX)
	return 1;
    a->printed = FALSE;
    return 0;
}

Agsym_t *agraphattr(Agraph_t * g, char *name, char *value)
{
    if (g == NULL)
	g = AG.proto_g;
    if (g != g->root)
	return NULL;
    return agattr(g, name, value);
}

Agsym_t *agnodeattr(Agraph_t * g, char *name, char *value)
{
    if (g == NULL)
	g = AG.proto_g;
    if (g != g->root)
	return NULL;
    return agattr(g->proto->n, name, value);
}

Agsym_t *agedgeattr(Agraph_t * g, char *name, char *value)
{
    if (g == NULL)
	g = AG.proto_g;
    if (g != g->root)
	return NULL;
    return agattr(g->proto->e, name, value);
}

/* attribute dictionaries */

static void agfreesym(void *ptr)
{
    Agsym_t *a;
    a = (Agsym_t *) ptr;
    agstrfree(a->name);
    agstrfree(a->value);
    free(a);
}

void agFREEdict(Agraph_t * g, Agdict_t * dict)
{
    int i;
    Agsym_t *a;

    g = g;
    dtclose(dict->dict);
    if (dict->list) {
	i = 0;
	while ((a = dict->list[i++]))
	    agfreesym(a);
	free(dict->list);
    }
    free(dict);
}

Agdict_t *agNEWdict(char *name)
{
    Agdict_t *dict;
    static Dtdisc_t symdisc = {
	offsetof(Agsym_t, name),	/* key */
	-1,			/* size */
	-1,			/* link */
	(Dtmake_f) 0,
	(Dtfree_f) 0,
	(Dtcompar_f) 0,		/* use strcmp */
	(Dthash_f) 0,
	(Dtmemory_f) 0,
	(Dtevent_f) 0
    };

    dict = NEW(Agdict_t);
    dict->name = name;
    dict->dict = dtopen(&symdisc, Dttree);
    dict->list = NULL;
    return dict;
}

void agcopydict(Agdict_t * to_dict, Agdict_t * from_dict)
{
    int i, n;
    Agsym_t *a, *b;

    n = dtsize(from_dict->dict);
    for (i = 0; i < n; i++) {
	a = from_dict->list[i];
	b = agNEWsym(to_dict, a->name, a->value);
	b->printed = a->printed;
	b->fixed = a->fixed;
#ifdef WIN32
	/* Microsoft C is a thing of wonder. */
	fprintf(stderr, "", a->name, a->value);
#endif
    }
}

Agsym_t *agfindattr(void *obj, char *name)
{
    Agsym_t *rv;
    Agdict_t *dict = agdictof(obj);

    rv = (Agsym_t *) dtmatch(dict->dict, name);
    return rv;
}

Agsym_t *agfstattr(void *obj)
{
	Agdict_t *dict = agdictof(obj);
	return (Agsym_t *)dtfirst(dict->dict);
}

Agsym_t *agnxtattr(void *obj, Agsym_t *a)
{
	Agdict_t *dict = agdictof(obj);
	return (Agsym_t *)dtnext(dict->dict, a);
}

Agsym_t *aglstattr(void *obj)
{
	Agdict_t *dict = agdictof(obj);
	return (Agsym_t *)dtlast(dict->dict);
}

Agsym_t *agprvattr(void *obj, Agsym_t *a)
{
	Agdict_t *dict = agdictof(obj);
	return (Agsym_t *)dtprev(dict->dict, a);
}

	/* this is normally called by the aginit() macro */
int aginitlib(int gs, int ns, int es)
{
    int rv = 0;
    if (AG.proto_g == NULL) {
	AG.graph_nbytes = gs;
	AG.node_nbytes = ns;
	AG.edge_nbytes = es;
	AG.init_called = TRUE;
	if (initproto()) {
	    agerr(AGERR, "aginitlib: initproto failed\n");
	    rv = 1;
	}
    } else
	if ((AG.graph_nbytes != gs) || (AG.node_nbytes != ns)
	    || (AG.edge_nbytes != es))
	agerr(AGWARN, "aginit() called multiply with inconsistent args\n");
    return rv;
}

char *agget(void *obj, char *attr)
{
    return agxget(obj, agindex(obj, attr));
}

int agset(void *obj, char *attr, char *value)
{
    return agxset(obj, agindex(obj, attr), value);
}

int agindex(void *obj, char *name)
{
    Agsym_t *a;
    int rv = -1;

    a = agfindattr(obj, name);
    if (a)
	rv = a->index;
    return rv;
}

char *agxget(void *obj, int index)
{
    if (index >= 0)
	return ((Agraph_t *) obj)->attr[index];
    return NULL;
}

int agxset(void *obj, int index, char *buf)
{
	char **p;
    if (index >= 0) {
	Agraph_t *gobj = (Agraph_t *)obj;
	p = gobj->attr;
	agstrfree(p[index]);
	p[index] = agstrdup(buf);
	/* the index-th attr was set by agxset */
	gobj->didset[index / CHAR_BIT] |= 1 << (index % CHAR_BIT);
	return 0;
    } else
	return -1;
}

int agsafeset(void* obj, char* name, char* value, char* def)
{
    Agsym_t* a = agfindattr(obj, name);

    if (a == NULL) {
	if (!def) def = "";
	switch (TAG_OF(obj)) {
	case TAG_GRAPH:
	    a = agraphattr(((Agraph_t*)obj)->root, name, def);
	    break;
	case TAG_NODE:
	    a = agnodeattr(((Agnode_t*)obj)->graph, name, def);
	    break;
	case TAG_EDGE:
	    a = agedgeattr(((Agedge_t*)obj)->head->graph, name, def);
	    break;
	}
    }
    return agxset(obj, a->index, value);
}

/* agcopyattr:
 * Assumes attributes have already been declared.
 * Do not copy key attribute for edges, as this must be distinct.
 * Returns non-zero on failure or if objects have different type.
 */
int agcopyattr(void *oldobj, void *newobj)
{
    Agdict_t *d = agdictof(oldobj);
    Agsym_t **list = d->list;
    Agsym_t *sym;
    Agsym_t *newsym;
    int r = 0;
    int isEdge = (TAG_OF(oldobj) == TAG_EDGE);

    if (TAG_OF(oldobj) != TAG_OF(newobj)) return 1;
    while (!r && (sym = *list++)) {
	if (isEdge && sym->index == KEYX) continue;
        newsym = agfindattr(newobj,sym->name);
	if (!newsym) return 1;
	r = agxset(newobj, newsym->index, agxget(oldobj, sym->index));
    }
    return r;
}

