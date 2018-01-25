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

#ifndef SPARSEGRAPH_H
#define SPARSEGRAPH_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef __cplusplus
    enum Style { regular, invisible };
    struct vtx_data {
	int nedges;
	int *edges;
	float *ewgts;
	Style *styles;
	float *edists; /* directed dist reflecting the direction of the edge */
    };

    typedef int DistType;	/* must be signed!! */
#if 0
    inline double max(double x, double y) {
	if (x >= y)
	    return x;
	else
	    return y;
    } inline double min(double x, double y) {
	if (x <= y)
	    return x;
	else
	    return y;
    }

    inline int max(int x, int y) {
	if (x >= y)
	    return x;
	else
	    return y;
    }

    inline int min(int x, int y) {
	if (x <= y)
	    return x;
	else
	    return y;
    }
#endif

    struct Point {
	double x;
	double y;
	int operator==(Point other) {
	    return x == other.x && y == other.y;
    }};
#else

#ifdef USE_STYLES
    typedef enum { regular, invisible } Style;
#endif
    typedef struct {
	int nedges;		/* no. of neighbors, including self */
	int *edges;		/* edges[0..(nedges-1)] are neighbors; edges[0] is self */
	float *ewgts;		/* preferred edge lengths */
    } v_data; 

    typedef struct {
	int nedges;		/* no. of neighbors, including self */
	int *edges;		/* edges[0..(nedges-1)] are neighbors; edges[0] is self */
	float *ewgts;		/* preferred edge lengths */
	float *eweights;	/* edge weights */
#ifdef USE_STYLES
	Style *styles;
#endif
#ifdef DIGCOLA
	float *edists; /* directed dist reflecting the direction of the edge */
#endif
    } vtx_data;

    typedef int DistType;	/* must be signed!! */

extern void freeGraphData(vtx_data * graph);
extern void freeGraph(v_data * graph);

#endif

#endif

#ifdef __cplusplus
}
#endif
