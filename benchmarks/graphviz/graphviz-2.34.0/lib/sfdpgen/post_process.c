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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "memory.h"
#include "globals.h"

#include "sparse_solve.h"
#include "post_process.h"
#include "overlap.h"
#include "spring_electrical.h"
#include "call_tri.h"
#include "sfdpinternal.h"

#define node_degree(i) (ia[(i)+1] - ia[(i)])

#ifdef UNUSED
static void  get_neighborhood_precision_recall(char *outfile, SparseMatrix A0, real *ideal_dist_matrix, real *dist_matrix){
  SparseMatrix A = A0;
  int i, j, k, n = A->m;
  //  int *ia, *ja;
  int *g_order = NULL, *p_order = NULL;/* ordering using graph/physical distance */
  real *gdist, *pdist, radius;
  int np_neighbors;
  int ng_neighbors; /*number of (graph theoretical) neighbors */
  real node_dist;/* distance of a node to the center node */
  real true_positive;
  real recall;
  FILE *fp;
  
  fp = fopen(outfile,"w");

  if (!SparseMatrix_is_symmetric(A, FALSE)){
    A = SparseMatrix_symmetrize(A, FALSE);
  } 
  //  ia = A->ia;
  // ja = A->ja;

  for (k = 5; k <= 50; k+= 5){
    recall = 0;
    for (i = 0; i < n; i++){
      gdist = &(ideal_dist_matrix[i*n]);
      vector_ordering(n, gdist, &g_order, TRUE);
      pdist = &(dist_matrix[i*n]);
      vector_ordering(n, pdist, &p_order, TRUE);
      ng_neighbors = MIN(n-1, k); /* set the number of closest neighbor in the graph space to consider, excluding the node itself */
      np_neighbors = ng_neighbors;/* set the number of closest neighbor in the embedding to consider, excluding the node itself */
      radius = pdist[p_order[np_neighbors]];
      true_positive = 0;
      for (j = 1; j <= ng_neighbors; j++){
	node_dist = pdist[g_order[j]];/* the phisical distance for j-th closest node (in graph space) */
	if (node_dist <= radius) true_positive++;
      }
      recall += true_positive/np_neighbors;
    }
    recall /= n;

    fprintf(fp,"%d %f\n", k, recall);
  }
  fprintf(stderr,"wrote precision/recall in file %s\n", outfile);
  fclose(fp);

  if (A != A0) SparseMatrix_delete(A);
  FREE(g_order); FREE(p_order);
}



void dump_distance_edge_length(char *outfile, SparseMatrix A, int dim, real *x){
  int weighted = TRUE;
  int n, i, j, nzz;
  real *dist_matrix = NULL;
  int flag;
  real *dij, *xij, wij, top = 0, bot = 0, t;

  int *p = NULL;
  real dmin, dmax, xmax, xmin, bsta, bwidth, *xbin, x25, x75, median;
  int nbins = 30, nsta,  nz = 0;
  FILE *fp;

  fp = fopen(outfile,"w");

  flag = SparseMatrix_distance_matrix(A, weighted,  &dist_matrix);
  assert(!flag);

  n = A->m;
  dij = MALLOC(sizeof(real)*(n*(n-1)/2));
  xij = MALLOC(sizeof(real)*(n*(n-1)/2));
  for (i = 0; i < n; i++){
    for (j = i+1; j < n; j++){
      dij[nz] = dist_matrix[i*n+j];
      xij[nz] = distance(x, dim, i, j);
      if (dij[nz] > 0){
	wij = 1/(dij[nz]*dij[nz]);
      } else {
	wij = 1;
      }
      top += wij * dij[nz] * xij[nz];
      bot += wij*xij[nz]*xij[nz];   
      nz++;
    }
  }
  if (bot > 0){
    t = top/bot;
  } else {
    t = 1;
  }
  fprintf(stderr,"scaling factor = %f\n", t);

  for (i = 0; i < nz; i++) xij[i] *= t;

  vector_ordering(nz, dij, &p, TRUE);
  dmin = dij[p[0]];
  dmax = dij[p[nz-1]];
  nbins = MIN(nbins, dmax/MAX(1,dmin));/* scale by dmin since edge length of 1 is treated as 72 points in stress/maxent/full_stress */
  bwidth = (dmax - dmin)/nbins;

  nsta = 0; bsta = dmin;
  xbin = MALLOC(sizeof(real)*nz);
  nzz = nz;

  for (i = 0; i < nbins; i++){
    /* the bin is [dmin + i*(dmax-dmin)/nbins, dmin + (i+1)*(dmax-dmin)/nbins] */
    nz = 0; xmin = xmax = xij[p[nsta]];
    while (nsta < nzz && dij[p[nsta]] >= bsta && dij[p[nsta]] <= bsta + bwidth){
      xbin[nz++] = xij[p[nsta]];
      xmin = MIN(xmin, xij[p[nsta]]);
      xmax = MAX(xmax, xij[p[nsta]]);
      nsta++;
    }
    /* find the median, and 25/75% */
    if (nz > 0){
      median = vector_median(nz, xbin);
      x25 = vector_percentile(nz, xbin, 0.25);
      x75 = vector_percentile(nz, xbin, 0.75);
      fprintf(fp, "%d %f %f %f %f %f %f %f\n", nz, bsta, bsta + bwidth, xmin, x25, median, x75, xmax);
    } else {
      xmin = xmax = median = x25 = x75 = (bsta + 0.5*bwidth);
    }
    bsta += bwidth;
  }

  FREE(dij); FREE(xij); FREE(xbin); FREE(p);
  FREE(dist_matrix);

}

