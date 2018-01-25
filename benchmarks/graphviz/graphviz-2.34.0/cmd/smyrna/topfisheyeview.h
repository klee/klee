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
#ifndef TOPFISHEYEVIEW_H
#define TOPFISHEYEVIEW_H

#include "smyrnadefs.h"
#include "hier.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
    void fisheye_polar(double x_focus, double y_focus, topview * t);
    void fisheye_spherical(double x_focus, double y_focus, double z_focus,
			   topview * t);
#endif
    void drawtopologicalfisheye(topview * t);
    void changetopfishfocus(topview * t, float *x, float *y,
			    float *z, int num_foci);
#if 0
    void drawtopologicalfisheye2(topview * t);
    int get_active_frame(topview * t);
    void get_interpolated_coords(double x0, double y0, double x1,
				 double y1, int fr, int total_fr,
				 double *x, double *y);
    void refresh_old_values(topview * t);
#endif
    void prepare_topological_fisheye(Agraph_t* g,topview * t);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
