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


#include "neato.h"
#include "dijkstra.h"
#include "bfs.h"
#include "pca.h"
#include "matrix_ops.h"
#include "conjgrad.h"
#include "embed_graph.h"
#include "kkutils.h"
#include "stress.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>


#ifndef HAVE_DRAND48
extern double drand48(void);
#endif

#define Dij2			/* If defined, the terms in the stress energy are normalized 
				   by d_{ij}^{-2} otherwise, they are normalized by d_{ij}^{-1}
				 */

#ifdef NONCORE
/* Set 'max_nodes_in_mem' so that 
 * 4*(max_nodes_in_mem^2) is smaller than the available memory (in bytes)
 * 4 = sizeof(float)
 */
#define max_nodes_in_mem 18000
#endif

 /* relevant when using sparse distance matrix not within subspace */
#define smooth_pivots true

/* dimensionality of subspace; relevant 
 * when optimizing within subspace) 
 */
#define stress_pca_dim 50

 /* a structure used for storing sparse distance matrix */
typedef struct {
    int nedges;
    int *edges;
    DistType *edist;
    boolean free_mem;
} dist_data;

static double compute_stressf(float **coords, float *lap, int dim, int n, int exp)
{
    /* compute the overall stress */

    int i, j, l, neighbor, count;
    double sum, dist, Dij;
    sum = 0;
    for (count = 0, i = 0; i < n - 1; i++) {
	count++;		/* skip diagonal entry */
	for (j = 1; j < n - i; j++, count++) {
	    dist = 0;
	    neighbor = i + j;
	    for (l = 0; l < dim; l++) {
		dist +=
		    (coords[l][i] - coords[l][neighbor]) * (coords[l][i] -
							    coords[l]
							    [neighbor]);
	    }
	    dist = sqrt(dist);
	    if (exp == 2) {
#ifdef Dij2
		Dij = 1.0 / sqrt(lap[count]);
		sum += (Dij - dist) * (Dij - dist) * (lap[count]);
#else
		Dij = 1.0 / lap[count];
		sum += (Dij - dist) * (Dij - dist) * (lap[count]);
#endif
	    } else {
		Dij = 1.0 / lap[count];
		sum += (Dij - dist) * (Dij - dist) * (lap[count]);
	    }
	}
    }

    return sum;
}

static double
compute_stress1(double **coords, dist_data * distances, int dim, int n, int exp)
{
    /* compute the overall stress */

    int i, j, l, node;
    double sum, dist, Dij;
    sum = 0;
    if (exp == 2) {
	for (i = 0; i < n; i++) {
	    for (j = 0; j < distances[i].nedges; j++) {
		node = distances[i].edges[j];
		if (node <= i) {
		    continue;
		}
		dist = 0;
		for (l = 0; l < dim; l++) {
		    dist +=
			(coords[l][i] - coords[l][node]) * (coords[l][i] -
							    coords[l]
							    [node]);
		}
		dist = sqrt(dist);
		Dij = distances[i].edist[j];
#ifdef Dij2
		sum += (Dij - dist) * (Dij - dist) / (Dij * Dij);
#else
		sum += (Dij - dist) * (Dij - dist) / Dij;
#endif
	    }
	}
    } else {
	for (i = 0; i < n; i++) {
	    for (j = 0; j < distances[i].nedges; j++) {
		node = distances[i].edges[j];
		if (node <= i) {
		    continue;
		}
		dist = 0;
		for (l = 0; l < dim; l++) {
		    dist +=
			(coords[l][i] - coords[l][node]) * (coords[l][i] -
							    coords[l]
							    [node]);
		}
		dist = sqrt(dist);
		Dij = distances[i].edist[j];
		sum += (Dij - dist) * (Dij - dist) / Dij;
	    }
	}
    }

    return sum;
}

/* initLayout:
 * Initialize node coordinates. If the node already has
 * a position, use it.
 * Return true if some node is fixed.
 */
int
initLayout(vtx_data * graph, int n, int dim, double **coords,
	   node_t ** nodes)
{
    node_t *np;
    double *xp;
    double *yp;
    double *pt;
    int i, d;
    int pinned = 0;

    xp = coords[0];
    yp = coords[1];
    for (i = 0; i < n; i++) {
	np = nodes[i];
	if (hasPos(np)) {
	    pt = ND_pos(np);
	    *xp++ = *pt++;
	    *yp++ = *pt++;
	    if (dim > 2) {
		for (d = 2; d < dim; d++)
		    coords[d][i] = *pt++;
	    }
	    if (isFixed(np))
		pinned = 1;
	} else {
	    *xp++ = drand48();
	    *yp++ = drand48();
	    if (dim > 2) {
		for (d = 2; d < dim; d++)
		    coords[d][i] = drand48();
	    }
	}
    }

    for (d = 0; d < dim; d++)
	orthog1(n, coords[d]);

    return pinned;
}