real get_full_stress(SparseMatrix A, int dim, real *x, int weighting_scheme){
  /* get the full weighted stress, \sum_ij w_ij (t ||x_i-x_j||^2-d_ij)^2, where t
     is the optimal scaling factor t = \sum w_ij ||x_i-x_j||^2/(\sum w_ij d_ij ||x_i - x_j||),

     Matrix A is assume to be sparse and a full distance matrix will be generated.
     We assume undirected graph and only check an undirected edge i--j, not i->j and j->i.
  */
  int weighted = TRUE;
  int n, i, j;
  real *ideal_dist_matrix = NULL, *dist_matrix;
  int flag;
  real t, top = 0, bot = 0, lstress, stress = 0, dij, wij, xij;
  real sne_disimilarity = 0;

  flag = SparseMatrix_distance_matrix(A, weighted,  &ideal_dist_matrix);
  assert(!flag);

  n = A->m;
  dist_matrix = MALLOC(sizeof(real)*n*n);

  for (i = 0; i < n; i++){
    for (j = i+1; j < n; j++){
      dij = ideal_dist_matrix[i*n+j];
      assert(dij >= 0);
      xij = distance(x, dim, i, j);
      if (dij > 0){
	switch (weighting_scheme){
	case WEIGHTING_SCHEME_SQR_DIST:
	  wij = 1/(dij*dij);
	  break;
	case WEIGHTING_SCHEME_INV_DIST:
	  wij = 1/(dij);
	  break;
	  break;
	case WEIGHTING_SCHEME_NONE:
	  wij = 1;
	default:
	  assert(0);
	}
      } else {
	wij = 1;
      }
      top += wij * dij * xij;
      bot += wij*xij*xij;
    }
  }
  if (bot > 0){
    t = top/bot;
  } else {
    t = 1;
  }

  for (i = 0; i < n; i++){
    dist_matrix[i*n+i] = 0.;
    for (j = i+1; j < n; j++){
      dij = ideal_dist_matrix[i*n+j];
      assert(dij >= 0);
      xij = distance(x, dim, i, j);
      dist_matrix[i*n+j] = xij*t;
      dist_matrix[j*n+i] = xij*t;
      if (dij > 0){
	wij = 1/(dij*dij);
      } else {
	wij = 1;
      }
      lstress = (xij*t - dij);
      stress += wij*lstress*lstress;
    }
  }

  {int K = 20;
    sne_disimilarity = get_sne_disimilarity(n, ideal_dist_matrix, dist_matrix, K);
  }

  get_neighborhood_precision_recall("/tmp/recall.txt", A, ideal_dist_matrix, dist_matrix);
  get_neighborhood_precision_recall("/tmp/precision.txt", A, dist_matrix, ideal_dist_matrix);

  fprintf(stderr,"sne_disimilarity = %f\n",sne_disimilarity);
  if (n > 0) fprintf(stderr,"sstress per edge = %f\n",stress/n/(n-1)*2);

  FREE(dist_matrix);  
  FREE(ideal_dist_matrix);
  return stress;
}
#endif


SparseMatrix ideal_distance_matrix(SparseMatrix A, int dim, real *x){
  /* find the ideal distance between edges, either 1, or |N[i] \Union N[j]| - |N[i] \Intersection N[j]|
   */
  SparseMatrix D;
  int *ia, *ja, i, j, k, l, nz;
  real *d;
  int *mask = NULL;
  real len, di, sum, sumd;

  assert(SparseMatrix_is_symmetric(A, FALSE));

  D = SparseMatrix_copy(A);
  ia = D->ia;
  ja = D->ja;
  if (D->type != MATRIX_TYPE_REAL){
    FREE(D->a);
    D->type = MATRIX_TYPE_REAL;
    D->a = N_GNEW(D->nz,real);
  }
  d = (real*) D->a;

  mask = N_GNEW(D->m,int);
  for (i = 0; i < D->m; i++) mask[i] = -1;

  for (i = 0; i < D->m; i++){
    di = node_degree(i);
    mask[i] = i;
    for (j = ia[i]; j < ia[i+1]; j++){
      if (i == ja[j]) continue;
      mask[ja[j]] = i;
    }
    for (j = ia[i]; j < ia[i+1]; j++){
      k = ja[j];
      if (i == k) continue;
      len = di + node_degree(k);
      for (l = ia[k]; l < ia[k+1]; l++){
	if (mask[ja[l]] == i) len--;
      }
      d[j] = len;
      assert(len > 0);
    }
    
  }

  sum = 0; sumd = 0;
  nz = 0;
  for (i = 0; i < D->m; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      if (i == ja[j]) continue;
      nz++;
      sum += distance(x, dim, i, ja[j]);
      sumd += d[j];
    }
  }
  sum /= nz; sumd /= nz;
  sum = sum/sumd;

  for (i = 0; i < D->m; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      if (i == ja[j]) continue;
      d[j] = sum*d[j];
    }
  }


  return D;
}


