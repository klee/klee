/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**
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

/* 
 * Interface to Mosek (www.mosek.com) quadratic programming solver for solving
 * instances of the "Variable Placement with Separation Constraints" problem,
 * and also DiG-CoLa style level constraints.
 *
 * Tim Dwyer, 2006
 */
#ifdef MOSEK
#include <stdio.h>
#include <assert.h>
#include "defs.h"
#include "mosek_quad_solve.h"
#include "quad_prog_vpsc.h"

/* #define DUMP_CONSTRAINTS */
/* #define EQUAL_WIDTH_LEVELS */

static FILE *logfile;
static void MSKAPI printstr(void *handle, char str[])
{
    fprintf(logfile, "%s", str);
}

#define INIT_sub_val(a,b) \
    MSKidxt subi[2]; \
    double vali[2]; \
    subi[0] = a; \
    subi[1] = b; \
    vali[0] = 1.0; \
    vali[1] = -1.0;

#define INIT_sub_val3(a,b,c) \
    MSKidxt subi[3]; \
    double vali[3]; \
    subi[0] = a; \
    subi[1] = b; \
    subi[2] = c; \
    vali[0] = 1.0; \
    vali[1] = -2.0; \
    vali[2] = 1.0;

/**********************
lap: the upper RHS of the symmetric graph laplacian matrix which will be transformed
	to the hessian of the non-linear part of the optimisation function
n: number of nodes (length of coords array)
ordering: array containing sequences of nodes for each level,
	ie, ordering[levels[i]] is first node of (i+1)th level
level_indexes: array of starting node for each level in ordering
	ie, levels[i] is index to first node of (i+1)th level
	also, levels[0] is number of nodes in first level
	and, levels[i]-levels[i-1] is number of nodes in ith level
	and, n - levels[num_divisions-1] is number of nodes in last level
num_divisions: number of divisions between levels, ie number of levels - 1
separation: the minimum separation between nodes on different levels
***********************/
MosekEnv *mosek_init_hier(float *lap, int n, int *ordering,
    int *level_indexes, int num_divisions,
    float separation)
{
    int count = 0;
    int i, j, num_levels = num_divisions + 1;
    int num_constraints;
    MosekEnv *mskEnv = GNEW(MosekEnv);
    DigColaLevel *levels;
    int nonzero_lapsize = (n * (n - 1)) / 2;
    /* vars for nodes (except x0) + dummy nodes between levels
     * x0 is fixed at 0, and therefore is not included in the opt problem
     * add 2 more vars for top and bottom constraints
     */
    mskEnv->num_variables = n + num_divisions + 1;

    logfile = fopen("quad_solve_log", "w");
    levels = assign_digcola_levels(ordering, n, level_indexes, num_divisions);
#ifdef DUMP_CONSTRAINTS
    print_digcola_levels(logfile, levels, num_levels);
#endif

    /* nonlinear coefficients matrix of objective function */
    /* int lapsize=mskEnv->num_variables+(mskEnv->num_variables*(mskEnv->num_variables-1))/2; */
    mskEnv->qval = N_GNEW(nonzero_lapsize, double);
    mskEnv->qsubi = N_GNEW(nonzero_lapsize, int);
    mskEnv->qsubj = N_GNEW(nonzero_lapsize, int);

    /* solution vector */
    mskEnv->xx = N_GNEW(mskEnv->num_variables, double);

    /* constraint matrix */
    separation /= 2.0;		/* separation between each node and it's adjacent constraint */
    num_constraints = get_num_digcola_constraints(levels,
				    num_levels) + num_divisions + 1;
    /* constraints of the form x_i - x_j >= sep so 2 non-zero entries per constraint in LHS matrix
     * except x_0 (fixed at 0) constraints which have 1 nz val each.
     */
#ifdef EQUAL_WIDTH_LEVELS
    num_constraints += num_divisions;
#endif
    /* pointer to beginning of nonzero sequence in a column */

    for (i = 0; i < n - 1; i++) {
	for (j = i; j < n - 1; j++) {
	    mskEnv->qval[count] = -2 * lap[count + n];
	    assert(mskEnv->qval[count] != 0);
	    mskEnv->qsubi[count] = j;
	    mskEnv->qsubj[count] = i;
	    count++;
	}
    }
#ifdef DUMP_CONSTRAINTS
    fprintf(logfile, "Q=[");
    int lapcntr = n;
    for (i = 0; i < mskEnv->num_variables; i++) {
	if (i != 0)
	    fprintf(logfile, ";");
	for (j = 0; j < mskEnv->num_variables; j++) {
	    if (j < i || i >= n - 1 || j >= n - 1) {
		fprintf(logfile, "0 ");
	    } else {
		fprintf(logfile, "%f ", -2 * lap[lapcntr++]);
	    }
	}
    }
    fprintf(logfile, "]\nQ=Q-diag(diag(Q))+Q'\n");
#endif
    fprintf(logfile, "\n");
    /* Make the mosek environment. */
    mskEnv->r = MSK_makeenv(&mskEnv->env, NULL, NULL, NULL, NULL);

    /* Check whether the return code is ok. */
    if (mskEnv->r == MSK_RES_OK) {
	/* Directs the log stream to the user
	 * specified procedure 'printstr'. 
	 */
	MSK_linkfunctoenvstream(mskEnv->env, MSK_STREAM_LOG, NULL,
				printstr);
    }

    /* Initialize the environment. */
    mskEnv->r = MSK_initenv(mskEnv->env);
    if (mskEnv->r == MSK_RES_OK) {
	/* Make the optimization task. */
	mskEnv->r =
	    MSK_maketask(mskEnv->env, num_constraints,
			 mskEnv->num_variables, &mskEnv->task);

	if (mskEnv->r == MSK_RES_OK) {
	    int c_ind = 0;
	    int c_var = n - 1;
	    mskEnv->r =
		MSK_linkfunctotaskstream(mskEnv->task, MSK_STREAM_LOG,
					 NULL, printstr);
	    /* Resize the task. */
	    if (mskEnv->r == MSK_RES_OK)
		mskEnv->r = MSK_resizetask(mskEnv->task, num_constraints, mskEnv->num_variables, 0,	/* no cones!! */
					   /* each constraint applies to 2 vars */
					   2 * num_constraints +
					   num_divisions, nonzero_lapsize);

	    /* Append the constraints. */
	    if (mskEnv->r == MSK_RES_OK)
		mskEnv->r = MSK_append(mskEnv->task, 1, num_constraints);

	    /* Append the variables. */
	    if (mskEnv->r == MSK_RES_OK)
		mskEnv->r =
		    MSK_append(mskEnv->task, 0, mskEnv->num_variables);
	    /* Put variable bounds. */
	    for (j = 0;
		 j < mskEnv->num_variables && mskEnv->r == MSK_RES_OK; ++j)
		mskEnv->r =
		    MSK_putbound(mskEnv->task, 0, j, MSK_BK_RA,
				 -MSK_INFINITY, MSK_INFINITY);
	    for (j = 0; j < levels[0].num_nodes && mskEnv->r == MSK_RES_OK;
		 j++) {
		int node = levels[0].nodes[j] - 1;
		if (node >= 0) {
		    INIT_sub_val(c_var,node);
		    mskEnv->r =
			MSK_putavec(mskEnv->task, 1, c_ind, 2, subi, vali);
		} else {
		    /* constraint for y0 (fixed at 0) */
		    mskEnv->r =
			MSK_putaij(mskEnv->task, c_ind, c_var, 1.0);
		}
		mskEnv->r =
		    MSK_putbound(mskEnv->task, 1, c_ind, MSK_BK_LO,
				 separation, MSK_INFINITY);
		c_ind++;
	    }
	    for (i = 0; i < num_divisions && mskEnv->r == MSK_RES_OK; i++) {
		c_var = n + i;
		for (j = 0;
		     j < levels[i].num_nodes && mskEnv->r == MSK_RES_OK;
		     j++) {
		    /* create separation constraint a>=b+separation */
		    int node = levels[i].nodes[j] - 1;
		    if (node >= 0) {	/* no constraint for fixed node */
			INIT_sub_val(node,c_var);
			mskEnv->r =
			    MSK_putavec(mskEnv->task, 1, c_ind, 2, subi,
					vali);
		    } else {
			/* constraint for y0 (fixed at 0) */
			mskEnv->r =
			    MSK_putaij(mskEnv->task, c_ind, c_var, -1.0);
		    }
		    mskEnv->r =
			MSK_putbound(mskEnv->task, 1, c_ind, MSK_BK_LO,
				     separation, MSK_INFINITY);
		    c_ind++;
		}
		for (j = 0;
		     j < levels[i + 1].num_nodes
		     && mskEnv->r == MSK_RES_OK; j++) {
		    int node = levels[i + 1].nodes[j] - 1;
		    if (node >= 0) {
			INIT_sub_val(c_var,node);
			mskEnv->r =
			    MSK_putavec(mskEnv->task, 1, c_ind, 2, subi,
					vali);
		    } else {
			/* constraint for y0 (fixed at 0) */
			mskEnv->r =
			    MSK_putaij(mskEnv->task, c_ind, c_var, 1.0);
		    }
		    mskEnv->r =
			MSK_putbound(mskEnv->task, 1, c_ind, MSK_BK_LO,
				     separation, MSK_INFINITY);
		    c_ind++;
		}
	    }
	    c_var = n + i;
	    for (j = 0; j < levels[i].num_nodes && mskEnv->r == MSK_RES_OK;
		 j++) {
		/* create separation constraint a>=b+separation */
		int node = levels[i].nodes[j] - 1;
		if (node >= 0) {	/* no constraint for fixed node */
		    INIT_sub_val(node,c_var);
		    mskEnv->r =
			MSK_putavec(mskEnv->task, 1, c_ind, 2, subi, vali);
		} else {
		    /* constraint for y0 (fixed at 0) */
		    mskEnv->r =
			MSK_putaij(mskEnv->task, c_ind, c_var, -1.0);
		}
		mskEnv->r =
		    MSK_putbound(mskEnv->task, 1, c_ind, MSK_BK_LO,
				 separation, MSK_INFINITY);
		c_ind++;
	    }
	    /* create constraints preserving the order of dummy vars */
	    for (i = 0; i < num_divisions + 1 && mskEnv->r == MSK_RES_OK;
		 i++) {
		int c_var = n - 1 + i, c_var2 = c_var + 1;
		INIT_sub_val(c_var,c_var2);
		mskEnv->r =
		    MSK_putavec(mskEnv->task, 1, c_ind, 2, subi, vali);
		mskEnv->r =
		    MSK_putbound(mskEnv->task, 1, c_ind, MSK_BK_LO, 0,
				 MSK_INFINITY);
		c_ind++;
	    }
#ifdef EQUAL_WIDTH_LEVELS
	    for (i = 1; i < num_divisions + 1 && mskEnv->r == MSK_RES_OK;
		 i++) {
		int c_var = n - 1 + i, c_var_lo = c_var - 1, c_var_hi =
		    c_var + 1;
		INIT_sub_val3(c_var_lo, c_var, c_var_h);
		mskEnv->r =
		    MSK_putavec(mskEnv->task, 1, c_ind, 3, subi, vali);
		mskEnv->r =
		    MSK_putbound(mskEnv->task, 1, c_ind, MSK_BK_FX, 0, 0);
		c_ind++;
	    }
#endif
	    assert(c_ind == num_constraints);
#ifdef DUMP_CONSTRAINTS
	    fprintf(logfile, "A=[");
	    for (i = 0; i < num_constraints; i++) {
		if (i != 0)
		    fprintf(logfile, ";");
		for (j = 0; j < mskEnv->num_variables; j++) {
		    double aij;
		    MSK_getaij(mskEnv->task, i, j, &aij);
		    fprintf(logfile, "%f ", aij);
		}
	    }
	    fprintf(logfile, "]\n");
	    fprintf(logfile, "b=[");
	    for (i = 0; i < num_constraints; i++) {
		fprintf(logfile, "%f ", separation);
	    }
	    fprintf(logfile, "]\n");
#endif
	    if (mskEnv->r == MSK_RES_OK) {
		/*
		 * The lower triangular part of the Q
		 * matrix in the objective is specified.
		 */
		mskEnv->r =
		    MSK_putqobj(mskEnv->task, nonzero_lapsize,
				mskEnv->qsubi, mskEnv->qsubj,
				mskEnv->qval);
	    }
	}
    }
    delete_digcola_levels(levels, num_levels);
    return mskEnv;
}

