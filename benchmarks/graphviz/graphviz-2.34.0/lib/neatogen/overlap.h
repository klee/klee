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

#ifndef OVERLAP_H
#define OVERLAP_H

#include "post_process.h"

typedef  StressMajorizationSmoother OverlapSmoother;

#define OverlapSmoother_struct StressMajorizationSmoother_struct

void OverlapSmoother_delete(OverlapSmoother sm);

OverlapSmoother OverlapSmoother_new(SparseMatrix A, int m,
                                    int dim, real lambda0, real *x, real *width, int include_original_graph, int neighborhood_only,
                                    real *max_overlap, real *min_overlap,
                                    int edge_labeling_scheme, int n_constr_nodes, int *constr_nodes, SparseMatrix A_constr, int shrink
                                    );

enum {ELSCHEME_NONE = 0, ELSCHEME_PENALTY, ELSCHEME_PENALTY2, ELSCHEME_STRAIGHTLINE_PENALTY, ELSCHEME_STRAIGHTLINE_PENALTY2};

struct relative_position_constraints_struct{
  real constr_penalty; /* penalty parameter used in making edge labels as much on the line as possible */
  int edge_labeling_scheme;/* specifying whether to treat node of the form |edgelabel|* as a special node representing an edge label.
                               0 (no action, default), 1 (penalty based method to make that kind of node close to the center of its neighbor),
                               2 (penalty based method to make that kind of node close to the "old" center of its neighbor),
                               3 (two step process of overlap removal and straightening) */
  int n_constr_nodes;/*n_constr_nodes: number of nodes that has constraints, these are nodes that is
                       constrained to be close to the average of its neighbors.*/
  int *constr_nodes;/*constr_nodes: a list of nodes that need to be constrained. If NULL, unused.*/
  int *irn;/* working arrays to hold the Laplacian of the constrain graph */
  int *jcn;
  real *val;
  SparseMatrix A_constr; /*A_constr: neighbors of node i are in the row i of this matrix. i needs to sit
                           in between these neighbors as much as possible. this must not be NULL
                           if constr_nodes != NULL.*/

};

typedef struct relative_position_constraints_struct*  relative_position_constraints;

real OverlapSmoother_smooth(OverlapSmoother sm, int dim, real *x);

void remove_overlap(int dim, SparseMatrix A, real *x, real *label_sizes, int ntry, real initial_scaling, 
		    int edge_labeling_scheme, int n_constr_nodes, int *constr_nodes, SparseMatrix A_constr, int *flag);

#endif