StressMajorizationSmoother StressMajorizationSmoother2_new(SparseMatrix A, int dim, real lambda0, real *x, 
							  int ideal_dist_scheme){
  /* use up to dist 2 neighbor */
  /* use up to dist 2 neighbor. This is used in overcoming pherical effect with ideal distance of
     2-neighbors equal graph distance etc.
   */
  StressMajorizationSmoother sm;
  int i, j, k, l, m = A->m, *ia = A->ia, *ja = A->ja, *iw, *jw, *id, *jd;
  int *mask, nz;
  real *d, *w, *lambda;
  real *avg_dist, diag_d, diag_w, dist, s = 0, stop = 0, sbot = 0;
  SparseMatrix ID;

  assert(SparseMatrix_is_symmetric(A, FALSE));

  ID = ideal_distance_matrix(A, dim, x);

  sm = GNEW(struct StressMajorizationSmoother_struct);
  sm->scaling = 1.;
  sm->data = NULL;
  sm->scheme = SM_SCHEME_NORMAL;
  sm->tol_cg = 0.01;
  sm->maxit_cg = sqrt((double) A->m);

  lambda = sm->lambda = N_GNEW(m,real);
  for (i = 0; i < m; i++) sm->lambda[i] = lambda0;
  mask = N_GNEW(m,int);

  avg_dist = N_GNEW(m,real);

  for (i = 0; i < m ;i++){
    avg_dist[i] = 0;
    nz = 0;
    for (j = ia[i]; j < ia[i+1]; j++){
      if (i == ja[j]) continue;
      avg_dist[i] += distance(x, dim, i, ja[j]);
      nz++;
    }
    assert(nz > 0);
    avg_dist[i] /= nz;
  }


  for (i = 0; i < m; i++) mask[i] = -1;

  nz = 0;
  for (i = 0; i < m; i++){
    mask[i] = i;
    for (j = ia[i]; j < ia[i+1]; j++){
      k = ja[j];
      if (mask[k] != i){
	mask[k] = i;
	nz++;
      }
    }
    for (j = ia[i]; j < ia[i+1]; j++){
      k = ja[j];
      for (l = ia[k]; l < ia[k+1]; l++){
	if (mask[ja[l]] != i){
	  mask[ja[l]] = i;
	  nz++;
	}
      }
    }
  }

  sm->Lw = SparseMatrix_new(m, m, nz + m, MATRIX_TYPE_REAL, FORMAT_CSR);
  sm->Lwd = SparseMatrix_new(m, m, nz + m, MATRIX_TYPE_REAL, FORMAT_CSR);
  if (!(sm->Lw) || !(sm->Lwd)) {
    StressMajorizationSmoother_delete(sm);
    return NULL;
  }

  iw = sm->Lw->ia; jw = sm->Lw->ja;

  w = (real*) sm->Lw->a; d = (real*) sm->Lwd->a;

  id = sm->Lwd->ia; jd = sm->Lwd->ja;
  iw[0] = id[0] = 0;

  nz = 0;
  for (i = 0; i < m; i++){
    mask[i] = i+m;
    diag_d = diag_w = 0;
    for (j = ia[i]; j < ia[i+1]; j++){
      k = ja[j];
      if (mask[k] != i+m){
	mask[k] = i+m;

	jw[nz] = k;
	if (ideal_dist_scheme == IDEAL_GRAPH_DIST){
	  dist = 1;
	} else if (ideal_dist_scheme == IDEAL_AVG_DIST){
	  dist = (avg_dist[i] + avg_dist[k])*0.5;
	} else if (ideal_dist_scheme == IDEAL_POWER_DIST){
	  dist = pow(distance_cropped(x,dim,i,k),.4);
	} else {
	  fprintf(stderr,"ideal_dist_scheme value wrong");
	  assert(0);
	  exit(1);
	}

	/*	
	  w[nz] = -1./(ia[i+1]-ia[i]+ia[ja[j]+1]-ia[ja[j]]);
	  w[nz] = -2./(avg_dist[i]+avg_dist[k]);*/
	/* w[nz] = -1.;*//* use unit weight for now, later can try 1/(deg(i)+deg(k)) */
	w[nz] = -1/(dist*dist);

	diag_w += w[nz];

	jd[nz] = k;
	/*
	  d[nz] = w[nz]*distance(x,dim,i,k);
	  d[nz] = w[nz]*dd[j];
	  d[nz] = w[nz]*(avg_dist[i] + avg_dist[k])*0.5;
	*/
	d[nz] = w[nz]*dist;
	stop += d[nz]*distance(x,dim,i,k);
	sbot += d[nz]*dist;
	diag_d += d[nz];

	nz++;
      }
    }

    /* distance 2 neighbors */
    for (j = ia[i]; j < ia[i+1]; j++){
      k = ja[j];
      for (l = ia[k]; l < ia[k+1]; l++){
	if (mask[ja[l]] != i+m){
	  mask[ja[l]] = i+m;

	  if (ideal_dist_scheme == IDEAL_GRAPH_DIST){
	    dist = 2;
	  } else if (ideal_dist_scheme == IDEAL_AVG_DIST){
	    dist = (avg_dist[i] + 2*avg_dist[k] + avg_dist[ja[l]])*0.5;
	  } else if (ideal_dist_scheme == IDEAL_POWER_DIST){
	    dist = pow(distance_cropped(x,dim,i,ja[l]),.4);
	  } else {
	    fprintf(stderr,"ideal_dist_scheme value wrong");
	    assert(0);
	    exit(1);
	  }

	  jw[nz] = ja[l];
	  /*
	    w[nz] = -1/(ia[i+1]-ia[i]+ia[ja[l]+1]-ia[ja[l]]);
	    w[nz] = -2/(avg_dist[i] + 2*avg_dist[k] + avg_dist[ja[l]]);*/
	  /* w[nz] = -1.;*//* use unit weight for now, later can try 1/(deg(i)+deg(k)) */

	  w[nz] = -1/(dist*dist);

	  diag_w += w[nz];

	  jd[nz] = ja[l];
	  /*
	    d[nz] = w[nz]*(distance(x,dim,i,k)+distance(x,dim,k,ja[l]));
	    d[nz] = w[nz]*(dd[j]+dd[l]);
	    d[nz] = w[nz]*(avg_dist[i] + 2*avg_dist[k] + avg_dist[ja[l]])*0.5;
	  */
	  d[nz] = w[nz]*dist;
	  stop += d[nz]*distance(x,dim,ja[l],k);
	  sbot += d[nz]*dist;
	  diag_d += d[nz];

	  nz++;
	}
      }
    }
    jw[nz] = i;
    lambda[i] *= (-diag_w);/* alternatively don't do that then we have a constant penalty term scaled by lambda0 */

    w[nz] = -diag_w + lambda[i];
    jd[nz] = i;
    d[nz] = -diag_d;
    nz++;

    iw[i+1] = nz;
    id[i+1] = nz;
  }
  s = stop/sbot;
  for (i = 0; i < nz; i++) d[i] *= s;

  sm->scaling = s;
  sm->Lw->nz = nz;
  sm->Lwd->nz = nz;

  FREE(mask);
  FREE(avg_dist);
  SparseMatrix_delete(ID);
  return sm;
}
	
