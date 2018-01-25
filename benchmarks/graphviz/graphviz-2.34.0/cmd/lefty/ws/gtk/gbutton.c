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

#define WBU widget->u.b

gint bwcallback(GtkWidget * w, GdkEvent * event, gpointer * data);

int GBcreatewidget(Gwidget_t * parent, Gwidget_t * widget,
		   int attrn, Gwattr_t * attrp)
{

    PIXsize_t ps;
    char *s;
    int ai;
    GdkColor c;
    int color;
    GtkWidget *label;

    if (!parent) {
	Gerr(POS, G_ERRNOPARENTWIDGET);
	return -1;
    }
    WBU->func = NULL;
    ps.x = ps.y = MINBWSIZE;
    s = NULL;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    GETSIZE(attrp[ai].u.s, ps, MINBWSIZE);
	    break;
	case G_ATTRBORDERWIDTH:
	    break;
	case G_ATTRTEXT:
	    s = attrp[ai].u.t;
	    label = gtk_label_new(s);
	    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	    gtk_widget_show(label);
	    break;
	case G_ATTRCOLOR:
	    color = attrp[ai].u.c.index;
	    if (color != 0 && color != 1) {
		Gerr(POS, G_ERRBADCOLORINDEX, color);
		return -1;
	    }
	    c.red = attrp[ai].u.c.r * 257;
	    c.green = attrp[ai].u.c.g * 257;
	    c.blue = attrp[ai].u.c.b * 257;
	    /*  if (XAllocColor (
	       Gdisplay, DefaultColormap (Gdisplay, Gscreenn), &c
	       )) {
	       if (color == 0)
	       ADD2ARGS (XtNbackground, c.pixel);
	       else
	       ADD2ARGS (XtNforeground, c.pixel);
	       }
	     */
	    break;
	case G_ATTRWINDOWID:
	    Gerr(POS, G_ERRCANNOTSETATTR1, "windowid");
	    return -1;
	case G_ATTRBUTTONCB:
	    WBU->func = (Gbuttoncb) attrp[ai].u.func;
	    break;
	case G_ATTRUSERDATA:
	    widget->udata = attrp[ai].u.u;
	    break;
	default:
	    Gerr(POS, G_ERRBADATTRID, attrp[ai].id);
	    return -1;
	}
    }


    widget->w = gtk_button_new();
    gtk_button_set_relief(widget->w, GTK_RELIEF_NONE);
    if (label != NULL)
	gtk_container_add(GTK_CONTAINER(widget->w), label);
    if (GTK_BOX(parent->w)) {
	gtk_box_pack_start(GTK_BOX(parent->w), widget->w, FALSE, TRUE, 0);
    } else {
	gtk_container_add(GTK_CONTAINER(parent->w), widget->w);
    }

    gtk_signal_connect(G_OBJECT(widget->w), "clicked",
		       GTK_SIGNAL_FUNC(bwcallback), widget->udata);
    gtk_widget_show(widget->w);
    return 0;
}


int GBsetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    PIXsize_t ps;
    int ai;
    GdkColor c;
    int color;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:

	    break;
	case G_ATTRBORDERWIDTH:

	    break;
	case G_ATTRTEXT:

	    break;
	case G_ATTRCOLOR:

	    break;
	case G_ATTRWINDOWID:
	    Gerr(POS, G_ERRCANNOTSETATTR2, "windowid");
	    return -1;
	case G_ATTRBUTTONCB:
	    WBU->func = (Gbuttoncb) attrp[ai].u.func;
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


int GBgetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    int width, height;
    char *s;
    int ai;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:

	    break;
	case G_ATTRBORDERWIDTH:

	    break;
	case G_ATTRTEXT:

	    break;
	case G_ATTRWINDOWID:

	    break;
	case G_ATTRBUTTONCB:
	    attrp[ai].u.func = WBU->func;
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


int GBdestroywidget(Gwidget_t * widget)
{
    gtk_widget_destroy(widget->w);
    return 0;
}


gint bwcallback(GtkWidget * w, GdkEvent * event, gpointer * data)
{
    Gwidget_t *widget;
    unsigned long l;

    widget = findwidget((unsigned long) w, G_BUTTONWIDGET);

    /* calls   TXTtoggle in the case of txtview */
    if (WBU->func)
	(*WBU->func) (widget - &Gwidgets[0], widget->udata);

    return 1;
}
