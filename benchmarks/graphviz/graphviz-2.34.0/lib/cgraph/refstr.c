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

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/*
 * reference counted strings.
 */

static unsigned long HTML_BIT;	/* msbit of unsigned long */
static unsigned long CNT_BITS;	/* complement of HTML_BIT */

typedef struct refstr_t {
    Dtlink_t link;
    unsigned long refcnt;
    char *s;
    char store[1];		/* this is actually a dynamic array */
} refstr_t;

static Dtdisc_t Refstrdisc = {
    offsetof(refstr_t, s),	/* key */
    -1,				/* size */
    0,				/* link offset */
    NIL(Dtmake_f),
    agdictobjfree,
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    agdictobjmem,
    NIL(Dtevent_f)
};

static Dict_t *Refdict_default;

/* refdict:
 * Return the string dictionary associated with g.
 * If necessary, create it.
 * As a side-effect, set html masks. This assumes 8-bit bytes.
 */
static Dict_t *refdict(Agraph_t * g)
{
    Dict_t **dictref;

    if (g)
	dictref = &(g->clos->strdict);
    else
	dictref = &Refdict_default;
    if (*dictref == NIL(Dict_t *)) {
	*dictref = agdtopen(g, &Refstrdisc, Dttree);
	HTML_BIT = ((unsigned int) 1) << (sizeof(unsigned int) * 8 - 1);
	CNT_BITS = ~HTML_BIT;
    }
    return *dictref;
}

int agstrclose(Agraph_t * g)
{
    return agdtclose(g, refdict(g));
}

static refstr_t *refsymbind(Dict_t * strdict, char *s)
{
    refstr_t key, *r;
    key.s = s;
    r = (refstr_t *) dtsearch(strdict, &key);
    return r;
}

static char *refstrbind(Dict_t * strdict, char *s)
{
    refstr_t *r;
    r = refsymbind(strdict, s);
    if (r)
	return r->s;
    else
	return NIL(char *);
}

char *agstrbind(Agraph_t * g, char *s)
{
    return refstrbind(refdict(g), s);
}

char *agstrdup(Agraph_t * g, char *s)
{
    refstr_t *r;
    Dict_t *strdict;
    size_t sz;

    if (s == NIL(char *))
	 return NIL(char *);
    strdict = refdict(g);
    r = refsymbind(strdict, s);
    if (r)
	r->refcnt++;
    else {
	sz = sizeof(refstr_t) + strlen(s);
	if (g)
	    r = (refstr_t *) agalloc(g, sz);
	else
	    r = (refstr_t *) malloc(sz);
	r->refcnt = 1;
	strcpy(r->store, s);
	r->s = r->store;
	dtinsert(strdict, r);
    }
    return r->s;
}

char *agstrdup_html(Agraph_t * g, char *s)
{
    refstr_t *r;
    Dict_t *strdict;
    size_t sz;

    if (s == NIL(char *))
	 return NIL(char *);
    strdict = refdict(g);
    r = refsymbind(strdict, s);
    if (r)
	r->refcnt++;
    else {
	sz = sizeof(refstr_t) + strlen(s);
	if (g)
	    r = (refstr_t *) agalloc(g, sz);
	else
	    r = (refstr_t *) malloc(sz);
	r->refcnt = 1 | HTML_BIT;
	strcpy(r->store, s);
	r->s = r->store;
	dtinsert(strdict, r);
    }
    return r->s;
}

int agstrfree(Agraph_t * g, char *s)
{
    refstr_t *r;
    Dict_t *strdict;

    if (s == NIL(char *))
	 return FAILURE;

    strdict = refdict(g);
    r = refsymbind(strdict, s);
    if (r && (r->s == s)) {
	r->refcnt--;
	if ((r->refcnt && CNT_BITS) == 0) {
	    agdtdelete(g, strdict, r);
	    /*
	       if (g) agfree(g,r);
	       else free(r);
	     */
	}
    }
    if (r == NIL(refstr_t *))
	return FAILURE;
    return SUCCESS;
}

/* aghtmlstr:
 * Return true if s is an HTML string.
 * We assume s points to the datafield store[0] of a refstr.
 */
int aghtmlstr(char *s)
{
    refstr_t *key;

    if (s == NULL)
	return 0;
    key = (refstr_t *) (s - offsetof(refstr_t, store[0]));
    return (key->refcnt & HTML_BIT);
}

void agmarkhtmlstr(char *s)
{
    refstr_t *key;

    if (s == NULL)
	return;
    key = (refstr_t *) (s - offsetof(refstr_t, store[0]));
    key->refcnt |= HTML_BIT;
}

#ifdef DEBUG
static int refstrprint(Dict_t * dict, void *ptr, void *user)
{
    refstr_t *r;

    NOTUSED(dict);
    r = ptr;
    NOTUSED(user);
    write(2, r->s, strlen(r->s));
    write(2, "\n", 1);
    return 0;
}

void agrefstrdump(Agraph_t * g)
{
    dtwalk(Refdict_default, refstrprint, 0);
}
#endif
