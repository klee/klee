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

static Agraph_t *Ag_dictop_G;

/* only indirect call through dtopen() is expected */
void *agdictobjmem(Dict_t * dict, Void_t * p, size_t size, Dtdisc_t * disc)
{
    Agraph_t *g;

    NOTUSED(dict);
    NOTUSED(disc);
    g = Ag_dictop_G;
    if (g) {
	if (p)
	    agfree(g, p);
	else
	    return agalloc(g, size);
    } else {
	if (p)
	    free(p);
	else
	    return malloc(size);
    }
    return NIL(void *);
}

void agdictobjfree(Dict_t * dict, Void_t * p, Dtdisc_t * disc)
{
    Agraph_t *g;

    NOTUSED(dict);
    NOTUSED(disc);
    g = Ag_dictop_G;
    if (g)
	agfree(g, p);
    else
	free(p);
}

Dict_t *agdtopen(Agraph_t * g, Dtdisc_t * disc, Dtmethod_t * method)
{
    Dtmemory_f memf;
    Dict_t *d;

    memf = disc->memoryf;
    disc->memoryf = agdictobjmem;
    Ag_dictop_G = g;
    d = dtopen(disc, method);
    disc->memoryf = memf;
    Ag_dictop_G = NIL(Agraph_t*);
    return d;
}

long agdtdelete(Agraph_t * g, Dict_t * dict, void *obj)
{
    Ag_dictop_G = g;
    return (long) dtdelete(dict, obj);
}

int agobjfinalize(Void_t * obj)
{
    agfree(Ag_dictop_G, obj);
    return 0;
}

int agdtclose(Agraph_t * g, Dict_t * dict)
{
    Dtmemory_f memf;
    Dtdisc_t *disc;

    disc = dtdisc(dict, NIL(Dtdisc_t *), 0);
    memf = disc->memoryf;
    disc->memoryf = agdictobjmem;
    Ag_dictop_G = g;
    if (dtclose(dict))
	return 1;
    disc->memoryf = memf;
    Ag_dictop_G = NIL(Agraph_t*);
    return 0;
}

void agdtdisc(Agraph_t * g, Dict_t * dict, Dtdisc_t * disc)
{
    if (disc && (dtdisc(dict, NIL(Dtdisc_t *), 0) != disc)) {
	dtdisc(dict, disc, 0);
    }
    /* else unchanged, disc is same as old disc */
}
