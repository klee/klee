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
#include "logic.h"
#include "memory.h"

LeafList_t *RTreeNewLeafList(Leaf_t * lp)
{
    LeafList_t *llp;

    if ((llp = NEW(LeafList_t))) {
	llp->leaf = lp;
	llp->next = 0;
    }
    return llp;
}

LeafList_t *RTreeLeafListAdd(LeafList_t * llp, Leaf_t * lp)
{
    LeafList_t *nlp;
    if (!lp)
	return llp;

    nlp = RTreeNewLeafList(lp);
    nlp->next = llp;
    return nlp;
}

void RTreeLeafListFree(LeafList_t * llp)
{
    LeafList_t *tlp;
    while (llp->next) {
	tlp = llp->next;
	free(llp);
	llp = tlp;
    }
    free(llp);
    return;
}

/* Allocate space for a node in the list used in DeletRect to
 * store Nodes that are too empty.
 */
static struct ListNode *RTreeNewListNode(void)
{
    return NEW(struct ListNode);
}

#if UNUSED
static void RTreeFreeListNode(struct ListNode *p)
{
    free(p);
}
#endif

/* Add a node to the reinsertion list.  All its branches will later
 * be reinserted into the index structure.
 */
static int RTreeReInsert(RTree_t * rtp, Node_t * n, struct ListNode **ee)
{
    register struct ListNode *l;

    if (!(l = RTreeNewListNode()))
	return -1;
    l->node = n;
    l->next = *ee;
    *ee = l;
    return 0;
}

RTree_t *RTreeOpen()
{
    RTree_t *rtp;

    if ((rtp = NEW(RTree_t)))
	rtp->root = RTreeNewIndex(rtp);
    return rtp;
}

/* Make a new index, empty.  Consists of a single node. */
Node_t *RTreeNewIndex(RTree_t * rtp)
{
    Node_t *x;
    x = RTreeNewNode(rtp);
    x->level = 0;		/* leaf */
    rtp->LeafCount++;
    return x;
}

static int RTreeClose2(RTree_t * rtp, Node_t * n)
{
    int i;

    if (n->level > 0) {
	for (i = 0; i < NODECARD; i++) {
	    if (!n->branch[i].child)
		continue;
	    if (!RTreeClose2(rtp, n->branch[i].child)) {
		free(n->branch[i].child);
		DisconBranch(n, i);
		rtp->EntryCount--;
		if (rtp->StatFlag)
		    rtp->ElimCount++;
	    }
	}
    } else {
	for (i = 0; i < NODECARD; i++) {
	    if (!n->branch[i].child)
		continue;
	    // free(n->branch[i].child);
	    DisconBranch(n, i);
	    rtp->EntryCount--;
	    if (rtp->StatFlag)
	        rtp->ElimCount++;
	}
	//free(n);
    }
    return 0;
}


int RTreeClose(RTree_t * rtp)
{
    RTreeClose2(rtp, rtp->root);
    free(rtp->root);
    free(rtp);
    return 0;
}

#ifdef RTDEBUG
/* Print out all the nodes in an index.
** Prints from root downward.
*/
void PrintIndex(Node_t * n)
{
    int i;
    Node_t *nn;
    assert(n);
    assert(n->level >= 0);

    if (n->level > 0) {
	for (i = 0; i < NODECARD; i++) {
	    if ((nn = n->branch[i].child) != NULL)
		PrintIndex(nn);
	}
    }

    PrintNode(n);
}

/* Print out all the data rectangles in an index.
*/
void PrintData(Node_t * n)
{
    int i;
    Node_t *nn;
    assert(n);
    assert(n->level >= 0);

    if (n->level == 0)
	PrintNode(n);
    else {
	for (i = 0; i < NODECARD; i++) {
	    if ((nn = n->branch[i].child) != NULL)
		PrintData(nn);
	}
    }
}
#endif

/* RTreeSearch in an index tree or subtree for all data retangles that
** overlap the argument rectangle.
** Returns the number of qualifying data rects.
*/
LeafList_t *RTreeSearch(RTree_t * rtp, Node_t * n, Rect_t * r)
{
    register int i;
    LeafList_t *llp = 0;

    assert(n);
    assert(n->level >= 0);
    assert(r);

    rtp->SeTouchCount++;

    if (n->level > 0) {		/* this is an internal node in the tree */
	for (i = 0; i < NODECARD; i++)
	    if (n->branch[i].child && Overlap(r, &n->branch[i].rect)) {
		LeafList_t *tlp = RTreeSearch(rtp, n->branch[i].child, r);
		if (llp) {
		    LeafList_t *xlp = llp;
		    while (xlp->next)
			xlp = xlp->next;
		    xlp->next = tlp;
		} else
		    llp = tlp;
	    }
    } else {			/* this is a leaf node */
	for (i = 0; i < NODECARD; i++) {
	    if (n->branch[i].child && Overlap(r, &n->branch[i].rect)) {
		llp = RTreeLeafListAdd(llp, (Leaf_t *) & n->branch[i]);
#				ifdef RTDEBUG
		PrintRect(&n->branch[i].rect);
#				endif
	    }
	}
    }
    return llp;
}

