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

#ifndef GLMOTION_H
#define GLMOTION_H

#include "viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
    void glmotion_main(ViewInfo * v, GdkEventMotion * event,
		       GtkWidget * widget);
    void glmotion_rotate(ViewInfo * v);
    void glmotion_adjust_pan(ViewInfo * v, float panx, float pany);
    void graph_zoom(float real_zoom);
#endif

    void glmotion_zoom(ViewInfo * v);
    void glmotion_pan(ViewInfo * v);
    void glmotion_zoom_inc(int zoomin);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
