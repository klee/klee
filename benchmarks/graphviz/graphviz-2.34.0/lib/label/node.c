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

#include <stdlib.h>

#include "index.h"
#include <stdio.h>
#include <assert.h>
#include "node.h"

/* Make a new node and initialize to have all branch cells empty.
*/
Node_t *RTreeNewNode(RTree_t * rtp)
{
    register Node_t *n;

    rtp->NodeCount++;
    n = (Node_t *) malloc(sizeof(Node_t));
    InitNode(n);
    return n;
}

void RTreeFreeNode(RTree_t * rtp, Node_t * p)
{
    rtp->NodeCount--;
    if (p->level == 0)
	rtp->LeafCount--;
    else
	rtp->NonLeafCount--;
    free(p);
}

/* Initialize a Node structure.
*/
void InitNode(Node_t * n)
{
    register int i;
    n->count = 0;
    n->level = -1;
    for (i = 0; i < NODECARD; i++)
	InitBranch(&(n->branch[i]));
}

/* Initialize one branch cell in a node.
*/
void InitBranch(Branch_t * b)
{
    InitRect(&(b->rect));
    b->child = NULL;
}

#ifdef RTDEBUG
/* Print out the data in a node.
*/
void PrintNode(Node_t * n)
{
    int i;
    assert(n);

    fprintf(stderr, "node");
    if (n->level == 0)
	fprintf(stderr, " LEAF");
    else if (n->level > 0)
	fprintf(stderr, " NONLEAF");
    else
	fprintf(stderr, " TYPE=?");
    fprintf(stderr, "  level=%d  count=%d  child address=%X\n",
	    n->level, n->count, (unsigned int) n);

    for (i = 0; i < NODECARD; i++) {
	if (n->branch[i].child != NULL)
	    PrintBranch(i, &n->branch[i]);
    }
}

void PrintBranch(int i, Branch_t * b)
{
    fprintf(stderr, "  child[%d] X%X\n", i, (unsigned int) b->child);
    PrintRect(&(b->rect));
}
#endif

/* Find the smallest rectangle that includes all rectangles in
** branches of a node.
*/
Rect_t NodeCover(Node_t * n)
{
    register int i, flag;
    Rect_t r;
    assert(n);

    InitRect(&r);
    flag = 1;
    for (i = 0; i < NODECARD; i++)
	if (n->branch[i].child) {
	    if (flag) {
		r = n->branch[i].rect;
		flag = 0;
	    } else
		r = CombineRect(&r, &(n->branch[i].rect));
	}
    return r;
}

/* Pick a branch.  Pick the one that will need the smallest increase
** in area to accomodate the new rectangle.  This will result in the
** least total area for the covering rectangles in the current node.
** In case of a tie, pick the one which was smaller before, to get
** the best resolution when searching.
*/
int PickBranch(Rect_t * r, Node_t * n)
{
    register Rect_t *rr=0;
    register int i=0, flag=1, increase=0, bestIncr=0, area=0, bestArea=0;
    int best=0;
    assert(r && n);

    for (i = 0; i < NODECARD; i++) {
	if (n->branch[i].child) {
	    Rect_t rect;
	    rr = &n->branch[i].rect;
	    area = RectArea(rr);
	    /* increase = RectArea(&CombineRect(r, rr)) - area; */
	    rect = CombineRect(r, rr);
	    increase = RectArea(&rect) - area;
	    if (increase < bestIncr || flag) {
		best = i;
		bestArea = area;
		bestIncr = increase;
		flag = 0;
	    } else if (increase == bestIncr && area < bestArea) {
		best = i;
		bestArea = area;
		bestIncr = increase;
	    }
#			ifdef RTDEBUG
	    fprintf(stderr,
		    "i=%d  area before=%d  area after=%d  increase=%d\n",
		    i, area, area + increase, increase);
#			endif
	}
    }
#	ifdef RTDEBUG
    fprintf(stderr, "\tpicked %d\n", best);
#	endif
    return best;
}

/* Add a branch to a node.  Split the node if necessary.
** Returns 0 if node not split.  Old node updated.
** Returns 1 if node split, sets *new to address of new node.
** Old node updated, becomes one of two.
*/
int AddBranch(RTree_t * rtp, Branch_t * b, Node_t * n, Node_t ** new)
{
    register int i;

    assert(b);
    assert(n);

    if (n->count < NODECARD) {	/* split won't be necessary */
	for (i = 0; i < NODECARD; i++) {	/* find empty branch */
	    if (n->branch[i].child == NULL) {
		n->branch[i] = *b;
		n->count++;
		break;
	    }
	}
	assert(i < NODECARD);
	return 0;
    } else {
	if (rtp->StatFlag) {
	    if (rtp->Deleting)
		rtp->DeTouchCount++;
	    else
		rtp->InTouchCount++;
	}
	assert(new);
	SplitNode(rtp, n, b, new);
	if (n->level == 0)
	    rtp->LeafCount++;
	else
	    rtp->NonLeafCount++;
	return 1;
    }
}

/* Disconnect a dependent node.
*/
void DisconBranch(Node_t * n, int i)
{
    assert(n && i >= 0 && i < NODECARD);
    assert(n->branch[i].child);

    InitBranch(&(n->branch[i]));
    n->count--;
}
