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

#ifndef BLOCK_H
#define BLOCK_H

#include  <nodelist.h>

    typedef struct block block_t;

    typedef struct {
	block_t *first;
	block_t *last;
    } blocklist_t;

    struct block {
	Agnode_t *child;	/* if non-null, points to node in parent block */
	block_t *next;		/* sibling block */
	Agraph_t *sub_graph;	/* nodes and edges in this block */
	double radius;		/* radius of block and subblocks */
	double rad0;		/* radius of block */
	nodelist_t *circle_list;	/* ordered list of nodes in block */
	blocklist_t children;	/* child blocks */
	double parent_pos;	/* if block has 1 node, angle to place parent */
	int flags;
    };

    extern block_t *mkBlock(Agraph_t *);
    extern void freeBlock(block_t * sp);
    extern int blockSize(block_t * sp);

    extern void initBlocklist(blocklist_t *);
    extern void appendBlock(blocklist_t * sp, block_t * sn);
    extern void insertBlock(blocklist_t * sp, block_t * sn);
/* extern void freeBlocklist (blocklist_t* sp); */

#ifdef DEBUG
    extern void printBlocklist(blocklist_t * snl);
#endif

#define CHILD(b) ((b)->child)
#define BLK_PARENT(b) (CHILD(b)? PARENT(CHILD(b)) : 0)
#define BLK_FLAGS(b) ((b)->flags)

#define COALESCED_F   (1 << 0)
#define COALESCED(b) (BLK_FLAGS(b)&COALESCED_F)
#define SET_COALESCED(b) (BLK_FLAGS(b) |= COALESCED_F)

#endif

#ifdef __cplusplus
}
#endif
