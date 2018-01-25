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


#include <deglist.h>
#include <circular.h>
#include <blockpath.h>
#include <assert.h>

typedef struct {
    Dtlink_t link;
    int deg;
    Agnode_t *np;		/* linked list of nodes of same degree */
} degitem;

static degitem *mkItem(Dt_t * d, degitem * obj, Dtdisc_t * disc)
{
    degitem *ap = GNEW(degitem);

    ap->np = NULL;
    ap->deg = obj->deg;
    return ap;
}

static void freeItem(Dt_t * d, degitem * obj, Dtdisc_t * disc)
{
    free(obj);
}

static int cmpDegree(Dt_t * d, int *key1, int *key2, Dtdisc_t * disc)
{
    if (*key1 < *key2)
	return -1;
    else if (*key1 > *key2)
	return 1;
    else
	return 0;
}

static Dtdisc_t nodeDisc = {
    offsetof(degitem, deg),	/* key */
    sizeof(int),		/* size */
    offsetof(degitem, link),	/* link */
    (Dtmake_f) mkItem,
    (Dtfree_f) freeItem,
    (Dtcompar_f) cmpDegree,
    (Dthash_f) 0,
    (Dtmemory_f) 0,
    (Dtevent_f) 0
};

/* mkDeglist:
 * Create an empty list of nodes.
 */
deglist_t *mkDeglist(void)
{
    deglist_t *s = dtopen(&nodeDisc, Dtoset);
    return s;
}

/* freeDeglist:
 * Delete the node list.
 * Nodes are not deleted.
 */
void freeDeglist(deglist_t * s)
{
    dtclose(s);
}

/* insertDeglist:
 * Add a node to the node list.
 * Nodes are kept sorted by DEGREE, smallest degrees first.
 */
void insertDeglist(deglist_t * ns, Agnode_t * n)
{
    degitem key;
    degitem *kp;

    key.deg = DEGREE(n);
    kp = dtinsert(ns, &key);
    ND_next(n) = kp->np;
    kp->np = n;
}

/* removeDeglist:
 * Remove n from list, if it is in the list.
 */
void removeDeglist(deglist_t * list, Agnode_t * n)
{
    degitem key;
    degitem *ip;
    Agnode_t *np;
    Agnode_t *prev;

    key.deg = DEGREE(n);
    ip = (degitem *) dtsearch(list, &key);
    assert(ip);
    if (ip->np == n) {
	ip->np = ND_next(n);
	if (ip->np == NULL)
	    dtdelete(list, ip);
    } else {
	prev = ip->np;
	np = ND_next(prev);
	while (np && (np != n)) {
	    prev = np;
	    np = ND_next(np);
	}
	if (np)
	    ND_next(prev) = ND_next(np);
    }
}

/* firstDeglist:
 * Return the first node in the list (smallest degree)
 * Remove from list.
 * If the list is empty, return NULL
 */
Agnode_t *firstDeglist(deglist_t * list)
{
    degitem *ip;
    Agnode_t *np;

    ip = (degitem *) dtfirst(list);
    if (ip) {
	np = ip->np;
	ip->np = ND_next(np);
	if (ip->np == NULL)
	    dtdelete(list, ip);
	return np;
    } else
	return 0;
}

#ifdef DEBUG
void printDeglist(deglist_t * dl)
{
    degitem *ip;
    node_t *np;
    fprintf(stderr, " dl:\n");
    for (ip = (degitem *) dtfirst(dl); ip; ip = (degitem *) dtnext(dl, ip)) {
	np = ip->np;
	if (np)
	    fprintf(stderr, " (%d)", ip->deg);
	for (; np; np = ND_next(np)) {
	    fprintf(stderr, " %s", agnameof(np));
	}
	fprintf(stderr, "\n");
    }

}
#endif