float *circuitModel(vtx_data * graph, int nG)
{
    int i, j, e, rv, count;
    float *Dij = N_NEW(nG * (nG + 1) / 2, float);
    double **Gm;
    double **Gm_inv;

    Gm = new_array(nG, nG, 0.0);
    Gm_inv = new_array(nG, nG, 0.0);

    /* set non-diagonal entries */
    if (graph->ewgts) {
	for (i = 0; i < nG; i++) {
	    for (e = 1; e < graph[i].nedges; e++) {
		j = graph[i].edges[e];
		/* conductance is 1/resistance */
		Gm[i][j] = Gm[j][i] = -1.0 / graph[i].ewgts[e];	/* negate */
	    }
	}
    } else {
	for (i = 0; i < nG; i++) {
	    for (e = 1; e < graph[i].nedges; e++) {
		j = graph[i].edges[e];
		/* conductance is 1/resistance */
		Gm[i][j] = Gm[j][i] = -1.0;	/* ewgts are all 1 */
	    }
	}
    }

    rv = solveCircuit(nG, Gm, Gm_inv);

    if (rv) {
	float v;
	count = 0;
	for (i = 0; i < nG; i++) {
	    for (j = i; j < nG; j++) {
		if (i == j)
		    v = 0.0;
		else
		    v = (float) (Gm_inv[i][i] + Gm_inv[j][j] -
				 2.0 * Gm_inv[i][j]);
		Dij[count++] = v;
	    }
	}
    } else {
	free(Dij);
	Dij = NULL;
    }
    free_array(Gm);
    free_array(Gm_inv);
    return Dij;
}

/* sparse_stress_subspace_majorization_kD:
 * Optimization of the stress function using sparse distance matrix, within a vector-space
 * Fastest and least accurate method
 *
 * NOTE: We use integral shortest path values here, assuming
 * this is only to get an initial layout. In general, if edge lengths
 * are involved, we may end up with 0 length edges. 
 */
