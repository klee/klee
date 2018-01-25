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

#include <stdio.h>
#include "compat.h"
#include <stdlib.h>
#include "gui.h"
/* #include "abstring.h" */
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include "viewport.h"
#include "memory.h"


static char guibuffer[BUFSIZ];	//general purpose buffer

#ifdef WIN32
extern int strcasecmp(const char *s1, const char *s2);
extern int strncasecmp(const char *s1, const char *s2, unsigned int n);
#endif

GdkWindow *window1;
GtkWidget *statusbar1;

GladeXML *xml;			//global libglade vars
GtkWidget *gladewidget;

//1 subgraph 2 node 3 edge
int frmObjectTypeIndex;
Agraph_t *frmObjectg;


GtkComboBox *cbSelectGraph;	//combo at top left

GtkWidget *AttrWidgets[MAXIMUM_WIDGET_COUNT];
GtkWidget *AttrLabels[MAXIMUM_WIDGET_COUNT];
int attr_widgets_modified[MAXIMUM_WIDGET_COUNT];
int widgetcounter;		//number of attributes counted dynamically, might be removed in the future 
attribute attr[MAXIMUM_WIDGET_COUNT];



//loads object properties form and does some fixes
//call this function only ones
void create_object_properties(void)
{
#ifdef UNUSED
    char *data0 = "TEST0";
    char *data1 = "TEST1";
    char *data2 = "TEST2";
    char *data3 = "TEST3";
    char *data4 = "TEST4";
#endif
    GladeXML *xml;
    GtkWidget *widget;
    xml = glade_xml_new(smyrnaGlade, NULL, NULL);

    widget = glade_xml_get_widget(xml, "listPoints");
    gtk_clist_set_column_title((GtkCList *) widget, 0, "Def");
    gtk_clist_set_column_title((GtkCList *) widget, 1, "x");
    gtk_clist_set_column_title((GtkCList *) widget, 2, "y");
    gtk_clist_set_column_title((GtkCList *) widget, 3, "z");
    gtk_clist_column_titles_show((GtkCList *) widget);
    gtk_widget_show(widget);
    widget = glade_xml_get_widget(xml, "win");
    gtk_widget_show(widget);


}

//call this after create_object_properties()
void object_properties_node_init(void)
{

}
void object_properties_edge_init(void)	//customize window for Edges
{
}
void object_properties_cluster_init(void)	//customize window for Cluster
{

}
void object_properties_graph_init(void)	//customize window for Graph , this shows the graph default values
{

}



GtkComboBox *get_SelectGraph(void)
{
    GtkComboBox *cb;
    if (!cbSelectGraph) {

	cb = (GtkComboBox *) gtk_combo_box_new_text();
	gtk_widget_show((GtkWidget *) cb);
	gtk_box_pack_start(GTK_BOX(gladewidget), (GtkWidget *) cb, FALSE,
			   FALSE, 0);


	gtk_layout_put((GtkLayout *) gladewidget, (GtkWidget *) cb, 780,
		       3);
	//signal


	return cb;
    } else
	return cbSelectGraph;
}

void Color_Widget(char *colorstring, GtkWidget * widget)
{
  GtkRcStyle *rc_style;
  GdkColor color;

  gdk_color_parse (colorstring, &color);


  rc_style = gtk_rc_style_new ();
  /* Set foreground (fg) color in normal state to red */
  rc_style->fg[GTK_STATE_NORMAL] = color;
  rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_FG;

  gtk_widget_modify_style (widget, rc_style);
  gtk_rc_style_unref (rc_style);
}

void Color_Widget_bg(char *colorstring, GtkWidget * widget)
{
    GdkColor color;
    gdk_color_parse(colorstring, &color);
    gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, &color);
    gtk_widget_modify_base(widget, GTK_STATE_NORMAL, &color);
}


