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

typedef struct IMapEntry_s {
    Dtlink_t namedict_link;
    Dtlink_t iddict_link;
    unsigned long id;
    char *str;
} IMapEntry_t;

static int idcmpf(Dict_t * d, void *arg_p0, void *arg_p1, Dtdisc_t * disc)
{
    IMapEntry_t *p0, *p1;

    NOTUSED(d);
    p0 = arg_p0;
    p1 = arg_p1;
    NOTUSED(disc);
    return (p0->id - p1->id);
}

/* note, OK to compare pointers into shared string pool 
 * but can't probe with an arbitrary string pointer
 */
static int namecmpf(Dict_t * d, void *arg_p0, void *arg_p1,
		    Dtdisc_t * disc)
{
    IMapEntry_t *p0, *p1;

    NOTUSED(d);
    p0 = arg_p0;
    p1 = arg_p1;
    NOTUSED(disc);
    return (p0->str - p1->str);
}

static Dtdisc_t LookupByName = {
    0,				/* object ptr is passed as key */
    0,				/* size (ignored) */
    offsetof(IMapEntry_t, namedict_link),
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    namecmpf,
    NIL(Dthash_f),
    agdictobjmem,
    NIL(Dtevent_f)
};

static Dtdisc_t LookupById = {
    0,				/* object ptr is passed as key */
    0,				/* size (ignored) */
    offsetof(IMapEntry_t, iddict_link),
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    idcmpf,
    NIL(Dthash_f),
    agdictobjmem,
    NIL(Dtevent_f)
};

int aginternalmaplookup(Agraph_t * g, int objtype, char *str,
			unsigned long *result)
{
    Dict_t *d;
    IMapEntry_t *sym, template;
    char *search_str;

    if (objtype == AGINEDGE)
	objtype = AGEDGE;
    if ((d = g->clos->lookup_by_name[objtype])) {
	if ((search_str = agstrbind(g, str))) {
	    template.str = search_str;
	    sym = (IMapEntry_t *) dtsearch(d, &template);
	    if (sym) {
		*result = sym->id;
		return TRUE;
	    }
	}
    }
    return FALSE;
}

/* caller GUARANTEES that this is a new entry */
void aginternalmapinsert(Agraph_t * g, int objtype, char *str,
			 unsigned long id)
{
    IMapEntry_t *ent;
    Dict_t *d_name_to_id, *d_id_to_name;

    ent = AGNEW(g, IMapEntry_t);
    ent->id = id;
    ent->str = agstrdup(g, str);

    if (objtype == AGINEDGE)
	objtype = AGEDGE;
    if ((d_name_to_id = g->clos->lookup_by_name[objtype]) == NIL(Dict_t *))
	d_name_to_id = g->clos->lookup_by_name[objtype] =
	    agdtopen(g, &LookupByName, Dttree);
    if ((d_id_to_name = g->clos->lookup_by_id[objtype]) == NIL(Dict_t *))
	d_id_to_name = g->clos->lookup_by_id[objtype] =
	    agdtopen(g, &LookupById, Dttree);
    dtinsert(d_name_to_id, ent);
    dtinsert(d_id_to_name, ent);
}

static IMapEntry_t *find_isym(Agraph_t * g, int objtype, unsigned long id)
{
    Dict_t *d;
    IMapEntry_t *isym, itemplate;

    if (objtype == AGINEDGE)
	objtype = AGEDGE;
    if ((d = g->clos->lookup_by_id[objtype])) {
	itemplate.id = id;
	isym = (IMapEntry_t *) dtsearch(d, &itemplate);
    } else
	isym = NIL(IMapEntry_t *);
    return isym;
}

char *aginternalmapprint(Agraph_t * g, int objtype, unsigned long id)
{
    IMapEntry_t *isym;

    if ((isym = find_isym(g, objtype, id)))
	return isym->str;
    return NILstr;
}


int aginternalmapdelete(Agraph_t * g, int objtype, unsigned long id)
{
    IMapEntry_t *isym;

    if (objtype == AGINEDGE)
	objtype = AGEDGE;
    if ((isym = find_isym(g, objtype, id))) {
	dtdelete(g->clos->lookup_by_name[objtype], isym);
	dtdelete(g->clos->lookup_by_id[objtype], isym);
	agstrfree(g, isym->str);
	agfree(g, isym);
	return TRUE;
    }
    return FALSE;
}

void aginternalmapclearlocalnames(Agraph_t * g)
{
    int i;
    IMapEntry_t *sym, *nxt;
    Dict_t **d_name;
    /* Dict_t **d_id; */

    Ag_G_global = g;
    d_name = g->clos->lookup_by_name;
    /* d_id = g->clos->lookup_by_id; */
    for (i = 0; i < 3; i++) {
	if (d_name[i]) {
	    for (sym = dtfirst(d_name[i]); sym; sym = nxt) {
		nxt = dtnext(d_name[i], sym);
		if (sym->str[0] == LOCALNAMEPREFIX)
		    aginternalmapdelete(g, i, sym->id);
	    }
	}
    }
}

static void closeit(Dict_t ** d)
{
    int i;

    for (i = 0; i < 3; i++) {
	if (d[i]) {
	    dtclose(d[i]);
	    d[i] = NIL(Dict_t *);
	}
    }
}

void aginternalmapclose(Agraph_t * g)
{
    Ag_G_global = g;
    closeit(g->clos->lookup_by_name);
    closeit(g->clos->lookup_by_id);
}
