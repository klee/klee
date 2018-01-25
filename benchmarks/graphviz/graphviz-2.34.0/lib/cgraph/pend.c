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

#include <cghdr.h>

static char DRName[] = "_AG_pending";

typedef struct symlist_s {
    Agsym_t *sym;
    struct symlist_s *link;
} symlist_t;

/* this record describes one pending callback on one object */
typedef struct {
    Dtlink_t link;
    unsigned long key;		/* universal key for main or sub-object */
    Agraph_t *g;
    Agobj_t *obj;
    symlist_t *symlist;		/* attributes involved */
} pending_cb_t;

typedef struct {
    Agrec_t h;
    struct {
	Dict_t *g, *n, *e;
    } ins, mod, del;
} pendingset_t;

static void free_symlist(pending_cb_t * pcb)
{
    symlist_t *s, *t;

    for (s = pcb->symlist; s; s = t) {
	t = s->link;
	agfree(pcb->g, s);
    }
}

static void freef(Dict_t * dict, void *ptr, Dtdisc_t * disc)
{
    pending_cb_t *pcb;

    NOTUSED(dict);
    NOTUSED(disc);
    pcb = ptr;
    free_symlist(pcb);
    agfree(pcb->g, pcb);
}

static Dtdisc_t Disc = {
    offsetof(pending_cb_t, key),	/* sort by 'key' */
    sizeof(unsigned long),
    0,				/* link offset */
    NIL(Dtmake_f),
    freef,
    NIL(Dtcompar_f),
    NIL(Dthash_f)
};

static Dict_t *dictof(pendingset_t * ds, Agobj_t * obj, int kind)
{
    Dict_t **dict_ref = NIL(Dict_t **);

    dict_ref = 0;
    switch (AGTYPE(obj)) {
    case AGRAPH:
	switch (kind) {
	case CB_INITIALIZE:
	    dict_ref = &(ds->ins.g);
	    break;
	case CB_UPDATE:
	    dict_ref = &(ds->mod.g);
	    break;
	case CB_DELETION:
	    dict_ref = &(ds->del.g);
	    break;
	default:
	    break;
	}
	break;
    case AGNODE:
	switch (kind) {
	case CB_INITIALIZE:
	    dict_ref = &(ds->ins.n);
	    break;
	case CB_UPDATE:
	    dict_ref = &(ds->mod.n);
	    break;
	case CB_DELETION:
	    dict_ref = &(ds->del.n);
	    break;
	default:
	    break;
	}
	break;
    case AGEDGE:
	switch (kind) {
	case CB_INITIALIZE:
	    dict_ref = &(ds->ins.e);
	    break;
	case CB_UPDATE:
	    dict_ref = &(ds->mod.e);
	    break;
	case CB_DELETION:
	    dict_ref = &(ds->del.e);
	    break;
	default:
	    break;
	}
	break;
    default:
	break;
    }

    if (dict_ref == 0)
	agerr(AGERR, "pend dictof a bad object");
    if (*dict_ref == NIL(Dict_t *))
	*dict_ref = agdtopen(agraphof(obj), &Disc, Dttree);
    return *dict_ref;
}

static unsigned long genkey(Agobj_t * obj)
{
    return obj->tag.id;
}

static pending_cb_t *lookup(Dict_t * dict, Agobj_t * obj)
{
    pending_cb_t key, *rv;

    key.key = genkey(obj);
    rv = (pending_cb_t *) dtsearch(dict, &key);
    return rv;
}

static void record_sym(Agobj_t * obj, pending_cb_t * handle,
		       Agsym_t * optsym)
{
    symlist_t *sym, *nsym, *psym;

    psym = NIL(symlist_t *);
    for (sym = handle->symlist; sym; psym = sym, sym = sym->link) {
	if (sym->sym == optsym)
	    break;
	if (sym == NIL(symlist_t *)) {
	    nsym = agalloc(agraphof(obj), sizeof(symlist_t));
	    nsym->sym = optsym;
	    if (psym)
		psym->link = nsym;
	    else
		handle->symlist = nsym;
	}
	/* else we already have a callback registered */
    }
}

