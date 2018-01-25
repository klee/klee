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

/*
 * dynamic attributes
 */

/* to create a graph's data dictionary */

#define MINATTR	4		/* minimum allocation */

static void freesym(Dict_t * d, Void_t * obj, Dtdisc_t * disc);

Dtdisc_t AgDataDictDisc = {
    (int) offsetof(Agsym_t, name),	/* use symbol name as key */
    -1,
    (int) offsetof(Agsym_t, link),
    NIL(Dtmake_f),
    freesym,
    NIL(Dtcompar_f),
    NIL(Dthash_f)
};

static char DataDictName[] = "_AG_datadict";
static void init_all_attrs(Agraph_t * g);
static Agdesc_t ProtoDesc = { 1, 0, 1, 0, 1, 1 };
static Agraph_t *ProtoGraph;

Agdatadict_t *agdatadict(Agraph_t * g, int cflag)
{
    Agdatadict_t *rv;
    rv = (Agdatadict_t *) aggetrec(g, DataDictName, FALSE);
    if (rv || !cflag)
	return rv;
    init_all_attrs(g);
    rv = (Agdatadict_t *) aggetrec(g, DataDictName, FALSE);
    return rv;
}

Dict_t *agdictof(Agraph_t * g, int kind)
{
    Agdatadict_t *dd;
    Dict_t *dict;

    dd = agdatadict(g, FALSE);
    if (dd)
	switch (kind) {
	case AGRAPH:
	    dict = dd->dict.g;
	    break;
	case AGNODE:
	    dict = dd->dict.n;
	    break;
	case AGINEDGE:
	case AGOUTEDGE:
	    dict = dd->dict.e;
	    break;
	default:
	    agerr(AGERR,"agdictof: unknown kind %d\n", kind);
	    dict = NIL(Dict_t *);
	    break;
    } else
	dict = NIL(Dict_t *);
    return dict;
}

Agsym_t *agnewsym(Agraph_t * g, char *name, char *value, int id, int kind)
{
    Agsym_t *sym;
    sym = agalloc(g, sizeof(Agsym_t));
    sym->kind = kind;
    sym->name = agstrdup(g, name);
    sym->defval = agstrdup(g, value);
    sym->id = id;
    return sym;
}

static void agcopydict(Dict_t * src, Dict_t * dest, Agraph_t * g, int kind)
{
    Agsym_t *sym, *newsym;

    assert(dtsize(dest) == 0);
    for (sym = (Agsym_t *) dtfirst(src); sym;
	 sym = (Agsym_t *) dtnext(src, sym)) {
	newsym = agnewsym(g, sym->name, sym->defval, sym->id, kind);
	newsym->print = sym->print;
	newsym->fixed = sym->fixed;
	dtinsert(dest, newsym);
    }
}

static Agdatadict_t *agmakedatadict(Agraph_t * g)
{
    Agraph_t *par;
    Agdatadict_t *parent_dd, *dd;

    dd = (Agdatadict_t *) agbindrec(g, DataDictName, sizeof(Agdatadict_t),
				    FALSE);
    dd->dict.n = agdtopen(g, &AgDataDictDisc, Dttree);
    dd->dict.e = agdtopen(g, &AgDataDictDisc, Dttree);
    dd->dict.g = agdtopen(g, &AgDataDictDisc, Dttree);
    if ((par = agparent(g))) {
	parent_dd = agdatadict(par, FALSE);
	assert(dd != parent_dd);
	dtview(dd->dict.n, parent_dd->dict.n);
	dtview(dd->dict.e, parent_dd->dict.e);
	dtview(dd->dict.g, parent_dd->dict.g);
    } else {
	if (ProtoGraph && (g != ProtoGraph)) {
	    /* it's not ok to dtview here for several reasons. the proto
	       graph could change, and the sym indices don't match */
	    parent_dd = agdatadict(ProtoGraph, FALSE);
	    agcopydict(parent_dd->dict.n, dd->dict.n, g, AGNODE);
	    agcopydict(parent_dd->dict.e, dd->dict.e, g, AGEDGE);
	    agcopydict(parent_dd->dict.g, dd->dict.g, g, AGRAPH);
	}
    }
    return dd;
}

/* look up an attribute with possible viewpathing */
Agsym_t *agdictsym(Dict_t * dict, char *name)
{
    Agsym_t key;
    key.name = (char *) name;
    return (Agsym_t *) dtsearch(dict, &key);
}