StressMajorizationSmoother SparseStressMajorizationSmoother_new(SparseMatrix A, int dim, real lambda0, real *x,
								int weighting_scheme, int scale_initial_coord){
  /* solve a stress model to achieve the ideal distance among a sparse set of edges recorded in A.
     A must be a real matrix.
   */
  StressMajorizationSmoother sm;
  int i, j, k, m = A->m, *ia, *ja, *iw, *jw, *id, *jd;
  int nz;
  real *d, *w, *lambda;
  real diag_d, diag_w, *a, dist, s = 0, stop = 0, sbot = 0;
  real xdot = 0;

  assert(SparseMatrix_is_symmetric(A, FALSE) && A->type == MATRIX_TYPE_REAL);

  /* if x is all zero, make it random */
  for (i = 0; i < m*dim; i++) xdot += x[i]*x[i];
  if (xdot == 0){
    for (i = 0; i < m*dim; i++) x[i] = 72*drand();
  }

  ia = A->ia;
  ja = A->ja;
  a = (real*) A->a;


  sm = MALLOC(sizeof(struct StressMajorizationSmoother_struct));
  sm->scaling = 1.;
  sm->data = NULL;
  sm->scheme = SM_SCHEME_NORMAL;
  sm->D = A;
  sm->tol_cg = 0.01;
  sm->maxit_cg = sqrt((double) A->m);

  lambda = sm->lambda = MALLOC(sizeof(real)*m);
  for (i = 0; i < m; i++) sm->lambda[i] = lambda0;

  nz = A->nz;

  sm->Lw = SparseMatrix_new(m, m, nz + m, MATRIX_TYPE_REAL, FORMAT_CSR);
  sm->Lwd = SparseMatrix_new(m, m, nz + m, MATRIX_TYPE_REAL, FORMAT_CSR);
  if (!(sm->Lw) || !(sm->Lwd)) {
    StressMajorizationSmoother_delete(sm);
    return NULL;
  }

  iw = sm->Lw->ia; jw = sm->Lw->ja;
  id = sm->Lwd->ia; jd = sm->Lwd->ja;
  w = (real*) sm->Lw->a; d = (real*) sm->Lwd->a;
  iw[0] = id[0] = 0;

  nz = 0;
  for (i = 0; i < m; i++){
    diag_d = diag_w = 0;
    for (j = ia[i]; j < ia[i+1]; j++){
      k = ja[j];
      if (k != i){

	jw[nz] = k;
	dist = a[j];
	switch (weighting_scheme){
	case WEIGHTING_SCHEME_SQR_DIST:
	  if (dist*dist == 0){
	    w[nz] = -100000;
	  } else {
	    w[nz] = -1/(dist*dist);
	  }
	  break;
	case WEIGHTING_SCHEME_INV_DIST:
	  if (dist*dist == 0){
	    w[nz] = -100000;
	  } else {
	    w[nz] = -1/(dist);
	  }
	  break;
	case WEIGHTING_SCHEME_NONE:
	  w[nz] = -1;
	  break;
	default:
	  assert(0);
	  return NULL;
	}
	diag_w += w[nz];
	jd[nz] = k;
	d[nz] = w[nz]*dist;

	stop += d[nz]*distance(x,dim,i,k);
	sbot += d[nz]*dist;
	diag_d += d[nz];

	nz++;
      }
    }

    jw[nz] = i;
    lambda[i] *= (-diag_w);/* alternatively don't do that then we have a constant penalty term scaled by lambda0 */
    w[nz] = -diag_w + lambda[i];

    jd[nz] = i;
    d[nz] = -diag_d;
    nz++;

    iw[i+1] = nz;
    id[i+1] = nz;
  }
  if (scale_initial_coord){
    s = stop/sbot;
  } else {
    s = 1.;
  }
  if (s == 0) {
    return NULL;
  }
  for (i = 0; i < nz; i++) d[i] *= s;


  sm->scaling = s;
  sm->Lw->nz = nz;
  sm->Lwd->nz = nz;

  return sm;
}

static real total_distance(int m, int dim, real* x, real* y){
  real total = 0, dist = 0;
  int i, j;

  for (i = 0; i < m; i++){
    dist = 0.;
    for (j = 0; j < dim; j++){
      dist += (y[i*dim+j] - x[i*dim+j])*(y[i*dim+j] - x[i*dim+j]);
    }
    total += sqrt(dist);
  }
  return total;

}



void SparseStressMajorizationSmoother_delete(SparseStressMajorizationSmoother sm){
  return StressMajorizationSmoother_delete(sm);
}