#ifdef UNUSED
void Color_Widget_fg(int r, int g, int b, GtkWidget * widget)
{
    GdkColor color;
    gdk_color_parse("red", &color);
    gtk_widget_modify_fg(widget, GTK_STATE_NORMAL, &color);

}
#endif

void load_graph_properties(Agraph_t * graph)
{
    //dlgOpenGraph , GtkDialog
    gtk_entry_set_text((GtkEntry *)
		       glade_xml_get_widget(xml, "entryGraphFileName"),
		       view->Topview->Graphdata.GraphFileName);
}

int update_graph_properties(Agraph_t * graph)	//updates graph from gui
{
    FILE *file;
    int respond = 0;

    //check if file is changed
    if (strcasecmp
	(gtk_entry_get_text
	 ((GtkEntry *) glade_xml_get_widget(xml, "entryGraphFileName")),
	 view->Topview->Graphdata.GraphFileName) != 0) {


	if ((file = fopen(gtk_entry_get_text((GtkEntry *)
					     glade_xml_get_widget(xml,
								  "entryGraphFileName")),
			  "r"))) {
	    fclose(file);
	    Dlg = (GtkMessageDialog *) gtk_message_dialog_new(NULL,
							      GTK_DIALOG_MODAL,
							      GTK_MESSAGE_QUESTION,
							      GTK_BUTTONS_YES_NO,
							      "File name you have entered already exists\n,this will cause overwriting on existing file.\nAre you sure?");
	    respond = gtk_dialog_run((GtkDialog *) Dlg);
	    gtk_object_destroy((GtkObject *) Dlg);

	    if (respond == GTK_RESPONSE_NO)
		return 0;
	}
	//now check if filename is legal, try to open it to write
	if ((file = fopen(gtk_entry_get_text((GtkEntry *)
					     glade_xml_get_widget(xml,
								  "entryGraphFileName")),
			  "w")))
	    fclose(file);
	else {
	    Dlg = (GtkMessageDialog *) gtk_message_dialog_new(NULL,
							      GTK_DIALOG_MODAL,
							      GTK_MESSAGE_WARNING,
							      GTK_BUTTONS_OK,
							      "File name is invalid or I/O error!");

	    respond = gtk_dialog_run((GtkDialog *) Dlg);
	    gtk_object_destroy((GtkObject *) Dlg);
	    GTK_DIALOG(Dlg);

	    return 0;
	}

    }

    //if it comes so far graph deserves new values

    view->Topview->Graphdata.GraphFileName =
	(char *) gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml,
							 "entryGraphFileName"));
    return 1;
}



char *get_attribute_string_value_from_widget(attribute * att)
{
    GdkColor color;
    switch (att->Type) {
    case 'F':
	sprintf(guibuffer, "%f",
		gtk_spin_button_get_value((GtkSpinButton *) att->
					  attrWidget));
	return guibuffer;
	break;
    case 'C':
	gtk_color_button_get_color((GtkColorButton *) att->attrWidget,
				   &color);
	sprintf(guibuffer, "#%x%x%x", color.red / 255, color.green / 255,
		color.blue / 255);
	return guibuffer;
	break;
    default:
	strcpy(guibuffer,
	       gtk_entry_get_text((GtkEntry *) att->attrWidget));
	return guibuffer;
    }
}
#if UNUSED
void change_selected_graph_attributes(Agraph_t * g, char *attrname,
				      char *attrvalue)
{
    agattr(g, AGRAPH, attrname, "");
    agset(view->g[view->activeGraph], attrname, attrvalue);


}
void change_selected_node_attributes(Agraph_t * g, char *attrname,
				     char *attrvalue)
{
    int ind = 0;
    Agsym_t* ap = agattr(g, AGNODE, attrname, NULL);

    if (!ap)
	ap = agattr(g, AGNODE, attrname, "");
	
    for (ind = 0; ind < view->Topview->Nodecount; ind++) {
	if (view->Topview->Nodes[ind].data.Selected == 1)
	    agxset(view->Topview->Nodes[ind].Node, ap, attrvalue);
    }
}
void change_selected_edge_attributes(Agraph_t * g, char *attrname,
				     char *attrvalue)
{
    int ind = 0;
    Agsym_t* ap = agattr(g, AGEDGE, attrname, NULL);

    if (!ap)
	ap = agattr(g, AGEDGE, attrname, "");
	
    for (ind = 0; ind < view->Topview->Edgecount; ind++) {
	if (view->Topview->Edges[ind].data.Selected == 1)
	    agxset(view->Topview->Edges[ind].Edge, ap, attrvalue);
    }
}
#endif