/*
b: coefficients of linear part of optimisation function
n: number of nodes
coords: optimal y* vector, coord[i] is coordinate of node[i]
hierarchy_boundaries: y coord of boundaries between levels 
	(ie, solution values for the dummy variables used in constraints)
*/
void mosek_quad_solve_hier(MosekEnv * mskEnv, float *b, int n,
			   float *coords, float *hierarchy_boundaries)
{
    int i, j;
    for (i = 1; i < n && mskEnv->r == MSK_RES_OK; i++) {
	mskEnv->r = MSK_putcj(mskEnv->task, i - 1, -2 * b[i]);
    }
#ifdef DUMP_CONSTRAINTS
    fprintf(logfile, "x0=[");
    for (j = 0; j < mskEnv->num_variables; j++) {
	fprintf(logfile, "%f ", j < n ? b[j] : 0);
    }
    fprintf(logfile, "]\n");
    fprintf(logfile, "f=[");
    double *c = N_GNEW(mskEnv->num_variables, double);
    MSK_getc(mskEnv->task, c);
    for (j = 0; j < mskEnv->num_variables; j++) {
	fprintf(logfile, "%f ", c[j]);
    }
    free(c);
    fprintf(logfile, "]\n");
#endif
    if (mskEnv->r == MSK_RES_OK)
	mskEnv->r = MSK_optimize(mskEnv->task);

    if (mskEnv->r == MSK_RES_OK) {
	MSK_getsolutionslice(mskEnv->task,
			     MSK_SOL_ITR,
			     MSK_SOL_ITEM_XX,
			     0, mskEnv->num_variables, mskEnv->xx);

#ifdef DUMP_CONSTRAINTS
	fprintf(logfile, "Primal solution\n");
#endif
	coords[0] = 0;
	for (j = 0; j < mskEnv->num_variables; ++j) {
#ifdef DUMP_CONSTRAINTS
	    fprintf(logfile, "x[%d]: %.2f\n", j, mskEnv->xx[j]);
#endif
	    if (j < n - 1) {
		coords[j + 1] = -mskEnv->xx[j];
	    } else if (j >= n && j < mskEnv->num_variables - 1) {
		hierarchy_boundaries[j - n] = -mskEnv->xx[j];
	    }
	}
    }
    fprintf(logfile, "Return code: %d\n", mskEnv->r);
}

