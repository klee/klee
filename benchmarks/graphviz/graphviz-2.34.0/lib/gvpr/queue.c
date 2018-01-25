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


/*
 * Queue implementation using cdt
 *
 */

#include <queue.h>
#include <ast.h>

typedef struct {
    Dtlink_t link;
    void *np;
} nsitem;

static Void_t *makef(Dt_t * d, nsitem * obj, Dtdisc_t * disc)
{
    nsitem *p;

    p = oldof(0, nsitem, 1, 0);
    p->np = obj->np;
    return p;
}

static void freef(Dt_t * d, nsitem * obj, Dtdisc_t * disc)
{
    free(obj);
}

static Dtdisc_t ndisc = {
    offsetof(nsitem, np),
    sizeof(void *),
    offsetof(nsitem, link),
    (Dtmake_f) makef,
    (Dtfree_f) freef,
    0,
    0,
    0,
    0
};

queue *mkQ(Dtmethod_t * meth)
{
    queue *nq;

    nq = dtopen(&ndisc, meth);
    return nq;
}

void push(queue * nq, void *n)
{
    nsitem obj;

    obj.np = n;
    dtinsert(nq, &obj);
}

void *pop(queue * nq, int delete)
{
    nsitem *obj;
    void *n;

    obj = dtfirst(nq);
    if (obj) {
	n = obj->np;
	if (delete)
	    dtdelete(nq, 0);
	return n;
    } else
	return 0;
}

void freeQ(queue * nq)
{
    dtclose(nq);
}
