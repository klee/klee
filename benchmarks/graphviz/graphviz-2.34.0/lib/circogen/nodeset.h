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

#ifndef NODESET_H
#define NODESET_H

#include  <render.h>

    typedef struct {
	Dtlink_t link;
	Agnode_t *np;
    } nsitem_t;

    typedef Dt_t nodeset_t;

    extern nodeset_t *mkNodeset(void);
    extern void freeNodeset(nodeset_t *);
    extern void clearNodeset(nodeset_t *);
    extern void insertNodeset(nodeset_t * ns, Agnode_t * n);
    extern void removeNodeset(nodeset_t *, Agnode_t * n);
    extern int sizeNodeset(nodeset_t * ns);

#ifdef DEBUG
    extern void printNodeset(nodeset_t *);
#endif

#endif

#ifdef __cplusplus
}
#endif
