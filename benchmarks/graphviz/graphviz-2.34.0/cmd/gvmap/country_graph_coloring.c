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

#define STANDALONE
#include "country_graph_coloring.h"
#include "power.h"

/* #include "general.h" */
/* #include "SparseMatrix.h" */
/* #include "country_graph_coloring.h" */
/* #include "power.h" */
#include "PriorityQueue.h"
#include "time.h"

/* int Verbose = FALSE; */

static void get_local_12_norm(int n, int i, int *ia, int *ja, int *p, real *norm){
  int j, nz;
  norm[0] = n; norm[1] = 0;
  for (j = ia[i]; j < ia[i+1]; j++){
    if (ja[j] == i) continue;
    norm[0] = MIN(norm[0], ABS(p[i] - p[ja[j]]));
    nz++;
    norm[1] += ABS(p[i] - p[ja[j]]);
  }
  if (nz > 0) norm[1] /= nz;
}
static void get_12_norm(int n, int *ia, int *ja, int *p, real *norm){
  /* norm[0] := antibandwidth
     norm[1] := (\sum_{{i,j}\in E} |p[i] - p[j]|)/|E|
     norm[2] := (\sum_{i\in V} (Min_{{j,i}\in E} |p[i] - p[j]|)/|V|
  */
  int i, j, nz = 0;
  real tmp;
  norm[0] = n; norm[1] = 0; norm[2] = 0;
  for (i = 0; i < n; i++){
    tmp = n;
    for (j = ia[i]; j < ia[i+1]; j++){
      if (ja[j] == i) continue;
      norm[0] = MIN(norm[0], ABS(p[i] - p[ja[j]]));
      norm[1] += ABS(p[i] - p[ja[j]]);
      tmp = MIN(tmp, ABS(p[i] - p[ja[j]]));
      nz++;
    }
    norm[2] += tmp;
  }
  norm[2] /= n;
  norm[1] /= nz;
}

static void update_pmin_pmax_aband(int n, int u, int *ia, int *ja, int *p, int *pmin, int *pmax, int *aband_local){
  int j, aband_u;
  pmin[u] = n; pmax[u] = -1; aband_u = n;
  for (j = ia[u]; j < ia[u+1]; j++) {
    if (ja[j] == u) continue;
    pmin[u] = MIN(pmin[u], ABS(p[u] - p[ja[j]]));
    pmax[u] = MIN(pmax[u], ABS(p[u] - p[ja[j]]));
    aband_u = MIN(aband_u, ABS(p[u] - p[ja[j]]));
  }
  aband_local[u] = aband_u;
}


static int check_swap(int n, int *ia, int *ja,
		      int u, int p_u, int v, int p_v, int *aband_local, int *p, int *p_inv, int aband, PriorityQueue pq, int *pmax, int *pmin, real lambda){
  /* check if u should swap with v to improve u, without demaging v. Return TRUE if swap is successful. FALSE otherwise. */

  int j, aband_v, aband_v1, aband_u, aband_u1;

  aband_v = aband_local[v];
  aband_u = aband_local[u];
  
  /* if swaping u and v makes v worse & becomes/remains critical, don't do. We first quick check using the max/min neighbor indices.
     No need to check the other way around since the calling function have ensured that.
   */
  if (ABS(p_u - pmin[v]) < aband_v && ABS(p_u - pmin[v]) <= lambda*aband) return FALSE;
  if (ABS(p_u - pmax[v]) < aband_v && ABS(p_u - pmax[v]) <= lambda*aband) return FALSE;

  /* now check in details whether v should swap to u. Do not accept if this makes the antiband width of u worse */
  aband_u1 = n;
  for (j = ja[u]; j < ja[u+1]; j++){
    if (ja[j] == u) continue;
    if (ABS(p_v - p[ja[j]]) < aband_u) {
      return FALSE;
    }
    aband_u1 = MIN(aband_u1, ABS(p_v - p[ja[j]]));
  }
  
  /* now check in details whether u should swap to v. Do not accept if this makes antibandwidth of v worse && make/keep v in the critical group */
  aband_v1 = n;
  for (j = ja[v]; j < ja[v+1]; j++){
    if (ja[j] == v) continue;
    if (ABS(p_u - p[ja[j]]) < aband_v && ABS(p_u - p[ja[j]]) <= lambda*aband) {
      return FALSE;
    }
    aband_v1 = MIN(aband_v1, ABS(p_u - p[ja[j]]));
  }
  
  /* now check if local antiband width has been improved. By that we mean u is improved, or u unchanged, but v improved. */
  assert(aband_u1 >= aband_u);
  if (aband_u1 > aband_u || (aband_u1 == aband_u && aband_v1 > aband_v)){
    p[u] = p_v;
    p[v] = p[u];
    p_inv[p[u]] = u;
    p_inv[p[v]] = v;

    update_pmin_pmax_aband(n, u, ia, ja, p, pmin, pmax, aband_local);
    update_pmin_pmax_aband(n, v, ia, ja, p, pmin, pmax, aband_local);

    /* this may be expensive, but I see no way to keep pmin/pmax/aband_local consistent other than this update of u/v's neighbors */    
    for (j = ia[u]; j < ia[u+1]; j++) {
      update_pmin_pmax_aband(n, ja[j], ia, ja, p, pmin, pmax, aband_local);
    }

    for (j = ia[u]; j < ia[u+1]; j++) {
      update_pmin_pmax_aband(n, ja[j], ia, ja, p, pmin, pmax, aband_local);
    }
    return TRUE;
  }
  

  return FALSE;
}

