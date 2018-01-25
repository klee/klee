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


#include "kkutils.h"
#include "closest.h"
#include <stdlib.h>

/*****************************************
** This module contains functions that  **
** given a 1-D layout construct a graph **
** where close nodes in this layout are **
** adjacent                             **
*****************************************/

typedef struct {
    /* this structure represents two nodes in the 1-D layout */
    int left;			/* the left node in the pair */
    int right;			/* the right node in the pair */
    double dist;		/* distance between the nodes in the layout */
} Pair;

#define LT(p,q) ((p).dist < (q).dist)
#define EQ(p,q) ((p).dist == (q).dist)

/*
Pair(int v, int u) {left=v; right=u;}
bool operator>(Pair other) {return dist>other.dist;}
bool operator>=(Pair other) {return dist>=other.dist;}
bool operator<(Pair other) {return dist<other.dist;}
bool operator<=(Pair other) {return dist<=other.dist;}
bool operator==(Pair other) {return dist==other.dist;}
*/

typedef struct {
    Pair *data;
    int max_size;
    int top;
} PairStack;

static void initStack(PairStack * s, int n)
{
    s->data = N_GNEW(n, Pair);
    s->max_size = n;
    s->top = 0;
}

static void freeStack(PairStack * s)
{
    free(s->data);
}

#define push(s,x) { \
	if (s->top>=s->max_size) { \
		s->max_size *= 2; \
		s->data = (Pair*) realloc(s->data, s->max_size*sizeof(Pair)); \
	} \
	s->data[s->top++] = x; \
}

#define pop(s,x) ((s->top==0) ? FALSE : (s->top--, x = s->data[s->top], TRUE))

#define read_top(h,x) ((s->top==0) ? FALSE : (x = s->data[s->top-1], TRUE))

#define sub(h,i) (h->data[i])

/* An auxulliary data structure (a heap) for 
 * finding the closest pair in the layout
 */
typedef struct {
    Pair *data;
    int heapSize;
    int maxSize;
} PairHeap;

#define left(i) (2*(i))
#define right(i) (2*(i)+1)
#define parent(i) ((i)/2)
#define insideHeap(h,i) ((i)<h->heapSize)
#define greaterPriority(h,i,j) \
  (LT(h->data[i],h->data[j]) || ((EQ(h->data[i],h->data[j])) && (rand()%2)))

#define exchange(h,i,j) {Pair temp; \
        temp=h->data[i]; \
        h->data[i]=h->data[j]; \
        h->data[j]=temp; \
}
#define assign(h,i,j) {h->data[i]=h->data[j]}

static void heapify(PairHeap * h, int i)
{
    int l, r, largest;
    while (1) {
	l = left(i);
	r = right(i);
	if (insideHeap(h, l) && greaterPriority(h, l, i))
	    largest = l;
	else
	    largest = i;
	if (insideHeap(h, r) && greaterPriority(h, r, largest))
	    largest = r;
	if (largest == i)
	    break;

	exchange(h, largest, i);
	i = largest;
    }
}

#ifdef UNUSED
static void mkHeap(PairHeap * h, int size)
{
    h->data = N_GNEW(size, Pair);
    h->maxSize = size;
    h->heapSize = 0;
}
#endif

static void freeHeap(PairHeap * h)
{
    free(h->data);
}

static void initHeap(PairHeap * h, double *place, int *ordering, int n)
{
    int i;
    Pair edge;
    int j;

    h->heapSize = n - 1;
#ifdef REDO
    if (h->heapSize > h->maxSize) {
	h->maxSize = h->heapSize;
	h->data = (Pair *) realloc(h->data, h->maxSize * sizeof(Pair));
    }
#else
    h->maxSize = h->heapSize;
    h->data = N_GNEW(h->maxSize, Pair);
#endif

    for (i = 0; i < n - 1; i++) {
	edge.left = ordering[i];
	edge.right = ordering[i + 1];
	edge.dist = place[ordering[i + 1]] - place[ordering[i]];
	h->data[i] = edge;
    }
    for (j = (n - 1) / 2; j >= 0; j--) {
	heapify(h, j);
    }
}

static boolean extractMax(PairHeap * h, Pair * max)
{
    if (h->heapSize == 0)
	return FALSE;

    *max = h->data[0];
    h->data[0] = h->data[h->heapSize - 1];
    h->heapSize--;
    heapify(h, 0);
    return TRUE;
}

static void insert(PairHeap * h, Pair edge)
{
    int i = h->heapSize;
    if (h->heapSize == h->maxSize) {
	h->maxSize *= 2;
	h->data = (Pair *) realloc(h->data, h->maxSize * sizeof(Pair));
    }
    h->heapSize++;
    h->data[i] = edge;
    while (i > 0 && greaterPriority(h, i, parent(i))) {
	exchange(h, i, parent(i));
	i = parent(i);
    }
}

/*
static bool
isheap(PairHeap* h)
{
	int i,l,r;
	for (i=0; i<h->heapSize; i++) {
		l=left(i); r=right(i);
		if (insideHeap(h,l) && greaterPriority(h,l,i))
			return FALSE;
		if (insideHeap(h,r) && greaterPriority(h,r,i))
			return FALSE;
	}
	return TRUE;
}
*/