/* look up attribute in local dictionary with no view pathing */
Agsym_t *aglocaldictsym(Dict_t * dict, char *name)
{
    Agsym_t *rv;
    Dict_t *view;

    view = dtview(dict, NIL(Dict_t *));
    rv = agdictsym(dict, name);
    dtview(dict, view);
    return rv;
}

Agsym_t *agattrsym(void *obj, char *name)
{
    Agattr_t *data;
    Agsym_t *rv;
    char *arg = name;

    data = agattrrec((Agobj_t *) obj);
    if (data)
	rv = agdictsym(data->dict, arg);
    else
	rv = NILsym;
    return rv;
}

/* to create a graph's, node's edge's string attributes */

char *AgDataRecName = "_AG_strdata";

static int topdictsize(Agobj_t * obj)
{
    Dict_t *d;

    d = agdictof(agroot(agraphof(obj)), AGTYPE(obj));
    return d ? dtsize(d) : 0;
}

/* g can be either the enclosing graph, or ProtoGraph */
static Agrec_t *agmakeattrs(Agraph_t * context, void *obj)
{
    int sz;
    Agattr_t *rec;
    Agsym_t *sym;
    Dict_t *datadict;

    rec = agbindrec(obj, AgDataRecName, sizeof(Agattr_t), FALSE);
    datadict = agdictof(context, AGTYPE(obj));
    assert(datadict);
    if (rec->dict == NIL(Dict_t *)) {
	rec->dict = agdictof(agroot(context), AGTYPE(obj));
	/* don't malloc(0) */
	sz = topdictsize(obj);
	if (sz < MINATTR)
	    sz = MINATTR;
	rec->str = agalloc(agraphof(obj), sz * sizeof(char *));
	/* doesn't call agxset() so no obj-modified callbacks occur */
	for (sym = (Agsym_t *) dtfirst(datadict); sym;
	     sym = (Agsym_t *) dtnext(datadict, sym))
	    rec->str[sym->id] = agstrdup(agraphof(obj), sym->defval);
    } else {
	assert(rec->dict == datadict);
    }
    return (Agrec_t *) rec;
}

static void freeattr(Agobj_t * obj, Agattr_t * attr)
{
    int i, sz;
    Agraph_t *g;

    g = agraphof(obj);
    sz = topdictsize(obj);
    for (i = 0; i < sz; i++)
	agstrfree(g, attr->str[i]);
    agfree(g, attr->str);
}

static void freesym(Dict_t * d, Void_t * obj, Dtdisc_t * disc)
{
    Agsym_t *sym;

    NOTUSED(d);
    sym = (Agsym_t *) obj;
    NOTUSED(disc);
    agstrfree(Ag_G_global, sym->name);
    agstrfree(Ag_G_global, sym->defval);
    agfree(Ag_G_global, sym);
}

Agattr_t *agattrrec(void *obj)
{
    return (Agattr_t *) aggetrec(obj, AgDataRecName, FALSE);
}


static void addattr(Agraph_t * g, Agobj_t * obj, Agsym_t * sym)
{
    Agattr_t *attr;

    attr = (Agattr_t *) agattrrec(obj);
    assert(attr != NIL(Agattr_t *));
    if (sym->id >= MINATTR)
	attr->str = (char **) AGDISC(g, mem)->resize(AGCLOS(g, mem),
						     attr->str,
						     sym->id *
						     sizeof(char *),
						     (sym->id +
						      1) * sizeof(char *));
    attr->str[sym->id] = agstrdup(g, sym->defval);
    /* agmethod_upd(g,obj,sym);  JCE and GN didn't like this. */
}