static int sparse_stress_subspace_majorization_kD(vtx_data * graph,	/* Input graph in sparse representation */
						  int n,	/* Number of nodes */
						  int nedges_graph,	/* Number of edges */
						  double **coords,	/* coordinates of nodes (output layout)  */
						  int dim,	/* dimemsionality of layout */
						  int smart_ini,	/* smart initialization */
						  int exp,	/* scale exponent */
						  int reweight_graph,	/* difference model */
						  int n_iterations,	/* max #iterations */
						  int dist_bound,	/* neighborhood size in sparse distance matrix    */
						  int num_centers	/* #pivots in sparse distance matrix  */
    )
{
    int iterations;		/* output: number of iteration of the process */

    double conj_tol = tolerance_cg;	/* tolerance of Conjugate Gradient */

	/*************************************************
	** Computation of pivot-based, sparse, subspace-restricted **
	** k-D  stress minimization by majorization                **    
	*************************************************/

    int i, j, k, node;

	/*************************************************
	** First compute the subspace in which we optimize     **
	** The subspace is  the high-dimensional embedding     **
	*************************************************/

    int subspace_dim = MIN(stress_pca_dim, n);	/* overall dimensionality of subspace */
    double **subspace = N_GNEW(subspace_dim, double *);
    double *d_storage = N_GNEW(subspace_dim * n, double);
    int num_centers_local;
    DistType **full_coords;
    /* if i is a pivot than CenterIndex[i] is its index, otherwise CenterIndex[i]= -1 */
    int *CenterIndex;
    int *invCenterIndex;	/* list the pivot nodes  */
    Queue Q;
    float *old_weights;
    /* this matrix stores the distance between  each node and each "center" */
    DistType **Dij;
    /* this vector stores the distances of each node to the selected "centers" */
    DistType *dist;
    DistType max_dist;
    DistType *storage;
    int *visited_nodes;
    dist_data *distances;
    int available_space;
    int *storage1 = NULL;
    DistType *storage2 = NULL;
    int num_visited_nodes;
    int num_neighbors;
    int index;
    int nedges;
    DistType *dist_list;
    vtx_data *lap;
    int *edges;
    float *ewgts;
    double degree;
    double **directions;
    float **tmp_mat;
    float **matrix;
    double dist_ij;
    double *b;
    double *b_restricted;
    double L_ij;
    double old_stress, new_stress;
    boolean converged;

    for (i = 0; i < subspace_dim; i++) {
	subspace[i] = d_storage + i * n;
    }

    /* compute PHDE: */
    num_centers_local = MIN(n, MAX(2 * subspace_dim, 50));
    full_coords = NULL;
    /* High dimensional embedding */
    embed_graph(graph, n, num_centers_local, &full_coords, reweight_graph);
    /* Centering coordinates */
    center_coordinate(full_coords, n, num_centers_local);
    /* PCA */
    PCA_alloc(full_coords, num_centers_local, n, subspace, subspace_dim);

    free(full_coords[0]);
    free(full_coords);

	/*************************************************
	** Compute the sparse-shortest-distances matrix 'distances' **
	*************************************************/

    CenterIndex = N_GNEW(n, int);
    for (i = 0; i < n; i++) {
	CenterIndex[i] = -1;
    }
    invCenterIndex = NULL;

    mkQueue(&Q, n);
    old_weights = graph[0].ewgts;

    if (reweight_graph) {
	/* weight graph to separate high-degree nodes */
	/* in the future, perform slower Dijkstra-based computation */
	compute_new_weights(graph, n);
    }

    /* compute sparse distance matrix */
    /* first select 'num_centers' pivots from which we compute distance */
    /* to all other nodes */

    Dij = NULL;
    dist = N_GNEW(n, DistType);
    if (num_centers == 0) {	/* no pivots, skip pivots-to-nodes distance calculation */
	goto after_pivots_selection;
    }

    invCenterIndex = N_GNEW(num_centers, int);

    storage = N_GNEW(n * num_centers, DistType);
    Dij = N_GNEW(num_centers, DistType *);
    for (i = 0; i < num_centers; i++)
	Dij[i] = storage + i * n;

    /* select 'num_centers' pivots that are uniformaly spreaded over the graph */

    /* the first pivots is selected randomly */
    node = rand() % n;
    CenterIndex[node] = 0;
    invCenterIndex[0] = node;

    if (reweight_graph) {
	dijkstra(node, graph, n, Dij[0]);
    } else {
	bfs(node, graph, n, Dij[0], &Q);
    }

    /* find the most distant node from first pivot */
    max_dist = 0;
    for (i = 0; i < n; i++) {
	dist[i] = Dij[0][i];
	if (dist[i] > max_dist) {
	    node = i;
	    max_dist = dist[i];
	}
    }
    /* select other dim-1 nodes as pivots */
    for (i = 1; i < num_centers; i++) {
	CenterIndex[node] = i;
	invCenterIndex[i] = node;
	if (reweight_graph) {
	    dijkstra(node, graph, n, Dij[i]);
	} else {
	    bfs(node, graph, n, Dij[i], &Q);
	}
	max_dist = 0;
	for (j = 0; j < n; j++) {
	    dist[j] = MIN(dist[j], Dij[i][j]);
	    if (dist[j] > max_dist
		|| (dist[j] == max_dist && rand() % (j + 1) == 0)) {
		node = j;
		max_dist = dist[j];
	    }
	}
    }

  after_pivots_selection:

    /* Construct a sparse distance matrix 'distances' */

    /* initialize dist to -1, important for 'bfs_bounded(..)' */
    for (i = 0; i < n; i++) {
	dist[i] = -1;
    }

    visited_nodes = N_GNEW(n, int);
    distances = N_GNEW(n, dist_data);
    available_space = 0;
    nedges = 0;
    for (i = 0; i < n; i++) {
	if (CenterIndex[i] >= 0) {	/* a pivot node */
	    distances[i].edges = N_GNEW(n - 1, int);
	    distances[i].edist = N_GNEW(n - 1, DistType);
	    distances[i].nedges = n - 1;
	    nedges += n - 1;
	    distances[i].free_mem = TRUE;
	    index = CenterIndex[i];
	    for (j = 0; j < i; j++) {
		distances[i].edges[j] = j;
		distances[i].edist[j] = Dij[index][j];
	    }
	    for (j = i + 1; j < n; j++) {
		distances[i].edges[j - 1] = j;
		distances[i].edist[j - 1] = Dij[index][j];
	    }
	    continue;
	}

	/* a non pivot node */

	if (dist_bound > 0) {
	    if (reweight_graph) {
		num_visited_nodes =
		    dijkstra_bounded(i, graph, n, dist, dist_bound,
				     visited_nodes);
	    } else {
		num_visited_nodes =
		    bfs_bounded(i, graph, n, dist, &Q, dist_bound,
				visited_nodes);
	    }
	    /* filter the pivots out of the visited nodes list, and the self loop: */
	    for (j = 0; j < num_visited_nodes;) {
		if (CenterIndex[visited_nodes[j]] < 0
		    && visited_nodes[j] != i) {
		    /* not a pivot or self loop */
		    j++;
		} else {
		    dist[visited_nodes[j]] = -1;
		    visited_nodes[j] = visited_nodes[--num_visited_nodes];
		}
	    }
	} else {
	    num_visited_nodes = 0;
	}
	num_neighbors = num_visited_nodes + num_centers;
	if (num_neighbors > available_space) {
	    available_space = (dist_bound + 1) * n;
	    storage1 = N_GNEW(available_space, int);
	    storage2 = N_GNEW(available_space, DistType);
	    distances[i].free_mem = TRUE;
	} else {
	    distances[i].free_mem = FALSE;
	}
	distances[i].edges = storage1;
	distances[i].edist = storage2;
	distances[i].nedges = num_neighbors;
	nedges += num_neighbors;
	for (j = 0; j < num_visited_nodes; j++) {
	    storage1[j] = visited_nodes[j];
	    storage2[j] = dist[visited_nodes[j]];
	    dist[visited_nodes[j]] = -1;
	}
	/* add all pivots: */
	for (j = num_visited_nodes; j < num_neighbors; j++) {
	    index = j - num_visited_nodes;
	    storage1[j] = invCenterIndex[index];
	    storage2[j] = Dij[index][i];
	}

	storage1 += num_neighbors;
	storage2 += num_neighbors;
	available_space -= num_neighbors;
    }

    free(dist);
    free(visited_nodes);

    if (Dij != NULL) {
	free(Dij[0]);
	free(Dij);
    }

	/*************************************************
	** Laplacian computation **
	*************************************************/

    lap = N_GNEW(n, vtx_data);
    edges = N_GNEW(nedges + n, int);
    ewgts = N_GNEW(nedges + n, float);
    for (i = 0; i < n; i++) {
	lap[i].edges = edges;
	lap[i].ewgts = ewgts;
	lap[i].nedges = distances[i].nedges + 1;	/*add the self loop */
	dist_list = distances[i].edist - 1;	/* '-1' since edist[0] goes for number '1' entry in the lap */
	degree = 0;
	if (exp == 2) {
	    for (j = 1; j < lap[i].nedges; j++) {
		edges[j] = distances[i].edges[j - 1];
#ifdef Dij2
		ewgts[j] = (float) -1.0 / ((float) dist_list[j] * (float) dist_list[j]);	/* cast to float to prevent overflow */
#else
		ewgts[j] = -1.0 / (float) dist_list[j];
#endif
		degree -= ewgts[j];
	    }
	} else {
	    for (j = 1; j < lap[i].nedges; j++) {
		edges[j] = distances[i].edges[j - 1];
		ewgts[j] = -1.0 / (float) dist_list[j];
		degree -= ewgts[j];
	    }
	}
	edges[0] = i;
	ewgts[0] = (float) degree;
	edges += lap[i].nedges;
	ewgts += lap[i].nedges;
    }

	/*************************************************
	** initialize direction vectors  **
	** to get an intial layout       **
	*************************************************/

    /* the layout is subspace*directions */
    directions = N_GNEW(dim, double *);
    directions[0] = N_GNEW(dim * subspace_dim, double);
    for (i = 1; i < dim; i++) {
	directions[i] = directions[0] + i * subspace_dim;
    }

    if (smart_ini) {
	/* smart initialization */
	for (k = 0; k < dim; k++) {
	    for (i = 0; i < subspace_dim; i++) {
		directions[k][i] = 0;
	    }
	}
	if (dim != 2) {
	    /* use the first vectors in the eigenspace */
	    /* each direction points to its "principal axes" */
	    for (k = 0; k < dim; k++) {
		directions[k][k] = 1;
	    }
	} else {
	    /* for the frequent 2-D case we prefer iterative-PCA over PCA */
	    /* Note that we don't want to mix the Lap's eigenspace with the HDE */
	    /* in the computation since they have different scales */

	    directions[0][0] = 1;	/* first pca projection vector */
	    if (!iterativePCA_1D(subspace, subspace_dim, n, directions[1])) {
		for (k = 0; k < subspace_dim; k++) {
		    directions[1][k] = 0;
		}
		directions[1][1] = 1;
	    }
	}

    } else {
	/* random initialization */
	for (k = 0; k < dim; k++) {
	    for (i = 0; i < subspace_dim; i++) {
		directions[k][i] = (double) (rand()) / RAND_MAX;
	    }
	}
    }

    /* compute initial k-D layout */

    for (k = 0; k < dim; k++) {
	right_mult_with_vector_transpose(subspace, n, subspace_dim,
					 directions[k], coords[k]);
    }

	/*************************************************
	** compute restriction of the laplacian to subspace: **	
	*************************************************/

    tmp_mat = NULL;
    matrix = NULL;
    mult_sparse_dense_mat_transpose(lap, subspace, n, subspace_dim,
				    &tmp_mat);
    mult_dense_mat(subspace, tmp_mat, subspace_dim, n, subspace_dim,
		   &matrix);
    free(tmp_mat[0]);
    free(tmp_mat);

	/*************************************************
	** Layout optimization  **
	*************************************************/

    b = N_GNEW(n, double);
    b_restricted = N_GNEW(subspace_dim, double);
    old_stress = compute_stress1(coords, distances, dim, n, exp);
    for (converged = FALSE, iterations = 0;
	 iterations < n_iterations && !converged; iterations++) {

	/* Axis-by-axis optimization: */
	for (k = 0; k < dim; k++) {
	    /* compute the vector b */
	    /* multiply on-the-fly with distance-based laplacian */
	    /* (for saving storage we don't construct this Lap explicitly) */
	    for (i = 0; i < n; i++) {
		degree = 0;
		b[i] = 0;
		dist_list = distances[i].edist - 1;
		edges = lap[i].edges;
		ewgts = lap[i].ewgts;
		for (j = 1; j < lap[i].nedges; j++) {
		    node = edges[j];
		    dist_ij = distance_kD(coords, dim, i, node);
		    if (dist_ij > 1e-30) {	/* skip zero distances */
			L_ij = -ewgts[j] * dist_list[j] / dist_ij;	/* L_ij=w_{ij}*d_{ij}/dist_{ij} */
			degree -= L_ij;
			b[i] += L_ij * coords[k][node];
		    }
		}
		b[i] += degree * coords[k][i];
	    }
	    right_mult_with_vector_d(subspace, subspace_dim, n, b,
				     b_restricted);
	    if (conjugate_gradient_f(matrix, directions[k], b_restricted,
				 subspace_dim, conj_tol, subspace_dim,
				 FALSE)) {
		iterations = -1;
		goto finish0;
	    }
	    right_mult_with_vector_transpose(subspace, n, subspace_dim,
					     directions[k], coords[k]);
	}

	if ((converged = (iterations % 2 == 0))) {	/* check for convergence each two iterations */
	    new_stress = compute_stress1(coords, distances, dim, n, exp);
	    converged =
		fabs(new_stress - old_stress) / (new_stress + 1e-10) <
		Epsilon;
	    old_stress = new_stress;
	}
    }
finish0:
    free(b_restricted);
    free(b);

    if (reweight_graph) {
	restore_old_weights(graph, n, old_weights);
    }

    for (i = 0; i < n; i++) {
	if (distances[i].free_mem) {
	    free(distances[i].edges);
	    free(distances[i].edist);
	}
    }

    free(distances);
    free(lap[0].edges);
    free(lap[0].ewgts);
    free(lap);
    free(CenterIndex);
    free(invCenterIndex);
    free(directions[0]);
    free(directions);
    if (matrix != NULL) {
	free(matrix[0]);
	free(matrix);
    }
    free(subspace[0]);
    free(subspace);
    freeQueue(&Q);

    return iterations;
}