/**********************
lap: the upper RHS of the symmetric graph laplacian matrix which will be transformed
	to the hessian of the non-linear part of the optimisation function
	has dimensions num_variables, dummy vars do not have entries in lap
cs: array of pointers to separation constraints
***********************/
MosekEnv *mosek_init_sep(float *lap, int num_variables, int num_dummy_vars,
			 Constraint ** cs, int num_constraints)
{
    int i, j;
    MosekEnv *mskEnv = GNEW(MosekEnv);
    int count = 0;
    int nonzero_lapsize = num_variables * (num_variables - 1) / 2;
    /* fix var 0 */
    mskEnv->num_variables = num_variables + num_dummy_vars - 1;

    fprintf(stderr, "MOSEK!\n");
    logfile = fopen("quad_solve_log", "w");

    /* nonlinear coefficients matrix of objective function */
    mskEnv->qval = N_GNEW(nonzero_lapsize, double);
    mskEnv->qsubi = N_GNEW(nonzero_lapsize, int);
    mskEnv->qsubj = N_GNEW(nonzero_lapsize, int);

    /* solution vector */
    mskEnv->xx = N_GNEW(mskEnv->num_variables, double);

    /* pointer to beginning of nonzero sequence in a column */

    for (i = 0; i < num_variables - 1; i++) {
	for (j = i; j < num_variables - 1; j++) {
	    mskEnv->qval[count] = -2 * lap[count + num_variables];
	    /* assert(mskEnv->qval[count]!=0); */
	    mskEnv->qsubi[count] = j;
	    mskEnv->qsubj[count] = i;
	    count++;
	}
    }
#ifdef DUMP_CONSTRAINTS
    fprintf(logfile, "Q=[");
    count = 0;
    for (i = 0; i < num_variables - 1; i++) {
	if (i != 0)
	    fprintf(logfile, ";");
	for (j = 0; j < num_variables - 1; j++) {
	    if (j < i) {
		fprintf(logfile, "0 ");
	    } else {
		fprintf(logfile, "%f ", -2 * lap[num_variables + count++]);
	    }
	}
    }
    fprintf(logfile, "]\nQ=Q-diag(diag(Q))+Q'\n");
#endif
    /* Make the mosek environment. */
    mskEnv->r = MSK_makeenv(&mskEnv->env, NULL, NULL, NULL, NULL);

    /* Check whether the return code is ok. */
    if (mskEnv->r == MSK_RES_OK) {
	/* Directs the log stream to the user
	   specified procedure 'printstr'. */
	MSK_linkfunctoenvstream(mskEnv->env, MSK_STREAM_LOG, NULL,
				printstr);
    }

    /* Initialize the environment. */
    mskEnv->r = MSK_initenv(mskEnv->env);
    if (mskEnv->r == MSK_RES_OK) {
	/* Make the optimization task. */
	mskEnv->r =
	    MSK_maketask(mskEnv->env, num_constraints,
			 mskEnv->num_variables, &mskEnv->task);

	if (mskEnv->r == MSK_RES_OK) {
	    mskEnv->r =
		MSK_linkfunctotaskstream(mskEnv->task, MSK_STREAM_LOG,
					 NULL, printstr);
	    /* Resize the task. */
	    if (mskEnv->r == MSK_RES_OK)
		mskEnv->r = MSK_resizetask(mskEnv->task, num_constraints, mskEnv->num_variables, 0,	/* no cones!! */
					   /* number of non-zero constraint matrix entries:
					    *   each constraint applies to 2 vars
					    */
					   2 * num_constraints,
					   nonzero_lapsize);

	    /* Append the constraints. */
	    if (mskEnv->r == MSK_RES_OK)
		mskEnv->r = MSK_append(mskEnv->task, 1, num_constraints);

	    /* Append the variables. */
	    if (mskEnv->r == MSK_RES_OK)
		mskEnv->r =
		    MSK_append(mskEnv->task, 0, mskEnv->num_variables);
	    /* Put variable bounds. */
	    for (j = 0;
		 j < mskEnv->num_variables && mskEnv->r == MSK_RES_OK; j++)
		mskEnv->r =
		    MSK_putbound(mskEnv->task, 0, j, MSK_BK_RA,
				 -MSK_INFINITY, MSK_INFINITY);
	    for (i = 0; i < num_constraints; i++) {
		int u = getLeftVarID(cs[i]) - 1;
		int v = getRightVarID(cs[i]) - 1;
		double separation = getSeparation(cs[i]);
		if (u < 0) {
		    mskEnv->r =
			MSK_putbound(mskEnv->task, 0, v, MSK_BK_RA,
				     -MSK_INFINITY, -separation);
		    assert(mskEnv->r == MSK_RES_OK);
		} else if (v < 0) {
		    mskEnv->r =
			MSK_putbound(mskEnv->task, 0, u, MSK_BK_RA,
				     separation, MSK_INFINITY);
		    assert(mskEnv->r == MSK_RES_OK);
		} else {
		    /* fprintf(stderr,"u=%d,v=%d,sep=%f\n",u,v,separation); */
		    INIT_sub_val(u,v);
		    mskEnv->r =
			MSK_putavec(mskEnv->task, 1, i, 2, subi, vali);
		    assert(mskEnv->r == MSK_RES_OK);
		    mskEnv->r =
			MSK_putbound(mskEnv->task, 1, i, MSK_BK_LO,
				     separation, MSK_INFINITY);
		    assert(mskEnv->r == MSK_RES_OK);
		}
	    }
	    if (mskEnv->r == MSK_RES_OK) {
		/*
		 * The lower triangular part of the Q
		 * matrix in the objective is specified.
		 */
		mskEnv->r =
		    MSK_putqobj(mskEnv->task, nonzero_lapsize,
				mskEnv->qsubi, mskEnv->qsubj,
				mskEnv->qval);
		assert(mskEnv->r == MSK_RES_OK);
	    }
	}
    }
    return mskEnv;
}