static Agsym_t *setattr(Agraph_t * g, int kind, char *name, char *value)
{
    Agdatadict_t *dd;
    Dict_t *ldict, *rdict;
    Agsym_t *lsym, *psym, *rsym, *rv;
    Agraph_t *root;
    Agnode_t *n;
    Agedge_t *e;

    assert(value);
    root = agroot(g);
    dd = agdatadict(g, TRUE);	/* force initialization of string attributes */
    ldict = agdictof(g, kind);
    lsym = aglocaldictsym(ldict, name);
    if (lsym) {			/* update old local definiton */
	agstrfree(g, lsym->defval);
	lsym->defval = agstrdup(g, value);
	rv = lsym;
    } else {
	psym = agdictsym(ldict, name);	/* search with viewpath up to root */
	if (psym) {		/* new local definition */
	    lsym = agnewsym(g, name, value, psym->id, kind);
	    dtinsert(ldict, lsym);
	    rv = lsym;
	} else {		/* new global definition */
	    rdict = agdictof(root, kind);
	    rsym = agnewsym(g, name, value, dtsize(rdict), kind);
	    dtinsert(rdict, rsym);
	    switch (kind) {
	    case AGRAPH:
		agapply(root, (Agobj_t *) root, (agobjfn_t) addattr,
			rsym, TRUE);
		break;
	    case AGNODE:
		for (n = agfstnode(root); n; n = agnxtnode(root, n))
		    addattr(g, (Agobj_t *) n, rsym);
		break;
	    case AGINEDGE:
	    case AGOUTEDGE:
		for (n = agfstnode(root); n; n = agnxtnode(root, n))
		    for (e = agfstout(root, n); e; e = agnxtout(root, e))
			addattr(g, (Agobj_t *) e, rsym);
		break;
	    }
	    rv = rsym;
	}
    }
    if (rv && (kind == AGRAPH))
	agxset(g, rv, value);
    agmethod_upd(g, g, rv);	/* JCE and GN wanted this */
    return rv;
}

static Agsym_t *getattr(Agraph_t * g, int kind, char *name)
{
    Agsym_t *rv = 0;
    Dict_t *dict;
    dict = agdictof(g, kind);
    if (dict)
	rv = agdictsym(dict, name);	/* viewpath up to root */
    return rv;
}

/*
 * create or update an existing attribute and return its descriptor.
 * if the new value is NIL(char*), this is only a search, no update.
 * when a new attribute is created, existing graphs/nodes/edges
 * receive its default value.
 */
Agsym_t *agattr(Agraph_t * g, int kind, char *name, char *value)
{
    Agsym_t *rv;

    if (g == 0) {
	if (ProtoGraph == 0)
	    ProtoGraph = agopen(0, ProtoDesc, 0);
	g = ProtoGraph;
    }
    if (value)
	rv = setattr(g, kind, name, value);
    else
	rv = getattr(g, kind, name);
    return rv;
}

Agsym_t *agnxtattr(Agraph_t * g, int kind, Agsym_t * attr)
{
    Dict_t *d;
    Agsym_t *rv;

    if ((d = agdictof(g, kind))) {
	if (attr)
	    rv = (Agsym_t *) dtnext(d, attr);
	else
	    rv = (Agsym_t *) dtfirst(d);
    } else
	rv = 0;
    return rv;
}

/* Create or delete attributes associated with an object */

void agraphattr_init(Agraph_t * g)
{
    /* Agdatadict_t *dd; */
    /* Agrec_t                      *attr; */
    Agraph_t *context;

    g->desc.has_attrs = 1;
    /* dd = */ agmakedatadict(g);
    if (!(context = agparent(g)))
	context = g;
    /* attr = */ agmakeattrs(context, g);
}

int agraphattr_delete(Agraph_t * g)
{
    Agdatadict_t *dd;
    Agattr_t *attr;

    Ag_G_global = g;
    if ((attr = agattrrec(g))) {
	freeattr((Agobj_t *) g, attr);
	agdelrec(g, attr->h.name);
    }

    if ((dd = agdatadict(g, FALSE))) {
	if (agdtclose(g, dd->dict.n)) return 1;
	if (agdtclose(g, dd->dict.e)) return 1;
	if (agdtclose(g, dd->dict.g)) return 1;
	agdelrec(g, dd->h.name);
    }
    return 0;
}

void agnodeattr_init(Agraph_t * g, Agnode_t * n)
{
    Agattr_t *data;

    data = agattrrec(n);
    if ((!data) || (!data->dict))
	(void) agmakeattrs(g, n);
}

void agnodeattr_delete(Agnode_t * n)
{
    Agattr_t *rec;

    if ((rec = agattrrec(n))) {
	freeattr((Agobj_t *) n, rec);
	agdelrec(n, AgDataRecName);
    }
}

void agedgeattr_init(Agraph_t * g, Agedge_t * e)
{
    Agattr_t *data;

    data = agattrrec(e);
    if ((!data) || (!data->dict))
	(void) agmakeattrs(g, e);
}