/* compute_weighted_apsp_packed:
 * Edge lengths can be any float > 0
 */
static float *compute_weighted_apsp_packed(vtx_data * graph, int n)
{
    int i, j, count;
    float *Dij = N_NEW(n * (n + 1) / 2, float);

    float *Di = N_NEW(n, float);
    Queue Q;

    mkQueue(&Q, n);

    count = 0;
    for (i = 0; i < n; i++) {
	dijkstra_f(i, graph, n, Di);
	for (j = i; j < n; j++) {
	    Dij[count++] = Di[j];
	}
    }
    free(Di);
    freeQueue(&Q);
    return Dij;
}


/* mdsModel:
 * Update matrix with actual edge lengths
 */
float *mdsModel(vtx_data * graph, int nG)
{
    int i, j, e;
    float *Dij;
    int shift = 0;
    double delta;

    if (graph->ewgts == NULL)
	return 0;

    /* first, compute shortest paths to fill in non-edges */
    Dij = compute_weighted_apsp_packed(graph, nG);

    /* then, replace edge entries will user-supplied len */
    for (i = 0; i < nG; i++) {
	shift += i;
	for (e = 1; e < graph[i].nedges; e++) {
	    j = graph[i].edges[e];
	    if (j < i)
		continue;
	    delta += abs(Dij[i * nG + j - shift] - graph[i].ewgts[e]);
	    Dij[i * nG + j - shift] = graph[i].ewgts[e];
	}
    }
    if (Verbose) {
	fprintf(stderr, "mdsModel: delta = %f\n", delta);
    }
    return Dij;
}

