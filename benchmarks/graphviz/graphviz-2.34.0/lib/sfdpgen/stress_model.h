#ifndef STRESS_MODEL_H
#define STRESS_MODEL_H

void stress_model(int dim, SparseMatrix A, SparseMatrix D, real **x, int edge_len_weighted, int maxit, real tol, int *flag);

#endif