void agedgeattr_delete(Agedge_t * e)
{
    Agattr_t *rec;

    if ((rec = agattrrec(e))) {
	freeattr((Agobj_t *) e, rec);
	agdelrec(e, AgDataRecName);
    }
}

char *agget(void *obj, char *name)
{
    Agsym_t *sym;
    Agattr_t *data;
    char *rv;

    sym = agattrsym(obj, name);
    if (sym == NILsym)
	rv = 0;			/* note was "", but this provides more info */
    else {
	data = agattrrec((Agobj_t *) obj);
	rv = (char *) (data->str[sym->id]);
    }
    return rv;
}

char *agxget(void *obj, Agsym_t * sym)
{
    Agattr_t *data;
    char *rv;

    data = agattrrec((Agobj_t *) obj);
    assert((sym->id >= 0) && (sym->id < topdictsize(obj)));
    rv = (char *) (data->str[sym->id]);
    return rv;
}

int agset(void *obj, char *name, char *value)
{
    Agsym_t *sym;
    int rv;

    sym = agattrsym(obj, name);
    if (sym == NILsym)
	rv = FAILURE;
    else
	rv = agxset(obj, sym, value);
    return rv;
}

int agxset(void *obj, Agsym_t * sym, char *value)
{
    Agraph_t *g;
    Agobj_t *hdr;
    Agattr_t *data;
    Agsym_t *lsym;

    g = agraphof(obj);
    hdr = (Agobj_t *) obj;
    data = agattrrec(hdr);
    assert((sym->id >= 0) && (sym->id < topdictsize(obj)));
    agstrfree(g, data->str[sym->id]);
    data->str[sym->id] = agstrdup(g, value);
    if (hdr->tag.objtype == AGRAPH) {
	/* also update dict default */
	Dict_t *dict;
	dict = agdatadict(g, FALSE)->dict.g;
	if ((lsym = aglocaldictsym(dict, sym->name))) {
	    agstrfree(g, lsym->defval);
	    lsym->defval = agstrdup(g, value);
	} else {
	    lsym = agnewsym(g, sym->name, value, sym->id, AGTYPE(hdr));
	    dtinsert(dict, lsym);
	}
    }
    agmethod_upd(g, obj, sym);
    return SUCCESS;
}

int agsafeset(void *obj, char *name, char *value, char *def)
{
    Agsym_t *a;

    a = agattr(agraphof(obj), AGTYPE(obj), name, 0);
    if (!a)
	a = agattr(agraphof(obj), AGTYPE(obj), name, def);
    return agxset(obj, a, value);
}


/*
 * attach attributes to the already created graph objs.
 * presumably they were already initialized, so we don't invoke
 * any of the old methods.
 */
static void init_all_attrs(Agraph_t * g)
{
    Agraph_t *root;
    Agnode_t *n;
    Agedge_t *e;

    root = agroot(g);
    agapply(root, (Agobj_t *) root, (agobjfn_t) agraphattr_init,
	    NIL(Agdisc_t *), TRUE);
    for (n = agfstnode(root); n; n = agnxtnode(root, n)) {
	agnodeattr_init(g, n);
	for (e = agfstout(root, n); e; e = agnxtout(root, e)) {
	    agedgeattr_init(g, e);
	}
    }
}

/* agcopyattr:
 * Assumes attributes have already been declared.
 * Do not copy key attribute for edges, as this must be distinct.
 * Returns non-zero on failure or if objects have different type.
 */
int agcopyattr(void *oldobj, void *newobj)
{
    Agraph_t *g;
    Agsym_t *sym;
    Agsym_t *newsym;
    char* val;
    char* nval;
    int r = 1;

    g = agraphof(oldobj);
    if (AGTYPE(oldobj) != AGTYPE(newobj))
	return 1;
    sym = 0;
    while ((sym = agnxtattr(g, AGTYPE(oldobj), sym))) {
	newsym = agattrsym(newobj, sym->name);
	if (!newsym)
	    return 1;
	val = agxget(oldobj, sym);
	r = agxset(newobj, newsym, val);
	/* FIX(?): Each graph has its own string cache, so a whole new refstr is possibly
	 * allocated. If the original was an html string, make sure the new one is as well.
	 * If cgraph goes to single string table, this can be removed.
	 */
	if (aghtmlstr (val)) {
	    nval = agxget (newobj, newsym);
	    agmarkhtmlstr (nval);
	}
    }
    return r;
}