void load_attributes(void)
{
    FILE *file;
    char line[BUFSIZ];
    char *ss;
    char *pch;
    int ind = 0;
    int attrcount = 0;
    static char *smyrna_attrs;

    if (!smyrna_attrs) {
	smyrna_attrs = smyrnaPath("attrs.txt");
    }
    //loads attributes from a text file
    file = fopen(smyrna_attrs, "r");
    if (file != NULL) {
	while (fgets(line, sizeof line, file) != NULL) {
	    pch = strtok(line, ",");
	    ind = 0;
	    while (pch != NULL) {
		ss = strdup(pch);
		//                              ABRemove(&ss,'\"');
		//                              ABRemove(&ss,' ');
		pch = strtok(NULL, ",");
		switch (ind) {
		case 0:
		    attr[attrcount].Type = ss[0];
		    break;
		case 1:
		    attr[attrcount].Name = strdup(ss);
		    break;
		case 2:
		    attr[attrcount].Default = strdup(ss);
		    break;
		case 3:
		    if (strstr(ss, "ANY_ELEMENT")) {
			attr[attrcount].ApplyTo[GVE_GRAPH] = 1;
			attr[attrcount].ApplyTo[GVE_CLUSTER] = 1;
			attr[attrcount].ApplyTo[GVE_NODE] = 1;
			attr[attrcount].ApplyTo[GVE_EDGE] = 1;
		    } else {
			attr[attrcount].ApplyTo[GVE_GRAPH] =
			    strstr(ss, "GRAPH") ? 1 : 0;
			attr[attrcount].ApplyTo[GVE_CLUSTER] =
			    strstr(ss, "CLUSTER") ? 1 : 0;
			attr[attrcount].ApplyTo[GVE_NODE] =
			    strstr(ss, "NODE") ? 1 : 0;
			attr[attrcount].ApplyTo[GVE_EDGE] =
			    strstr(ss, "EDGE") ? 1 : 0;
		    }
		    break;
		case 4:
		    if (strstr(ss, "ALL_ENGINES")) {
			attr[attrcount].Engine[GVK_DOT] = 1;
			attr[attrcount].Engine[GVK_NEATO] = 1;
			attr[attrcount].Engine[GVK_TWOPI] = 1;
			attr[attrcount].Engine[GVK_CIRCO] = 1;
			attr[attrcount].Engine[GVK_FDP] = 1;
		    } else {
			attr[attrcount].Engine[GVK_DOT] =
			    strstr(ss, "DOT") ? 1 : 0;
			attr[attrcount].Engine[GVK_NEATO] =
			    strstr(ss, "NEATO") ? 1 : 0;
			attr[attrcount].Engine[GVK_TWOPI] =
			    strstr(ss, "TWOPI") ? 1 : 0;
			attr[attrcount].Engine[GVK_CIRCO] =
			    strstr(ss, "CIRCO") ? 1 : 0;
			attr[attrcount].Engine[GVK_FDP] =
			    strstr(ss, "FDP") ? 1 : 0;
		    }
		    break;
		default:
		    attr[attrcount].ComboValues =
			RALLOC(attr[attrcount].ComboValuesCount,
			       attr[attrcount].ComboValues, char *);
		    attr[attrcount].ComboValues[attr[attrcount].
						ComboValuesCount] =
			strdup(ss);
		    attr[attrcount].ComboValuesCount++;
		    break;
		}
		ind++;
	    }
	    attrcount++;
	}
	fclose (file);
    }
}
void show_gui_warning(char *str)
{
    Dlg = (GtkMessageDialog *) gtk_message_dialog_new(NULL,
						      GTK_DIALOG_MODAL,
						      GTK_MESSAGE_WARNING,
						      GTK_BUTTONS_OK, str);

    respond = gtk_dialog_run((GtkDialog *) Dlg);
    gtk_object_destroy((GtkObject *) Dlg);
}



