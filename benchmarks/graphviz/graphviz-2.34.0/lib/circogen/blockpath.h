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

#ifndef BLOCKPATH_H
#define BLOCKPATH_H

#include <circular.h>

    extern nodelist_t *layout_block(Agraph_t * g, block_t * sn, double);
    extern int cmpNodeDegree(Dt_t *, Agnode_t **, Agnode_t **, Dtdisc_t *);

#ifdef DEBUG
    extern void prTree(Agraph_t * g);
#endif

#endif

#ifdef __cplusplus
}
#endif
