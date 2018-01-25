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

#ifdef HAVE_ANN
//----------------------------------------------------------------------
//		File:			get_nearest_neighb_graph.cpp
//----------------------------------------------------------------------

#include <cstdlib>						// C standard library
#include <cstdio>						// C I/O (for sscanf)
#include <cstring>						// string manipulation
#include <fstream>						// file I/O
#include <ANN/ANN.h>					// ANN declarations

using namespace std;					// make std:: accessible
int                             dim                             = 4;                    // dimension


static void printPt(ostream &out, ANNpoint p)			// print point
{
  out << "" << p[0];
  for (int i = 1; i < dim; i++) {
    out << "," << p[i];
  }
  out << "";
}

static void sortPtsX(int n, ANNpointArray pts){
  /* sort so that edges always go from left to right in x-doordinate */
  ANNpoint p;
  ANNcoord x, y;
  int i, j;
  for (i = 0; i < n; i++){
    for (j = 0; j < dim; j++){
      p = pts[i];
      if (p[0] < p[2] || (p[0] == p[2] && p[1] < p[3])) continue;
      x = p[0]; y = p[1];
      p[0] = p[2];
      p[1] = p[3];
      p[2] = x;
      p[3] = y;
    }
  }
}

static void sortPtsY(int n, ANNpointArray pts){
  /* sort so that edges always go from left to right in x-doordinate */
  ANNpoint p;
  ANNcoord x, y;
  int i, j;
  for (i = 0; i < n; i++){
    for (j = 0; j < dim; j++){
      p = pts[i];
      if (p[1] < p[3] || (p[1] == p[3] && p[0] < p[2])) continue;
      x = p[0]; y = p[1];
      p[0] = p[2];
      p[1] = p[3];
      p[2] = x;
      p[3] = y;
    }
  }
}


extern "C" void nearest_neighbor_graph_ann(int nPts, int dim, int k, double eps, double *x, int *nz0, int **irn0, int **jcn0, double **val0);

void nearest_neighbor_graph_ann(int nPts, int dim, int k, double eps, double *x, int *nz0, int **irn0, int **jcn0, double **val0){

  /* Gives a nearest neighbor graph is a list of dim-dimendional points. The connectivity is in irn/jcn, and the distance in val.
     
    nPts: number of points
    dim: dimension
    k: number of neighbors needed
    eps: error tolerance
    x: nPts*dim vector. The i-th point is x[i*dim : i*dim + dim - 1]
    nz: number of entries in the connectivity matrix irn/jcn/val
    irn, jcn: the connectivity 
    val: the distance

    note that there could be repeates
  */

  ANNpointArray		dataPts;				// data points
  ANNidxArray			nnIdx;					// near neighbor indices
  ANNdistArray		dists;					// near neighbor distances
  ANNkd_tree*			kdTree;					// search structure
  
  double *xx;
  int *irn, *jcn;
  double *val;
  int nz;
  
  irn = *irn0;
  jcn = *jcn0;
  val = *val0;


  dataPts = annAllocPts(nPts, dim);			// allocate data points
  nnIdx = new ANNidx[k];						// allocate near neigh indices
  dists = new ANNdist[k];						// allocate near neighbor dists

  for (int i = 0; i < nPts; i++){
    xx =  dataPts[i];
    for (int j = 0; j < dim; j++) xx[j] = x[i*dim + j];
  }

  //========= graph when sort based on x ========
  nz = 0;
  sortPtsX(nPts, dataPts);
  kdTree = new ANNkd_tree(					// build search structure
			  dataPts,					// the data points
			  nPts,						// number of points
			  dim);						// dimension of space
  for (int ip = 0; ip < nPts; ip++){
    kdTree->annkSearch(						// search
		       dataPts[ip],						// query point
		       k,								// number of near neighbors
		       nnIdx,							// nearest neighbors (returned)
		       dists,							// distance (returned)
		       eps);							// error bound

    for (int i = 0; i < k; i++) {			// print summary
      if (nnIdx[i] == ip) continue;
      val[nz] = dists[i];
      irn[nz] = ip;
      jcn[nz++] = nnIdx[i];
      //cout << ip << "--" << nnIdx[i] << " [len = " << dists[i]<< ", weight = \"" << 1./(dists[i]) << "\", wt = \"" << 1./(dists[i]) << "\"]\n";
      //printPt(cout, dataPts[ip]);
      //cout << "--";
      //printPt(cout, dataPts[nnIdx[i]]);
      //cout << "\n";
    }
  }
  

  //========= graph when sort based on y ========
  sortPtsY(nPts, dataPts);
  kdTree = new ANNkd_tree(					// build search structure
			  dataPts,					// the data points
			  nPts,						// number of points
			  dim);						// dimension of space
  for (int ip = 0; ip < nPts; ip++){
    kdTree->annkSearch(						// search
		       dataPts[ip],						// query point
		       k,								// number of near neighbors
		       nnIdx,							// nearest neighbors (returned)
		       dists,							// distance (returned)
		       eps);							// error bound
      
    for (int i = 0; i < k; i++) {			// print summary
      if (nnIdx[i] == ip) continue;
      val[nz] = dists[i];
      irn[nz] = ip;
      jcn[nz++] = nnIdx[i];
      // cout << ip << "--" << nnIdx[i] << " [len = " << dists[i]<< ", weight = \"" << 1./(dists[i]) << "\", wt = \"" << 1./(dists[i]) << "\"]\n";
    }
  }
    
  delete [] nnIdx;							// clean things up
  delete [] dists;
  delete kdTree;
    
  *nz0 = nz;
    
  annClose();									// done with ANN



}
 
#endif  /* HAVE_ANN */