static void
find_closest_pairs(double *place, int n, int num_pairs,
		   PairStack * pairs_stack)
{
    /* Fill the stack 'pairs_stack' with 'num_pairs' closest pairs int the 1-D layout 'place' */
    int i;
    PairHeap heap;
    int *left = N_GNEW(n, int);
    int *right = N_GNEW(n, int);
    Pair pair = { 0, 0 }, new_pair;

    /* Order the nodes according to their place */
    int *ordering = N_GNEW(n, int);
    int *inv_ordering = N_GNEW(n, int);

    for (i = 0; i < n; i++) {
	ordering[i] = i;
    }
    quicksort_place(place, ordering, 0, n - 1);
    for (i = 0; i < n; i++) {
	inv_ordering[ordering[i]] = i;
    }

    /* Intialize heap with all consecutive pairs */
    initHeap(&heap, place, ordering, n);

    /* store the leftmost and rightmost neighbors of each node that were entered into heap */
    for (i = 1; i < n; i++) {
	left[ordering[i]] = ordering[i - 1];
    }
    for (i = 0; i < n - 1; i++) {
	right[ordering[i]] = ordering[i + 1];
    }

    /* extract the 'num_pairs' closest pairs */
    for (i = 0; i < num_pairs; i++) {
	int left_index;
	int right_index;
	int neighbor;

	if (!extractMax(&heap, &pair)) {
	    break;		/* not enough pairs */
	}
	push(pairs_stack, pair);
	/* insert to heap "descendant" pairs */
	left_index = inv_ordering[pair.left];
	right_index = inv_ordering[pair.right];
	if (left_index > 0) {
	    neighbor = ordering[left_index - 1];
	    if (inv_ordering[right[neighbor]] < right_index) {
		/* we have a new pair */
		new_pair.left = neighbor;
		new_pair.right = pair.right;
		new_pair.dist = place[pair.right] - place[neighbor];
		insert(&heap, new_pair);
		right[neighbor] = pair.right;
		left[pair.right] = neighbor;
	    }
	}
	if (right_index < n - 1) {
	    neighbor = ordering[right_index + 1];
	    if (inv_ordering[left[neighbor]] > left_index) {
		/* we have a new pair */
		new_pair.left = pair.left;
		new_pair.right = neighbor;
		new_pair.dist = place[neighbor] - place[pair.left];
		insert(&heap, new_pair);
		left[neighbor] = pair.left;
		right[pair.left] = neighbor;
	    }
	}
    }
    free(left);
    free(right);
    free(ordering);
    free(inv_ordering);
    freeHeap(&heap);
}

static void add_edge(vtx_data * graph, int u, int v)
{
    int i;
    for (i = 0; i < graph[u].nedges; i++) {
	if (graph[u].edges[i] == v) {
	    /* edge already exist */
	    return;
	}
    }
    /* add the edge */
    graph[u].edges[graph[u].nedges++] = v;
    graph[v].edges[graph[v].nedges++] = u;
    if (graph[0].ewgts != NULL) {
	graph[u].ewgts[0]--;
	graph[v].ewgts[0]--;
    }
}

static void
construct_graph(int n, PairStack * edges_stack, vtx_data ** New_graph)
{
    /* construct an unweighted graph using the edges 'edges_stack' */
    int i;
    vtx_data *new_graph;

    /* first compute new degrees and nedges; */
    int *degrees = N_GNEW(n, int);
    int top = edges_stack->top;
    int new_nedges = 2 * top + n;
    Pair pair;
    int *edges = N_GNEW(new_nedges, int);
    float *weights = N_GNEW(new_nedges, float);

    for (i = 0; i < n; i++) {
	degrees[i] = 1;		/* save place for the self loop */
    }
    for (i = 0; i < top; i++) {
	pair = sub(edges_stack, i);
	degrees[pair.left]++;
	degrees[pair.right]++;
    }

    /* copy graph into new_graph: */
    for (i = 0; i < new_nedges; i++) {
	weights[i] = 1.0;
    }

    *New_graph = new_graph = N_GNEW(n, vtx_data);
    for (i = 0; i < n; i++) {
	new_graph[i].nedges = 1;
	new_graph[i].ewgts = weights;
#ifdef USE_STYLES
	new_graph[i].styles = NULL;
#endif
	new_graph[i].edges = edges;
	*edges = i;		/* self loop for Lap */
	*weights = 0;		/* self loop weight for Lap */
	weights += degrees[i];
	edges += degrees[i];	/* reserve space for possible more edges */
    }

    free(degrees);

    /* add all edges from stack */
    while (pop(edges_stack, pair)) {
	add_edge(new_graph, pair.left, pair.right);
    }
}

void
closest_pairs2graph(double *place, int n, int num_pairs, vtx_data ** graph)
{
    /* build a graph with with edges between the 'num_pairs' closest pairs in the 1-D space: 'place' */
    PairStack pairs_stack;
    initStack(&pairs_stack, num_pairs);
    find_closest_pairs(place, n, num_pairs, &pairs_stack);
    construct_graph(n, &pairs_stack, graph);
    freeStack(&pairs_stack);
}