real SparseStressMajorizationSmoother_smooth(SparseStressMajorizationSmoother sm, int dim, real *x, int maxit_sm, real tol){

  return StressMajorizationSmoother_smooth(sm, dim, x, maxit_sm, tol);


}
static void get_edge_label_matrix(relative_position_constraints data, int m, int dim, real *x, SparseMatrix *LL, real **rhs){
  int edge_labeling_scheme = data->edge_labeling_scheme;
  int n_constr_nodes = data->n_constr_nodes;
  int *constr_nodes = data->constr_nodes;
  SparseMatrix A_constr = data->A_constr;
  int *ia = A_constr->ia, *ja = A_constr->ja, ii, jj, nz, l, ll, i, j;
  int *irn = data->irn, *jcn = data->jcn;
  real *val = data->val, dist, kk, k;
  real *x00 = NULL;
  SparseMatrix Lc = NULL;
  real constr_penalty = data->constr_penalty;

  if (edge_labeling_scheme == ELSCHEME_PENALTY || edge_labeling_scheme == ELSCHEME_STRAIGHTLINE_PENALTY){
    /* for an node with two neighbors j--i--k, and assume i needs to be between j and k, then the contribution to P is
       .   i    j     k
       i   1   -0.5  -0.5
       j  -0.5 0.25  0.25
       k  -0.5 0.25  0.25
       in general, if i has m neighbors j, k, ..., then
       p_ii = 1
       p_jj = 1/m^2
       p_ij = -1/m
       p jk = 1/m^2
       .    i     j    k
       i    1    -1/m  -1/m ...
       j   -1/m  1/m^2 1/m^2 ...
       k   -1/m  1/m^2 1/m^2 ...
    */
    if (!irn){
      assert((!jcn) && (!val));
      nz = 0;
      for (i = 0; i < n_constr_nodes; i++){
	ii = constr_nodes[i];
	k = ia[ii+1] - ia[ii];/*usually k = 2 */
	nz += (k+1)*(k+1);
	
      }
      irn = data->irn = MALLOC(sizeof(int)*nz);
      jcn = data->jcn = MALLOC(sizeof(int)*nz);
      val = data->val = MALLOC(sizeof(double)*nz);
    }
    nz = 0;
    for (i = 0; i < n_constr_nodes; i++){
      ii = constr_nodes[i];
      jj = ja[ia[ii]]; ll = ja[ia[ii] + 1];
      if (jj == ll) continue; /* do not do loops */
      dist = distance_cropped(x, dim, jj, ll);
      dist *= dist;

      k = ia[ii+1] - ia[ii];/* usually k = 2 */
      kk = k*k;
      irn[nz] = ii; jcn[nz] = ii; val[nz++] = constr_penalty/(dist);
      k = constr_penalty/(k*dist); kk = constr_penalty/(kk*dist);
      for (j = ia[ii]; j < ia[ii+1]; j++){
	irn[nz] = ii; jcn[nz] = ja[j]; val[nz++] = -k;
      }
      for (j = ia[ii]; j < ia[ii+1]; j++){
	jj = ja[j];
	irn[nz] = jj; jcn[nz] = ii; val[nz++] = -k;
	for (l = ia[ii]; l < ia[ii+1]; l++){
	  ll = ja[l];
	  irn[nz] = jj; jcn[nz] = ll; val[nz++] = kk;
	}
      }
    }
    Lc = SparseMatrix_from_coordinate_arrays(nz, m, m, irn, jcn, val, MATRIX_TYPE_REAL, sizeof(real));
  } else if (edge_labeling_scheme == ELSCHEME_PENALTY2 || edge_labeling_scheme == ELSCHEME_STRAIGHTLINE_PENALTY2){
    /* for an node with two neighbors j--i--k, and assume i needs to be between the old position of j and k, then the contribution to P is
       1/d_jk, and to the right hand side: {0,...,average_position_of_i's neighbor if i is an edge node,...}
    */
    if (!irn){
      assert((!jcn) && (!val));
      nz = n_constr_nodes;
      irn = data->irn = MALLOC(sizeof(int)*nz);
      jcn = data->jcn = MALLOC(sizeof(int)*nz);
      val = data->val = MALLOC(sizeof(double)*nz);
    }
    x00 = MALLOC(sizeof(real)*m*dim);
    for (i = 0; i < m*dim; i++) x00[i] = 0;
    nz = 0;
    for (i = 0; i < n_constr_nodes; i++){
      ii = constr_nodes[i];
      jj = ja[ia[ii]]; ll = ja[ia[ii] + 1];
      dist = distance_cropped(x, dim, jj, ll);
      irn[nz] = ii; jcn[nz] = ii; val[nz++] = constr_penalty/(dist);
      for (j = ia[ii]; j < ia[ii+1]; j++){
	jj = ja[j];
	for (l = 0; l < dim; l++){
	  x00[ii*dim+l] += x[jj*dim+l];
	}
      }
      for (l = 0; l < dim; l++) {
	x00[ii*dim+l] *= constr_penalty/(dist)/(ia[ii+1] - ia[ii]);
      }
    }
    Lc = SparseMatrix_from_coordinate_arrays(nz, m, m, irn, jcn, val, MATRIX_TYPE_REAL, sizeof(real));
    
  }
  *LL = Lc;
  *rhs = x00;
}

real get_stress(int m, int dim, int *iw, int *jw, real *w, real *d, real *x, real scaling, void *data, int weighted){
  int i, j;
  real res = 0., dist;
  /* we use the fact that d_ij = w_ij*graph_dist(i,j). Also, d_ij and x are scalinged by *scaling, so divide by it to get actual unscaled streee. */
  for (i = 0; i < m; i++){
    for (j = iw[i]; j < iw[i+1]; j++){
      if (i == jw[j]) {
	continue;
      }
      dist = d[j]/w[j];/* both negative*/
      if (weighted){
	res += -w[j]*(dist - distance(x, dim, i, jw[j]))*(dist - distance(x, dim, i, jw[j]));
      } else {
	res += (dist - distance(x, dim, i, jw[j]))*(dist - distance(x, dim, i, jw[j]));
      }
    }
  }
  return 0.5*res/scaling/scaling;

}

static void uniform_stress_augment_rhs(int m, int dim, real *x, real *y, real alpha, real M){
  int i, j, k;
  real dist, distij;
  for (i = 0; i < m; i++){
    for (j = i+1; j < m; j++){
      dist = distance_cropped(x, dim, i, j);
      for (k = 0; k < dim; k++){
	distij = (x[i*dim+k] - x[j*dim+k])/dist;
	y[i*dim+k] += alpha*M*distij;
	y[j*dim+k] += alpha*M*(-distij);
      }
    }
  }
}

static real uniform_stress_solve(SparseMatrix Lw, real alpha, int dim, real *x0, real *rhs, real tol, int maxit, int *flag){
  Operator Ax;
  Operator Precon;

  Ax = Operator_uniform_stress_matmul(Lw, alpha);
  Precon = Operator_uniform_stress_diag_precon_new(Lw, alpha);

  return cg(Ax, Precon, Lw->m, dim, x0, rhs, tol, maxit, flag);

}

