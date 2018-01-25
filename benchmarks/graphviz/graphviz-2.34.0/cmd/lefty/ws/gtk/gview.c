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

#define WVU widget->u.v

int GVcreatewidget(Gwidget_t * parent, Gwidget_t * widget,
		   int attrn, Gwattr_t * attrp)
{

    PIXsize_t ps;
    PIXpoint_t po;
    int haveorigin, ai, color;
    char *s;
    GdkColor c;

    WVU->func = NULL;
    WVU->closing = FALSE;
    s = "LEFTY";
    po.x = po.y = 0;
    ps.x = ps.y = MINVWSIZE;
    haveorigin = FALSE;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRORIGIN:
	    haveorigin = TRUE;
	    GETORIGIN(attrp[ai].u.p, po);
	    break;
	case G_ATTRSIZE:
	    GETSIZE(attrp[ai].u.s, ps, MINVWSIZE);
	    break;
	case G_ATTRNAME:
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
	    /* if (XAllocColor (
	       Gdisplay, DefaultColormap (Gdisplay, Gscreenn), &c
	       )) {
	       if (color == 0)
	       ADD2ARGS (XtNbackground, c.pixel);
	       else
	       ADD2ARGS (XtNforeground, c.pixel);
	       }
	     */
	    break;
	case G_ATTRZORDER:
	    Gerr(POS, G_ERRCANNOTSETATTR1, "zorder");
	    return -1;
	case G_ATTRWINDOWID:
	    Gerr(POS, G_ERRCANNOTSETATTR1, "windowid");
	    return -1;
	case G_ATTREVENTCB:
	    WVU->func = (Gviewcb) attrp[ai].u.func;
	    break;
	case G_ATTRUSERDATA:
	    widget->udata = attrp[ai].u.u;
	    break;
	default:
	    Gerr(POS, G_ERRBADATTRID, attrp[ai].id);
	    return -1;
	}
    }
    /*  if (haveorigin) {
       hints.x = po.x, hints.y = po.y;
       hints.width = ps.x, hints.height = ps.y;
       hints.flags = USPosition;
       Glazyrealize (widget->w, TRUE, &hints);
       } else
       Glazyrealize (widget->w, FALSE, NULL);
     */

    widget->w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_events(widget->w, GDK_ALL_EVENTS_MASK);

    gtk_window_set_default_size(widget->w, ps.x, ps.y);
    gtk_widget_set_uposition(widget->w, po.x, po.y);
    gtk_widget_show(widget->w);

    gdk_window_set_title(widget->w->window, s);
    return 0;
}


int GVsetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    PIXpoint_t po;
    PIXsize_t ps;
    int ai;
    GdkColor c;
    int color;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRORIGIN:
	    GETORIGIN(attrp[ai].u.p, po);
	    gtk_widget_set_uposition(widget->w, po.x, po.y);
	    break;
	case G_ATTRSIZE:
	    GETSIZE(attrp[ai].u.s, ps, MINVWSIZE);
	    gtk_window_set_default_size(widget->w, ps.x, ps.y);
	    break;
	case G_ATTRNAME:

	    break;
	case G_ATTRCOLOR:

	    break;
	case G_ATTRZORDER:

	    break;
	case G_ATTRWINDOWID:
	    Gerr(POS, G_ERRCANNOTSETATTR2, "windowid");
	    return -1;
	case G_ATTREVENTCB:
	    WVU->func = (Gviewcb) attrp[ai].u.func;
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


int GVgetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    int x, y;
    int width, height;
    int ai;

    for (ai = 0; ai < attrn; ai++) {
	RESETARGS;
	switch (attrp[ai].id) {
	case G_ATTRORIGIN:
	    break;
	case G_ATTRSIZE:
	    break;
	case G_ATTRNAME:
	    Gerr(POS, G_ERRNOTIMPLEMENTED);
	    return -1;
	case G_ATTRZORDER:
	    Gerr(POS, G_ERRNOTIMPLEMENTED);
	    return -1;
	case G_ATTRWINDOWID:
	    break;
	case G_ATTREVENTCB:
	    attrp[ai].u.func = WVU->func;
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


int GVdestroywidget(Gwidget_t * widget)
{
    WVU->closing = TRUE;
    gtk_widget_destroy(widget->w);
    return 0;
}


void Gwmdelaction(GtkWidget * w, GdkEvent * evp, char **app,
		  unsigned int *anp)
{
    Gwidget_t *widget;
    Gevent_t gev;

    widget = findwidget((unsigned long) w, G_VIEWWIDGET);
    if (!widget)
	exit(0);

    gev.type = 0, gev.code = 0, gev.data = 0;
    gev.wi = widget - &Gwidgets[0];
    if (WVU->func)
	(*WVU->func) (&gev);
    else
	exit(0);
}
