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


#include "common.h"
#include "g.h"
#include "gcommon.h"

#define WQU widget->u.q

GtkWidget *w;
void butcallback(gpointer * l);
void file_ok_sel(GtkWidget * w2, GtkFileSelection * fs);

int GQcreatewidget(Gwidget_t * parent, Gwidget_t * widget,
		   int attrn, Gwattr_t * attrp)
{

    int ai;

    WQU->mode = G_QWSTRING;
    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRMODE:
	    if (Strcmp("string", attrp[ai].u.t) == 0)
		WQU->mode = G_QWSTRING;
	    else if (Strcmp("file", attrp[ai].u.t) == 0)
		WQU->mode = G_QWFILE;
	    else if (Strcmp("choice", attrp[ai].u.t) == 0)
		WQU->mode = G_QWCHOICE;
	    else {
		Gerr(POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
		return -1;
	    }
	    break;
	case G_ATTRUSERDATA:
	    widget->udata = attrp[ai].u.u;
	    break;
	default:
	    Gerr(POS, G_ERRBADATTRID, attrp[ai].id);
	    return -1;
	}
    }
    return 0;
}


int GQsetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    int ai;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRMODE:
	    Gerr(POS, G_ERRCANNOTSETATTR2, "mode");
	    return -1;
	case G_ATTRUSERDATA:
	    widget->udata = attrp[ai].u.u;
	    break;
	default:
	    Gerr(POS, G_ERRBADATTRID, attrp[ai].id);
	    return -1;
	}
    }
    return 0;
}


int GQgetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    int ai;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRMODE:
	    switch (WQU->mode) {
	    case G_QWSTRING:
		attrp[ai].u.t = "string";
		break;
	    case G_QWFILE:
		attrp[ai].u.t = "file";
		break;
	    case G_QWCHOICE:
		attrp[ai].u.t = "choice";
		break;
	    }
	    break;
	case G_ATTRUSERDATA:
	    attrp[ai].u.u = widget->udata;
	    break;
	default:
	    Gerr(POS, G_ERRBADATTRID, attrp[ai].id);
	    return -1;
	}
    }
    return 0;
}


int GQdestroywidget(Gwidget_t * widget)
{
    switch (WQU->mode) {
    case G_QWSTRING:
	gtk_widget_destroy(widget->w);
	break;
    case G_QWFILE:
	break;
    case G_QWCHOICE:
	gtk_widget_destroy(widget->w);
	break;
    }
    return 0;
}


