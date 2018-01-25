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

#ifndef POST_PROCESS_H
#define POST_PROCESS_H

#include "spring_electrical.h"

enum {SM_SCHEME_NORMAL, SM_SCHEME_NORMAL_ELABEL, SM_SCHEME_UNIFORM_STRESS, SM_SCHEME_MAXENT, SM_SCHEME_STRESS_APPROX, SM_SCHEME_STRESS};

struct StressMajorizationSmoother_struct {
  SparseMatrix D;/* distance matrix. The diagonal is removed hence the ia, ja structure is different from Lw and Lwd!! */
  SparseMatrix Lw;/* the weighted laplacian. with offdiag = -1/w_ij */
  SparseMatrix Lwd;/* the laplacian like matrix with offdiag = -scaling*d_ij/w_ij. RHS in stress majorization = Lwd.x */
  real* lambda;
  void (*data_deallocator)(void*);
  void *data;
  int scheme;
  real scaling;/* scaling. It is multiplied to Lwd. need to divide coordinate x at the end of the stress majorization process */
  real tol_cg;/* tolerance and maxit for conjugate gradient that solves the Laplacian system.
		 typically the Laplacian only needs to be solved very crudely as it is part of an
		 outer iteration.*/
  int maxit_cg;
};

typedef struct StressMajorizationSmoother_struct *StressMajorizationSmoother;

void StressMajorizationSmoother_delete(StressMajorizationSmoother sm);

enum {IDEAL_GRAPH_DIST, IDEAL_AVG_DIST, IDEAL_POWER_DIST};
StressMajorizationSmoother StressMajorizationSmoother2_new(SparseMatrix A, int dim, real lambda, real *x, int ideal_dist_scheme);

real StressMajorizationSmoother_smooth(StressMajorizationSmoother sm, int dim, real *x, int maxit, real tol);
/*-------------------- triangle/neirhborhood graph based smoother ------------------- */
typedef  StressMajorizationSmoother TriangleSmoother;

#define TriangleSmoother_struct StressMajorizationSmoother_struct

void TriangleSmoother_delete(TriangleSmoother sm);

TriangleSmoother TriangleSmoother_new(SparseMatrix A, int dim, real lambda, real *x, int use_triangularization);

void TriangleSmoother_smooth(TriangleSmoother sm, int dim, real *x);



/*------------------ spring and spring-electrical based smoother */

struct SpringSmoother_struct {
  SparseMatrix D;
  spring_electrical_control ctrl;
};

typedef struct SpringSmoother_struct *SpringSmoother;

SpringSmoother SpringSmoother_new(SparseMatrix A, int dim, spring_electrical_control ctrl, real *x);

void SpringSmoother_delete(SpringSmoother sm);

void SpringSmoother_smooth(SpringSmoother sm, SparseMatrix A, real *node_weights, int dim, real *x);
/*------------------------------------------------------------------*/

void post_process_smoothing(int dim, SparseMatrix A, spring_electrical_control ctrl, real *node_weights, real *x, int *flag);

/*-------------------- sparse stress majorizationp ------------------- */
typedef  StressMajorizationSmoother SparseStressMajorizationSmoother;

#define SparseStressMajorizationSmoother_struct StressMajorizationSmoother_struct

void SparseStressMajorizationSmoother_delete(SparseStressMajorizationSmoother sm);

enum {WEIGHTING_SCHEME_NONE, WEIGHTING_SCHEME_INV_DIST, WEIGHTING_SCHEME_SQR_DIST};
SparseStressMajorizationSmoother SparseStressMajorizationSmoother_new(SparseMatrix A, int dim, real lambda, real *x, 
								      int weighting_scheme, int scale_initial_coord);

real SparseStressMajorizationSmoother_smooth(SparseStressMajorizationSmoother sm, int dim, real *x, int maxit_sm, real tol);

real get_stress(int m, int dim, int *iw, int *jw, real *w, real *d, real *x, real scaling, void *data, int weighted);

real get_full_stress(SparseMatrix A, int dim, real *x, int weighting_scheme);
void dump_distance_edge_length(char *outfile, SparseMatrix A, int dim, real *x);

/*--------------------------------------------------------------*/

#endif