real StressMajorizationSmoother_smooth(StressMajorizationSmoother sm, int dim, real *x, int maxit_sm, real tol) {
  SparseMatrix Lw = sm->Lw, Lwd = sm->Lwd, Lwdd = NULL;
  int i, j, k, m, *id, *jd, *iw, *jw, idiag, flag = 0, iter = 0;
  real *w, *dd, *d, *y = NULL, *x0 = NULL, *x00 = NULL, diag, diff = 1, *lambda = sm->lambda, res, alpha = 0., M = 0.;
  SparseMatrix Lc = NULL;
  real dij, dist;


  Lwdd = SparseMatrix_copy(Lwd);
  m = Lw->m;
  x0 = N_GNEW(dim*m,real);
  if (!x0) goto RETURN;

  x0 = MEMCPY(x0, x, sizeof(real)*dim*m);
  y = N_GNEW(dim*m,real);
  if (!y) goto RETURN;

  id = Lwd->ia; jd = Lwd->ja;
  d = (real*) Lwd->a;
  dd = (real*) Lwdd->a;
  w = (real*) Lw->a;
  iw = Lw->ia; jw = Lw->ja;

#ifdef DEBUG_PRINT
  if (Verbose) fprintf(stderr, "initial stress = %f\n", get_stress(m, dim, iw, jw, w, d, x, sm->scaling, sm->data, 1));
#endif
  /* for the additional matrix L due to the position constraints */
  if (sm->scheme == SM_SCHEME_NORMAL_ELABEL){
    get_edge_label_matrix(sm->data, m, dim, x, &Lc, &x00);
    if (Lc) Lw = SparseMatrix_add(Lw, Lc);
  } else if (sm->scheme == SM_SCHEME_UNIFORM_STRESS){
    alpha = ((real*) (sm->data))[0];
    M = ((real*) (sm->data))[1];
  }

  while (iter++ < maxit_sm && diff > tol){
#ifdef GVIEWER
    if (Gviewer) {
      drawScene();
      if (iter%2 == 0) gviewer_dump_current_frame();
    }
#endif

    if (sm->scheme != SM_SCHEME_STRESS_APPROX){
      for (i = 0; i < m; i++){
	idiag = -1;
	diag = 0.;
	for (j = id[i]; j < id[i+1]; j++){
	  if (i == jd[j]) {
	    idiag = j;
	    continue;
	  }
	  
	  dist = distance(x, dim, i, jd[j]);
	  //if (d[j] >= -0.0001*dist){
	  //   /* sometimes d[j] = 0 and ||x_i-x_j||=0*/
	  //   dd[j] = d[j]/MAX(0.0001, dist);
	  if (d[j] == 0){
	    dd[j] = 0;
	  } else {
	    if (dist == 0){
	      dij = d[j]/w[j];/* the ideal distance */
	      /* perturb so points do not sit at the same place */
	      for (k = 0; k < dim; k++) x[jd[j]*dim+k] += 0.0001*(drand()+.0001)*dij;
	      dist = distance(x, dim, i, jd[j]);	
	    }
	    dd[j] = d[j]/dist;
	    
#if 0	  
	    /* if two points are at the same place, we do not want a huge entry,
	       as this cause problem with CG./ In any case, 
	       at thw limit d[j] == ||x[i] - x[jd[j]]||, 
	       or close. */
	    if (dist < -d[j]*0.0000001){
	      dd[j] = -10000.;
	    } else {
	      dd[j] = d[j]/dist;
	    }
#endif
	    
	  }
	diag += dd[j];
	}
	assert(idiag >= 0);
	dd[idiag] = -diag;
      }
      /* solve (Lw+lambda*I) x = Lwdd y + lambda x0 */

      SparseMatrix_multiply_dense(Lwdd, FALSE, x, FALSE, &y, FALSE, dim);
    } else {
      for (i = 0; i < m; i++){
	for (j = 0; j < dim; j++){
	  y[i*dim+j] = 0;/* for stress_approx scheme, the whole rhs is calculated in stress_maxent_augment_rhs */
	}
      }
    }

    if (lambda){/* is there a penalty term? */
      for (i = 0; i < m; i++){
	for (j = 0; j < dim; j++){
	  y[i*dim+j] += lambda[i]*x0[i*dim+j];
	}
      }
    } 

    /* additional term added to the rhs */
    switch (sm->scheme){
    case SM_SCHEME_NORMAL_ELABEL: {
      for (i = 0; i < m; i++){
	for (j = 0; j < dim; j++){
	  y[i*dim+j] += x00[i*dim+j];
	}
      }
      break;
    }
    case SM_SCHEME_UNIFORM_STRESS:{/* this part can be done more efficiently using octree approximation */
      uniform_stress_augment_rhs(m, dim, x, y, alpha, M);
      break;
    } 
#if UNIMPEMENTED
    case SM_SCHEME_MAXENT:{
#ifdef GVIEWER
      if (Gviewer){
	char *lab;
	lab = MALLOC(sizeof(char)*100);
	sprintf(lab,"maxent. alpha=%10.2g, iter=%d",stress_maxent_data_get_alpha(sm->data), iter);
	gviewer_set_label(lab);
	FREE(lab);
      }
#endif
      stress_maxent_augment_rhs_fast(sm, dim, x, y, &flag);
      if (flag) goto RETURN;
      break;
    }
    case SM_SCHEME_STRESS_APPROX:{
      stress_approx_augment_rhs(sm, dim, x, y, &flag);
      if (flag) goto RETURN;
      break;
    }
    case SM_SCHEME_STRESS:{
#ifdef GVIEWER
      if (Gviewer){
	char *lab;
	lab = MALLOC(sizeof(char)*100);
	sprintf(lab,"pmds(k), iter=%d", iter);
	gviewer_set_label(lab);
	FREE(lab);
      }
#endif
    }
#endif /* UNIMPEMENTED */
    default:
      break;
  }

#ifdef DEBUG_PRINT
    if (Verbose) {
      fprintf(stderr, "stress1 = %g\n",get_stress(m, dim, iw, jw, w, d, x, sm->scaling, sm->data, 1));
    }
#endif

    if (sm->scheme == SM_SCHEME_UNIFORM_STRESS){
      res = uniform_stress_solve(Lw, alpha, dim, x, y, sm->tol_cg, sm->maxit_cg, &flag);
    } else {
      res = SparseMatrix_solve(Lw, dim, x, y,  sm->tol_cg, sm->maxit_cg, SOLVE_METHOD_CG, &flag);
      //res = SparseMatrix_solve(Lw, dim, x, y,  sm->tol_cg, 1, SOLVE_METHOD_JACOBI, &flag);
    }

    if (flag) goto RETURN;
#ifdef DEBUG_PRINT
    if (Verbose) fprintf(stderr, "stress2 = %g\n",get_stress(m, dim, iw, jw, w, d, y, sm->scaling, sm->data, 1));
#endif
    diff = total_distance(m, dim, x, y)/sqrt(vector_product(m*dim, x, x));
#ifdef DEBUG_PRINT
    if (Verbose){
      fprintf(stderr, "Outer iter = %d, cg res = %g, ||x_{k+1}-x_k||/||x_k|| = %g\n",iter, res, diff);
    }
#endif


    MEMCPY(x, y, sizeof(real)*m*dim);
  }

#ifdef DEBUG
  _statistics[1] += iter-1;
#endif

#ifdef DEBUG_PRINT
  if (Verbose) fprintf(stderr, "iter = %d, final stress = %f\n", iter, get_stress(m, dim, iw, jw, w, d, x, sm->scaling, sm->data, 1));
#endif

 RETURN:
  SparseMatrix_delete(Lwdd);
  if (Lc) {
    SparseMatrix_delete(Lc);
    SparseMatrix_delete(Lw);
  }

  if (x0) FREE(x0);
  if (y) FREE(y);
  if (x00) FREE(x00);
  return diff;
  
}

