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

#ifndef _DEFS_H_
#define _DEFS_H_

#include "neato.h"

#include "sparsegraph.h"

#ifdef DIGCOLA
#ifdef IPSEPCOLA
    typedef struct cluster_data {
	int nvars;         /* total count of vars in clusters */
        int nclusters;     /* number of clusters */
        int *clustersizes; /* number of vars in each cluster */
        int **clusters;    /* list of var indices for constituents of each c */
	int ntoplevel;     /* number of nodes not in any cluster */
	int *toplevel;     /* array of nodes not in any cluster */
	boxf *bb;	   /* bounding box of each cluster */
    } cluster_data;
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif
