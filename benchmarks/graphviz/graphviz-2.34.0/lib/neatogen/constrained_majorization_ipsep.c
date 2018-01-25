/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**
 *
 * Authors:
 *   Tim Dwyer <tgdwyer@gmail.com>
 *
 * Copyright (C) 2005 Authors
 *
 * This version is released under the CPL (Common Public License) with
 * the Graphviz distribution.
 * A version is also available under the LGPL as part of the Adaptagrams
 * project: http://sourceforge.net/projects/adaptagrams.  
 * If you make improvements or bug fixes to this code it would be much
 * appreciated if you could also contribute those changes back to the
 * Adaptagrams repository.
 */

/**********************************************************
 * Based on constrained_majorization.c
 *
 * Perform stress majorization subject
 * to separation constraints, for background see the paper:
 * "IPSep-CoLa: An Incremental Procedure for Separation Constraint Layout of Graphs"
 * by Tim Dwyer, Yehuda Koren and Kim Marriott
 *
 * Available separation constraints so far are:
 *  o Directed edge constraints
 *  o Node non-overlap constraints
 *  o Cluster containment constraints
 *  o Cluster/node non-overlap constraints
 *
 * Tim Dwyer, 2006
 **********************************************************/

#include "digcola.h"
#ifdef IPSEPCOLA
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
#include <csolve_VPSC.h>
#include "quad_prog_vpsc.h"
#include "quad_prog_solver.h"
#include "matrix_ops.h"

#define localConstrMajorIterations 1000

