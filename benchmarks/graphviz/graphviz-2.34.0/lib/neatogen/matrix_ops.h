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

#ifdef __cplusplus
extern "C" {
#endif



#ifndef _MATRIX_OPS_H_
#define _MATRIX_OPS_H_

#include "sparsegraph.h"

    extern void cpvec(double *, int, int, double *);
    extern double dot(double *, int, int, double *);
    extern void scadd(double *, int, int, double, double *);
    extern void vecscale(double *, int, int, double, double *);
    extern double norm(double *, int, int);

    extern void orthog1(int n, double *vec);
    extern void init_vec_orth1(int n, double *vec);
    extern void right_mult_with_vector(vtx_data *, int, double *,
				       double *);
    extern void right_mult_with_vector_f(float **, int, double *,
					 double *);
    extern void vectors_subtraction(int, double *, double *, double *);
    extern void vectors_addition(int, double *, double *, double *);
    extern void vectors_scalar_mult(int, double *, double, double *);
    extern void copy_vector(int n, double *source, double *dest);
    extern double vectors_inner_product(int n, double *vector1,
					double *vector2);
    extern double max_abs(int n, double *vector);
#ifdef UNUSED
    extern void vectors_mult_addition(int, double *, double, double *);
    extern void orthogvec(int, double *, double *);
#endif

    /* sparse matrix extensions: */

#ifdef UNUSED
    extern void mat_mult_vec(vtx_data * L, int n, double *vec,
			     double *result);
#endif
    extern void right_mult_with_vector_transpose
	(double **, int, int, double *, double *);
    extern void right_mult_with_vector_d(double **, int, int, double *,
					 double *);
    extern void mult_dense_mat(double **, float **, int, int, int,
			       float ***C);
    extern void mult_dense_mat_d(double **, float **, int, int, int,
				 double ***CC);
    extern void mult_sparse_dense_mat_transpose(vtx_data *, double **, int,
						int, float ***);
    extern int power_iteration(double **, int, int, double **, double *, int);


/*****************************
** Single precision (float) **
** version                  **
*****************************/

    extern void orthog1f(int n, float *vec);
#ifdef UNUSED
    extern void right_mult_with_vectorf(vtx_data *, int, float *, float *);
    extern void right_mult_with_vector_fd(float **, int, float *,
					  double *);
#endif
    extern void right_mult_with_vector_ff(float *, int, float *, float *);
    extern void vectors_substractionf(int, float *, float *, float *);
    extern void vectors_additionf(int n, float *vector1, float *vector2,
				  float *result);
    extern void vectors_mult_additionf(int n, float *vector1, float alpha,
				       float *vector2);
    extern void vectors_scalar_multf(int n, float *vector, float alpha,
				     float *result);
    extern void copy_vectorf(int n, float *source, float *dest);
    extern double vectors_inner_productf(int n, float *vector1,
					 float *vector2);
    extern void set_vector_val(int n, double val, double *result);
    extern void set_vector_valf(int n, float val, float * result);
    extern double max_absf(int n, float *vector);
    extern void square_vec(int n, float *vec);
    extern void invert_vec(int n, float *vec);
    extern void sqrt_vec(int n, float *vec);
    extern void sqrt_vecf(int n, float *source, float *target);
    extern void invert_sqrt_vec(int n, float *vec);
#ifdef UNUSED
    extern void init_vec_orth1f(int n, float *vec);
    extern void mat_mult_vecf(vtx_data * L, int n, float *vec,
			      float *result);
#endif

#endif

#ifdef __cplusplus
}
#endif
