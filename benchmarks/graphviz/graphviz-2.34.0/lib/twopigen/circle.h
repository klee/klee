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

#ifndef CIRCLE_H
#define CIRCLE_H

#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
	unsigned long nStepsToLeaf;
	unsigned long subtreeSize;
	unsigned long nChildren;
	unsigned long nStepsToCenter;
	node_t *parent;
	double span;
	double theta;
    } rdata;

#define RDATA(n) ((rdata*)(ND_alg(n)))
#define SLEAF(n) (RDATA(n)->nStepsToLeaf)
#define STSIZE(n) (RDATA(n)->subtreeSize)
#define NCHILD(n) (RDATA(n)->nChildren)
#define SCENTER(n) (RDATA(n)->nStepsToCenter)
#define SPARENT(n) (RDATA(n)->parent)
#define SPAN(n) (RDATA(n)->span)
#define THETA(n) (RDATA(n)->theta)

    extern Agnode_t* circleLayout(Agraph_t * sg, Agnode_t * center);
    extern void twopi_layout(Agraph_t * g);
    extern void twopi_cleanup(Agraph_t * g);
    extern void twopi_init_graph(graph_t * g);

#ifdef __cplusplus
}
#endif
#endif
