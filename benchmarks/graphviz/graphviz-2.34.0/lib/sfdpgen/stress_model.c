#include "general.h"
#include "SparseMatrix.h"
#include "spring_electrical.h"
#include "post_process.h"
#include "stress_model.h"

void stress_model_core(int dim, SparseMatrix B, real **x, int edge_len_weighted, int maxit_sm, real tol, int *flag){
  int m;
  SparseStressMajorizationSmoother sm;
  real lambda = 0;
  /*int maxit_sm = 1000, i; tol = 0.001*/
  int i;
  SparseMatrix A = B;

  if (!SparseMatrix_is_symmetric(A, FALSE) || A->type != MATRIX_TYPE_REAL){
    if (A->type == MATRIX_TYPE_REAL){
      A = SparseMatrix_symmetrize(A, FALSE);
      A = SparseMatrix_remove_diagonal(A);
    } else {
      A = SparseMatrix_get_real_adjacency_matrix_symmetrized(A);
    } 
  }
  A = SparseMatrix_remove_diagonal(A);

  *flag = 0;
  m = A->m;
  if (!x) {
    *x = MALLOC(sizeof(real)*m*dim);
    srand(123);
    for (i = 0; i < dim*m; i++) (*x)[i] = drand();
  }

  if (edge_len_weighted){
    sm = SparseStressMajorizationSmoother_new(A, dim, lambda, *x, WEIGHTING_SCHEME_SQR_DIST, TRUE);/* do not under weight the long distances */
    //sm = SparseStressMajorizationSmoother_new(A, dim, lambda, *x, WEIGHTING_SCHEME_INV_DIST, TRUE);/* do not under weight the long distances */
  } else {
    sm = SparseStressMajorizationSmoother_new(A, dim, lambda, *x, WEIGHTING_SCHEME_NONE, TRUE);/* weight the long distances */
  }

  if (!sm) {
    *flag = -1;
    goto RETURN;
  }


  sm->tol_cg = 0.1; /* we found that there is no need to solve the Laplacian accurately */
  sm->scheme = SM_SCHEME_STRESS;
  SparseStressMajorizationSmoother_smooth(sm, dim, *x, maxit_sm, tol);
  for (i = 0; i < dim*m; i++) {
    (*x)[i] /= sm->scaling;
  }
  SparseStressMajorizationSmoother_delete(sm);

 RETURN:
  if (A != B) SparseMatrix_delete(A);

}


#ifdef GVIEWER
#include "gviewer.h"
#include "get_ps.h"
struct stress_model_data {
  int dim;
  SparseMatrix D;
  real **x;
  int edge_len_weighted;
  int maxit_sm;
  real tol;
  int *flag;
};

void stress_model_gv(void* data){
  struct stress_model_data* d;

  d = (struct stress_model_data*) data;
  return stress_model_core(d->dim, d->D, d->x, d->edge_len_weighted, d->maxit_sm, d->tol, d->flag);
}
void stress_model(int dim, SparseMatrix A, SparseMatrix D, real **x, int edge_len_weighted, int maxit_sm, real tol, int *flag){
  struct stress_model_data data = {dim, D, x, edge_len_weighted, maxit_sm, tol, flag};

  int argcc = 1;
  char **argvv;

  if (!Gviewer) return stress_model_core(dim, D, x, edge_len_weighted, maxit_sm, tol, flag);
  argcc = 1;
  argvv = malloc(sizeof(char*)*argcc);
  argvv[0] = malloc(sizeof(char));
  argvv[0][0] = '1';
  //  gviewer_init(&argcc, argvv, 0.1, 20, 60, 2*1010, 2*770, A, dim, *x, &(data), stress_model_gv);
  gviewer_set_edge_color_scheme(COLOR_SCHEME_NO);
  gviewer_toggle_bgcolor();
  gviewer_init(&argcc, argvv, 0.1, 20, 60, 720, 720, A, dim, *x, &(data), stress_model_gv);
  free(argvv);

}
#else
void stress_model(int dim, SparseMatrix A, SparseMatrix D, real **x, int edge_len_weighted, int maxit_sm, real tol, int *flag){
  return stress_model_core(dim, D, x, edge_len_weighted, maxit_sm, tol, flag);
}
#endif

