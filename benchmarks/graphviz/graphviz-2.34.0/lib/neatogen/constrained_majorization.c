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

#include "digcola.h"
#ifdef DIGCOLA
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <float.h>
#include "stress.h"
#include "dijkstra.h"
#include "bfs.h"
#include "matrix_ops.h"
#include "kkutils.h"
#include "conjgrad.h"
#include "quad_prog_solver.h"
#include "matrix_ops.h"

#define localConstrMajorIterations 15
#define levels_sep_tol 1e-1

int stress_majorization_with_hierarchy(vtx_data * graph,	/* Input graph in sparse representation  */
				       int n,	/* Number of nodes */
				       int nedges_graph,	/* Number of edges */
				       double **d_coords,	/* Coordinates of nodes (output layout)  */
				       node_t ** nodes,	/* Original nodes */
				       int dim,	/* Dimemsionality of layout */
				       int opts,	/* options */
				       int model,	/* difference model */
				       int maxi,	/* max iterations */
				       double levels_gap)
{
    int iterations = 0;		/* Output: number of iteration of the process */

	/*************************************************
	** Computation of full, dense, unrestricted k-D ** 
	** stress minimization by majorization          ** 
	** This function imposes HIERARCHY CONSTRAINTS  **
	*************************************************/

    int i, j, k;
    boolean directionalityExist = FALSE;
    float *lap1 = NULL;
    float *dist_accumulator = NULL;
    float *tmp_coords = NULL;
    float **b = NULL;
#ifdef NONCORE
    FILE *fp = NULL;
#endif
    double *degrees = NULL;
    float *lap2 = NULL;
    int lap_length;
    float *f_storage = NULL;
    float **coords = NULL;

    double conj_tol = tolerance_cg;	/* tolerance of Conjugate Gradient */
    float **unpackedLap = NULL;
    CMajEnv *cMajEnv = NULL;
    double y_0;
    int length;
    int smart_ini = opts & opt_smart_init;
    DistType diameter;
    float *Dij = NULL;
    /* to compensate noises, we never consider gaps smaller than 'abs_tol' */
    double abs_tol = 1e-2;
    /* Additionally, we never consider gaps smaller than 'abs_tol'*<avg_gap> */
    double relative_tol = levels_sep_tol;
    int *ordering = NULL, *levels = NULL;
    float constant_term;
    int count;
    double degree;
    int step;
    float val;
    double old_stress, new_stress;
    boolean converged;
    int len;
    int num_levels;
    float *hierarchy_boundaries;

    if (graph[0].edists != NULL) {
	for (i = 0; i < n; i++) {
	    for (j = 1; j < graph[i].nedges; j++) {
		directionalityExist = directionalityExist
		    || (graph[i].edists[j] != 0);
	    }
	}
    }
    if (!directionalityExist) {
	return stress_majorization_kD_mkernel(graph, n, nedges_graph,
					      d_coords, nodes, dim, opts,
					      model, maxi);
    }

	/******************************************************************
	** First, partition nodes into layers: These are our constraints **
	******************************************************************/

    if (smart_ini) {
	double *x;
	double *y;
	if (dim > 2) {
	    /* the dim==2 case is handled below                      */
	    if (stress_majorization_kD_mkernel(graph, n, nedges_graph,
					   d_coords + 1, nodes, dim - 1,
					   opts, model, 15) < 0)
		return -1;
	    /* now copy the y-axis into the (dim-1)-axis */
	    for (i = 0; i < n; i++) {
		d_coords[dim - 1][i] = d_coords[1][i];
	    }
	}

	x = d_coords[0];
	y = d_coords[1];
	if (compute_y_coords(graph, n, y, n)) {
	    iterations = -1;
	    goto finish;
	}
	if (compute_hierarchy(graph, n, abs_tol, relative_tol, y, &ordering,
			  &levels, &num_levels)) {
	    iterations = -1;
	    goto finish;
	}
	if (num_levels < 1) {
	    /* no hierarchy found, use faster algorithm */
	    return stress_majorization_kD_mkernel(graph, n, nedges_graph,
						  d_coords, nodes, dim,
						  opts, model, maxi);
	}

	if (levels_gap > 0) {
	    /* ensure that levels are separated in the initial layout */
	    double displacement = 0;
	    int stop;
	    for (i = 0; i < num_levels; i++) {
		displacement +=
		    MAX((double) 0,
			levels_gap - (y[ordering[levels[i]]] +
				      displacement -
				      y[ordering[levels[i] - 1]]));
		stop = i < num_levels - 1 ? levels[i + 1] : n;
		for (j = levels[i]; j < stop; j++) {
		    y[ordering[j]] += displacement;
		}
	    }
	}
	if (dim == 2) {
	    if (IMDS_given_dim(graph, n, y, x, Epsilon)) {
		iterations = -1;
		goto finish;
	    }
	}
    } else {
	initLayout(graph, n, dim, d_coords, nodes);
	if (compute_hierarchy(graph, n, abs_tol, relative_tol, NULL, &ordering,
			  &levels, &num_levels)) {
	    iterations = -1;
	    goto finish;
	}
    }
    if (n == 1)
	return 0;

    hierarchy_boundaries = N_GNEW(num_levels, float);

	/****************************************************
	** Compute the all-pairs-shortest-distances matrix **
	****************************************************/

    if (maxi == 0)
	return iterations;

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
	Dij = compute_apsp_packed(graph, n);
    }
    if (Verbose) {
	fprintf(stderr, ": %.2f sec\n", elapsed_sec());
	fprintf(stderr, "Setting initial positions");
	start_timer();
    }

    diameter = -1;
    length = n + n * (n - 1) / 2;
    for (i = 0; i < length; i++) {
	if (Dij[i] > diameter) {
	    diameter = (int) Dij[i];
	}
    }

    if (!smart_ini) {
	/* for numerical stability, scale down layout            */
	/* No Jiggling, might conflict with constraints                  */
	double max = 1;
	for (i = 0; i < dim; i++) {
	    for (j = 0; j < n; j++) {
		if (fabs(d_coords[i][j]) > max) {
		    max = fabs(d_coords[i][j]);
		}
	    }
	}
	for (i = 0; i < dim; i++) {
	    for (j = 0; j < n; j++) {
		d_coords[i][j] *= 10 / max;
	    }
	}
    }

    if (levels_gap > 0) {
	int length = n + n * (n - 1) / 2;
	double sum1, sum2, scale_ratio;
	int count;
	sum1 = (float) (n * (n - 1) / 2);
	sum2 = 0;
	for (count = 0, i = 0; i < n - 1; i++) {
	    count++;		// skip self distance
	    for (j = i + 1; j < n; j++, count++) {
		sum2 += distance_kD(d_coords, dim, i, j) / Dij[count];
	    }
	}
	scale_ratio = sum2 / sum1;
	/* double scale_ratio=10; */
	for (i = 0; i < length; i++) {
	    Dij[i] *= (float) scale_ratio;
	}
    }

	/**************************
	** Layout initialization **
	**************************/

    for (i = 0; i < dim; i++) {
	orthog1(n, d_coords[i]);
    }

    /* for the y-coords, don't center them, but translate them so y[0]=0 */
    y_0 = d_coords[1][0];
    for (i = 0; i < n; i++) {
	d_coords[1][i] -= y_0;
    }

    coords = N_GNEW(dim, float *);
    f_storage = N_GNEW(dim * n, float);
    for (i = 0; i < dim; i++) {
	coords[i] = f_storage + i * n;
	for (j = 0; j < n; j++) {
	    coords[i][j] = (float) (d_coords[i][j]);
	}
    }

    /* compute constant term in stress sum
     * which is \sum_{i<j} w_{ij}d_{ij}^2
     */
    constant_term = (float) (n * (n - 1) / 2);

    if (Verbose)
	fprintf(stderr, ": %.2f sec", elapsed_sec());

	/**************************
	** Laplacian computation **
	**************************/

    lap2 = Dij;
    lap_length = n + n * (n - 1) / 2;
    square_vec(lap_length, lap2);
    /* compute off-diagonal entries */
    invert_vec(lap_length, lap2);

    /* compute diagonal entries */
    count = 0;
    degrees = N_GNEW(n, double);
    set_vector_val(n, 0, degrees);
    for (i = 0; i < n - 1; i++) {
	degree = 0;
	count++;		// skip main diag entry
	for (j = 1; j < n - i; j++, count++) {
	    val = lap2[count];
	    degree += val;
	    degrees[i + j] -= val;
	}
	degrees[i] -= degree;
    }
    for (step = n, count = 0, i = 0; i < n; i++, count += step, step--) {
	lap2[count] = (float) degrees[i];
    }

