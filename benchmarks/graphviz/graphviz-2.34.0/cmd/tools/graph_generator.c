/* $Id$Revision: */
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#include <graph_generator.h>

void makePath(int n, edgefn ef)
{
    int i;

    if (n == 1) {
	ef (1, 0);
	return;
    }
    for (i = 2; i <= n; i++)
	ef (i - 1, i);
}

void makeComplete(int n, edgefn ef)
{
    int i, j;

    if (n == 1) {
	ef (1, 0);
	return;
    }
    for (i = 1; i < n; i++) {
	for (j = i + 1; j <= n; j++) {
	    ef ( i, j);
	}
    }
}

void makeCircle(int n, edgefn ef)
{
    int i;

    if (n < 3) {
	fprintf(stderr, "Warning: degenerate circle of %d vertices\n", n);
	makePath(n, ef);
	return;
    }

    for (i = 1; i < n; i++)
	ef ( i, i + 1);
    ef (1, n);
}

void makeStar(int n, edgefn ef)
{
    int i;

    if (n < 3) {
	fprintf(stderr, "Warning: degenerate star of %d vertices\n", n);
	makePath(n, ef);
	return;
    }

    for (i = 2; i <= n; i++)
	ef (1, i);
}

void makeWheel(int n, edgefn ef)
{
    int i;

    if (n < 4) {
	fprintf(stderr, "Warning: degenerate wheel of %d vertices\n", n);
	makeComplete(n, ef);
	return;
    }

    makeStar(n, ef);

    for (i = 2; i < n; i++)
	ef( i, i + 1);
    ef (2, n);
}

void makeCompleteB(int dim1, int dim2, edgefn ef)
{
    int i, j;

    for (i = 1; i <= dim1; i++) {
	for (j = 1; j <= dim2; j++) {
	    ef ( i, dim1 + j);
	}
    }
}

void makeTorus(int dim1, int dim2, edgefn ef)
{
    int i, j, n = 0;

    for (i = 1; i <= dim1; i++) {
	for (j = 1; j < dim2; j++) {
	    ef( n + j, n + j + 1);
	}
	ef( n + 1, n + dim2);
	n += dim2;
    }

    for (i = 1; i <= dim2; i++) {
	for (j = 1; j < dim1; j++) {
	    ef( dim2 * (j - 1) + i, dim2 * j + i);
	}
	ef( i, dim2 * (dim1 - 1) + i);
    }
}

void makeTwistedTorus(int dim1, int dim2, int t1, int t2, edgefn ef)
{
    int i, j, li, lj;

    for (i = 0; i < dim1; i++) {
	for (j = 0; j < dim2; j++) {
	    li = (i + t1) % dim1;
	    lj = (j + 1) % dim2;
	    ef (i+j*dim1+1, li+lj*dim1+1);

	    li = (i + 1) % dim1;
	    lj = (j + t2) % dim2;
	    ef(i+j*dim1+1, li+lj*dim1+1);
	}
    }
}

void makeCylinder(int dim1, int dim2, edgefn ef)
{
    int i, j, n = 0;

    for (i = 1; i <= dim1; i++) {
	for (j = 1; j < dim2; j++) {
	    ef( n + j, n + j + 1);
	}
	ef( n + 1, n + dim2);
	n += dim2;
    }

    for (i = 1; i <= dim2; i++) {
	for (j = 1; j < dim1; j++) {
	    ef( dim2 * (j - 1) + i, dim2 * j + i);
	}
    }
}

#define OUTE(h) if (tl < (hd=(h))) ef( tl, hd)