void StressMajorizationSmoother_delete(StressMajorizationSmoother sm){
  if (!sm) return;
  if (sm->Lw) SparseMatrix_delete(sm->Lw);
  if (sm->Lwd) SparseMatrix_delete(sm->Lwd);
  if (sm->lambda) FREE(sm->lambda);
  if (sm->data) sm->data_deallocator(sm->data);
  FREE(sm);
}


TriangleSmoother TriangleSmoother_new(SparseMatrix A, int dim, real lambda0, real *x, int use_triangularization){
  TriangleSmoother sm;
  int i, j, k, m = A->m, *ia = A->ia, *ja = A->ja, *iw, *jw, jdiag, nz;
  SparseMatrix B;
  real *avg_dist, *lambda, *d, *w, diag_d, diag_w, dist;
  real s = 0, stop = 0, sbot = 0;

  assert(SparseMatrix_is_symmetric(A, FALSE));

  avg_dist = N_GNEW(m,real);

  for (i = 0; i < m ;i++){
    avg_dist[i] = 0;
    nz = 0;
    for (j = ia[i]; j < ia[i+1]; j++){
      if (i == ja[j]) continue;
      avg_dist[i] += distance(x, dim, i, ja[j]);
      nz++;
    }
    assert(nz > 0);
    avg_dist[i] /= nz;
  }

  sm = N_GNEW(1,struct TriangleSmoother_struct);
  sm->scaling = 1;
  sm->data = NULL;
  sm->scheme = SM_SCHEME_NORMAL;
  sm->tol_cg = 0.01;
  sm->maxit_cg = sqrt((double) A->m);

  lambda = sm->lambda = N_GNEW(m,real);
  for (i = 0; i < m; i++) sm->lambda[i] = lambda0;
  
  if (m > 2){
    if (use_triangularization){
      B= call_tri(m, dim, x);
    } else {
      B= call_tri2(m, dim, x);
    }
  } else {
    B = SparseMatrix_copy(A);
  }



  sm->Lw = SparseMatrix_add(A, B);

  SparseMatrix_delete(B);
  sm->Lwd = SparseMatrix_copy(sm->Lw);
  if (!(sm->Lw) || !(sm->Lwd)) {
    TriangleSmoother_delete(sm);
    return NULL;
  }

  iw = sm->Lw->ia; jw = sm->Lw->ja;

  w = (real*) sm->Lw->a; d = (real*) sm->Lwd->a;

  for (i = 0; i < m; i++){
    diag_d = diag_w = 0;
    jdiag = -1;
    for (j = iw[i]; j < iw[i+1]; j++){
      k = jw[j];
      if (k == i){
	jdiag = j;
	continue;
      }
      /*      w[j] = -1./(ia[i+1]-ia[i]+ia[ja[j]+1]-ia[ja[j]]);
	      w[j] = -2./(avg_dist[i]+avg_dist[k]);
	      w[j] = -1.*/;/* use unit weight for now, later can try 1/(deg(i)+deg(k)) */
      dist = pow(distance_cropped(x,dim,i,k),0.6);
      w[j] = 1/(dist*dist);
      diag_w += w[j];

      /*      d[j] = w[j]*distance(x,dim,i,k);
	      d[j] = w[j]*(avg_dist[i] + avg_dist[k])*0.5;*/
      d[j] = w[j]*dist;
      stop += d[j]*distance(x,dim,i,k);
      sbot += d[j]*dist;
      diag_d += d[j];

    }

    lambda[i] *= (-diag_w);/* alternatively don't do that then we have a constant penalty term scaled by lambda0 */

    assert(jdiag >= 0);
    w[jdiag] = -diag_w + lambda[i];
    d[jdiag] = -diag_d;
  }

  s = stop/sbot;
  for (i = 0; i < iw[m]; i++) d[i] *= s;
  sm->scaling = s;

  FREE(avg_dist);

  return sm;
}

void TriangleSmoother_delete(TriangleSmoother sm){

  StressMajorizationSmoother_delete(sm);

}

void TriangleSmoother_smooth(TriangleSmoother sm, int dim, real *x){

  StressMajorizationSmoother_smooth(sm, dim, x, 50, 0.001);
}