/*
Generic Open File dialog, if a file is selected and return value is 1, else 0
file name is copied to char* filename,which should be allocated before using the function
*/
int openfiledlg(int filtercnt, char **filters, agxbuf * xbuf)
{
    GtkWidget *dialog;
    GtkFileFilter *filter;
    int id, rv;
    filter = gtk_file_filter_new();
    if (filtercnt >= 1) {
	for (id = 0; id < filtercnt; id++) {
	    gtk_file_filter_add_pattern(filter, filters[id]);
	}
    }

    dialog = gtk_file_chooser_dialog_new("Open File",
					 NULL,
					 GTK_FILE_CHOOSER_ACTION_OPEN,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 GTK_STOCK_OPEN,
					 GTK_RESPONSE_ACCEPT, NULL);

    if (filtercnt >= 1)
	gtk_file_chooser_set_filter((GtkFileChooser *) dialog, filter);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	agxbput(xbuf,
		gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));
	rv = 1;
    } else
	rv = 0;

    gtk_widget_destroy(dialog);
    return rv;
}

int savefiledlg(int filtercnt, char **filters, agxbuf * xbuf)
{
    GtkWidget *dialog;
    GtkFileFilter *filter;
    int id, rv;
    filter = gtk_file_filter_new();
    if (filtercnt >= 1) {
	for (id = 0; id < filtercnt; id++) {
	    gtk_file_filter_add_pattern(filter, filters[id]);
	}
    }

    dialog = gtk_file_chooser_dialog_new("Save File",
					 NULL,
					 GTK_FILE_CHOOSER_ACTION_OPEN,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 GTK_STOCK_OPEN,
					 GTK_RESPONSE_ACCEPT, NULL);

    if (filtercnt >= 1)
	gtk_file_chooser_set_filter((GtkFileChooser *) dialog, filter);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	agxbput(xbuf,
		gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));
	rv = 1;
    } else
	rv = 0;

    gtk_widget_destroy(dialog);
    return rv;
}

/*
	this function is designed to return a GtkTextView object's text in agxbuf
	send an initialized agxbuf and a GtkTextView object
	null termination is taken care by agxbuf
*/
void get_gtktextview_text(GtkTextView * w, agxbuf * xbuf)
{
    int charcnt;
    GtkTextBuffer *gtkbuf;
    GtkTextIter startit;
    GtkTextIter endit;
    gtkbuf = gtk_text_view_get_buffer(w);
    charcnt = gtk_text_buffer_get_char_count(gtkbuf);
    gtk_text_buffer_get_start_iter(gtkbuf, &startit);
    gtk_text_buffer_get_end_iter(gtkbuf, &endit);

    agxbput(xbuf, gtk_text_buffer_get_text(gtkbuf, &startit, &endit, 0));
}


void append_textview(GtkTextView * textv, const char *s, size_t bytes)
{

    GtkTextIter endit;
    GtkTextBuffer *gtkbuf;
    /*get text view buffer */
    gtkbuf = gtk_text_view_get_buffer(textv);
    /*set iterator to the end of the buffer */
    gtk_text_buffer_get_end_iter(gtkbuf, &endit);
    /* insert buf to the end */
    gtk_text_buffer_insert(gtkbuf, &endit, s, (gint)bytes);

}
