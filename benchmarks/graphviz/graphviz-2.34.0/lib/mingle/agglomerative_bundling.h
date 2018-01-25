/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#ifndef AGGLOMERATIVE_BUNDLING_H
#define AGGLOMERATIVE_BUNDLING_H

typedef struct Agglomerative_Ink_Bundling_struct *Agglomerative_Ink_Bundling;

struct Agglomerative_Ink_Bundling_struct {
  int level;/* 0, 1, ... */
  int n;
  SparseMatrix A; /* n x n matrix, where n is the number of edges/bundles in this level */
  SparseMatrix P; /* prolongation matrix from level + 1 to level */
  SparseMatrix R0;/* this is basically R[level - 1].R[level - 2]...R[0], which gives the map of bundling i to the original edges: first row of R0 gives
		     the nodes on the finest grid corresponding to the coarsest node 1, etc */
  SparseMatrix R;/* striction mtrix from level to level + 1*/
  Agglomerative_Ink_Bundling next;
  Agglomerative_Ink_Bundling prev;
  real *inks; /* amount of ink needed to draw this edge/bundle. Dimension n. */
  real total_ink; /* amount of ink needed to draw this edge/bundle. Dimension n. */
  pedge* edges; /* the original edge info. This does not vary level to level and is of dimenion n0, where n0 is the number of original edges */
  int delete_top_level_A;/*whether the top level matrix should be deleted on garbage collecting the grid */
};

pedge* agglomerative_ink_bundling(int dim, SparseMatrix A, pedge* edges, int nneighbor, int max_recursion, real angle_param, real angle, int open_gl, int *flag);

#endif /* AGGLOMERATIVE_BUNDLING_H */