/* Insert a data rectangle into an index structure.
** RTreeInsert provides for splitting the root;
** returns 1 if root was split, 0 if it was not.
** The level argument specifies the number of steps up from the leaf
** level to insert; e.g. a data rectangle goes in at level = 0.
** RTreeInsert2 does the recursion.
*/
static int RTreeInsert2(RTree_t *, Rect_t *, void *, Node_t *, Node_t **,
			int);
/*static int RTreeInsert2(RTree_t*, Rect_t*, int, Node_t*, Node_t**, int); */

int
RTreeInsert(RTree_t * rtp, Rect_t * r, void *data, Node_t ** n, int level)
{
    /* RTreeInsert(RTree_t*rtp, Rect_t*r, int data, Node_t**n, int level) { */
    register int i;
    register Node_t *newroot;
    Node_t *newnode=0;
    Branch_t b;
    int result = 0;


    assert(r && n);
    assert(level >= 0 && level <= (*n)->level);
    for (i = 0; i < NUMDIMS; i++)
	assert(r->boundary[i] <= r->boundary[NUMDIMS + i]);

#	ifdef RTDEBUG
    fprintf(stderr, "RTreeInsert  level=%d\n", level);
#	endif

    if (rtp->StatFlag) {
	if (rtp->Deleting)
	    rtp->ReInsertCount++;
	else
	    rtp->InsertCount++;
    }
    if (!rtp->Deleting)
	rtp->RectCount++;

    if (RTreeInsert2(rtp, r, data, *n, &newnode, level)) {	/* root was split */
	if (rtp->StatFlag) {
	    if (rtp->Deleting)
		rtp->DeTouchCount++;
	    else
		rtp->InTouchCount++;
	}

	newroot = RTreeNewNode(rtp);	/* grow a new root, make tree taller */
	rtp->NonLeafCount++;
	newroot->level = (*n)->level + 1;
	b.rect = NodeCover(*n);
	b.child = *n;
	AddBranch(rtp, &b, newroot, NULL);
	b.rect = NodeCover(newnode);
	b.child = newnode;
	AddBranch(rtp, &b, newroot, NULL);
	*n = newroot;
	// rtp->root = newroot;
	rtp->EntryCount += 2;
	result = 1;
    }

    return result;
}

/* Inserts a new data rectangle into the index structure.
** Recursively descends tree, propagates splits back up.
** Returns 0 if node was not split.  Old node updated.
** If node was split, returns 1 and sets the pointer pointed to by
** new to point to the new node.  Old node updated to become one of two.
** The level argument specifies the number of steps up from the leaf
** level to insert; e.g. a data rectangle goes in at level = 0.
*/
static int
RTreeInsert2(RTree_t * rtp, Rect_t * r, void *data,
	     Node_t * n, Node_t ** new, int level)
{
    /*static int */
    /* RTreeInsert2(RTree_t*rtp, Rect_t*r,
       int data, Node_t*n, Node_t**new, int level) {
     */
    register int i=0;
    Branch_t b;
    Node_t *n2=0;

    assert(r && n && new);
    assert(level >= 0 && level <= n->level);

    if (rtp->StatFlag) {
	if (rtp->Deleting)
	    rtp->DeTouchCount++;
	else
	    rtp->InTouchCount++;
    }

    /* Still above level for insertion, go down tree recursively */
    if (n->level > level) {
	i = PickBranch(r, n);
	if (!RTreeInsert2(rtp, r, data, n->branch[i].child, &n2, level)) {	/* recurse: child was not split */
	    n->branch[i].rect = CombineRect(r, &(n->branch[i].rect));
	    return 0;
	} else {		/* child was split */
	    n->branch[i].rect = NodeCover(n->branch[i].child);
	    b.child = n2;
	    b.rect = NodeCover(n2);
	    rtp->EntryCount++;
	    return AddBranch(rtp, &b, n, new);
	}
    } else if (n->level == level) {	/* at level for insertion. */
	/*Add rect, split if necessary */
	b.rect = *r;
	b.child = (Node_t *) data;
	rtp->EntryCount++;
	return AddBranch(rtp, &b, n, new);
    } else {			/* Not supposed to happen */
	assert(FALSE);
	return 0;
    }
}

