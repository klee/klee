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

#ifndef MG_H
#define MG_H

typedef struct Multilevel_MQ_Clustering_struct *Multilevel_MQ_Clustering;

struct Multilevel_MQ_Clustering_struct {
  int level;/* 0, 1, ... */
  int n;
  SparseMatrix A; /* n x n matrix */
  SparseMatrix P; 
  SparseMatrix R; 
  Multilevel_MQ_Clustering next;
  Multilevel_MQ_Clustering prev;
  int delete_top_level_A;
  int *matching; /* dimension n. matching[i] is the clustering assignment of node i */

  /*

   .                                 |E(i,i)|                                     |E(i,j)|
   MQ/2 = (1/k) * \sum_{i=1...k} ------------ - (1/(k*(k-1))) * \sum_{i<j} -------------------  
   .                                |V(i)|^2                                   |V(i)|*|V(j)|
   .   = mq_in/k - mq_out/(k*(k-1))
  */

  real mq;
  real mq_in, mq_out;/* mqs(A) = deg_in(A)/|A|^2 - deg_out(A)/|A|/(|V|-|A|) */
  int ncluster; /* number of clusters */

  real *deg_intra;/* dimension n. deg[i] equal to the sum of edge weights within cluster i */
  real *dout;/* dimension n, dout[i] = \sum_{j -- i} a(i,j)/|j| is the scaled sum of outdegree */
  real *wgt; /* total vertex weight each coarse grid vertex represent */
};

/* find a clustering of vertices by maximize modularity quality
   A: symmetric square matrix n x n. If real value, value will be used as edges weights, otherwise edge weights are considered as 1.
   inplace: whether A can e modified. If true, A will be modified by removing diagonal.

   maxcluster: used to specify the maximum number of cluster desired, e.g., maxcluster=10 means that a maximum of 10 clusters
   .   is desired. this may not always be realized, and modularity quality may be low when this is specified. Default: maxcluster = 0 (no limit)

   use_value: whether to use the entry value, or treat edge weights as 1.
   nclusters: on output the number of clusters
   assignment: dimension n. Node i is assigned to cluster "assignment[i]". 0 <= assignment < nclusters.
   .   If *assignment = NULL on entry, it will be allocated. Otherwise used.
   mq: achieve modularity
*/
void mq_clustering(SparseMatrix A, int inplace, int maxcluster, int use_value,
			   int *nclusters, int **assignment, real *mq, int *flag);

#endif
