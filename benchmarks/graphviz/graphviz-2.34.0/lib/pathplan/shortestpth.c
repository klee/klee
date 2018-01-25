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


#include "vis.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static COORD unseen = (double) INT_MAX;

/* shortestPath:
 * Given a VxV weighted adjacency matrix, compute the shortest
 * path vector from root to target. The returned vector (dad) encodes the
 * shorted path from target to the root. That path is given by
 * i, dad[i], dad[dad[i]], ..., root
 * We have dad[root] = -1.
 *
 * Based on Dijkstra's algorithm (Sedgewick, 2nd. ed., p. 466).
 *
 * This implementation only uses the lower left triangle of the
 * adjacency matrix, i.e., the values a[i][j] where i >= j.
 */
int *shortestPath(int root, int target, int V, array2 wadj)
{
    int *dad;
    COORD *vl;
    COORD *val;
    int min;
    int k, t;

    /* allocate arrays */
    dad = (int *) malloc(V * sizeof(int));
    vl = (COORD *) malloc((V + 1) * sizeof(COORD));	/* One extra for sentinel */
    val = vl + 1;

    /* initialize arrays */
    for (k = 0; k < V; k++) {
	dad[k] = -1;
	val[k] = -unseen;
    }
    val[-1] = -(unseen + (COORD) 1);	/* Set sentinel */
    min = root;

    /* use (min >= 0) to fill entire tree */
    while (min != target) {
	k = min;
	val[k] *= -1;
	min = -1;
	if (val[k] == unseen)
	    val[k] = 0;

	for (t = 0; t < V; t++) {
	    if (val[t] < 0) {
		COORD newpri;
		COORD wkt;

		/* Use lower triangle */
		if (k >= t)
		    wkt = wadj[k][t];
		else
		    wkt = wadj[t][k];

		newpri = -(val[k] + wkt);
		if ((wkt != 0) && (val[t] < newpri)) {
		    val[t] = newpri;
		    dad[t] = k;
		}
		if (val[t] > val[min])
		    min = t;
	    }
	}
    }

    free(vl);
    return dad;
}

/* makePath:
 * Given two points p and q in two polygons pp and qp of a vconfig_t conf, 
 * and the visibility vectors of p and q relative to conf, 
 * compute the shortest path from p to q.
 * If dad is the returned array and V is the number of polygon vertices in
 * conf, then the path is V(==q), dad[V], dad[dad[V]], ..., V+1(==p).
 * NB: This is the only path that is guaranteed to be valid.
 * We have dad[V+1] = -1.
 * 
 */
int *makePath(Ppoint_t p, int pp, COORD * pvis,
	      Ppoint_t q, int qp, COORD * qvis, vconfig_t * conf)
{
    int V = conf->N;

    if (directVis(p, pp, q, qp, conf)) {
	int *dad = (int *) malloc(sizeof(int) * (V + 2));
	dad[V] = V + 1;
	dad[V + 1] = -1;
	return dad;
    } else {
	array2 wadj = conf->vis;
	wadj[V] = qvis;
	wadj[V + 1] = pvis;
	return (shortestPath(V + 1, V, V + 2, wadj));
    }
}
