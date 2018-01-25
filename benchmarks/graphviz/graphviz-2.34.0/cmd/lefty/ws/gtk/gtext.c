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

#define WTU widget->u.t

int pos = 0;

gint keyevent(GtkWidget * w, GdkEventKey * event, gpointer data);

int GTcreatewidget(Gwidget_t * parent, Gwidget_t * widget,
		   int attrn, Gwattr_t * attrp)
{

    PIXsize_t ps;
    char *s;
    int ai;
    int color;
    GdkColor c;

    if (!parent) {
	Gerr(POS, G_ERRNOPARENTWIDGET);
	return -1;
    }

    WTU->func = NULL;
    ps.x = ps.y = MINTWSIZE;
    s = "oneline";

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    GETSIZE(attrp[ai].u.s, ps, MINTWSIZE);
	    break;
	case G_ATTRBORDERWIDTH:
	    break;
	case G_ATTRTEXT:
	    break;
	case G_ATTRAPPENDTEXT:
	    Gerr(POS, G_ERRCANNOTSETATTR1, "appendtext");
	    return -1;
	case G_ATTRMODE:
	    s = attrp[ai].u.t;
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
/*	                    if (XAllocColor (
			        Gdisplay, DefaultColormap (Gdisplay, Gscreenn), &c
		            )) {
		                if (color == 0)
		                    ADD2ARGS (XtNbackground, c.pixel);
		                else
		                    ADD2ARGS (XtNforeground, c.pixel);
		            }
*/ break;
	case G_ATTRWINDOWID:
	    Gerr(POS, G_ERRCANNOTSETATTR1, "windowid");
	    return -1;
	case G_ATTRNEWLINECB:
	    WTU->func = (Gtwnewlinecb) attrp[ai].u.func;
	    break;
	case G_ATTRUSERDATA:
	    widget->udata = attrp[ai].u.u;
	    break;
	default:
	    Gerr(POS, G_ERRBADATTRID, attrp[ai].id);
	    return -1;
	}
    }

    widget->w = gtk_text_new(NULL, NULL);
    gtk_text_set_editable(widget->w, TRUE);
    gtk_box_pack_start(GTK_BOX(parent->w), widget->w, TRUE, TRUE, 0);

    gtk_signal_connect(GTK_OBJECT(widget->w), "key_press_event",
		       GTK_SIGNAL_FUNC(keyevent), NULL);
    gtk_widget_show(widget->w);

    return 0;
}


int GTsetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    PIXsize_t ps;
    int ai, li;
    GdkColor c;
    int color;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    GETSIZE(attrp[ai].u.s, ps, MINTWSIZE);
	    break;
	case G_ATTRBORDERWIDTH:

	    break;
	case G_ATTRTEXT:

	    break;
	case G_ATTRAPPENDTEXT:

	    break;
	case G_ATTRMODE:

	    break;
	case G_ATTRCOLOR:

	    break;
	case G_ATTRWINDOWID:
	    Gerr(POS, G_ERRCANNOTSETATTR2, "windowid");
	    return -1;
	case G_ATTRNEWLINECB:
	    WTU->func = (Gtwnewlinecb) attrp[ai].u.func;
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


int GTgetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    int width, height;
    GtkWidget w;
    int rtn, ai;
    long fi, li;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:

	    break;
	case G_ATTRBORDERWIDTH:

	    break;
	case G_ATTRTEXT:

	    break;
	case G_ATTRAPPENDTEXT:
	    Gerr(POS, G_ERRCANNOTGETATTR, "appendtext");
	    return -1;
	case G_ATTRSELECTION:

	    break;
	case G_ATTRMODE:

	    break;
	case G_ATTRWINDOWID:

	    break;
	case G_ATTRNEWLINECB:
	    attrp[ai].u.func = WTU->func;
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


int GTdestroywidget(Gwidget_t * widget)
{
    gtk_widget_destroy(widget->w);
    return 0;
}


gint keyevent(GtkWidget * w, GdkEventKey * event, gpointer data)
{
    Gwidget_t *widget;
    widget = findwidget((unsigned long) w, G_TEXTWIDGET);

    if (event->keyval == 65293) {
	Gbufp = gtk_editable_get_chars(w, pos, -1);
	pos = gtk_text_get_point(w);

	if (WTU->func) {
	    /* calls TXTprocess in the case of txtview   */
	    (*WTU->func) (widget - &Gwidgets[0], Gbufp);
	}
    }

    return FALSE;
}
