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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NODELIST_H
#define NODELIST_H

#include  <render.h>

    typedef struct nodelistitem nodelistitem_t;

    struct nodelistitem {
	node_t *curr;
	nodelistitem_t *next;
	nodelistitem_t *prev;
    };

    typedef struct {
	nodelistitem_t *first;
	nodelistitem_t *last;
	int sz;
    } nodelist_t;

    extern nodelist_t *mkNodelist(void);
    extern void freeNodelist(nodelist_t *);
    extern int sizeNodelist(nodelist_t * list);

    extern void appendNodelist(nodelist_t *, nodelistitem_t *,
			       Agnode_t * n);
/* extern void removeNodelist(nodelist_t* list, Agnode_t* n); */
/* extern int node_exists(nodelist_t* list, Agnode_t* n); */
/* extern int nodename_exists(nodelist_t* list, char* n); */
    extern int node_position(nodelist_t * list, Agnode_t * n);

    extern void realignNodelist(nodelist_t * list, nodelistitem_t * n);
    extern void insertNodelist(nodelist_t *, Agnode_t *, Agnode_t *, int);

    extern void reverseAppend(nodelist_t *, nodelist_t *);
    extern nodelist_t *reverseNodelist(nodelist_t * list);
    extern nodelist_t *cloneNodelist(nodelist_t * list);

#ifdef DEBUG
    extern void printNodelist(nodelist_t * list);
#endif

#endif

#ifdef __cplusplus
}
#endif
