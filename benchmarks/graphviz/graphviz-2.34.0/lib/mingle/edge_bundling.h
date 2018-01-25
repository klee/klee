/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/
#ifndef EDGE_BUNDLING_H
#define EDGE_BUNDLING_H

#include <SparseMatrix.h>

struct pedge_struct {
  real wgt; /* weight, telling how many original edges this edge represent. If this edge consists of multiple sections of different weights then this is a lower bound. This only applied for agglomerative bundling */
  int npoints;/* number of poly points */
  int len;/* length of arra x. len >= npoints */
  int dim;/* dim >= 2. Point i is stored from x[i*dim]*/
  real edge_length;
  real *x;/* coordinates of the npoints poly points. Dimension dim*npoints */
  real *wgts;/* number of original edges each section represnet. Dimension npoint - 1. This only applied for agglomerative bundling Null for other methods */
};

typedef struct pedge_struct* pedge;

pedge* edge_bundling(SparseMatrix A, int dim, real *x, int maxit_outer, real K, int method, int nneighbor, int compatibility_method, int max_recursion, real angle_param, real angle, int open_gl);
void pedge_delete(pedge e);
pedge pedge_realloc(pedge e, int np);
pedge pedge_wgts_realloc(pedge e, int n);
void pedge_export_mma(FILE *fp, int ne, pedge *edges);
void pedge_export_gv(FILE *fp, int ne, pedge *edges);
enum {METHOD_NONE = -1, METHOD_FD, METHOD_INK_AGGLOMERATE, METHOD_INK};
enum {COMPATIBILITY_DIST = 0, COMPATIBILITY_FULL};
pedge pedge_new(int np, int dim, real *x);
pedge pedge_wgt_new(int np, int dim, real *x, real wgt);
pedge pedge_double(pedge e);

/* flip the polyline so that last point becomes the first, second last the second, etc*/
pedge pedge_flip(pedge e);


#endif /* EDGE_BUNDLING_H */