void improve_antibandwidth_by_swapping_cheap(SparseMatrix A, int *p){
  /*on entry:
    A: the graph, must be symmetric matrix
    p: a permutation array of length n
    lambda: threshold for deciding critical vertices.*/
  real lambda = 1.2;
  int n = A->m;
  int *p_inv;
  int i, j;
  int *pmax, *pmin;/* max and min index of neighbors */
  int *ia = A->ia, *ja = A->ja;
  int aband = n;/* global antibandwidth*/
  int *aband_local;/* antibandwidth for each node */
  PriorityQueue pq = NULL;
  int progress = TRUE, u, v, gain, aband_u, p_u, p_v, swapped;
  
  pq = PriorityQueue_new(n, n);
  p_inv = MALLOC(sizeof(int)*n);
  pmax = MALLOC(sizeof(int)*n);
  pmin = MALLOC(sizeof(int)*n);
  aband_local = MALLOC(sizeof(int)*n);

  while (progress) {
    progress = FALSE;
    for (i = 0; i < n; i++){
      pmax[i] = -1; pmin[i] = n+1;
      assert(p[i] >= 0 && p[i] < n);
      p_inv[p[i]] = i;
      aband_local[i] = n;
      for (j = ia[i]; j < ia[i+1]; j++){
	if (ja[j] == i) continue;
	pmax[i] = MAX(pmax[i], p[ja[j]]);
	pmin[i] = MIN(pmin[i], p[ja[j]]);
	aband_local[i] = MIN(aband_local[i], ABS(p[i] - p[ja[j]]));
      }
      aband = MIN(aband, aband_local[i]);
    }
    fprintf(stderr," antibandwidth = %d", aband);
    
    /* find critical vertices */
    for (i = 0; i < n; i++){
      if (aband_local[i] <= lambda*aband){
	PriorityQueue_push(pq, i, n - aband_local[i]);
      }
    }
    
    /* check critcal nodes u to see if any swaps with v are possible */
    while (PriorityQueue_pop(pq, &u, &gain)){
      aband_u = n - gain;
      p_u = p[u];
      assert(aband_u <= lambda*aband);
      assert(aband_u == aband_local[u]);
      swapped = FALSE;
      for (p_v = 0; p_v <= pmin[u] - aband_u; p_v++){
	v = p_inv[p_v];
	if (check_swap(n, ia, ja, u, p_u, v, p_v, aband_local, p, p_inv, aband, pq, pmax, pmin, lambda)){
	  swapped = TRUE; progress = TRUE;
	  break;
	}
      }
      if (swapped) continue;
      
      for (p_v = pmax[u] + aband_u; p_v < n; p_v++){
	v = p_inv[p_v];
	if (check_swap(n, ia, ja, u, p_u, v, p_v, aband_local, p, p_inv, aband, pq, pmax, pmin, lambda)){
	  swapped = TRUE; progress = TRUE;
	  break;
	}
      }
      if (swapped) continue;
      
      for (p_v = pmin[u] + aband_u; p_v <= pmax[u] - aband_u; p_v++) {
	v = p_inv[p_v];
	if (check_swap(n, ia, ja, u, p_u, v, p_v, aband_local, p, p_inv, aband, pq, pmax, pmin, lambda)){
	  progress = TRUE;
	  break;
	}
      }
      
      
    }
  }
  

  FREE(p_inv);
  FREE(pmax);
  FREE(pmin);
  FREE(aband_local);
  PriorityQueue_delete(pq);

}