int stress_majorization_cola(vtx_data * graph,	/* Input graph in sparse representation  */
			     int n,	/* Number of nodes */
			     int nedges_graph,	/* Number of edges */
			     double **d_coords,	/* Coordinates of nodes (output layout)  */
			     node_t ** nodes,	/* Original nodes */
			     int dim,	/* Dimemsionality of layout */
			     int model,	/* difference model */
			     int maxi,	/* max iterations */
			     ipsep_options * opt)
{
    int iterations = 0;		/* Output: number of iteration of the process */

	/*************************************************
	** Computation of full, dense, unrestricted k-D ** 
	** stress minimization by majorization          ** 
	** This function imposes HIERARCHY CONSTRAINTS  **
	*************************************************/

    int i, j, k;
    float *lap1 = NULL;
    float *dist_accumulator = NULL;
    float *tmp_coords = NULL;
    float **b = NULL;
    double *degrees = NULL;
    float *lap2 = NULL;
    int lap_length;
    float *f_storage = NULL;
    float **coords = NULL;
    int orig_n = n;

    /*double conj_tol=tolerance_cg; *//* tolerance of Conjugate Gradient */
    CMajEnvVPSC *cMajEnvHor = NULL;
    CMajEnvVPSC *cMajEnvVrt = NULL;
    double y_0;
    int length;
    DistType diameter;
    float *Dij = NULL;
    float constant_term;
    int count;
    double degree;
    int step;
    float val;
    double old_stress, new_stress = 0;
    boolean converged;
    int len;
    double nsizeScale = 0;
    float maxEdgeLen = 0;
    double max = 1;

    initLayout(graph, n, dim, d_coords, nodes);
    if (n == 1)
	return 0;

    for (i = 0; i < n; i++) {
	for (j = 1; j < graph[i].nedges; j++) {
	    maxEdgeLen = MAX(graph[i].ewgts[j], maxEdgeLen);
	}
    }

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

    /* for numerical stability, scale down layout                */
    /* No Jiggling, might conflict with constraints                      */
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

    if (opt->clusters->nclusters > 0) {
	int nn = n + opt->clusters->nclusters * 2;
	int clap_length = nn + nn * (nn - 1) / 2;
	float *clap = N_GNEW(clap_length, float);
	int c0, c1;
	float v;
	c0 = c1 = 0;
	for (i = 0; i < nn; i++) {
	    for (j = 0; j < nn - i; j++) {
		if (i < n && j < n - i) {
		    v = lap2[c0++];
		} else {
		    /* v=j==1?i%2:0; */
		    if (j == 1 && i % 2 == 1) {
			v = maxEdgeLen;
			v *= v;
			if (v > 0.01) {
			    v = 1.0 / v;
			}
		    } else
			v = 0;
		}
		clap[c1++] = v;
	    }
	}
	free(lap2);
	lap2 = clap;
	n = nn;
	lap_length = clap_length;
    }
    /* compute diagonal entries */
    count = 0;
    degrees = N_GNEW(n, double);
    set_vector_val(n, 0, degrees);
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
	lap2[count] = (float) degrees[i];
    }

    coords = N_GNEW(dim, float *);
    f_storage = N_GNEW(dim * n, float);
    for (i = 0; i < dim; i++) {
	coords[i] = f_storage + i * n;
	for (j = 0; j < n; j++) {
	    coords[i][j] = j < orig_n ? (float) (d_coords[i][j]) : 0;
	}
    }

    /* compute constant term in stress sum
     * which is \sum_{i<j} w_{ij}d_{ij}^2
     */
    constant_term = (float) (n * (n - 1) / 2);

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

    old_stress = DBL_MAX;	/* at least one iteration */

    if ((cMajEnvHor = initCMajVPSC(n, lap2, graph, opt, 0)) == NULL) {
	iterations = -1;
	goto finish;
    }
    if ((cMajEnvVrt = initCMajVPSC(n, lap2, graph, opt, opt->diredges)) == NULL) {
	iterations = -1;
	goto finish;
    }

    lap1 = N_GNEW(lap_length, float);

    for (converged = FALSE, iterations = 0;
	 iterations < maxi && !converged; iterations++) {

	/* First, construct Laplacian of 1/(d_ij*|p_i-p_j|)  */
	set_vector_val(n, 0, degrees);
	sqrt_vecf(lap_length, lap2, lap1);
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
	new_stress += constant_term;	/* only after mult by 2 */
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
	if (Verbose && (iterations % 1 == 0)) {
	    fprintf(stderr, "%.3f ", new_stress);
	    if (iterations % 10 == 0)
		fprintf(stderr, "\n");
	}
	converged = new_stress < old_stress
	    && fabs(new_stress - old_stress) / fabs(old_stress + 1e-10) <
	    Epsilon;
	/*converged = converged || (iterations>1 && new_stress>old_stress); */
	/* in first iteration we allowed stress increase, which 
	 * might result ny imposing constraints
	 */
	old_stress = new_stress;

	/* in determining non-overlap constraints we gradually scale up the
	 * size of nodes to avoid local minima
	 */
	if ((iterations >= maxi - 1 || converged) && opt->noverlap == 1
	    && nsizeScale < 0.999) {
	    nsizeScale += 0.1;
	    if (Verbose)
		fprintf(stderr, "nsizescale=%f,iterations=%d\n",
			nsizeScale, iterations);
	    iterations = 0;
	    converged = FALSE;
	}


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

	if (opt->noverlap == 1 && nsizeScale > 0.001) {
	    generateNonoverlapConstraints(cMajEnvHor, nsizeScale, coords,
					  0,
					  nsizeScale < 0.5 ? FALSE : TRUE,
					  opt);
	}
	if (cMajEnvHor->m > 0) {
#ifdef MOSEK
	    if (opt->mosek) {
		mosek_quad_solve_sep(cMajEnvHor->mosekEnv, n, b[0],
				     coords[0]);
	    } else
#endif				/* MOSEK */
		constrained_majorization_vpsc(cMajEnvHor, b[0], coords[0],
					      localConstrMajorIterations);
	} else {
	    /* if there are no constraints then use conjugate gradient
	     * optimisation which should be considerably faster
	     */
	    if (conjugate_gradient_mkernel(lap2, coords[0], b[0], n,
				       tolerance_cg, n) < 0) {
		iterations = -1;
		goto finish;
	    }
	}
	if (opt->noverlap == 1 && nsizeScale > 0.001) {
	    generateNonoverlapConstraints(cMajEnvVrt, nsizeScale, coords,
					  1, FALSE, opt);
	}
	if (cMajEnvVrt->m > 0) {
#ifdef MOSEK
	    if (opt->mosek) {
		mosek_quad_solve_sep(cMajEnvVrt->mosekEnv, n, b[1],
				     coords[1]);
	    } else
#endif				/* MOSEK */
		if (constrained_majorization_vpsc(cMajEnvVrt, b[1], coords[1],
					      localConstrMajorIterations) < 0) {
		iterations = -1;
		goto finish;
	    }
	} else {
	    conjugate_gradient_mkernel(lap2, coords[1], b[1], n,
				       tolerance_cg, n);
	}
    }
    if (Verbose) {
	fprintf(stderr, "\nfinal e = %f %d iterations %.2f sec\n",
		new_stress, iterations, elapsed_sec());
    }
    deleteCMajEnvVPSC(cMajEnvHor);
    deleteCMajEnvVPSC(cMajEnvVrt);

    if (opt->noverlap == 2) {
	/* fprintf(stderr, "Removing overlaps as post-process...\n"); */
	removeoverlaps(orig_n, coords, opt);
    }

finish:
    if (coords != NULL) {
	for (i = 0; i < dim; i++) {
	    for (j = 0; j < orig_n; j++) {
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
    free(lap1);

    return iterations;
}
#endif				/* IPSEPCOLA */
