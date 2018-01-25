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

#ifndef MENUCALLBACKS_H
#define MENUCALLBACKS_H

#include "gui.h"

#ifdef __cplusplus
extern "C" {
#endif

//file
    _BB void mAttributesSlot(GtkWidget * widget, gpointer user_data);
    _BB void mOpenSlot(GtkWidget * widget, gpointer user_data);
    _BB void mSaveSlot(GtkWidget * widget, gpointer user_data);
    _BB void mSaveAsSlot(GtkWidget * widget, gpointer user_data);
    _BB void mCloseSlot(GtkWidget * widget, gpointer user_data);
    _BB void mOptionsSlot(GtkWidget * widget, gpointer user_data);
    _BB void mQuitSlot(GtkWidget * widget, gpointer user_data);
//edit
    _BB void mCutSlot(GtkWidget * widget, gpointer user_data);
    _BB void mCopySlot(GtkWidget * widget, gpointer user_data);
    _BB void mPasteSlot(GtkWidget * widget, gpointer user_data);
    _BB void mDeleteSlot(GtkWidget * widget, gpointer user_data);
    _BB void mTopviewSettingsSlot(GtkWidget * widget, gpointer user_data);
    _BB void mNodeFindSlot(GtkWidget * widget, gpointer user_data);

//view
    _BB void mShowToolBoxSlot(GtkWidget * widget, gpointer user_data);
    _BB void mShowHostSelectionSlot(GtkWidget * widget,
				    gpointer user_data);
    _BB void mMenuPan(GtkWidget * widget, gpointer user_data);
    _BB void mMenuZoom(GtkWidget * widget, gpointer user_data);
    _BB void mShowConsoleSlot(GtkWidget * widget, gpointer user_data);
    _BB void mHideConsoleSlot(GtkWidget * widget, gpointer user_data);


//Graph
    _BB void mNodeListSlot(GtkWidget * widget, gpointer user_data);
    _BB void mNewNodeSlot(GtkWidget * widget, gpointer user_data);
    _BB void mNewEdgeSlot(GtkWidget * widget, gpointer user_data);
    _BB void mNewClusterSlot(GtkWidget * widget, gpointer user_data);
    _BB void mGraphPropertiesSlot(GtkWidget * widget, gpointer user_data);
    _BB void mClusterPropertiesSlot(GtkWidget * widget,
				    gpointer user_data);
    _BB void mNodePropertiesSlot(GtkWidget * widget, gpointer user_data);
    _BB void mEdgePropertiesSlot(GtkWidget * widget, gpointer user_data);
    _BB void mShowCodeSlot(GtkWidget * widget, gpointer user_data);
    _BB void mDotSlot(GtkWidget * widget, gpointer user_data);
    _BB void mNeatoSlot(GtkWidget * widget, gpointer user_data);
    _BB void mTwopiSlot(GtkWidget * widget, gpointer user_data);
    _BB void mCircoSlot(GtkWidget * widget, gpointer user_data);
    _BB void mFdpSlot(GtkWidget * widget, gpointer user_data);
    _BB void mSfdpSlot(GtkWidget * widget, gpointer user_data);

//select
    _BB void mSelectAllSlot(GtkWidget * widget, gpointer user_data);
    _BB void mUnselectAllSlot(GtkWidget * widget, gpointer user_data);
    _BB void mSelectAllNodesSlot(GtkWidget * widget, gpointer user_data);
    _BB void mSelectAllEdgesSlot(GtkWidget * widget, gpointer user_data);
    _BB void mSelectAllClustersSlot(GtkWidget * widget,
				    gpointer user_data);
    _BB void mUnselectAllNodesSlot(GtkWidget * widget, gpointer user_data);
    _BB void mUnselectAllEdgesSlot(GtkWidget * widget, gpointer user_data);
    _BB void mUnselectAllClustersSlot(GtkWidget * widget,
				      gpointer user_data);
    _BB void mSingleSelectSlot(GtkWidget * widget, gpointer user_data);
    _BB void mSelectAreaSlot(GtkWidget * widget, gpointer user_data);
    _BB void mSelectAreaXSlot(GtkWidget * widget, gpointer user_data);

//help
    _BB void mAbout(GtkWidget * widget, gpointer user_data);
    _BB void mHelp(GtkWidget * widget, gpointer user_data);
    _BB void mTestgvpr(GtkWidget * widget, gpointer user_data);
    void change_cursor(GdkCursorType C);
    int show_close_nosavedlg(void);

/*others from settings dialog*/
    _BB void on_gvprbuttonload_clicked(GtkWidget * widget,
				       gpointer user_data);
    _BB void on_gvprbuttonsave_clicked(GtkWidget * widget,
				       gpointer user_data);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