void
makeSquareGrid(int dim1, int dim2, int connect_corners, int partial, edgefn ef)
{
    int i, j, tl, hd;

    for (i = 0; i < dim1; i++)
	for (j = 0; j < dim2; j++) {
	    // write the neighbors of the node i*dim2+j+1
	    tl = i * dim2 + j + 1;
	    if (j > 0
		&& (!partial || j <= 2 * dim2 / 6 || j > 4 * dim2 / 6
		    || i <= 2 * dim1 / 6 || i > 4 * dim1 / 6)) {
		OUTE(i * dim2 + j);
	    }
	    if (j < dim2 - 1
		&& (!partial || j < 2 * dim2 / 6 || j >= 4 * dim2 / 6
		    || i <= 2 * dim1 / 6 || i > 4 * dim1 / 6)) {
		OUTE(i * dim2 + j + 2);
	    }
	    if (i > 0) {
		OUTE((i - 1) * dim2 + j + 1);
	    }
	    if (i < dim1 - 1) {
		OUTE((i + 1) * dim2 + j + 1);
	    }
	    if (connect_corners == 1) {
		if (i == 0 && j == 0) {	// upper left
		    OUTE((dim1 - 1) * dim2 + dim2);
		} else if (i == (dim1 - 1) && j == 0) {	// lower left
		    OUTE(dim2);
		} else if (i == 0 && j == (dim2 - 1)) {	// upper right
		    OUTE((dim1 - 1) * dim2 + 1);
		} else if (i == (dim1 - 1) && j == (dim2 - 1)) {	// lower right
		    OUTE(1);
		}
	    } else if (connect_corners == 2) {
		if (i == 0 && j == 0) {	// upper left
		    OUTE(dim2);
		} else if (i == (dim1 - 1) && j == 0) {	// lower left
		    OUTE((dim1 - 1) * dim2 + dim2);
		} else if (i == 0 && j == (dim2 - 1)) {	// upper right
		    OUTE(1);
		} else if (i == (dim1 - 1) && j == (dim2 - 1)) {	// lower right
		    OUTE((dim1 - 1) * dim2 + 1);
		}
	    }
	}
}

static int
ipow (int base, int power)
{
    int ip;
    if (power == 0) return 1;
    if (power == 1) return base;

    ip = base;
    power--;
    while (power--) ip *= base;
    return ip; 
}

void makeTree(int depth, int nary, edgefn ef)
{
    unsigned int i, j;
    unsigned int n = (ipow(nary,depth)-1)/(nary-1); /* no. of non-leaf nodes */
    unsigned int idx = 2;

    for (i = 1; i <= n; i++) {
	for (j = 0; j < nary; j++) {
	    ef (i, idx++);
	}
    }
}

void makeBinaryTree(int depth, edgefn ef)
{
    unsigned int i;
    unsigned int n = (1 << depth) - 1;

    for (i = 1; i <= n; i++) {
	ef( i, 2 * i);
	ef( i, 2 * i + 1);
    }
}

typedef struct {
  int     nedges;
  int*    edges;
} vtx_data;

static void
constructSierpinski(int v1, int v2, int v3, int depth, vtx_data* graph)
{
    static int last_used_node_name = 3;
    int v4, v5, v6;

    int nedges;

    if (depth > 0) {
	v4 = ++last_used_node_name;
	v5 = ++last_used_node_name;
	v6 = ++last_used_node_name;
	constructSierpinski(v1, v4, v5, depth - 1, graph);
	constructSierpinski(v2, v5, v6, depth - 1, graph);
	constructSierpinski(v3, v4, v6, depth - 1, graph);
	return;
    }
    // depth==0, Construct graph:

    nedges = graph[v1].nedges;
    graph[v1].edges[nedges++] = v2;
    graph[v1].edges[nedges++] = v3;
    graph[v1].nedges = nedges;

    nedges = graph[v2].nedges;
    graph[v2].edges[nedges++] = v1;
    graph[v2].edges[nedges++] = v3;
    graph[v2].nedges = nedges;

    nedges = graph[v3].nedges;
    graph[v3].edges[nedges++] = v1;
    graph[v3].edges[nedges++] = v2;
    graph[v3].nedges = nedges;

    return;

}

#define NEW(t)           (t*)calloc((1),sizeof(t))
#define N_NEW(n,t)       (t*)calloc((n),sizeof(t))

void makeSierpinski(int depth, edgefn ef)
{
    vtx_data* graph;
    int* edges;
    int n;
    int nedges;
    int i, j;

    depth--;
    n = 3 * (1 + ((int) (pow(3.0, (double) depth) + 0.5) - 1) / 2);

    nedges = (int) (pow(3.0, depth + 1.0) + 0.5);

    graph = N_NEW(n + 1, vtx_data);
    edges = N_NEW(4 * n, int);

    for (i = 1; i <= n; i++) {
	graph[i].edges = edges;
	edges += 4;
	graph[i].nedges = 0;
    }

    constructSierpinski(1, 2, 3, depth, graph);

    for (i = 1; i <= n; i++) {
	int nghbr;
	// write the neighbors of the node i
	for (j = 0; j < graph[i].nedges; j++) {
	    nghbr = graph[i].edges[j];
	    if (i < nghbr) ef( i, nghbr);
	}
    }

    free(graph[1].edges);
    free(graph);
}