int GQqueryask(Gwidget_t * widget, char *prompt, char *args,
	       char *responsep, int responsen)
{
    GtkWidget *vbox, *hbox;
    GtkWidget *okBut, *cancelBut;
    GtkWidget *qstringlabel;
    GtkWidget *qstringentry;
    char buttons[20][40];
    char *s1, *s2;
    char c;
    long i;

    switch (WQU->mode) {
    case G_QWSTRING:
	widget->w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(widget->w), "popup");
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(widget->w), vbox);
	gtk_widget_show(vbox);

	qstringlabel = gtk_label_new("label");
	gtk_box_pack_start(GTK_BOX(vbox), qstringlabel, TRUE, TRUE, 0);
	gtk_widget_show(qstringlabel);
	qstringentry = gtk_entry_new();
	gtk_entry_set_max_length(G_OBJECT(qstringentry), 50);
	if (args)
	    gtk_entry_set_text(qstringentry, args);
	gtk_box_pack_start(GTK_BOX(vbox), qstringentry, TRUE, TRUE, 0);
	gtk_widget_show(qstringentry);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	cancelBut = gtk_button_new_with_label("Cancel");
	gtk_box_pack_start(GTK_BOX(hbox), cancelBut, TRUE, TRUE, 0);
	gtk_widget_show(cancelBut);

	okBut = gtk_button_new_with_label("OK");
	gtk_box_pack_start(GTK_BOX(hbox), okBut, TRUE, TRUE, 0);

	gtk_signal_connect_object(GTK_OBJECT(okBut), "clicked",
				  GTK_SIGNAL_FUNC(butcallback),
				  GINT_TO_POINTER(2));
	gtk_signal_connect_object(GTK_OBJECT(cancelBut), "clicked",
				  GTK_SIGNAL_FUNC(butcallback),
				  GINT_TO_POINTER(1));
	gtk_widget_show(okBut);

	gtk_label_set_text(qstringlabel, prompt);
	gtk_widget_show(widget->w);

	w = widget->w;
	WQU->state = 2;
	WQU->button = 0;
	while (WQU->state) {
	    if (WQU->state == 1) {
		if (WQU->button != 1)
		    strncpy(responsep, gtk_entry_get_text(qstringentry),
			    responsen);
		WQU->state = 0;
	    }
	    Gprocessevents(TRUE, G_ONEEVENT);
	}


	gtk_widget_hide(widget->w);
	Gpopdownflag = TRUE;
	if (WQU->button == 1)
	    return -1;
	break;
    case G_QWFILE:
	widget->w = gtk_file_selection_new("File selection");
	g_signal_connect(G_OBJECT(widget->w), "destroy",
			 G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT
			 (GTK_FILE_SELECTION(widget->w)->ok_button),
			 "clicked", G_CALLBACK(file_ok_sel), NULL);
	g_signal_connect_swapped(G_OBJECT
				 (GTK_FILE_SELECTION(widget->w)->
				  cancel_button), "clicked",
				 G_CALLBACK(gtk_widget_destroy),
				 G_OBJECT(widget->w));
	gtk_widget_show(widget->w);

	w = widget->w;
	WQU->state = 2;
	while (WQU->state) {
	    if (WQU->state == 1) {
		if (WQU->button > 0)
		    strncpy(responsep,
			    gtk_file_selection_get_filename
			    (GTK_FILE_SELECTION(widget->w)), responsen);
		WQU->state = 0;
	    }
	    Gprocessevents(TRUE, G_ONEEVENT);
	}
	gtk_widget_hide(widget->w);
	break;
    case G_QWCHOICE:
	if (!args)
	    return -1;

	widget->w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(widget->w), "popup");
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(widget->w), vbox);
	gtk_widget_show(vbox);

	qstringlabel = gtk_label_new("label");
	gtk_box_pack_start(GTK_BOX(vbox), qstringlabel, TRUE, TRUE, 0);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);
	gtk_widget_show(hbox);

	for (s1 = args, i = 1; *s1; i++) {
	    s2 = s1;
	    while (*s2 && *s2 != '|')
		s2++;
	    c = *s2, *s2 = 0;
	    strcpy(buttons[i], s1);
	    okBut = gtk_button_new_with_label(buttons[i]);
	    gtk_box_pack_start(GTK_BOX(hbox), okBut, TRUE, TRUE, 0);
	    gtk_signal_connect_object(GTK_OBJECT(okBut), "clicked",
				      GTK_SIGNAL_FUNC(butcallback),
				      GINT_TO_POINTER(i));
	    gtk_widget_show(okBut);

	    *s2 = c;
	    s1 = s2;
	    if (*s1)
		s1++;
	}
	gtk_widget_show(widget->w);
	w = widget->w;

	WQU->state = 2;
	while (WQU->state) {
	    if (WQU->state == 1) {
		if (WQU->button > 0)
		    strncpy(responsep, buttons[WQU->button], responsen);
		WQU->state = 0;
	    }
	    Gprocessevents(TRUE, G_ONEEVENT);
	}
	Gpopdownflag = TRUE;
	gtk_widget_hide(widget->w);
	break;
    }

    return 0;
}


void butcallback(gpointer * l)
{
    Gwidget_t *widget;

    widget = findwidget((unsigned long) w, G_QUERYWIDGET);
    if (GPOINTER_TO_INT(l) > -1) {
	WQU->button = GPOINTER_TO_INT(l);
    }
    WQU->state = 1;
}


void file_ok_sel(GtkWidget * w2, GtkFileSelection * fs)
{
    Gwidget_t *widget;
    widget = findwidget((unsigned long) w, G_QUERYWIDGET);
    WQU->state = 1;
    WQU->button = 1;
}