/*
n: size of b and coords, may be smaller than mskEnv->num_variables if we
have dummy vars
b: coefficients of linear part of optimisation function
coords: optimal y* vector, coord[i] is coordinate of node[i]
*/
void mosek_quad_solve_sep(MosekEnv * mskEnv, int n, float *b,
			  float *coords)
{
    int i, j;
    assert(n <= mskEnv->num_variables + 1);
    for (i = 0; i < n - 1 && mskEnv->r == MSK_RES_OK; i++) {
	mskEnv->r = MSK_putcj(mskEnv->task, i, -2 * b[i + 1]);
    }
    if (mskEnv->r == MSK_RES_OK)
	mskEnv->r = MSK_optimize(mskEnv->task);

    if (mskEnv->r == MSK_RES_OK) {
	MSK_getsolutionslice(mskEnv->task,
			     MSK_SOL_ITR,
			     MSK_SOL_ITEM_XX,
			     0, mskEnv->num_variables, mskEnv->xx);

#ifdef DUMP_CONSTRAINTS
	fprintf(logfile, "Primal solution\n");
#endif
	coords[0] = 0;
	for (j = 1; j <= n; j++) {
#ifdef DUMP_CONSTRAINTS
	    fprintf(logfile, "x[%d]: %.2f\n", j, mskEnv->xx[j - 1]);
#endif
	    coords[j] = -mskEnv->xx[j - 1];
	}
    }
    fprintf(logfile, "Return code: %d\n", mskEnv->r);
}

/*
please call to clean up
*/
void mosek_delete(MosekEnv * mskEnv)
{
    MSK_deletetask(&mskEnv->task);
    MSK_deleteenv(&mskEnv->env);

    if (logfile) {
	fclose(logfile);
	logfile = NULL;
    }
    free(mskEnv->qval);
    free(mskEnv->qsubi);
    free(mskEnv->qsubj);
    free(mskEnv->xx);
    free(mskEnv);
}
#endif				/* MOSEK */
