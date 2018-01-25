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

#ifndef SEARCH_G_H
#define SEARCH_G_H

#include "structures.h"

typedef struct snode snode;
typedef struct sedge sedge;

struct snode {
  int n_val, n_idx;
  snode* n_dad;
  sedge* n_edge;
  short   n_adj;
  short   save_n_adj;
  struct cell* cells[2];

    /* edges incident on this node 
     * -- stored as indices of the edges array in the graph
     */
  int* adj_edge_list;  
  int index;
  boolean isVert;  /* true if node corresponds to vertical segment */
};

struct sedge {
  double weight;    /* weight of edge */
  int cnt;          /* paths using edge */
      /* end-points of the edge 
       * -- stored as indices of the nodes vector in the graph
       */
  int v1, v2;      
};

typedef struct {
  int nnodes, nedges;
  int save_nnodes, save_nedges;
  snode* nodes;
  sedge* edges;
} sgraph;

extern void reset(sgraph*);
extern void gsave(sgraph*);
extern sgraph* createSGraph(int);
extern void freeSGraph (sgraph*);
extern void initSEdges (sgraph* g, int maxdeg);
extern int shortPath (sgraph* g, snode* from, snode* to);
extern snode* createSNode (sgraph*);
extern sedge* createSEdge (sgraph* g, snode* v0, snode* v1, double wt);

#endif