/* compute_apsp_packed:
 * Assumes integral weights > 0.
 */
float *compute_apsp_packed(vtx_data * graph, int n)
{
    int i, j, count;
    float *Dij = N_NEW(n * (n + 1) / 2, float);

    DistType *Di = N_NEW(n, DistType);
    Queue Q;

    mkQueue(&Q, n);

    count = 0;
    for (i = 0; i < n; i++) {
	bfs(i, graph, n, Di, &Q);
	for (j = i; j < n; j++) {
	    Dij[count++] = ((float) Di[j]);
	}
    }
    free(Di);
    freeQueue(&Q);
    return Dij;
}

#define max(x,y) ((x)>(y)?(x):(y))

float *compute_apsp_artifical_weights_packed(vtx_data * graph, int n)
{
    /* compute all-pairs-shortest-path-length while weighting the graph */
    /* so high-degree nodes are distantly located */

    float *Dij;
    int i, j;
    float *old_weights = graph[0].ewgts;
    int nedges = 0;
    float *weights;
    int *vtx_vec;
    int deg_i, deg_j, neighbor;

    for (i = 0; i < n; i++) {
	nedges += graph[i].nedges;
    }

    weights = N_NEW(nedges, float);
    vtx_vec = N_NEW(n, int);
    for (i = 0; i < n; i++) {
	vtx_vec[i] = 0;
    }

    if (graph->ewgts) {
	for (i = 0; i < n; i++) {
	    fill_neighbors_vec_unweighted(graph, i, vtx_vec);
	    deg_i = graph[i].nedges - 1;
	    for (j = 1; j <= deg_i; j++) {
		neighbor = graph[i].edges[j];
		deg_j = graph[neighbor].nedges - 1;
		weights[j] = (float)
		    max((float)
			(deg_i + deg_j -
			 2 * common_neighbors(graph, i, neighbor,
					      vtx_vec)),
			graph[i].ewgts[j]);
	    }
	    empty_neighbors_vec(graph, i, vtx_vec);
	    graph[i].ewgts = weights;
	    weights += graph[i].nedges;
	}
	Dij = compute_weighted_apsp_packed(graph, n);
    } else {
	for (i = 0; i < n; i++) {
	    graph[i].ewgts = weights;
	    fill_neighbors_vec_unweighted(graph, i, vtx_vec);
	    deg_i = graph[i].nedges - 1;
	    for (j = 1; j <= deg_i; j++) {
		neighbor = graph[i].edges[j];
		deg_j = graph[neighbor].nedges - 1;
		weights[j] =
		    ((float) deg_i + deg_j -
		     2 * common_neighbors(graph, i, neighbor, vtx_vec));
	    }
	    empty_neighbors_vec(graph, i, vtx_vec);
	    weights += graph[i].nedges;
	}
	Dij = compute_apsp_packed(graph, n);
    }

    free(vtx_vec);
    free(graph[0].ewgts);
    graph[0].ewgts = NULL;
    if (old_weights != NULL) {
	for (i = 0; i < n; i++) {
	    graph[i].ewgts = old_weights;
	    old_weights += graph[i].nedges;
	}
    }
    return Dij;
}

