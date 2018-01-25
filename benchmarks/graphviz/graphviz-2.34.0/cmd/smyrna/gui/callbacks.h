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
#ifndef CALLBACKS_H
#define CALLBACKS_H 1

#include <gtk/gtk.h>
#include "toolboxcallbacks.h"
#include "gui.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef	WIN32			//this shit is needed on WIN32 to get libglade see the callback
#define _BB  __declspec(dllexport)
#else
#define _BB
#endif

    _BB void new_graph_clicked(GtkWidget * widget, gpointer user_data);
    _BB void open_graph_clicked(GtkWidget * widget, gpointer user_data);
    _BB void save_graph_clicked(GtkWidget * widget, gpointer user_data);
    _BB void save_as_graph_clicked(GtkWidget * widget, gpointer user_data);
    _BB void remove_graph_clicked(GtkWidget * widget, gpointer user_data);
    void on_newNode(GtkWidget * button, gpointer user_data);
    void dlgOpenGraph_OK_Clicked(GtkWidget * button, gpointer data);
    _BB void btn_dot_clicked(GtkWidget * widget, gpointer user_data);
    _BB void btn_neato_clicked(GtkWidget * widget, gpointer user_data);
    _BB void btn_twopi_clicked(GtkWidget * widget, gpointer user_data);
    _BB void btn_circo_clicked(GtkWidget * widget, gpointer user_data);
    _BB void btn_fdp_clicked(GtkWidget * widget, gpointer user_data);

    _BB void graph_select_change(GtkWidget * widget, gpointer user_data);
    _BB void mGraphPropertiesSlot(GtkWidget * widget, gpointer user_data);


//dlgOpenGraph btnOK clicked
    _BB void on_dlgOpenGraph_btnOK_clicked(GtkWidget * widget,
					   gpointer user_data);
//dlgOpenGraph btncancelclicked
    _BB void on_dlgOpenGraph_btncancel_clicked(GtkWidget * widget,
					       gpointer user_data);

//MENU 


//frm Object OK button
//hides frmObject , set changed values to selected objects
    _BB void frmObjectBtnOK_clicked(GtkWidget * widget,
				    gpointer user_data);

//frm Object Cancel button
//hides frmObject , ignores changed values

    _BB void frmObjectBtnCancel_clicked(GtkWidget * widget,
					gpointer user_data);


    _BB void attr_widgets_modifiedSlot(GtkWidget * widget,
				       gpointer user_data);


/*console output widgets*/
    _BB void on_clearconsolebtn_clicked(GtkWidget * widget,
					gpointer user_data);
    _BB void on_hideconsolebtn_clicked(GtkWidget * widget,
				       gpointer user_data);
    _BB void on_consoledecbtn_clicked(GtkWidget * widget,
				      gpointer user_data);
    _BB void on_consoleincbtn_clicked(GtkWidget * widget,
				      gpointer user_data);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
