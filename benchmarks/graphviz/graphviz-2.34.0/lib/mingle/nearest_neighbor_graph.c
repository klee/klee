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

#include "general.h"
#include "SparseMatrix.h"
#include "nearest_neighbor_graph_ann.h"
#include "nearest_neighbor_graph.h"

SparseMatrix nearest_neighbor_graph(int nPts, int num_neigbors, int dim, double *x, double eps){
  /* Gives a nearest neighbor graph of a list of dim-dimendional points. The result is a sparse matrix
     of nPts x nPts, with num_neigbors entries per row.

    nPts: number of points
    num_neigbors: number of neighbors needed
    dim: dimension
    eps: error tolerance
    x: nPts*dim vector. The i-th point is x[i*dim : i*dim + dim - 1]

  */
  int *irn = NULL, *jcn = NULL, nz;
  real *val = NULL;
  SparseMatrix A;
  int k = num_neigbors;

  /* need to *2 as we do two sweeps of neighbors, so could have repeats */
  irn =  MALLOC(sizeof(int)*nPts*k*2);
  jcn =  MALLOC(sizeof(int)*nPts*k*2);
  val =  MALLOC(sizeof(double)*nPts*k*2);

#ifdef HAVE_ANN
  nearest_neighbor_graph_ann(nPts, dim, num_neigbors, eps, x, &nz, &irn, &jcn, &val);

  A = SparseMatrix_from_coordinate_arrays(nz, nPts, nPts, irn, jcn, (void *) val, MATRIX_TYPE_REAL, sizeof(real));
#else
  A = NULL;
#endif

  FREE(irn);
  FREE(jcn);
  FREE(val);

  return A;


}
