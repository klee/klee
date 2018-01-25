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

#include "geometry.h"
#include "render.h"

typedef struct freenode {
    struct freenode *nextfree;
} Freenode;

typedef struct freeblock {
    struct freeblock *next;
    struct freenode *nodes;
} Freeblock;

#include "mem.h"
#include <stdlib.h>
#include <stdio.h>

static int gcd(int y, int x)
{
    while (x != y) {
	if (y < x)
	    x = x - y;
	else
	    y = y - x;
    }
    return x;
}

#define LCM(x,y) ((x)%(y) == 0 ? (x) : (y)%(x) == 0 ? (y) : x*(y/gcd(x,y)))

void freeinit(Freelist * fl, int size)
{

    fl->head = NULL;
    fl->nodesize = LCM(size, sizeof(Freenode));
    if (fl->blocklist != NULL) {
	Freeblock *bp, *np;

	bp = fl->blocklist;
	while (bp != NULL) {
	    np = bp->next;
	    free(bp->nodes);
	    free(bp);
	    bp = np;
	}
    }
    fl->blocklist = NULL;
}

void *getfree(Freelist * fl)
{
    int i;
    Freenode *t;
    Freeblock *mem;

    if (fl->head == NULL) {
	int size = fl->nodesize;
	char *cp;

	mem = GNEW(Freeblock);
	mem->nodes = gmalloc(sqrt_nsites * size);
	cp = (char *) (mem->nodes);
	for (i = 0; i < sqrt_nsites; i++) {
	    makefree(cp + i * size, fl);
	}
	mem->next = fl->blocklist;
	fl->blocklist = mem;
    }
    t = fl->head;
    fl->head = t->nextfree;
    return ((void *) t);
}

void makefree(void *curr, Freelist * fl)
{
    ((Freenode *) curr)->nextfree = fl->head;
    fl->head = (Freenode *) curr;
}
