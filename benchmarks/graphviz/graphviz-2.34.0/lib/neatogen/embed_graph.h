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

#ifdef __cplusplus
extern "C" {
#endif



#ifndef EMBED_GRAPH_H_
#define EMBED_GRAPH_H_

#ifdef __cplusplus

    void embed_graph(vtx_data * graph, int n, int dim,
		     DistType ** (&coords), int);
    void center_coordinate(DistType ** coords, int n, int dim);
    void PCA(DistType ** coords, int dim, int n, double **(&new_coords),
	     int new_dim);
    void PCA(DistType ** coords, int dim, int n, double **(&new_coords),
	     int dim1, int dim2, boolean recompute);
    void PCA_orthog(DistType ** coords, int dim, int n,
		    double **(&new_coords), int new_dim, double *orthog);
    void iterativePCA(DistType ** coords, int dim, int n,
		      double **(&new_coords));

#else
#include <defs.h>

    extern void embed_graph(vtx_data * graph, int n, int dim, DistType ***,
			    int);
    extern void center_coordinate(DistType **, int, int);

#endif

#endif

#ifdef __cplusplus
}
#endif
