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

#ifndef DATALISTCALLBACKS_H
#define DATALISTCALLBACKS_H


#include "gui.h"
#include "tvnodes.h"

#ifdef __cplusplus
extern "C" {
#endif

    _BB void btnTVEdit_clicked_cb(GtkWidget * widget, gpointer user_data);
    _BB void btnTVDelete_clicked_cb(GtkWidget * widget,
				    gpointer user_data);
    _BB void btnTVFilter_clicked_cb(GtkWidget * widget,
				    gpointer user_data);
    _BB void btnTVFirst_clicked_cb(GtkWidget * widget, gpointer user_data);
    _BB void btnTVPrevious_clicked_cb(GtkWidget * widget,
				      gpointer user_data);
    _BB void btnTVNext_clicked_cb(GtkWidget * widget, gpointer user_data);
    _BB void btnTVLast_clicked_cb(GtkWidget * widget, gpointer user_data);
    _BB void btnTVGotopage_clicked_cb(GtkWidget * widget,
				      gpointer user_data);
    _BB void btnTVCancel_clicked_cb(GtkWidget * widget,
				    gpointer user_data);
    _BB void btnTVOK_clicked_cb(GtkWidget * widget, gpointer user_data);
    _BB void btnTVReverse_clicked_cb(GtkWidget * widget,
				     gpointer user_data);
    _BB void cgbTVSelect_toggled_cb(GtkWidget * widget,
				    gpointer user_data);
    _BB void btnTVFilterApply_clicked_cb(GtkWidget * widget,
					 gpointer user_data);
    _BB void btnTVFilterClear_clicked_cb(GtkWidget * widget,
					 gpointer user_data);
    _BB void btnTVSelectAll_clicked_cb(GtkWidget * widget,
				       gpointer user_data);
    _BB void btnTVUnselectAll_clicked_cb(GtkWidget * widget,
					 gpointer user_data);
    _BB void btnTVHighlightAll_clicked_cb(GtkWidget * widget,
					  gpointer user_data);
    _BB void btnTVUnhighlightAll_clicked_cb(GtkWidget * widget,
					    gpointer user_data);
    _BB void cgbTVSelect_toggled_cb(GtkWidget * widget,
				    gpointer user_data);
    _BB void cgbTVVisible_toggled_cb(GtkWidget * widget,
				     gpointer user_data);
    _BB void cgbTVHighlighted_toggled_cb(GtkWidget * widget,
					 gpointer user_data);
    _BB void btnTVShowAll_clicked_cb(GtkWidget * widget,
				     gpointer user_data);
    _BB void btnTVHideAll_clicked_cb(GtkWidget * widget,
				     gpointer user_data);

    _BB void btnTVSaveAs_clicked_cb(GtkWidget * widget,
				    gpointer user_data);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
