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


/************************************************

	Functions for computing the high-dimensional
	embedding and the PCA projection.

************************************************/


#include "dijkstra.h"
#include "bfs.h"
#include "kkutils.h"
#include "embed_graph.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
/* #include <math.h> */

void embed_graph(vtx_data * graph, int n, int dim, DistType *** Coords,
		 int reweight_graph)
{
/* Compute 'dim'-dimensional high-dimensional embedding (HDE) for the 'n' nodes
  The embedding is based on chossing 'dim' pivots, and associating each
  coordinate with a unique pivot, assigning it to the graph-theoretic distances 
  of all nodes from the pivots
*/

    int i, j;
    int node;
    DistType *storage = N_GNEW(n * dim, DistType);
    DistType **coords = *Coords;
    DistType *dist = N_GNEW(n, DistType);	/* this vector stores  the distances of
						   each nodes to the selected "pivots" */
    float *old_weights = graph[0].ewgts;
    Queue Q;
    DistType max_dist = 0;

    if (coords != NULL) {
	free(coords[0]);
	free(coords);
    }

    /* this matrix stores the distance between each node and each "pivot" */
    *Coords = coords = N_GNEW(dim, DistType *);
    for (i = 0; i < dim; i++)
	coords[i] = storage + i * n;

    if (reweight_graph) {
	compute_new_weights(graph, n);
    }

    /* select the first pivot */
    node = rand() % n;

    mkQueue(&Q, n);
    if (reweight_graph) {
	dijkstra(node, graph, n, coords[0]);
    } else {
	bfs(node, graph, n, coords[0], &Q);
    }

    for (i = 0; i < n; i++) {
	dist[i] = coords[0][i];
	if (dist[i] > max_dist) {
	    node = i;
	    max_dist = dist[i];
	}
    }

    /* select other dim-1 nodes as pivots */
    for (i = 1; i < dim; i++) {
	if (reweight_graph) {
	    dijkstra(node, graph, n, coords[i]);
	} else {
	    bfs(node, graph, n, coords[i], &Q);
	}
	max_dist = 0;
	for (j = 0; j < n; j++) {
	    dist[j] = MIN(dist[j], coords[i][j]);
	    if (dist[j] > max_dist) {
		node = j;
		max_dist = dist[j];
	    }
	}

    }

    free(dist);

    if (reweight_graph) {
	restore_old_weights(graph, n, old_weights);
    }

}

 /* Make each axis centered around 0 */
void center_coordinate(DistType ** coords, int n, int dim)
{
    int i, j;
    double sum, avg;
    for (i = 0; i < dim; i++) {
	sum = 0;
	for (j = 0; j < n; j++) {
	    sum += coords[i][j];
	}
	avg = sum / n;
	for (j = 0; j < n; j++) {
	    coords[i][j] -= (DistType) avg;
	}
    }
}
