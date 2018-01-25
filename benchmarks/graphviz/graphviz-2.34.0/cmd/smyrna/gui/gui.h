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

#ifndef GUI_H
#define GUI_H

#include "smyrnadefs.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkgl.h>
#include <glade/glade.h>
#include "callbacks.h"
#include "cgraph.h"
#include <agxbuf.h>

#define MAXIMUM_WIDGET_COUNT	97

#ifdef __cplusplus
extern "C" {
#endif

//GtkWidget *window1;           //main window
    extern GdkWindow *window1;
    extern GtkWidget *statusbar1;

    extern GladeXML *xml;	//global libglade vars
    extern GtkWidget *gladewidget;

    extern gve_element frmObjectTypeIndex;
    extern Agraph_t *frmObjectg;

    extern GtkComboBox *cbSelectGraph;	//combo at top left

    extern GtkWidget *AttrWidgets[MAXIMUM_WIDGET_COUNT];
    extern GtkWidget *AttrLabels[MAXIMUM_WIDGET_COUNT];
    extern int attr_widgets_modified[MAXIMUM_WIDGET_COUNT];
    extern int widgetcounter;	//number of attributes counted dynamically, might be removed in the future 
    extern attribute attr[MAXIMUM_WIDGET_COUNT];


    void create_object_properties(void);	//creates general purpose object properties template
    void object_properties_node_init(void);	//customize window for Nodes
    void object_properties_edge_init(void);	//customize window for Edges
    void object_properties_cluster_init(void);	//customize window for Cluster
    void object_properties_graph_init(void);	//customize window for Graph , this shows the graph default values
    void graph_properties_init(int newgraph);	//initialize little open graph dialog
    GtkComboBox *get_SelectGraph(void);	//freaking GLADE!!!!!
    int update_graph_properties(Agraph_t * graph);	//updates graph from gui
    void load_graph_properties(Agraph_t * graph);	//load from graph to gui

    void update_object_properties(int typeIndex, Agraph_t * g);	//updates objects from gui(node ,edge, cluster)
    int load_object_properties(gve_element typeIndex, Agraph_t * g);
    void load_attributes(void);	//loads attributes from a text file
    void change_selected_graph_attributes(Agraph_t * g, char *attrname,
					  char *attrvalue);
    void change_selected_node_attributes(Agraph_t * g, char *attrname,
					 char *attrvalue);
    void change_selected_edge_attributes(Agraph_t * g, char *attrname,
					 char *attrvalue);
    char *get_attribute_string_value_from_widget(attribute * att);


//GTK helpre functions
//void Color_Widget_bg (int r, int g, int b, GtkWidget *widget);        //change background color 
    void Color_Widget_bg(char *colorstring, GtkWidget * widget);
	void Color_Widget(char *colorstring, GtkWidget * widget);
/*generic warning pop up*/
    void show_gui_warning(char *str);
/*generic open file dialog*/
    int openfiledlg(int filtercnt, char **filters, agxbuf * xbuf);
/*generic save file dialog*/
    int savefiledlg(int filtercnt, char **filters, agxbuf * xbuf);
    void append_textview(GtkTextView * textv, const char *s, size_t bytes);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
