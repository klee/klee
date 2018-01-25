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

#ifndef HIER_H
#define HIER_H

#include "hierarchy.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
	int num_foci;
	int *foci_nodes;	/* Nodes in real graph */
	double *x_foci;		/* Universal coordinates */
	double *y_foci;
    } focus_t;

/* Conversion from logical to physical coordinates:
 * NoRescale - simple copy
 * For Scale, Polar and Rectilinear, the coordinates are all
 * scaled and translated to map into the rectangle
 *   ((margin,margin),(w,h)) 
 * where w = width*graphSize/100 - 2*margin
 * and   h = height*graphSize/100 - 2*margin
 * 
 * For Scale, this is all that is done.
 * For Polar and Rectilinear, more space is provided around the foci. 
 */
    typedef enum { NoRescale, Scale, Polar, Rectilinear } RescaleType;

    typedef struct {
/* First 5 must be set i rescale = Polar or Rectilinear */
	int width;		/* viewport width */
	int height;		/* viewport height */
	int margin;		/* viewport margin */
	int graphSize;		/* 0 -- 100: percent to shrink w x h */
	double distortion;	/* default of 1.0 */
	RescaleType rescale;
    } reposition_t;

    void positionAllItems(Hierarchy * hp, focus_t * fs,
			  reposition_t * parms);
    Hierarchy *makeHier(int nnodes, int nedges, v_data *, double *,
			double *, hierparms_t *);

    focus_t *initFocus(int ncnt);
    void freeFocus(focus_t * fs);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
