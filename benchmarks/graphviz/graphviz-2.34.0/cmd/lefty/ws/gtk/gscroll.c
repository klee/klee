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


int GScreatewidget(Gwidget_t * parent, Gwidget_t * widget,
		   int attrn, Gwattr_t * attrp)
{

    PIXsize_t ps;
    GtkWidget *draw;
    int ai;
    GdkColor c;
    int color;

    if (!parent) {
	Gerr(POS, G_ERRNOPARENTWIDGET);
	return -1;
    }
    ps.x = ps.y = MINSWSIZE;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    GETSIZE(attrp[ai].u.s, ps, MINSWSIZE);
	    break;
	case G_ATTRBORDERWIDTH:
	    break;
	case G_ATTRCHILDCENTER:
	    Gerr(POS, G_ERRCANNOTSETATTR1, "childcenter");
	    return -1;
	case G_ATTRMODE:
	    /*    if (Strcmp ("forcebars", attrp[ai].u.t) == 0)
	       ADD2ARGS (XtNforceBars, True);
	       else {
	       Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
	       return -1;
	       }
	     */
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
	    /*   if (XAllocColor (
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
	case G_ATTRUSERDATA:
	    widget->udata = attrp[ai].u.u;
	    break;
	default:
	    Gerr(POS, G_ERRBADATTRID, attrp[ai].id);
	    return -1;
	}
    }
    widget->w = gtk_scrolled_window_new(NULL, NULL);

    if (GTK_CONTAINER(parent->w)) {
	gtk_container_add(GTK_CONTAINER(parent->w), widget->w);
    } else if (GTK_BOX(parent->w)) {
	gtk_box_pack_start(GTK_BOX(parent->w), widget->w, TRUE, TRUE, 0);

    }
    gtk_widget_set_usize(widget->w, ps.x, ps.y);
    gtk_widget_show(widget->w);
    return 0;
}


int GSsetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    PIXpoint_t po;
    PIXsize_t ps;
    GdkColor c;
    int width, height;
    int ai;
    int color;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    GETSIZE(attrp[ai].u.s, ps, MINSWSIZE);
	    gtk_widget_set_usize(widget->w, ps.x, ps.y);
	    break;
	case G_ATTRBORDERWIDTH:
	    break;
	case G_ATTRCHILDCENTER:
	    GETORIGIN(attrp[ai].u.p, po);

	    break;
	case G_ATTRMODE:
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

	    break;
	case G_ATTRWINDOWID:
	    Gerr(POS, G_ERRCANNOTSETATTR2, "windowid");
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


int GSgetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    int x, y;
    Gwidget_t *child;
    int ai, wi;
    int width, height;
    child = 0;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    attrp[ai].u.s.x = width, attrp[ai].u.s.y = height;
	    break;
	case G_ATTRBORDERWIDTH:
	    attrp[ai].u.i = width;
	    break;
	case G_ATTRCHILDCENTER:
	    for (wi = 0; wi < Gwidgetn; wi++) {
		child = &Gwidgets[wi];
		if (child->inuse && child->pwi == widget - &Gwidgets[0])
		    break;
	    }
	    if (wi == Gwidgetn) {
		Gerr(POS, G_ERRNOCHILDWIDGET);
		return -1;
	    }

	    break;
	case G_ATTRMODE:

	    break;
	case G_ATTRWINDOWID:

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


int GSdestroywidget(Gwidget_t * widget)
{
    gtk_widget_destroy(widget->w);
    return 0;
}
