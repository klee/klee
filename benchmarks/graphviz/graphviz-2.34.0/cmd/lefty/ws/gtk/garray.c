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

#define WAU widget->u.a

int GAcreatewidget(Gwidget_t * parent, Gwidget_t * widget,
		   int attrn, Gwattr_t * attrp)
{
    PIXsize_t ps;
    int ai;
    GdkColor c;
    int color;

    if (!parent) {
	Gerr(POS, G_ERRNOPARENTWIDGET);
	return -1;
    }
    WAU->func = NULL;
    ps.x = ps.y = MINAWSIZE;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    GETSIZE(attrp[ai].u.s, ps, MINAWSIZE);
	    break;
	case G_ATTRBORDERWIDTH:
	    break;
	case G_ATTRMODE:
	    if (Strcmp("horizontal", attrp[ai].u.t) == 0) {
		WAU->mode = G_AWHARRAY;
	    } else if (Strcmp("vertical", attrp[ai].u.t) == 0) {
		WAU->mode = G_AWVARRAY;
	    } else {
		Gerr(POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
		return -1;
	    }
	    break;
	case G_ATTRLAYOUT:
	    if (Strcmp("on", attrp[ai].u.t) == 0)
		Gawsetmode(widget, FALSE);
	    else if (Strcmp("off", attrp[ai].u.t) == 0)
		Gawsetmode(widget, TRUE);
	    else {
		Gerr(POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
		return -1;
	    }
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
/*        			if (XAllocColor (
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
	case G_ATTRRESIZECB:
	    WAU->func = (Gawcoordscb) attrp[ai].u.func;
	    break;
	case G_ATTRUSERDATA:
	    widget->udata = attrp[ai].u.u;
	    break;
	default:
	    Gerr(POS, G_ERRBADATTRID, attrp[ai].id);
	    return -1;
	}
    }

    if (WAU->mode == G_AWHARRAY)
	widget->w = gtk_hbox_new(FALSE, 0);
    else
	widget->w = gtk_vbox_new(FALSE, 0);
    if (parent->type == 7) {
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW
					      (parent->w), widget->w);
    } else {
	gtk_container_add(GTK_CONTAINER(parent->w), widget->w);
    }
    gtk_widget_set_usize(widget->w, ps.x, ps.y);
    gtk_widget_show(widget->w);

    return 0;
}


int GAsetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    PIXsize_t ps;
    int ai;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    GETSIZE(attrp[ai].u.s, ps, MINAWSIZE);
	    break;
	case G_ATTRBORDERWIDTH:
	    break;
	case G_ATTRMODE:
	    Gerr(POS, G_ERRCANNOTSETATTR2, "mode");
	    return -1;
	case G_ATTRLAYOUT:
	    if (Strcmp("on", attrp[ai].u.t) == 0)
		Gawsetmode(widget, FALSE);
	    else if (Strcmp("off", attrp[ai].u.t) == 0)
		Gawsetmode(widget, TRUE);
	    else {
		Gerr(POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
		return -1;
	    }
	    break;
	case G_ATTRWINDOWID:
	    Gerr(POS, G_ERRCANNOTSETATTR2, "windowid");
	    return -1;
	case G_ATTRRESIZECB:
	    WAU->func = (Gawcoordscb) attrp[ai].u.func;
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


int GAgetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    int width, height;
    int ai;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    attrp[ai].u.s.x = width, attrp[ai].u.s.y = height;
	    break;
	case G_ATTRBORDERWIDTH:
	    attrp[ai].u.i = width;
	    break;
	case G_ATTRMODE:
	    attrp[ai].u.t =
		(WAU->mode == G_AWHARRAY) ? "horizontal" : "vertical";
	    break;
	case G_ATTRLAYOUT:
	    break;
	case G_ATTRWINDOWID:

	    break;
	case G_ATTRRESIZECB:
	    attrp[ai].u.func = WAU->func;
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


int GAdestroywidget(Gwidget_t * widget)
{
    gtk_widget_destroy(widget->w);
    return 0;
}


void Gawdefcoordscb(int wi, Gawdata_t * dp)
{
    Gawcarray_t *cp;
    int sx, sy, csx, csy, ci;

    sx = dp->sx, sy = dp->sy;
    csx = csy = 0;
    for (ci = 0; ci < dp->cj; ci++) {
	cp = &dp->carray[ci];
	if (!cp->flag)
	    continue;

	cp->ox = csx, cp->oy = csy;
	if (dp->type == G_AWVARRAY)
	    cp->sx = sx - 2 * cp->bs, csy += cp->sy + 2 * cp->bs;
	else
	    cp->sy = sy - 2 * cp->bs, csx += cp->sx + 2 * cp->bs;

    }
    if (dp->type == G_AWVARRAY)
	dp->sy = csy;
    else
	dp->sx = csx;
}


static void dolayout(Gwidget_t * widget, int flag)
{


}


int Gawsetmode(Gwidget_t * widget, int mode)
{

    dolayout(widget, TRUE);
    return 0;
}


int Gaworder(Gwidget_t * widget, void *data, Gawordercb func)
{

/*	(*func) (data, NULL);   */
    dolayout(widget, TRUE);
}