/* ================================ spring and spring-electrical based smoother ================ */
SpringSmoother SpringSmoother_new(SparseMatrix A, int dim, spring_electrical_control ctrl, real *x){
  SpringSmoother sm;
  int i, j, k, l, m = A->m, *ia = A->ia, *ja = A->ja, *id, *jd;
  int *mask, nz;
  real *d, *dd;
  real *avg_dist;
  SparseMatrix ID = NULL;

  assert(SparseMatrix_is_symmetric(A, FALSE));

  ID = ideal_distance_matrix(A, dim, x);
  dd = (real*) ID->a;

  sm = N_GNEW(1,struct SpringSmoother_struct);
  mask = N_GNEW(m,int);

  avg_dist = N_GNEW(m,real);

  for (i = 0; i < m ;i++){
    avg_dist[i] = 0;
    nz = 0;
    for (j = ia[i]; j < ia[i+1]; j++){
      if (i == ja[j]) continue;
      avg_dist[i] += distance(x, dim, i, ja[j]);
      nz++;
    }
    assert(nz > 0);
    avg_dist[i] /= nz;
  }


  for (i = 0; i < m; i++) mask[i] = -1;

  nz = 0;
  for (i = 0; i < m; i++){
    mask[i] = i;
    for (j = ia[i]; j < ia[i+1]; j++){
      k = ja[j];
      if (mask[k] != i){
	mask[k] = i;
	nz++;
      }
    }
    for (j = ia[i]; j < ia[i+1]; j++){
      k = ja[j];
      for (l = ia[k]; l < ia[k+1]; l++){
	if (mask[ja[l]] != i){
	  mask[ja[l]] = i;
	  nz++;
	}
      }
    }
  }

  sm->D = SparseMatrix_new(m, m, nz, MATRIX_TYPE_REAL, FORMAT_CSR);
  if (!(sm->D)){
    SpringSmoother_delete(sm);
    return NULL;
  }

  id = sm->D->ia; jd = sm->D->ja;
  d = (real*) sm->D->a;
  id[0] = 0;

  nz = 0;
  for (i = 0; i < m; i++){
    mask[i] = i+m;
    for (j = ia[i]; j < ia[i+1]; j++){
      k = ja[j];
      if (mask[k] != i+m){
	mask[k] = i+m;
	jd[nz] = k;
	d[nz] = (avg_dist[i] + avg_dist[k])*0.5;
	d[nz] = dd[j];
	nz++;
      }
    }

    for (j = ia[i]; j < ia[i+1]; j++){
      k = ja[j];
      for (l = ia[k]; l < ia[k+1]; l++){
	if (mask[ja[l]] != i+m){
	  mask[ja[l]] = i+m;
	  jd[nz] = ja[l];
	  d[nz] = (avg_dist[i] + 2*avg_dist[k] + avg_dist[ja[l]])*0.5;
	  d[nz] = dd[j]+dd[l];
	  nz++;
	}
      }
    }
    id[i+1] = nz;
  }
  sm->D->nz = nz;
  sm->ctrl = spring_electrical_control_new();
  *(sm->ctrl) = *ctrl;
  sm->ctrl->random_start = FALSE;
  sm->ctrl->multilevels = 1;
  sm->ctrl->step /= 2;
  sm->ctrl->maxiter = 20;

  FREE(mask);
  FREE(avg_dist);
  SparseMatrix_delete(ID);

  return sm;
}


void SpringSmoother_delete(SpringSmoother sm){
  if (!sm) return;
  if (sm->D) SparseMatrix_delete(sm->D);
  if (sm->ctrl) spring_electrical_control_delete(sm->ctrl);
}




void SpringSmoother_smooth(SpringSmoother sm, SparseMatrix A, real *node_weights, int dim, real *x){
  int flag = 0;

  spring_electrical_spring_embedding(dim, A, sm->D, sm->ctrl, node_weights, x, &flag);
  assert(!flag);

}

/*=============================== end of spring and spring-electrical based smoother =========== */

void post_process_smoothing(int dim, SparseMatrix A, spring_electrical_control ctrl, real *node_weights, real *x, int *flag){
#ifdef TIME
  clock_t  cpu;
#endif

#ifdef TIME
  cpu = clock();
#endif
  *flag = 0;

  switch (ctrl->smoothing){
  case SMOOTHING_RNG:
  case SMOOTHING_TRIANGLE:{
    TriangleSmoother sm;
    
    if (ctrl->smoothing == SMOOTHING_RNG){
      sm = TriangleSmoother_new(A, dim, 0, x, FALSE);
    } else {
      sm = TriangleSmoother_new(A, dim, 0, x, TRUE);
    }
    TriangleSmoother_smooth(sm, dim, x);
    TriangleSmoother_delete(sm);
    break;
  }
  case SMOOTHING_STRESS_MAJORIZATION_GRAPH_DIST:
  case SMOOTHING_STRESS_MAJORIZATION_POWER_DIST:
  case SMOOTHING_STRESS_MAJORIZATION_AVG_DIST:
    {
      StressMajorizationSmoother sm;
      int k, dist_scheme = IDEAL_AVG_DIST;

      if (ctrl->smoothing == SMOOTHING_STRESS_MAJORIZATION_GRAPH_DIST){
	dist_scheme = IDEAL_GRAPH_DIST;
      } else if (ctrl->smoothing == SMOOTHING_STRESS_MAJORIZATION_AVG_DIST){
	dist_scheme = IDEAL_AVG_DIST;
      } else if (ctrl->smoothing == SMOOTHING_STRESS_MAJORIZATION_POWER_DIST){
	dist_scheme = IDEAL_POWER_DIST;
      }

      for (k = 0; k < 1; k++){
	sm = StressMajorizationSmoother2_new(A, dim, 0.05, x, dist_scheme);
	StressMajorizationSmoother_smooth(sm, dim, x, 50, 0.001);
	StressMajorizationSmoother_delete(sm);
      }
      break;
    }
  case SMOOTHING_SPRING:{
    SpringSmoother sm;
    int k;

    for (k = 0; k < 1; k++){
      sm = SpringSmoother_new(A, dim, ctrl, x);
      SpringSmoother_smooth(sm, A, node_weights, dim, x);
      SpringSmoother_delete(sm);
    }

    break;
  }

  }/* end switch between smoothing methods */

#ifdef TIME
  if (Verbose) fprintf(stderr, "post processing %f\n",((real) (clock() - cpu)) / CLOCKS_PER_SEC);
#endif
}
