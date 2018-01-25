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

#ifndef CLUSTERING_H
#define CLUSTERING_H

typedef struct Multilevel_Modularity_Clustering_struct *Multilevel_Modularity_Clustering;

struct Multilevel_Modularity_Clustering_struct {
  int level;/* 0, 1, ... */
  int n;
  SparseMatrix A; /* n x n matrix */
  SparseMatrix P; 
  SparseMatrix R; 
  Multilevel_Modularity_Clustering next;
  Multilevel_Modularity_Clustering prev;
  int delete_top_level_A;
  int *matching; /* dimension n. matching[i] is the clustering assignment of node i */
  real modularity;
  real deg_total; /* total edge weights, including self-edges */
  real *deg;/* dimension n. deg[i] equal to the sum of edge weights connected to vertex i. I.e., sum of  row i */
  int agglomerate_regardless;/* whether to agglomerate nodes even if this causes modularity reduction. This is used if we want to force
				agglomeration so as to get less clusters
			      */


};

enum {CLUSTERING_MODULARITY = 0, CLUSTERING_MQ};

/* find a clustering of vertices by maximize modularity
   A: symmetric square matrix n x n. If real value, value will be used as edges weights, otherwise edge weights are considered as 1.
   inplace: whether A can e modified. If true, A will be modified by removing diagonal.

   maxcluster: used to specify the maximum number of cluster desired, e.g., maxcluster=10 means that a maximum of 10 clusters
   .   is desired. this may not always be realized, and modularity may be low when this is specified. Default: maxcluster = 0 (no limit)

   use_value: whether to use the entry value, or treat edge weights as 1.
   nclusters: on output the number of clusters
   assignment: dimension n. Node i is assigned to cluster "assignment[i]". 0 <= assignment < nclusters.
   .   If *assignment = NULL on entry, it will be allocated. Otherwise used.
   modularity: achieve modularity
*/
void modularity_clustering(SparseMatrix A, int inplace, int maxcluster, int use_value,
			   int *nclusters, int **assignment, real *modularity, int *flag);

#endif
