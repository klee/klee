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

#ifndef NODE_H
#define NODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <index.h>

typedef struct Branch {
    Rect_t rect;
    struct Node *child;
} Branch_t;

typedef struct Node {
    int count;
    int level;			/* 0 is leaf, others positive */
    struct Branch branch[NODECARD];
} Node_t;

void RTreeFreeNode(RTree_t *, Node_t *);
void InitNode(Node_t *);
void InitBranch(Branch_t *);
Rect_t NodeCover(Node_t *);
int PickBranch(Rect_t *, Node_t *);
int AddBranch(RTree_t *, Branch_t *, Node_t *, Node_t **);
void DisconBranch(Node_t *, int);
void PrintBranch(int, Branch_t *);
Node_t *RTreeNewNode(RTree_t *);
#ifdef RTDEBUG
void PrintNode(Node_t * n);
void PrintBranch(int i, Branch_t * b);
#endif

#ifdef __cplusplus
}
#endif

#endif				/*NODE_H */