void improve_antibandwidth_by_swapping(SparseMatrix A, int *p){
  int improved = TRUE;
  int n = A->m, i, j, *ia = A->ia, *ja = A->ja;
  real norm = n, norm1[3], norm2[3], norm11[3], norm22[3];
  real pi, pj;
  real start = clock();
  FILE *fp;
  
  if (Verbose){
    fprintf(stderr,"saving timing vs antiband data to timing_greedy\n");
    fp = fopen("timing_greedy","w");
  }
  assert(SparseMatrix_is_symmetric(A, TRUE));
  while (improved){
    improved = FALSE; norm = n;
    for (i = 0; i < n; i++){
      get_local_12_norm(n, i, ia, ja, p, norm1);
      for (j = 0; j < n; j++){
	if (j == i) continue;
	get_local_12_norm(n, j, ia, ja, p, norm2);
	norm = MIN(norm, norm2[0]);
	pi = (p)[i]; pj = (p)[j];
	(p)[i] = pj;
	(p)[j] = pi;
	get_local_12_norm(n, i, ia, ja, p, norm11);
	get_local_12_norm(n, j, ia, ja, p, norm22);
	if (MIN(norm11[0],norm22[0]) > MIN(norm1[0],norm2[0])){
	  //	    ||
	  //(MIN(norm11[0],norm22[0]) == MIN(norm1[0],norm2[0]) && norm11[1]+norm22[1] > norm1[1]+norm2[1])) {
	  improved = TRUE;
	  norm1[0] = norm11[0];
	  norm1[1] = norm11[1];
	  continue;
	}
	(p)[i] = pi;
	(p)[j] = pj;
      }
      if (i%100 == 0 && Verbose) {
	get_12_norm(n, ia, ja, p, norm1);
	fprintf(fp,"%f %f %f\n", (real) (clock() - start)/(CLOCKS_PER_SEC), norm1[0], norm1[2]);
      }
    }
    if (Verbose) {
      get_12_norm(n, ia, ja, p, norm1);
      fprintf(stderr, "aband = %f, aband_avg = %f\n", norm1[0], norm1[2]);
      fprintf(fp,"%f %f %f\n", (real) (clock() - start)/(CLOCKS_PER_SEC), norm1[0], norm1[2]);
    }
  }
}
  
void country_graph_coloring_internal(int seed, SparseMatrix A, int **p, real *norm_1, int do_swapping){
  int n = A->m, i, j, jj;
  SparseMatrix L, A2;
  int *ia = A->ia, *ja = A->ja;
  int a = -1.;
  real nrow;
  real *v = NULL;
  real norm1[3];
  clock_t start, start2;

  start = clock();
  assert(A->m == A->n);
  A2 = SparseMatrix_symmetrize(A, TRUE);
  ia = A2->ia; ja = A2->ja;

  /* Laplacian */
  L = SparseMatrix_new(n, n, 1, MATRIX_TYPE_REAL, FORMAT_COORD);
  for (i = 0; i < n; i++){
    nrow = 0.;
    for (j = ia[i]; j < ia[i+1]; j++){
      jj = ja[j];
      if (jj != i){
	nrow ++;
	L = SparseMatrix_coordinate_form_add_entries(L, 1, &i, &jj, &a);
      }
    }
    L = SparseMatrix_coordinate_form_add_entries(L, 1, &i, &i, &nrow);
  }
  L = SparseMatrix_from_coordinate_format(L);

  /* largest eigen vector */
  {
    int maxit = 100;
    real tol = 0.00001;
    real eig, *eigv;
    eigv = &eig;
    power_method(matvec_sparse, L, L->n, 1, seed, maxit, tol, &v, &eigv);
  }

  vector_ordering(n, v, p, TRUE);
  fprintf(stderr, "cpu time for spectral ordering (before greedy) = %f\n", (real) (clock() - start)/(CLOCKS_PER_SEC));

  start2 = clock();
  /* swapping */
  if (do_swapping) {
    if (do_swapping == DO_SWAPPING){
      improve_antibandwidth_by_swapping(A2, *p);
    } else if (do_swapping == DO_SWAPPING_CHEAP) {
      improve_antibandwidth_by_swapping_cheap(A2, *p);
    } else {
      assert(0);
    }
  }
  fprintf(stderr, "cpu time for greedy refinement = %f\n", (real) (clock() - start2)/(CLOCKS_PER_SEC));

  fprintf(stderr, "cpu time for spectral + greedy = %f\n", (real) (clock() - start)/(CLOCKS_PER_SEC));

  get_12_norm(n, ia, ja, *p, norm1);

  *norm_1 = norm1[0];
  if (A2 != A) SparseMatrix_delete(A2);
  SparseMatrix_delete(L);
}
void country_graph_coloring(int seed, SparseMatrix A, int **p, real *norm_1){
  country_graph_coloring_internal(seed, A, p, norm_1, DO_SWAPPING);
}

