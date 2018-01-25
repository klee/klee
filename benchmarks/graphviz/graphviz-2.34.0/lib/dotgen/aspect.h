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

#ifndef		ASPECT_H
#define		ASPECT_H

typedef struct aspect_t {
    double targetAR;      /* target aspect ratio */
    double combiAR;
    int prevIterations;   /* no. of iterations in previous pass */
    int curIterations;    /* no. of iterations in current pass */
    int nextIter;         /* dynamically adjusted no. of iterations */
    int nPasses;          /* bound on no. of top-level passes */
    int badGraph;         /* hack: set if graph is disconnected or has
                           * clusters. If so, turn off aspect */
} aspect_t;

extern aspect_t* setAspect (Agraph_t * g, aspect_t* adata);
extern void rank3(graph_t * g, aspect_t * asp);
extern void initEdgeTypes(graph_t * g);
extern void init_UF_size(graph_t * g);
extern int countDummyNodes(graph_t * g);

#endif				/* ASPECT_H */

