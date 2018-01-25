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
 * run time records
 */

static void set_data(Agobj_t * obj, Agrec_t * data, int mtflock)
{
    Agedge_t *e;

    obj->data = data;
    obj->tag.mtflock = mtflock;
    if ((AGTYPE(obj) == AGINEDGE) || (AGTYPE(obj) == AGOUTEDGE)) {
	e = agopp((Agedge_t *) obj);
	AGDATA(e) = data;
	e->base.tag.mtflock = mtflock;
    }
}

/* find record in circular list and do optional move-to-front */
Agrec_t *aggetrec(void *obj, char *name, int mtf)
{
    Agobj_t *hdr;
    Agrec_t *d, *first;

    hdr = (Agobj_t *) obj;
    first = d = hdr->data;
    while (d) {
	if ((d->name == name) || streq(name, d->name))
	    break;
	d = d->next;
	if (d == first) {
	    d = NIL(Agrec_t *);
	    break;
	}
    }
    if (d) {
	if (hdr->tag.mtflock) {
	    if (mtf && (hdr->data != d))
		agerr(AGERR, "move to front lock inconsistency");
	} else {
	    if ((d != first) || (mtf != hdr->tag.mtflock))
		set_data(hdr, d, mtf);	/* Always optimize */
	}
    }
    return d;
}

/* insert the record in data list of this object (only) */
static void objputrec(Agraph_t * g, Agobj_t * obj, void *arg)
{
    Agrec_t *firstrec, *newrec;

    newrec = arg;
    firstrec = obj->data;
    if (firstrec == NIL(Agrec_t *))
	newrec->next = newrec;	/* 0 elts */
    else {
	if (firstrec->next == firstrec) {
	    firstrec->next = newrec;	/* 1 elt */
	    newrec->next = firstrec;
	} else {
	    newrec->next = firstrec->next;
	    firstrec->next = newrec;
	}
    }
    if (NOT(obj->tag.mtflock))
	set_data(obj, newrec, FALSE);
}

/* attach a new record of the given size to the object.
 */
void *agbindrec(void *arg_obj, char *recname, unsigned int recsize,
		int mtf)
{
    Agraph_t *g;
    Agobj_t *obj;
    Agrec_t *rec;

    obj = (Agobj_t *) arg_obj;
    g = agraphof(obj);
    rec = aggetrec(obj, recname, FALSE);
    if ((rec == NIL(Agrec_t *)) && (recsize > 0)) {
	rec = (Agrec_t *) agalloc(g, (int) recsize);
	rec->name = agstrdup(g, recname);
	switch (obj->tag.objtype) {
	case AGRAPH:
	    objputrec(g, obj, rec);
	    break;
	case AGNODE:
	    objputrec(g, obj, rec);
	    break;
	case AGINEDGE:
	case AGOUTEDGE:
	    objputrec(g, obj, rec);
	    break;
	}
    }
    if (mtf)
	aggetrec(arg_obj, recname, TRUE);
    return (void *) rec;
}


/* if obj points to rec, move its data pointer. break any mtf lock(?) */
static void objdelrec(Agraph_t * g, Agobj_t * obj, void *arg_rec)
{
    Agrec_t *rec = (Agrec_t *) arg_rec, *newrec;
    if (obj->data == rec) {
	if (rec->next == rec)
	    newrec = NIL(Agrec_t *);
	else
	    newrec = rec->next;
	set_data(obj, newrec, FALSE);
    }
}

/* delete a record from a circular data list */
static void listdelrec(Agobj_t * obj, Agrec_t * rec)
{
    Agrec_t *prev;

    prev = obj->data;
    while (prev->next != rec) {
	prev = prev->next;
	assert(prev != obj->data);
    }
    /* following is a harmless no-op if the list is trivial */
    prev->next = rec->next;
}

int agdelrec(void *arg_obj, char *name)
{
    Agobj_t *obj;
    Agrec_t *rec;
    Agraph_t *g;

    obj = (Agobj_t *) arg_obj;
    g = agraphof(obj);
    rec = aggetrec(obj, name, FALSE);
    if (rec) {
	listdelrec(obj, rec);	/* zap it from the circular list */
	switch (obj->tag.objtype) {	/* refresh any stale pointers */
	case AGRAPH:
	    objdelrec(g, obj, rec);
	    break;
	case AGNODE:
	case AGINEDGE:
	case AGOUTEDGE:
	    agapply(agroot(g), obj, objdelrec, rec, FALSE);
	    break;
	}
	agstrfree(g, rec->name);
	agfree(g, rec);
	return SUCCESS;
    } else
	return FAILURE;
}

static void simple_delrec(Agraph_t * g, Agobj_t * obj, void *rec_name)
{
    agdelrec(obj, rec_name);
}

#ifdef OLD
void agclean(Agraph_t * g, char *graphdata, char *nodedata, char *edgedata)
{
    Agnode_t *n;
    Agedge_t *e;

    if (nodedata || edgedata) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (edgedata)
		for (e = agfstout(n); e; e = agnxtout(g, e)) {
		    agdelrec(e, edgedata);
		}
	    if (nodedata)
		agdelrec(n, nodedata);
	}
    }
    agdelrec(g, graphdata);
}
#endif

void aginit(Agraph_t * g, int kind, char *rec_name, int arg_rec_size, int mtf)
{
    Agnode_t *n;
    Agedge_t *e;
    Agraph_t *s;
	int		 rec_size;
	int		 recur;	/* if recursive on subgraphs */

	if (arg_rec_size < 0) {recur = 1; rec_size = -arg_rec_size;}
	else {recur = 0; rec_size= arg_rec_size;}
    switch (kind) {
    case AGRAPH:
	agbindrec(g, rec_name, rec_size, mtf);
	if (recur)
		for (s = agfstsubg(g); s; s = agnxtsubg(s))
			aginit(s,kind,rec_name,arg_rec_size,mtf);
	break;
    case AGNODE:
    case AGOUTEDGE:
    case AGINEDGE:
	for (n = agfstnode(g); n; n = agnxtnode(g, n))
	    if (kind == AGNODE)
		agbindrec(n, rec_name, rec_size, mtf);
	    else {
		for (e = agfstout(g, n); e; e = agnxtout(g, e))
		    agbindrec(e, rec_name, rec_size, mtf);
	    }
	break;
    default:
	break;
    }
}

void agclean(Agraph_t * g, int kind, char *rec_name)
{
    Agnode_t *n;
    Agedge_t *e;

    switch (kind) {
    case AGRAPH:
	agapply(g, (Agobj_t *) g, simple_delrec, rec_name, TRUE);
	break;
    case AGNODE:
    case AGOUTEDGE:
    case AGINEDGE:
	for (n = agfstnode(g); n; n = agnxtnode(g, n))
	    if (kind == AGNODE)
		agdelrec(n, rec_name);
	    else {
		for (e = agfstout(g, n); e; e = agnxtout(g, e))
		    agdelrec(e, rec_name);
	    }
	break;
    default:
	break;
    }
}

void agrecclose(Agobj_t * obj)
{
    Agraph_t *g;
    Agrec_t *rec, *nrec;

    g = agraphof(obj);
    if ((rec = obj->data)) {
	do {
	    nrec = rec->next;
	    agstrfree(g, rec->name);
	    agfree(g, rec);
	    rec = nrec;
	} while (rec != obj->data);
    }
    obj->data = NIL(Agrec_t *);
}