static pending_cb_t *insert(Dict_t * dict, Agraph_t * g, Agobj_t * obj,
			    Agsym_t * optsym)
{
    pending_cb_t *handle;
    handle = agalloc(agraphof(obj), sizeof(pending_cb_t));
    handle->obj = obj;
    handle->key = genkey(obj);
    handle->g = g;
    if (optsym) {
	handle->symlist =
	    (symlist_t *) agalloc(handle->g, sizeof(symlist_t));
	handle->symlist->sym = optsym;
    }
    dtinsert(dict, handle);
    return handle;
}

static void purge(Dict_t * dict, Agobj_t * obj)
{
    pending_cb_t *handle;

    if ((handle = lookup(dict, obj))) {
	dtdelete(dict, handle);
    }
}

void agrecord_callback(Agraph_t * g, Agobj_t * obj, int kind,
		       Agsym_t * optsym)
{
    pendingset_t *pending;
    Dict_t *dict;
    pending_cb_t *handle;

    pending =
	(pendingset_t *) agbindrec(g, DRName, sizeof(pendingset_t), FALSE);

    switch (kind) {
    case CB_INITIALIZE:
	assert(lookup(dictof(pending, obj, CB_UPDATE), obj) == 0);
	assert(lookup(dictof(pending, obj, CB_DELETION), obj) == 0);
	dict = dictof(pending, obj, CB_INITIALIZE);
	handle = lookup(dict, obj);
	if (handle == 0)
	    handle = insert(dict, g, obj, optsym);
	break;
    case CB_UPDATE:
	if (lookup(dictof(pending, obj, CB_INITIALIZE), obj))
	    break;
	if (lookup(dictof(pending, obj, CB_DELETION), obj))
	    break;
	dict = dictof(pending, obj, CB_UPDATE);
	handle = lookup(dict, obj);
	if (handle == 0)
	    handle = insert(dict, g, obj, optsym);
	record_sym(obj, handle, optsym);
	break;
    case CB_DELETION:
	purge(dictof(pending, obj, CB_INITIALIZE), obj);
	purge(dictof(pending, obj, CB_UPDATE), obj);
	dict = dictof(pending, obj, CB_DELETION);
	handle = lookup(dict, obj);
	if (handle == 0)
	    handle = insert(dict, g, obj, optsym);
	break;
    default:
	agerr(AGERR,"agrecord_callback of a bad object");
    }
}

static void cb(Dict_t * dict, int callback_kind)
{
    pending_cb_t *pcb;
    Agraph_t *g;
    symlist_t *psym;
    Agcbstack_t *stack;

    if (dict)
	while ((pcb = (pending_cb_t *) dtfirst(dict))) {
	    g = pcb->g;
	    stack = g->clos->cb;
	    switch (callback_kind) {
	    case CB_INITIALIZE:
		aginitcb(g, pcb->obj, stack);
		break;
	    case CB_UPDATE:
		for (psym = pcb->symlist; psym; psym = psym->link)
		    agupdcb(g, pcb->obj, psym->sym, stack);
		break;
	    case CB_DELETION:
		agdelcb(g, pcb->obj, stack);
		break;
	    }
	    dtdelete(dict, pcb);
	}
}

static void agrelease_callbacks(Agraph_t * g)
{
    pendingset_t *pending;
    if (NOT(g->clos->callbacks_enabled)) {
	g->clos->callbacks_enabled = TRUE;
	pending =
	    (pendingset_t *) agbindrec(g, DRName, sizeof(pendingset_t),
				       FALSE);
	/* this destroys objects in the opposite of their order of creation */
	cb(pending->ins.g, CB_INITIALIZE);
	cb(pending->ins.n, CB_INITIALIZE);
	cb(pending->ins.e, CB_INITIALIZE);

	cb(pending->mod.g, CB_UPDATE);
	cb(pending->mod.n, CB_UPDATE);
	cb(pending->mod.e, CB_UPDATE);

	cb(pending->del.e, CB_DELETION);
	cb(pending->del.n, CB_DELETION);
	cb(pending->del.g, CB_DELETION);
    }
}

int agcallbacks(Agraph_t * g, int flag)
{
    if (flag && NOT(g->clos->callbacks_enabled))
	agrelease_callbacks(g);
    if (g->clos->callbacks_enabled) {
	g->clos->callbacks_enabled = flag;
	return TRUE;
    }
    g->clos->callbacks_enabled = flag;
    return FALSE;
}