static void FreeListNode(register struct ListNode *p)
{
    free(p);
}

/* Delete a data rectangle from an index structure.
** Pass in a pointer to a Rect, the data of the record, ptr to ptr to root node.
** Returns 1 if record not found, 0 if success.
** RTreeDelete provides for eliminating the root.
*/
static int RTreeDelete2(RTree_t *, Rect_t *, void *, Node_t *,
			ListNode_t **);
/* static int RTreeDelete2(RTree_t*, Rect_t*, int, Node_t*, ListNode_t**); */

int RTreeDelete(RTree_t * rtp, Rect_t * r, void *data, Node_t ** nn)
{
    /* int */
    /* RTreeDelete(RTree_t*rtp, Rect_t*r, int data, Node_t**nn) { */
    register int i;
    register Node_t *t;
    struct ListNode *reInsertList = NULL;
    register struct ListNode *e;

    assert(r && nn);
    assert(*nn);
    assert(data >= 0);

    rtp->Deleting = TRUE;

#	ifdef RTDEBUG
    fprintf(stderr, "RTreeDelete\n");
#	endif

    if (!RTreeDelete2(rtp, r, data, *nn, &reInsertList)) {
	/* found and deleted a data item */
	if (rtp->StatFlag)
	    rtp->DeleteCount++;
	rtp->RectCount--;

	/* reinsert any branches from eliminated nodes */
	while (reInsertList) {
	    t = reInsertList->node;
	    for (i = 0; i < NODECARD; i++) {
		if (t->branch[i].child) {
		    RTreeInsert(rtp, &(t->branch[i].rect),
				/* (int)t->branch[i].child, nn, t->level); */
				t->branch[i].child, nn, t->level);
		    rtp->EntryCount--;
		}
	    }
	    e = reInsertList;
	    reInsertList = reInsertList->next;
	    RTreeFreeNode(rtp, e->node);
	    FreeListNode(e);
	}

	/* check for redundant root (not leaf, 1 child) and eliminate */
	if ((*nn)->count == 1 && (*nn)->level > 0) {
	    if (rtp->StatFlag)
		rtp->ElimCount++;
	    rtp->EntryCount--;
	    for (i = 0; i < NODECARD; i++) {
		if ((t = (*nn)->branch[i].child))
		    break;
	    }
	    RTreeFreeNode(rtp, *nn);
	    *nn = t;
	}
	rtp->Deleting = FALSE;
	return 0;
    } else {
	rtp->Deleting = FALSE;
	return 1;
    }
}

/* Delete a rectangle from non-root part of an index structure.
** Called by RTreeDelete.  Descends tree recursively,
** merges branches on the way back up.
*/
static int
RTreeDelete2(RTree_t * rtp, Rect_t * r, void *data, Node_t * n,
	     ListNode_t ** ee)
/* static int */
/* RTreeDelete2(RTree_t*rtp, Rect_t*r, int data, Node_t*n, ListNode_t**ee) */
{
    register int i;

    assert(r && n && ee);
    assert(data >= 0);
    assert(n->level >= 0);

    if (rtp->StatFlag)
	rtp->DeTouchCount++;

    if (n->level > 0) {		/* not a leaf node */
	for (i = 0; i < NODECARD; i++) {
	    if (n->branch[i].child && Overlap(r, &(n->branch[i].rect))) {
		if (!RTreeDelete2(rtp, r, data, n->branch[i].child, ee)) {	/*recurse */
		    if (n->branch[i].child->count >= rtp->MinFill)
			n->branch[i].rect = NodeCover(n->branch[i].child);
		    else {	/* not enough entries in child, eliminate child node */
			RTreeReInsert(rtp, n->branch[i].child, ee);
			DisconBranch(n, i);
			rtp->EntryCount--;
			if (rtp->StatFlag)
			    rtp->ElimCount++;
		    }
		    return 0;
		}
	    }
	}
	return 1;
    } else {			/* a leaf node */
	for (i = 0; i < NODECARD; i++) {
	    if (n->branch[i].child
		&& n->branch[i].child == (Node_t *) data) {
		DisconBranch(n, i);
		rtp->EntryCount--;
		return 0;
	    }
	}
	return 1;
    }
}

#ifdef UNUSED
/* Allocate space for a node in the list used in DeletRect to
** store Nodes that are too empty.
*/
struct ListNode *NewListNode()
{
    return (struct ListNode *) NEW(sizeof(struct ListNode));
}

#endif
