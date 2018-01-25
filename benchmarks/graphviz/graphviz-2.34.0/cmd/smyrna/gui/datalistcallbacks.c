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

#include "datalistcallbacks.h"
#include "tvnodes.h"
void btnTVEdit_clicked_cb(GtkWidget * widget, gpointer user_data)
{
}


void btnTVCancel_clicked_cb(GtkWidget * widget, gpointer user_data)
{
}
void btnTVOK_clicked_cb(GtkWidget * widget, gpointer user_data)
{
    gtk_widget_hide(glade_xml_get_widget(xml, "frmTVNodes"));
}

void on_TVNodes_close (GtkWidget * widget, gpointer user_data)
{
    gtk_widget_hide(glade_xml_get_widget(xml, "frmTVNodes"));
}

void btnTVReverse_clicked_cb(GtkWidget * widget, gpointer user_data)
{
}
void cgbTVSelect_toggled_cb(GtkWidget * widget, gpointer user_data)
{
}
void cgbTVVisible_toggled_cb(GtkWidget * widget, gpointer user_data)
{

}
void cgbTVHighlighted_toggled_cb(GtkWidget * widget, gpointer user_data)
{

}



void btnTVFilterApply_clicked_cb(GtkWidget * widget, gpointer user_data)
{
/*	GTK_RESPONSE_OK     = -5,
	GTK_RESPONSE_CANCEL = -6,
	GTK_RESPONSE_CLOSE  = -7,
	GTK_RESPONSE_YES    = -8,
	GTK_RESPONSE_NO     = -9,
	GTK_RESPONSE_APPLY  = -10,
	GTK_RESPONSE_HELP   = -11 */

    gtk_dialog_response((GtkDialog *)
			glade_xml_get_widget(xml, "dlgTVFilter"),
			GTK_RESPONSE_OK);

}


void btnTVShowAll_clicked_cb(GtkWidget * widget, gpointer user_data)
{
    tv_show_all();
}

void btnTVHideAll_clicked_cb(GtkWidget * widget, gpointer user_data)
{
    tv_hide_all();
}

void btnTVSaveAs_clicked_cb(GtkWidget * widget, gpointer user_data)
{
    tv_save_as(0);
}

void btnTVSaveWith_clicked_cb(GtkWidget * widget, gpointer user_data)
{
    tv_save_as(1);
}
