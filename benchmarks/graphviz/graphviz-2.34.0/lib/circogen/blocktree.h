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

#ifndef BLOCKTREE_H
#define BLOCKTREE_H

#include <render.h>
#include <circular.h>

    extern block_t *createBlocktree(Agraph_t * g, circ_state * state);
    extern void freeBlocktree(block_t *);
#ifdef DEBUG
    extern void print_blocktree(block_t * sn, int depth);
#endif

#endif

#ifdef __cplusplus
}
#endif