void country_graph_coloring_general(int seed, SparseMatrix A, int **p, real *norm_1, int do_swapping){
  country_graph_coloring_internal(seed, A, p, norm_1, do_swapping);
}


void improve_antibandwidth_by_swapping_for_fortran(int *n, int *nz, int *ja, int *ia, int *p, int *aprof, int *verbose){
  /* n: dimension
     nz: number of nonzeros
     ja: array of size nz, holds column indices. 1- based
     ia: array of size n+1, position of row starts. 1-based.
     p: holds the position of row/col i in the permuted matrix. 1-based
     aprof: aprof[0] is the antiband width on entry. aprof[1] is the abtiband on exit
     Verbose: a flag, when set to nonzero, will print the input matrix, as well as others things.
  */
  SparseMatrix A, A2;
  real norm1[3];
  int i, j, jj;
  clock_t start;
  real cpu;

  Verbose = *verbose;
  A = SparseMatrix_new(*n, *n, 1, MATRIX_TYPE_PATTERN, FORMAT_COORD);

  for (i = 0; i < *n; i++){
    for (j = ia[i] - 1; j < ia[i+1] - 1; j++){
      jj = ja[j] - 1;
      A = SparseMatrix_coordinate_form_add_entries(A, 1, &i, &jj, NULL);
    }
  }
  A2 = SparseMatrix_from_coordinate_format(A);
  if (Verbose && 0) SparseMatrix_print("A = ",A2);

  SparseMatrix_delete(A);
  A = SparseMatrix_symmetrize(A2, TRUE);

  for (i = 0; i < *n; i++) p[i]--;

  ia = A->ia; ja = A->ja;
  get_12_norm(*n, ia, ja, p, norm1);
  if (Verbose) fprintf(stderr,"on entry antibandwidth = %f\n", norm1[0]);
  aprof[0] = (int) norm1[0];

  start = clock();
  improve_antibandwidth_by_swapping(A, p);
  cpu = (clock() - start)/(CLOCKS_PER_SEC);
  fprintf(stderr,"cpu = %f\n", cpu);

  get_12_norm(*n, ia, ja, p, norm1);
  if (Verbose) fprintf(stderr,"on exit antibandwidth = %f\n", norm1[0]);
  aprof[1] = (int) norm1[0];
  SparseMatrix_delete(A);
  SparseMatrix_delete(A2);
  for (i = 0; i < *n; i++) p[i]++;
}

void improve_antibandwidth_by_swapping_for_fortran_(int *n, int *nz, int *ja, int *ia, int *p, int *aprof, int *Verbose){
  improve_antibandwidth_by_swapping_for_fortran(n, nz, ja, ia, p, aprof, Verbose);
}
void IMPROVE_ANTIBANDWIDTH_BY_SWAPPING_FOR_FORTRAN(int *n, int *nz, int *ja, int *ia, int *p, int *aprof, int *Verbose){
  improve_antibandwidth_by_swapping_for_fortran(n, nz, ja, ia, p, aprof, Verbose);
}
void IMPROVE_ANTIBANDWIDTH_BY_SWAPPING_FOR_FORTRAN_(int *n, int *nz, int *ja, int *ia, int *p, int *aprof, int *Verbose){
  improve_antibandwidth_by_swapping_for_fortran(n, nz, ja, ia, p, aprof, Verbose);
}