#ifdef NONCORE
    fpos_t pos;
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

    b = N_GNEW(dim, float *);
    b[0] = N_GNEW(dim * n, float);
    for (k = 1; k < dim; k++) {
	b[k] = b[0] + k * n;
    }

    tmp_coords = N_GNEW(n, float);
    dist_accumulator = N_GNEW(n, float);
#ifdef NONCORE
    if (n <= max_nodes_in_mem) {
#endif
	lap1 = N_GNEW(lap_length, float);
#ifdef NONCORE
    } else {
	lap1 = lap2;
	fp = fopen(FILENAME, "rb");
	fgetpos(fp, &pos);
    }
#endif

    old_stress = DBL_MAX;	/* at least one iteration */

    unpackedLap = unpackMatrix(lap2, n);
    cMajEnv =
	initConstrainedMajorization(lap2, n, ordering, levels, num_levels);

    for (converged = FALSE, iterations = 0;
	 iterations < maxi && !converged; iterations++) {

	/* First, construct Laplacian of 1/(d_ij*|p_i-p_j|)  */
	set_vector_val(n, 0, degrees);
#ifdef NONCORE
	if (n <= max_nodes_in_mem) {
#endif
	    sqrt_vecf(lap_length, lap2, lap1);
#ifdef NONCORE
	} else {
	    sqrt_vec(lap_length, lap1);
	}
#endif
	for (count = 0, i = 0; i < n - 1; i++) {
	    len = n - i - 1;
	    /* init 'dist_accumulator' with zeros */
	    set_vector_valf(n, 0, dist_accumulator);

	    /* put into 'dist_accumulator' all squared distances 
	     * between 'i' and 'i'+1,...,'n'-1
	     */
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
		if (dist_accumulator[j] >= FLT_MAX
		    || dist_accumulator[j] < 0) {
		    dist_accumulator[j] = 0;
		}
	    }

	    count++;		/* save place for the main diagonal entry */
	    degree = 0;
	    for (j = 0; j < len; j++, count++) {
		val = lap1[count] *= dist_accumulator[j];
		degree += val;
		degrees[i + j + 1] -= val;
	    }
	    degrees[i] -= degree;
	}
	for (step = n, count = 0, i = 0; i < n; i++, count += step, step--) {
	    lap1[count] = (float) degrees[i];
	}

	/* Now compute b[] (L^(X(t))*X(t)) */
	for (k = 0; k < dim; k++) {
	    /* b[k] := lap1*coords[k] */
	    right_mult_with_vector_ff(lap1, n, coords[k], b[k]);
	}

	/* compute new stress
	 * remember that the Laplacians are negated, so we subtract 
	 * instead of add and vice versa
	 */
	new_stress = 0;
	for (k = 0; k < dim; k++) {
	    new_stress += vectors_inner_productf(n, coords[k], b[k]);
	}
	new_stress *= 2;
	new_stress += constant_term;	// only after mult by 2              