void makeHypercube(int dim, edgefn ef)
{
    int i, j, n;
    int neighbor;

    n = 1 << dim;

    for (i = 0; i < n; i++) {
	for (j = 0; j < dim; j++) {
	    neighbor = (i ^ (1 << j)) + 1;
	    if (i < neighbor)
		ef( i + 1, neighbor);
	}
    }
}

void makeTriMesh(int sz, edgefn ef)
{
    int i, j, idx;

    if (sz == 1) {
	ef (1, 0);
	return;
    }
    ef(1,2);
    ef(1,3);
    idx = 2;
    for (i=2; i < sz; i++) {
	for (j=1; j <= i; j++) {
	    ef(idx,idx+i);
	    ef(idx,idx+i+1);
	    if (j < i)
		ef(idx,idx+1);
	    idx++;
	}
    }
    for (j=1; j < sz; j++) {
	ef (idx,idx+1);
	idx++;
    }
}

void makeBall(int w, int h, edgefn ef)
{
    int i, cap;

    makeCylinder (w, h, ef);

    for (i = 1; i <= h; i++)
	ef (0, i);

    cap = w*h+1;
    for (i = (w-1)*h+1; i <= w*h; i++)
	ef (i, cap);

}

/* makeRandom:
 * No. of nodes is largest 2^n - 1 less than or equal to h.
 */
void makeRandom(int h, int w, edgefn ef)
{
    int i, j, type, size, depth;

    srand(time(0));
    if (rand()%2==1)
	type = 1;
    else
	type = 0;

    size = depth = 0;
    while (size <= h) {
	size += ipow(2,depth);
	depth++;
    }
    depth--;
    if (size > h) {
	size -= ipow(2,depth);
	depth--;
    }

    if (type)
	makeBinaryTree (depth, ef);
    else
	makePath (size, ef);

    for (i=3; i<=size; i++) {
	for (j=1; j<i-1; j++) {
	    int th = rand()%(size*size);
	    if (((th<=(w*w))&&((i<5)||((i>h-4)&&(j>h-4)))) || (th<=w))
		ef(j,i);
	}
    }
}

void makeMobius(int w, int h, edgefn ef)
{
    int i, j;

    if (h == 1) {
	fprintf(stderr, "Warning: degenerate Moebius strip of %d vertices\n", w);
	makePath(w, ef);
	return;
    }
    if (w == 1) {
	fprintf(stderr, "Warning: degenerate Moebius strip of %d vertices\n", h);
	makePath(h, ef);
	return;
    }

    for(i=0; i < w-1; i++) {
        for(j = 1; j < h; j++){
            ef(j + i*h, j + (i+1)*h);
            ef(j + i*h, j+1 + i*h);
        }
    }

    for(i = 1; i < h; i++){
        ef (i + (w-1)*h, i+1 + (w-1)*h);
    }
    for(i=1; i < w; i++) {
        ef(i*h , (i+1)*h);
        ef(i*h, (w-i)*h+1);
    }

    ef(1,w*h);
}

typedef struct {
    int j, d;
} pair;

typedef struct {
    int top, root;
    int* p; 
} tree_t;

static tree_t*
mkTree (int sz)
{
    tree_t* tp = NEW(tree_t);
    tp->root = 0;
    tp->top = 0;
    tp->p = N_NEW(sz,int);
    return tp;
}

static void
freeTree (tree_t* tp)
{
    free (tp->p);
    free (tp);
}

static void
resetTree (tree_t* tp)
{
    tp->root = 0;
    tp->top = 0;
}

static int
treeRoot (tree_t* tp)
{
    return tp->root;
}

static int
prevRoot (tree_t* tp)
{
    return tp->p[tp->root];
}

static int
treeSize (tree_t* tp)
{
    return (tp->top - tp->root + 1);
}

