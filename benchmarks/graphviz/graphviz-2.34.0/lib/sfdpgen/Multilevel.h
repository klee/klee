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

#ifndef MULTILEVEL_H
#define MULTILEVEL_H

#include "SparseMatrix.h"

typedef struct Multilevel_struct *Multilevel;

struct Multilevel_struct {
  int level;/* 0, 1, ... */
  int n;
  SparseMatrix A;/* the weighting matrix */
  SparseMatrix D;/* the distance matrix. A and D should have same pattern, 
		    but different entry values. For spring-electrical method, D = NULL. */
  SparseMatrix P; 
  SparseMatrix R; 
  real *node_weights;
  Multilevel next;
  Multilevel prev;
  int delete_top_level_A;
  int coarsen_scheme_used;/* to get from previous level to here */
};

enum {MAX_IND_VTX_SET_U = -100, MAX_IND_VTX_SET_F = -1, MAX_IND_VTX_SET_C = 0};

enum {MAX_CLUSTER_SIZE = 4};

enum {EDGE_BASED_STA, COARSEN_INDEPENDENT_EDGE_SET, COARSEN_INDEPENDENT_EDGE_SET_HEAVEST_EDGE_PERNODE, COARSEN_INDEPENDENT_EDGE_SET_HEAVEST_EDGE_PERNODE_LEAVES_FIRST, COARSEN_INDEPENDENT_EDGE_SET_HEAVEST_EDGE_PERNODE_SUPERNODES_FIRST, COARSEN_INDEPENDENT_EDGE_SET_HEAVEST_EDGE_PERNODE_DEGREE_SCALED, COARSEN_INDEPENDENT_EDGE_SET_HEAVEST_CLUSTER_PERNODE_LEAVES_FIRST, EDGE_BASED_STO, VERTEX_BASED_STA, COARSEN_INDEPENDENT_VERTEX_SET, COARSEN_INDEPENDENT_VERTEX_SET_RS, VERTEX_BASED_STO, COARSEN_HYBRID};

enum {COARSEN_MODE_GENTLE, COARSEN_MODE_FORCEFUL};

struct Multilevel_control_struct {
  int minsize;
  real min_coarsen_factor;
  int maxlevel;
  int randomize;
  int coarsen_scheme;
  int coarsen_mode;
};

typedef struct Multilevel_control_struct *Multilevel_control;

Multilevel_control Multilevel_control_new(int scheme, int mode);

void Multilevel_control_delete(Multilevel_control ctrl);

void Multilevel_delete(Multilevel grid);

Multilevel Multilevel_new(SparseMatrix A, SparseMatrix D, real *node_weights, Multilevel_control ctrl);

Multilevel Multilevel_get_coarsest(Multilevel grid);

void print_padding(int n);

#define Multilevel_is_finest(grid) (!((grid)->prev))
#define Multilevel_is_coarsest(grid) (!((grid)->next))

void Multilevel_coarsen(SparseMatrix A, SparseMatrix *cA, SparseMatrix D, SparseMatrix *cD, real *node_wgt, real **cnode_wgt,
			SparseMatrix *P, SparseMatrix *R, Multilevel_control ctrl, int *coarsen_scheme_used);
#endif