#ifdef NONCORE
	if (n > max_nodes_in_mem) {
	    /* restore lap2 from disk */
	    fsetpos(fp, &pos);
	    fread(lap2, sizeof(float), lap_length, fp);
	}
#endif
	for (k = 0; k < dim; k++) {
	    right_mult_with_vector_ff(lap2, n, coords[k], tmp_coords);
	    new_stress -= vectors_inner_productf(n, coords[k], tmp_coords);
	}

#ifdef ALTERNATIVE_STRESS_CALC
	{
	    double mat_stress = new_stress;
	    double compute_stress(float **coords, float *lap, int dim,
				  int n);
	    new_stress = compute_stress(coords, lap2, dim, n);
	    if (fabs(mat_stress - new_stress) /
		min(mat_stress, new_stress) > 0.001) {
		fprintf(stderr,
			"Diff stress vals: %lf %lf (iteration #%d)\n",
			mat_stress, new_stress, iterations);
	    }
	}
#endif
	/* check for convergence */
	converged =
	    fabs(new_stress - old_stress) / fabs(old_stress + 1e-10) <
	    Epsilon;
	converged = converged || (iterations > 1
				  && new_stress > old_stress);
	/* in first iteration we allowed stress increase, which 
	 * might result ny imposing constraints
	 */
	old_stress = new_stress;

	for (k = 0; k < dim; k++) {
	    /* now we find the optimizer of trace(X'LX)+X'B by solving 'dim' 
	     * system of equations, thereby obtaining the new coordinates.
	     * If we use the constraints (given by the var's: 'ordering', 
	     * 'levels' and 'num_levels'), we cannot optimize 
	     * trace(X'LX)+X'B by simply solving equations, but we have
	     * to use a quadratic programming solver
	     * note: 'lap2' is a packed symmetric matrix, that is its 
	     * upper-triangular part is arranged in a vector row-wise
	     * also note: 'lap2' is really the negated laplacian (the 
	     * laplacian is -'lap2')
	     */

	    if (k == 1) {
		/* use quad solver in the y-dimension */
		constrained_majorization_new_with_gaps(cMajEnv, b[k],
						       coords, dim, k,
						       localConstrMajorIterations,
						       hierarchy_boundaries,
						       levels_gap);

	    } else {
		/* use conjugate gradient for all dimensions except y */
		if (conjugate_gradient_mkernel(lap2, coords[k], b[k], n,
					   conj_tol, n)) {
		    iterations = -1;
		    goto finish;
		}
	    }
	}
    }
    free(hierarchy_boundaries);
    deleteCMajEnv(cMajEnv);

    if (coords != NULL) {
	for (i = 0; i < dim; i++) {
	    for (j = 0; j < n; j++) {
		d_coords[i][j] = coords[i][j];
	    }
	}
	free(coords[0]);
	free(coords);
    }

    if (b) {
	free(b[0]);
	free(b);
    }
    free(tmp_coords);
    free(dist_accumulator);
    free(degrees);
    free(lap2);


#ifdef NONCORE
    if (n <= max_nodes_in_mem) {
#endif
	free(lap1);
#ifdef NONCORE
    }
    if (fp)
	fclose(fp);
#endif

finish:
    free(ordering);

    free(levels);

    if (unpackedLap) {
	free(unpackedLap[0]);
	free(unpackedLap);
    }
    return iterations;
}
#endif				/* DIGCOLA */