static int
treeTop (tree_t* tp)
{
    return (tp->top);
}

static void
treePop (tree_t* tp)
{
    tp->root = prevRoot(tp);
}

static void
addTree (tree_t* tp, int sz)
{
	tp->p[tp->top+1] = tp->root;
	tp->root = tp->top+1;
	tp->top += sz;
	if (sz > 1) tp->p[tp->top] = tp->top-1;
}

static void 
treeDup (tree_t* tp, int J)
{
    int M = treeSize(tp);
    int L = treeRoot(tp);
    int LL = prevRoot(tp);
    int i, LS = L + (J-1)*M - 1;
    for (i = L; i <= LS; i++) {
	if ((i-L)%M == 0)  tp->p[i+M] = LL;
	else tp->p[i+M] = tp->p[i] + M;
    }
    tp->top = LS + M;
}

/*****************/

typedef struct {
    int top;
    pair* v;
} stack;

static stack*
mkStack (int sz)
{
    stack* sp = NEW(stack);
    sp->top = 0;
    sp->v = N_NEW(sz,pair);
    return sp;
}

static void
freeStack (stack* sp)
{
    free (sp->v);
    free (sp);
}

static void
resetStack (stack* sp)
{
    sp->top = 0;
}

static void
push(stack* sp, int j, int d)
{
    sp->top++;
    sp->v[sp->top].j = j;
    sp->v[sp->top].d = d;
}

static pair
pop (stack* sp)
{
    sp->top--;
    return sp->v[sp->top+1];
}

/*****************/

static int*
genCnt(int NN)
{
    int* T = N_NEW(NN+1,int);
    int D, I, J, TD;
    int SUM;
    int NLAST = 1;
    T[1] = 1;
    while (NN > NLAST) {
	SUM = 0;
	for (D = 1; D <= NLAST; D++) {
	    I = NLAST+1;
	    TD = T[D]*D;
	    for (J = 1; J <= NLAST; J++) {
		I = I-D;
		if (I <= 0) break;
		SUM += T[I]*TD;
	    }
	}
	NLAST++;
	T[NLAST] = SUM/(NLAST-1);
    }
    return T;
}

static double 
drand(void)
{
    double d;
    d = rand();
    d = d / RAND_MAX;
    return d;
}

static void
genTree (int NN, int* T, stack* stack, tree_t* TREE)
{
    double v;
    pair p;
    int Z, D, N, J, TD, M, more;

    N = NN;

    while (1) {
	while (N > 2) {
	    v = (N-1)*T[N];
	    Z = v*drand();
	    D = 0;
	    more = 1;
	    do {
		D++;
		TD = D*T[D];
		M = N;
		J = 0;
		do {
		    J++;
		    M -= D;
		    if (M < 1) break;
		    Z -= T[M]*TD;
		    if (Z < 0) more = 0;
		} while (Z >= 0);
	    } while (more);
	    push(stack, J, D);
	    N = M;
	}
	addTree (TREE, N);
	 
	while (1) {
	    p = pop(stack);
	    N = p.d;
	    if (N != 0) {
		push(stack,p.j,0);
		break;
	    }
	    J = p.j;
	    if (J > 1) treeDup (TREE, J);
	    if (treeTop(TREE) == NN) return;
	    treePop(TREE);
	}
    }

}

static void
writeTree (tree_t* tp, edgefn ef)
{
    int i;
    for (i = 2; i <= tp->top; i++)
	ef (tp->p[i], i);
}

struct treegen_s {
    int N;
    int* T;
    stack* sp;
    tree_t* tp;
};

treegen_t* 
makeTreeGen (int N)
{
    treegen_t* tg = NEW(treegen_t);

    tg->N = N;
    tg->T = genCnt(N);
    tg->sp = mkStack(N+1);
    tg->tp = mkTree(N+1);
    srand(time(0));

    return tg;
}

void makeRandomTree (treegen_t* tg, edgefn ef)
{
    resetStack(tg->sp);
    resetTree(tg->tp);
    genTree (tg->N, tg->T, tg->sp, tg->tp);
    writeTree (tg->tp, ef);
}

void 
freeTreeGen(treegen_t* tg)
{
    free (tg->T);
    freeStack (tg->sp);
    freeTree (tg->tp);
    free (tg);
}