#ifdef DEBUG
static void dumpMatrix(float *Dij, int n)
{
    int i, j, count = 0;
    for (i = 0; i < n; i++) {
	for (j = i; j < n; j++) {
	    fprintf(stderr, "%.02f  ", Dij[count++]);
	}
	fputs("\n", stderr);
    }
}
#endif

/* Accumulator type for diagonal of Laplacian. Needs to be as large
 * as possible. Use long double; configure to double if necessary.
 */
#define DegType long double

/* stress_majorization_kD_mkernel:
 * At present, if any nodes have pos set, smart_ini is false.
 */
int stress_majorization_kD_mkernel(vtx_data * graph,	/* Input graph in sparse representation */
				   int n,	/* Number of nodes */
				   int nedges_graph,	/* Number of edges */
				   double **d_coords,	/* coordinates of nodes (output layout) */
				   node_t ** nodes,	/* original nodes */
				   int dim,	/* dimemsionality of layout */
				   int opts,    /* options */
				   int model,	/* model */
				   int maxi	/* max iterations */
    )
{
    int iterations;		/* output: number of iteration of the process */

    double conj_tol = tolerance_cg;	/* tolerance of Conjugate Gradient */
    float *Dij = NULL;
    int i, j, k;
    float **coords = NULL;
    float *f_storage = NULL;
    float constant_term;
    int count;
    DegType degree;
    int lap_length;
    float *lap2 = NULL;
    DegType *degrees = NULL;
    int step;
    float val;
    double old_stress, new_stress;
    boolean converged;
    float **b = NULL;
    float *tmp_coords = NULL;
    float *dist_accumulator = NULL;
    float *lap1 = NULL;
    int smart_ini = opts & opt_smart_init;
    int exp = opts & opt_exp_flag;
    int len;
    int havePinned;		/* some node is pinned */
#ifdef ALTERNATIVE_STRESS_CALC
    double mat_stress;
#endif
#ifdef NONCORE
    FILE *fp = NULL;
#endif


	/*************************************************
	** Computation of full, dense, unrestricted k-D ** 
	** stress minimization by majorization          **    
	*************************************************/

	/****************************************************
	** Compute the all-pairs-shortest-distances matrix **
	****************************************************/

    if (maxi < 0)
	return 0;

    if (Verbose)
	start_timer();

    if (model == MODEL_SUBSET) {
	/* weight graph to separate high-degree nodes */
	/* and perform slower Dijkstra-based computation */
	if (Verbose)
	    fprintf(stderr, "Calculating subset model");
	Dij = compute_apsp_artifical_weights_packed(graph, n);
    } else if (model == MODEL_CIRCUIT) {
	Dij = circuitModel(graph, n);
	if (!Dij) {
	    agerr(AGWARN,
		  "graph is disconnected. Hence, the circuit model\n");
	    agerr(AGPREV,
		  "is undefined. Reverting to the shortest path model.\n");
	}
    } else if (model == MODEL_MDS) {
	if (Verbose)
	    fprintf(stderr, "Calculating MDS model");
	Dij = mdsModel(graph, n);
    }
    if (!Dij) {
	if (Verbose)
	    fprintf(stderr, "Calculating shortest paths");
	if (graph->ewgts)
	    Dij = compute_weighted_apsp_packed(graph, n);
	else
	    Dij = compute_apsp_packed(graph, n);
    }

    if (Verbose) {
	fprintf(stderr, ": %.2f sec\n", elapsed_sec());
	fprintf(stderr, "Setting initial positions");
	start_timer();
    }

	/**************************
	** Layout initialization **
	**************************/

    if (smart_ini && (n > 1)) {
	havePinned = 0;
	/* optimize layout quickly within subspace */
	/* perform at most 50 iterations within 30-D subspace to 
	   get an estimate */
	if (sparse_stress_subspace_majorization_kD(graph, n, nedges_graph,
					       d_coords, dim, smart_ini, exp,
					       (model == MODEL_SUBSET), 50,
					       neighborhood_radius_subspace,
					       num_pivots_stress) < 0) {
	    iterations = -1;
	    goto finish1;
	}

	for (i = 0; i < dim; i++) {
	    /* for numerical stability, scale down layout */
	    double max = 1;
	    for (j = 0; j < n; j++) {
		if (fabs(d_coords[i][j]) > max) {
		    max = fabs(d_coords[i][j]);
		}
	    }
	    for (j = 0; j < n; j++) {
		d_coords[i][j] /= max;
	    }
	    /* add small random noise */
	    for (j = 0; j < n; j++) {
		d_coords[i][j] += 1e-6 * (drand48() - 0.5);
	    }
	    orthog1(n, d_coords[i]);
	}
    } else {
	havePinned = initLayout(graph, n, dim, d_coords, nodes);
    }
    if (Verbose)
	fprintf(stderr, ": %.2f sec", elapsed_sec());
    if ((n == 1) || (maxi == 0))
	return 0;

    if (Verbose) {
	fprintf(stderr, ": %.2f sec\n", elapsed_sec());
	fprintf(stderr, "Setting up stress function");
	start_timer();
    }
    coords = N_NEW(dim, float *);
    f_storage = N_NEW(dim * n, float);
    for (i = 0; i < dim; i++) {
	coords[i] = f_storage + i * n;
	for (j = 0; j < n; j++) {
	    coords[i][j] = ((float) d_coords[i][j]);
	}
    }

    /* compute constant term in stress sum */
    /* which is \sum_{i<j} w_{ij}d_{ij}^2 */
    if (exp) {
#ifdef Dij2
	constant_term = ((float) n * (n - 1) / 2);
#else
	constant_term = 0;
	for (count = 0, i = 0; i < n - 1; i++) {
	    count++;		/* skip self distance */
	    for (j = 1; j < n - i; j++, count++) {
		constant_term += Dij[count];
	    }
	}
#endif
    } else {
	constant_term = 0;
	for (count = 0, i = 0; i < n - 1; i++) {
	    count++;		/* skip self distance */
	    for (j = 1; j < n - i; j++, count++) {
		constant_term += Dij[count];
	    }
	}
    }

	/**************************
	** Laplacian computation **
	**************************/

    lap_length = n * (n + 1) / 2;
    lap2 = Dij;
    if (exp == 2) {
#ifdef Dij2
    square_vec(lap_length, lap2);
#endif
    }
    /* compute off-diagonal entries */
    invert_vec(lap_length, lap2);

    /* compute diagonal entries */
    count = 0;
    degrees = N_NEW(n, DegType);
    /* set_vector_val(n, 0, degrees); */
    memset(degrees, 0, n * sizeof(DegType));
    for (i = 0; i < n - 1; i++) {
	degree = 0;
	count++;		/* skip main diag entry */
	for (j = 1; j < n - i; j++, count++) {
	    val = lap2[count];
	    degree += val;
	    degrees[i + j] -= val;
	}
	degrees[i] -= degree;
    }
    for (step = n, count = 0, i = 0; i < n; i++, count += step, step--) {
	lap2[count] = degrees[i];
    }

#ifdef NONCORE
    if (n > max_nodes_in_mem) {
#define FILENAME "tmp_Dij$$$.bin"
	fp = fopen(FILENAME, "wb");
	fwrite(lap2, sizeof(float), lap_length, fp);
	fclose(fp);
	fp = NULL;
    }
#endif

	/*************************
	** Layout optimization  **
	*************************/

    b = N_NEW(dim, float *);
    b[0] = N_NEW(dim * n, float);
    for (k = 1; k < dim; k++) {
	b[k] = b[0] + k * n;
    }

    tmp_coords = N_NEW(n, float);
    dist_accumulator = N_NEW(n, float);
    lap1 = NULL;
#ifdef NONCORE
    if (n <= max_nodes_in_mem) {
	lap1 = N_NEW(lap_length, float);
    } else {
	lap1 = lap2;
	fp = fopen(FILENAME, "rb");
	fgetpos(fp, &pos);
    }
#else
    lap1 = N_NEW(lap_length, float);
#endif


#ifdef USE_MAXFLOAT
    old_stress = MAXFLOAT;	/* at least one iteration */
#else
    old_stress = MAXDOUBLE;	/* at least one iteration */
#endif
    if (Verbose) {
	fprintf(stderr, ": %.2f sec\n", elapsed_sec());
	fprintf(stderr, "Solving model: ");
	start_timer();
    }

    for (converged = FALSE, iterations = 0;
	 iterations < maxi && !converged; iterations++) {

	/* First, construct Laplacian of 1/(d_ij*|p_i-p_j|)  */
	/* set_vector_val(n, 0, degrees); */
	memset(degrees, 0, n * sizeof(DegType));
	if (exp == 2) {
#ifdef Dij2
#ifdef NONCORE
	    if (n <= max_nodes_in_mem) {
		sqrt_vecf(lap_length, lap2, lap1);
	    } else {
		sqrt_vec(lap_length, lap1);
	    }
#else
	    sqrt_vecf(lap_length, lap2, lap1);
#endif
#endif
	}
	for (count = 0, i = 0; i < n - 1; i++) {
	    len = n - i - 1;
	    /* init 'dist_accumulator' with zeros */
	    set_vector_valf(len, 0, dist_accumulator);

	    /* put into 'dist_accumulator' all squared distances between 'i' and 'i'+1,...,'n'-1 */
	    for (k = 0; k < dim; k++) {
		set_vector_valf(len, coords[k][i], tmp_coords);
		vectors_mult_additionf(len, tmp_coords, -1,
				       coords[k] + i + 1);
		square_vec(len, tmp_coords);
		vectors_additionf(len, tmp_coords, dist_accumulator,
				  dist_accumulator);
	    }

	    /* convert to 1/d_{ij} */
	    invert_sqrt_vec(len, dist_accumulator);
	    /* detect overflows */
	    for (j = 0; j < len; j++) {
		if (dist_accumulator[j] >= MAXFLOAT
		    || dist_accumulator[j] < 0) {
		    dist_accumulator[j] = 0;
		}
	    }

	    count++;		/* save place for the main diagonal entry */
	    degree = 0;
	    if (exp == 2) {
		for (j = 0; j < len; j++, count++) {
#ifdef Dij2
		    val = lap1[count] *= dist_accumulator[j];
#else
		    val = lap1[count] = dist_accumulator[j];
#endif
		    degree += val;
		    degrees[i + j + 1] -= val;
		}
	    } else {
		for (j = 0; j < len; j++, count++) {
		    val = lap1[count] = dist_accumulator[j];
		    degree += val;
		    degrees[i + j + 1] -= val;
		}
	    }
	    degrees[i] -= degree;
	}
	for (step = n, count = 0, i = 0; i < n; i++, count += step, step--) {
	    lap1[count] = degrees[i];
	}

	/* Now compute b[] */
	for (k = 0; k < dim; k++) {
	    /* b[k] := lap1*coords[k] */
	    right_mult_with_vector_ff(lap1, n, coords[k], b[k]);
	}


	/* compute new stress  */
	/* remember that the Laplacians are negated, so we subtract instead of add and vice versa */
	new_stress = 0;
	for (k = 0; k < dim; k++) {
	    new_stress += vectors_inner_productf(n, coords[k], b[k]);
	}
	new_stress *= 2;
	new_stress += constant_term;	/* only after mult by 2 */
#ifdef NONCORE
	if (n > max_nodes_in_mem) {
	    /* restore lap2 from memory */
	    fsetpos(fp, &pos);
	    fread(lap2, sizeof(float), lap_length, fp);
	}
#endif
	for (k = 0; k < dim; k++) {
	    right_mult_with_vector_ff(lap2, n, coords[k], tmp_coords);
	    new_stress -= vectors_inner_productf(n, coords[k], tmp_coords);
	}
#ifdef ALTERNATIVE_STRESS_CALC
	mat_stress = new_stress;
	new_stress = compute_stressf(coords, lap2, dim, n);
	if (fabs(mat_stress - new_stress) / min(mat_stress, new_stress) >
	    0.001) {
	    fprintf(stderr, "Diff stress vals: %lf %lf (iteration #%d)\n",
		    mat_stress, new_stress, iterations);
	}
#endif
	/* Invariant: old_stress > 0. In theory, old_stress >= new_stress
	 * but we use fabs in case of numerical error.
	 */
	{
	    double diff = old_stress - new_stress;
	    double change = ABS(diff);
	    converged = (((change / old_stress) < Epsilon)
			 || (new_stress < Epsilon));
	}
	old_stress = new_stress;

	for (k = 0; k < dim; k++) {
	    node_t *np;
	    if (havePinned) {
		copy_vectorf(n, coords[k], tmp_coords);
		if (conjugate_gradient_mkernel(lap2, tmp_coords, b[k], n,
					   conj_tol, n) < 0) {
		    iterations = -1;
		    goto finish1;
		}
		for (i = 0; i < n; i++) {
		    np = nodes[i];
		    if (!isFixed(np))
			coords[k][i] = tmp_coords[i];
		}
	    } else {
		if (conjugate_gradient_mkernel(lap2, coords[k], b[k], n,
					   conj_tol, n) < 0) {
		    iterations = -1;
		    goto finish1;
		}
	    }
	}
	if (Verbose && (iterations % 5 == 0)) {
	    fprintf(stderr, "%.3f ", new_stress);
	    if ((iterations + 5) % 50 == 0)
		fprintf(stderr, "\n");
	}
    }
    if (Verbose) {
	fprintf(stderr, "\nfinal e = %f %d iterations %.2f sec\n",
		compute_stressf(coords, lap2, dim, n, exp),
		iterations, elapsed_sec());
    }

    for (i = 0; i < dim; i++) {
	for (j = 0; j < n; j++) {
	    d_coords[i][j] = coords[i][j];
	}
    }
#ifdef NONCORE
    if (fp)
	fclose(fp);
#endif
finish1:
    free(f_storage);
    free(coords);

    free(lap2);
    if (b) {
	free(b[0]);
	free(b);
    }
    free(tmp_coords);
    free(dist_accumulator);
    free(degrees);
    free(lap1);
    return iterations;
}
