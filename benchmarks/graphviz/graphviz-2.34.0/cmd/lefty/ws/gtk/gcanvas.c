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

#define WCU widget->u.c
#define WINDOW widget->u.c->window
#define GC widget->u.c->gc
#define ISVISIBLE(r) ( \
	(r.o.x <= WCU->clip.c.x) && (r.c.x >= WCU->clip.o.x) && \
	(r.o.y <= WCU->clip.c.y) && (r.c.y >= WCU->clip.o.y) \
)

#define IS8BIT(font) ((font)->min_byte1 == 0 && (font)->max_byte1 == 0)
#define max(a, b) (((a) >= (b)) ? (a) : (b))
#define min(a, b) (((a) <= (b)) ? (a) : (b))

#define SETFONT(font) { \
	if(font != WCU->font) { \
		WCU->font = font; \
		gdk_gc_set_font(GC, font); \
	} \
}

Gwidget_t *canvas;
static int curcursori = -1;

static gchar gstyles[][2] = {
    /* G_SOLID */ {16},
    /* G_DASHED */ {4, 4},
    /* G_DOTTED */ {2, 2},
    /* G_LONGDASHED */ {4, 12},
    /* G_SHORTDASHED */ {12, 4}
};

static char grays[][4] = {
    {0x00, 0x00, 0x00, 0x00,},
    {0x08, 0x00, 0x00, 0x00,},
    {0x08, 0x00, 0x02, 0x00,},
    {0x0A, 0x00, 0x02, 0x00,},
    {0x0A, 0x00, 0x0A, 0x00,},
    {0x0A, 0x04, 0x0A, 0x00,},
    {0x0A, 0x04, 0x0A, 0x01,},
    {0x0A, 0x05, 0x0A, 0x01,},
    {0x0A, 0x05, 0x0A, 0x05,},
    {0x0E, 0x05, 0x0A, 0x05,},
    {0x0E, 0x05, 0x0B, 0x05,},
    {0x0F, 0x05, 0x0B, 0x05,},
    {0x0F, 0x05, 0x0F, 0x05,},
    {0x0F, 0x0D, 0x0F, 0x05,},
    {0x0F, 0x0D, 0x0F, 0x07,},
    {0x0F, 0x0F, 0x0F, 0x07,},
    {0x0F, 0x0F, 0x0F, 0x0F,},
};

static void setgattr(Gwidget_t *, Ggattr_t *);

static PIXrect_t rdrawtopix(Gwidget_t *, Grect_t);
static PIXpoint_t pdrawtopix(Gwidget_t *, Gpoint_t);
static PIXsize_t sdrawtopix(Gwidget_t *, Gsize_t);
static Gpoint_t Gppixtodraw(Gwidget_t *, PIXpoint_t);
static Gsize_t spixtodraw(Gwidget_t *, PIXsize_t);
static Grect_t rpixtodraw(Gwidget_t *, PIXrect_t);
static PIXrect_t rdrawtobpix(Gbitmap_t *, Grect_t);
static PIXpoint_t pdrawtobpix(Gbitmap_t *, Gpoint_t);
static void adjustclip(Gwidget_t *);


int GCcreatewidget(Gwidget_t * parent, Gwidget_t * widget,
		   int attrn, Gwattr_t * attrp)
{
    PIXsize_t ps;
    int width, height;
    int ai, i;
    GdkGCValues gcv;
    int r, g, b, color;
    GdkColor *cp;

    if (!parent) {
	Gerr(POS, G_ERRNOPARENTWIDGET);
	return -1;
    }

    canvas = widget;
    WCU->func = NULL;
    WCU->needredraw = FALSE;
    WCU->buttonsdown = 0;
    WCU->bstate[0] = WCU->bstate[1] = WCU->bstate[2] = 0;
    ps.x = ps.y = MINCWSIZE;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    GETSIZE(attrp[ai].u.s, ps, MINCWSIZE);
	    break;
	case G_ATTRBORDERWIDTH:
	    break;
#ifdef FEATURE_GMAP
	case G_ATTRMODE:
	    if (Strcmp("gmap", attrp[ai].u.t) == 0) {
		gmapmode = TRUE;
	    } else {
		Gerr(POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
		return -1;
	    }
	    break;
#endif
	case G_ATTRCURSOR:
	    break;
	case G_ATTRCOLOR:
	    break;
	case G_ATTRVIEWPORT:
	    break;
	case G_ATTRWINDOW:
	    break;
	case G_ATTRWINDOWID:
	    Gerr(POS, G_ERRCANNOTSETATTR2, "windowid");
	    return -1;
	case G_ATTREVENTCB:
	    WCU->func = (Gcanvascb) attrp[ai].u.func;
	    break;
	case G_ATTRUSERDATA:
	    widget->udata = attrp[ai].u.u;
	    break;
	default:
	    Gerr(POS, G_ERRBADATTRID, attrp[ai].id);
	    return -1;
	}
    }
/*    	XtSetValues (widget->w, argp, argn);	*/

    widget->w = gtk_drawing_area_new();
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(parent->w),
					  widget->w);
    gtk_drawing_area_size(GTK_DRAWING_AREA(widget->w), ps.x, ps.y);

    gtk_widget_add_events(widget->w, GDK_ALL_EVENTS_MASK);
    gtk_signal_connect(GTK_OBJECT(widget->w), "key_release_event",
		       GTK_SIGNAL_FUNC(Gcwkeyaction), NULL);
    gtk_signal_connect(GTK_OBJECT(widget->w), "key_press_event",
		       GTK_SIGNAL_FUNC(Gcwkeyaction), NULL);
    gtk_signal_connect(G_OBJECT(widget->w), "button_press_event",
		       GTK_SIGNAL_FUNC(Gcwbutaction), NULL);
    gtk_signal_connect(G_OBJECT(widget->w), "button_release_event",
		       GTK_SIGNAL_FUNC(Gcwbutaction), NULL);
    gtk_signal_connect(G_OBJECT(widget->w), "visibility_notify_event",
		       GTK_SIGNAL_FUNC(cweventhandler), NULL);
    gtk_signal_connect(G_OBJECT(widget->w), "expose_event",
		       GTK_SIGNAL_FUNC(exposeeventhandler), NULL);
    gtk_signal_connect(G_OBJECT(widget->w), "motion_notify_event",
		       GTK_SIGNAL_FUNC(cweventhandler), NULL);

    gtk_widget_show(widget->w);
    gtk_widget_show(parent->w);

    GC = gdk_gc_new(widget->w->window);
    WCU->cmap = gdk_colormap_get_system();
    WCU->colors[0].color.pixel = WCU->colors[1].color.pixel = 1000000;
    if (WCU->colors[0].color.pixel == 1000000) {
	gdk_gc_get_values(GC, &gcv);
	WCU->colors[0].color = gcv.background;
    }
    if (WCU->colors[1].color.pixel == 1000000) {
	gdk_gc_get_values(GC, &gcv);
	WCU->colors[1].color = gcv.foreground;
    }

    WCU->colors[0].color.red = 65535;
    WCU->colors[0].color.green = 65535;
    WCU->colors[0].color.blue = 65535;
    WCU->colors[1].color.red = 0;
    WCU->colors[1].color.green = 0;
    WCU->colors[1].color.blue = 0;

    gdk_colormap_alloc_color(WCU->cmap, &WCU->colors[0].color, FALSE,
			     TRUE);
    WCU->colors[0].inuse = TRUE;
    gdk_colormap_alloc_color(WCU->cmap, &WCU->colors[1].color, FALSE,
			     TRUE);
    WCU->colors[1].inuse = TRUE;

    WCU->allocedcolor[0] = WCU->allocedcolor[1] = FALSE;
    for (i = 2; i < G_MAXCOLORS; i++)
	WCU->colors[i].inuse = FALSE;

    WCU->gattr.color = 1;
/*	gdk_gc_set_background(GC, widget->w->style->white_gc);
	gdk_gc_set_foreground(GC, widget->w->style->black_gc);
*/

    WCU->gattr.width = 0;
    WCU->gattr.mode = 0;
    WCU->gattr.fill = 0;
    WCU->gattr.style = 0;
    WCU->defgattr = WCU->gattr;
    WCU->font = NULL;
    WCU->wrect.o.x = 0.0, WCU->wrect.o.y = 0.0;
    WCU->wrect.c.x = 1.0, WCU->wrect.c.y = 1.0;
    WCU->vsize.x = ps.x, WCU->vsize.y = ps.y;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRCURSOR:
	    if (Strcmp(attrp[ai].u.t, "default") == 0) {
		curcursori = -1;
	    }
	    break;
	case G_ATTRCOLOR:
	    color = attrp[ai].u.c.index;
	    if (color < 0 || color > G_MAXCOLORS) {
		Gerr(POS, G_ERRBADCOLORINDEX, color);
		return -1;
	    }
	    r = attrp[ai].u.c.r * 257;
	    g = attrp[ai].u.c.g * 257;
	    b = attrp[ai].u.c.b * 257;
	    cp = &WCU->colors[color].color;
	    if (WCU->colors[color].inuse)
		if (cp->red != r || cp->green != g || cp->blue != b)
		    if (color > 1 || WCU->allocedcolor[color])
			gdk_colormap_free_colors(WCU->cmap, cp, 1);

	    cp->red = r, cp->green = g, cp->blue = b;
	    if (gdk_colormap_alloc_color(WCU->cmap, cp, TRUE, TRUE)) {
		WCU->colors[color].inuse = TRUE;
		if (color <= 1)
		    WCU->allocedcolor[color] = TRUE;
	    }
	    cp->red = r, cp->green = g, cp->blue = b;
	    if (color == WCU->gattr.color)
		WCU->gattr.color = -1;
	    break;
	case G_ATTRVIEWPORT:
	    WCU->vsize.x = (int) (attrp[ai].u.s.x + 0.5);
	    WCU->vsize.y = (int) (attrp[ai].u.s.y + 0.5);
	    break;
	case G_ATTRWINDOW:
	    WCU->wrect = attrp[ai].u.r;
	    break;
	}
    }
    adjustclip(widget);
    return 0;
}


int GCsetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    PIXsize_t ps;
    int ai, r, g, b, color;
    GdkColor *cp;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    GETSIZE(attrp[ai].u.s, ps, MINCWSIZE);
	    gtk_drawing_area_size(GTK_DRAWING_AREA(widget->w), ps.x, ps.y);
	    break;
	case G_ATTRBORDERWIDTH:
	    break;
	case G_ATTRCURSOR:
	    if (Strcmp(attrp[ai].u.t, "watch") == 0) {
		gdk_window_set_cursor(widget->w->window,
				      gdk_cursor_new(GDK_WATCH));
	    } else {
		gdk_window_set_cursor(widget->w->window,
				      gdk_cursor_new(GDK_LEFT_PTR));
	    }
	    Gsync();
	    break;
	case G_ATTRCOLOR:
	    color = attrp[ai].u.c.index;
	    if (color < 0 || color > G_MAXCOLORS) {
		Gerr(POS, G_ERRBADCOLORINDEX, color);
		return -1;
	    }
	    r = attrp[ai].u.c.r * 257;
	    g = attrp[ai].u.c.g * 257;
	    b = attrp[ai].u.c.b * 257;
	    cp = &WCU->colors[color].color;
	    if (WCU->colors[color].inuse)
		if (cp->red != r || cp->green != g || cp->blue != b)
		    if (color > 1 || WCU->allocedcolor[color])
			gdk_colormap_free_colors(WCU->cmap, cp, 1);

	    cp->red = r, cp->green = g, cp->blue = b;
	    if (gdk_colormap_alloc_color(WCU->cmap, cp, TRUE, TRUE)) {
		WCU->colors[color].inuse = TRUE;
		if (color <= 1)
		    WCU->allocedcolor[color] = TRUE;
	    }
	    cp->red = r, cp->green = g, cp->blue = b;
	    if (color == WCU->gattr.color)
		WCU->gattr.color = -1;
	    break;
	case G_ATTRVIEWPORT:
	    WCU->vsize.x = (int) (attrp[ai].u.s.x + 0.5);
	    WCU->vsize.y = (int) (attrp[ai].u.s.y + 0.5);
	    break;
	case G_ATTRWINDOW:
	    WCU->wrect = attrp[ai].u.r;
	    adjustclip(widget);
	    break;
	case G_ATTRWINDOWID:
	    Gerr(POS, G_ERRCANNOTSETATTR2, "windowid");
	    return -1;
	case G_ATTREVENTCB:
	    WCU->func = (Gcanvascb) attrp[ai].u.func;
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


int GCgetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{
    GdkColor *cp;
    int width, height;
    int ai, color;

    for (ai = 0; ai < attrn; ai++) {
	switch (attrp[ai].id) {
	case G_ATTRSIZE:
	    attrp[ai].u.s.x = width, attrp[ai].u.s.y = height;
	    break;
	case G_ATTRBORDERWIDTH:
	    attrp[ai].u.i = width;
	    break;
	case G_ATTRCURSOR:
/*				attrp[ai].u.t = (curcursori == -1) ? "default" : cursormap[curcursori].name;
*/
	    attrp[ai].u.t = (curcursori == -1) ? "default" : "watch";
	    break;
	case G_ATTRCOLOR:
	    color = attrp[ai].u.c.index;
	    if (color < 0 || color > G_MAXCOLORS) {
		Gerr(POS, G_ERRBADCOLORINDEX, color);
		return -1;
	    }
	    if (WCU->colors[color].inuse) {
		cp = &WCU->colors[color].color;
		attrp[ai].u.c.r = cp->red / 257.0;
		attrp[ai].u.c.g = cp->green / 257.0;
		attrp[ai].u.c.b = cp->blue / 257.0;
	    } else {
		attrp[ai].u.c.r = -1;
		attrp[ai].u.c.g = -1;
		attrp[ai].u.c.b = -1;
	    }
	    break;
	case G_ATTRVIEWPORT:
	    attrp[ai].u.s = WCU->vsize;
	    break;
	case G_ATTRWINDOW:
	    attrp[ai].u.r = WCU->wrect;
	    break;
	case G_ATTRWINDOWID:
	    sprintf(&Gbufp[0], "0x%lx", (unsigned long) widget->w);
	    break;
	case G_ATTREVENTCB:
	    attrp[ai].u.func = WCU->func;
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


int GCdestroywidget(Gwidget_t * widget)
{
    gtk_widget_destroy(widget->w);
    return 0;
}


int GCsetgfxattr(Gwidget_t * widget, Ggattr_t * ap)
{
    setgattr(widget, ap);
    WCU->defgattr = WCU->gattr;
    return 0;
}


int GCarrow(Gwidget_t * widget, Gpoint_t gp1, Gpoint_t gp2, Ggattr_t * ap)
{

    PIXpoint_t pp1, pp2, pa, pb, pd;
    Grect_t gr;
    double tangent, l;

    if (gp1.x < gp2.x)
	gr.o.x = gp1.x, gr.c.x = gp2.x;
    else
	gr.o.x = gp2.x, gr.c.x = gp1.x;
    if (gp1.y < gp2.y)
	gr.o.y = gp1.y, gr.c.y = gp2.y;
    else
	gr.o.y = gp2.y, gr.c.y = gp1.y;

    pp1 = pdrawtopix(widget, gp1), pp2 = pdrawtopix(widget, gp2);
    pd.x = pp1.x - pp2.x, pd.y = pp1.y - pp2.y;
    if (pd.x == 0 && pd.y == 0)
	return 0;
    tangent = atan2((double) pd.y, (double) pd.x);
    if ((l = sqrt((double) (pd.x * pd.x + pd.y * pd.y))) > 30)
	l = 30;
    pa.x = l * cos(tangent + M_PI / 7) + pp2.x;
    pa.y = l * sin(tangent + M_PI / 7) + pp2.y;
    pb.x = l * cos(tangent - M_PI / 7) + pp2.x;
    pb.y = l * sin(tangent - M_PI / 7) + pp2.y;
    setgattr(widget, ap);

    gdk_draw_line(widget->w->window, GC, pp1.x, pp1.y, pp2.x, pp2.y);
    gdk_draw_line(widget->w->window, GC, pa.x, pa.y, pp2.x, pp2.y);
    gdk_draw_line(widget->w->window, GC, pb.x, pb.y, pp2.x, pp2.y);
    return 0;
}


int GCline(Gwidget_t * widget, Gpoint_t gp1, Gpoint_t gp2, Ggattr_t * ap)
{
    PIXpoint_t pp1, pp2;
    Grect_t gr;

    if (gp1.x < gp2.x)
	gr.o.x = gp1.x, gr.c.x = gp2.x;
    else
	gr.o.x = gp2.x, gr.c.x = gp1.x;
    if (gp1.y < gp2.y)
	gr.o.y = gp1.y, gr.c.y = gp2.y;
    else
	gr.o.y = gp2.y, gr.c.y = gp1.y;

/*	if(!ISVISIBLE(gr))
		return 1;
*/

    pp1 = pdrawtopix(widget, gp1), pp2 = pdrawtopix(widget, gp2);
    setgattr(widget, ap);
    gdk_draw_line(widget->w->window, GC, pp1.x, pp1.y, pp2.x, pp2.y);
    return 0;
}


int GCbox(Gwidget_t * widget, Grect_t gr, Ggattr_t * ap)
{
    PIXrect_t pr;
    Gxy_t p;

    if (gr.o.x > gr.c.x)
	p.x = gr.o.x, gr.o.x = gr.c.x, gr.c.x = p.x;
    if (gr.o.y > gr.c.y)
	p.y = gr.o.y, gr.o.y = gr.c.y, gr.c.y = p.y;
/*	if(!ISVISIBLE(gr))
		return 1;
*/

    pr = rdrawtopix(widget, gr);
    setgattr(widget, ap);
    if (WCU->gattr.fill)
	gdk_draw_rectangle(widget->w->window, GC, TRUE, pr.o.x, pr.o.y,
			   pr.c.x - pr.o.x, pr.c.y - pr.o.y);
    else {
	gdk_draw_rectangle(widget->w->window, GC, FALSE, pr.o.x, pr.o.y,
			   pr.c.x - pr.o.x, pr.c.y - pr.o.y);
    }

    return 0;
}


int GCpolygon(Gwidget_t * widget, int gpn, Gpoint_t * gpp, Ggattr_t * ap)
{

    Grect_t gr;
    int n, i;

    if (gpn == 0)
	return 0;

    gr.o = gpp[0], gr.c = gpp[0];
    for (i = 1; i < gpn; i++) {
	gr.o.x = min(gr.o.x, gpp[i].x);
	gr.o.y = min(gr.o.y, gpp[i].y);
	gr.c.x = min(gr.c.x, gpp[i].x);
	gr.c.y = min(gr.c.y, gpp[i].y);
    }
/*	if(!ISVISIBLE(gr))
		return 1;
*/
    if (gpn + 1 > Gppn) {
	n = (((gpn + 1) + PPINCR - 1) / PPINCR) * PPINCR;
	Gppp = Marraygrow(Gppp, (long) n * PPSIZE);
	Gppn = n;
    }
    for (i = 0; i < gpn; i++)
	Gppp[i] = pdrawtopix(widget, gpp[i]);

    setgattr(widget, ap);
    if (WCU->gattr.fill) {
	if (Gppp[gpn - 1].x != Gppp[0].x || Gppp[gpn - 1].y != Gppp[0].y)
	    Gppp[gpn] = Gppp[0], gpn++;

	gdk_draw_polygon(widget->w, GC, TRUE, Gppp, gpn);
    } else
	gdk_draw_polygon(widget->w, GC, FALSE, Gppp, gpn);

    return 0;
}


static void bezier(PIXpoint_t p0, PIXpoint_t p1, PIXpoint_t p2,
		   PIXpoint_t p3)
{

    Gpoint_t gp0, gp1, gp2;
    Gsize_t s;
    PIXpoint_t p;
    double t;
    int n, i, steps;

    if ((s.x = p3.x - p0.x) < 0)
	s.x = -s.x;
    if ((s.y = p3.y - p0.y) < 0)
	s.y = -s.y;
    if (s.x > s.y)
	steps = s.x / 5 + 1;
    else
	steps = s.y / 5 + 1;
    for (i = 0; i <= steps; i++) {
	t = i / (double) steps;
	gp0.x = p0.x + t * (p1.x - p0.x);
	gp0.y = p0.y + t * (p1.y - p0.y);
	gp1.x = p1.x + t * (p2.x - p1.x);
	gp1.y = p1.y + t * (p2.y - p1.y);
	gp2.x = p2.x + t * (p3.x - p2.x);
	gp2.y = p2.y + t * (p3.y - p2.y);
	gp0.x = gp0.x + t * (gp1.x - gp0.x);
	gp0.y = gp0.y + t * (gp1.y - gp0.y);
	gp1.x = gp1.x + t * (gp2.x - gp1.x);
	gp1.y = gp1.y + t * (gp2.y - gp1.y);
	p.x = gp0.x + t * (gp1.x - gp0.x) + 0.5;
	p.y = gp0.y + t * (gp1.y - gp0.y) + 0.5;
	if (Gppi >= Gppn) {
	    n = (((Gppi + 1) + PPINCR - 1) / PPINCR) * PPINCR;
	    Gppp = Marraygrow(Gppp, (long) n * PPSIZE);
	    Gppn = n;
	}
	Gppp[Gppi++] = p;
    }
}


int GCsplinegon(Gwidget_t * widget, int gpn, Gpoint_t * gpp, Ggattr_t * ap)
{

    PIXpoint_t p0, p1, p2, p3;
    Grect_t gr;
    int n, i;

    if (gpn == 0)
	return 0;

    gr.o = gpp[0], gr.c = gpp[0];
    for (i = 1; i < gpn; i++) {
	gr.o.x = min(gr.o.x, gpp[i].x);
	gr.o.y = min(gr.o.y, gpp[i].y);
	gr.c.x = max(gr.c.x, gpp[i].x);
	gr.c.y = max(gr.c.y, gpp[i].y);
    }

    Gppi = 1;
    if (Gppi >= Gppn) {
	n = (((Gppi + 1) + PPINCR - 1) / PPINCR) * PPINCR;
	Gppp = Marraygrow(Gppp, (long) n * PPSIZE);
	Gppn = n;
    }
    Gppp[0] = p3 = pdrawtopix(widget, gpp[0]);
    for (i = 1; i < gpn; i += 3) {
	p0 = p3;
	p1 = pdrawtopix(widget, gpp[i]);
	p2 = pdrawtopix(widget, gpp[i + 1]);
	p3 = pdrawtopix(widget, gpp[i + 2]);
	bezier(p0, p1, p2, p3);
    }
    setgattr(widget, ap);
    gdk_draw_lines(widget->w->window, GC, Gppp, Gppi);
    return 0;
}


int GCarc(Gwidget_t * widget, Gpoint_t gc, Gsize_t gs, double ang1,
	  double ang2, Ggattr_t * ap)
{

    PIXpoint_t pc;
    PIXsize_t ps;
    Grect_t gr;

    gr.o.x = gc.x - gs.x, gr.o.y = gc.y - gs.y;
    gr.c.x = gc.x + gs.x, gr.c.y = gc.y + gs.y;
/*	if(!ISVISIBLE(gr))
		return 1;
*/

    pc = pdrawtopix(widget, gc), ps = sdrawtopix(widget, gs);
    setgattr(widget, ap);

    if (WCU->gattr.fill) {
	gdk_draw_arc(widget->w->window, GC, TRUE, pc.x - ps.x, pc.y - ps.y,
		     ps.x * 2, ps.y * 2, (int) (ang1 * 64), (ang2 * 64));
    } else {
	gdk_draw_arc(widget->w->window, GC, FALSE, pc.x - ps.x,
		     pc.y - ps.y, ps.x * 2, ps.y * 2, (int) (ang1 * 64),
		     (ang2 * 64));
    }
    return 0;
}


static GdkFont *findfont(char *name, int size)
{
    GdkFont *font;
    int fi, n, i;

    if (name[0] == '\000')
	return Gfontp[0].font;

    sprintf(&Gbufp[0], name, size);
    for (fi = 0; fi < Gfontn; fi++)
	if (Strcmp(&Gbufp[0], Gfontp[fi].name) == 0)
	    return Gfontp[fi].font;

    if (!(font = gdk_font_load(&Gbufp[0]))) {
	n = strlen(&Gbufp[0]) + 1;
	for (i = 1; i < size; i++) {
	    sprintf(&Gbufp[n], name, size - i);
	    if ((font = gdk_font_load(&Gbufp[n])))
		break;
	    sprintf(&Gbufp[n], name, size + i);
	    if ((font = gdk_font_load(&Gbufp[n])))
		break;
	}
    }
    if (!font)
	font = Gfontp[0].font;

    Gfontp = Marraygrow(Gfontp, (long) (Gfontn + 1) * FONTSIZE);
    Gfontp[Gfontn].name = strdup(&Gbufp[0]);
    Gfontp[Gfontn].font = font;
    Gfontn++;
    return font;
}


int GCtext(Gwidget_t * widget, Gtextline_t * tlp, int n, Gpoint_t go,
	   char *fn, double fs, char *justs, Ggattr_t * ap)
{

    Gsize_t gs;
    PIXpoint_t po;
    PIXsize_t ps;
    PIXrect_t pr;
    Grect_t gr;
    GdkFont *font;
    int dir, asc, des, x = 0, y, w, h, i;
    int lbearing, rbearing, width;

    po = pdrawtopix(widget, go);
    gs.x = 0, gs.y = fs;
    ps = sdrawtopix(widget, gs);
    if (!(font = findfont(fn, ps.y))) {
	printf("NO FONT\n");
	gdk_draw_rectangle(widget->w, GC, FALSE, po.x, po.y, 1, 1);
	return 0;
    }

    setgattr(widget, ap);
    SETFONT(font);

    for (w = h = 0, i = 0; i < n; i++) {
	gdk_text_extents(font, tlp[i].p, tlp[i].n, &lbearing, &rbearing,
			 &width, &asc, &des);

	tlp[i].w = width, tlp[i].h = asc + des;
	w = max(w, width), h += asc + des;

    }

    switch (justs[0]) {
    case 'l':
	po.x += w / 2;
	break;
    case 'r':
	po.x -= w / 2;
	break;
    }
    switch (justs[1]) {
    case 'd':
	po.y -= h;
	break;
    case 'c':
	po.y -= h / 2;
	break;
    }
    pr.o.x = po.x - w / 2, pr.o.y = po.y;
    pr.c.x = po.x + w / 2, pr.c.y = po.y + h;
    gr = rpixtodraw(widget, pr);

/*	if(!ISVISIBLE(gr))
		return 1;
*/

    for (i = 0; i < n; i++) {
	switch (tlp[i].j) {
	case 'l':
	    x = po.x - w / 2;
	    break;
	case 'n':
	    x = po.x - tlp[i].w / 2;
	    break;
	case 'r':
	    x = po.x - (tlp[i].w - w / 2);
	    break;
	}
	y = po.y + (i + 1) * tlp[i].h - des;

	gdk_draw_text(widget->w->window, font, GC, x, y, tlp[i].p,
		      tlp[i].n);
    }

    return 0;
}


int GCgettextsize(Gwidget_t * widget, Gtextline_t * tlp, int n, char *fn,
		  double fs, Gsize_t * gsp)
{

    Gsize_t gs;
    PIXsize_t ps;
    GdkFont *font;
    int i, dir, asc, des, rbearing, lbearing, width;

    gs.x = 0, gs.y = fs;
    ps = sdrawtopix(widget, gs);
    if (!(font = findfont(fn, ps.y))) {
	gsp->x = 1, gsp->y = 1;
	return 0;
    }
    SETFONT(font);
    for (ps.x = ps.y = 0, i = 0; i < n; i++) {
	gdk_text_extents(font, tlp[i].p, tlp[i].n, &lbearing, &rbearing,
			 &width, &asc, &des);
	ps.x = max(ps.x, width), ps.y += asc + des;
    }

    *gsp = spixtodraw(widget, ps);
    return 0;
}


int GCcreatebitmap(Gwidget_t * widget, Gbitmap_t * bitmap, Gsize_t s)
{

    return 0;
}


int GCdestroybitmap(Gbitmap_t * bitmap)
{

    return 0;
}


int GCreadbitmap(Gwidget_t * widget, Gbitmap_t * bitmap, FILE * fp)
{

    return 0;
}


int GCwritebitmap(Gbitmap_t * bitmap, FILE * fp)
{

    return 0;
}


int GCbitblt(Gwidget_t * widget, Gpoint_t gp, Grect_t gr,
	     Gbitmap_t * bitmap, char *mode, Ggattr_t * ap)
{
}


int GCgetmousecoords(Gwidget_t * widget, Gpoint_t * gpp, int *count)
{
    PIXpoint_t pp;
    int state;

    gdk_window_get_pointer(widget->w->window, &pp.x, &pp.y, &state);
    *gpp = Gppixtodraw(widget, pp);
    *count = 1;

    return 0;
}


int GCcanvasclear(Gwidget_t * widget)
{
    int gotit;
    GdkDrawable *drawable;
    GtkWidget *drawing_area = widget->w;
    drawable = drawing_area->window;

    gdk_window_clear(widget->w->window);
    gdk_draw_rectangle(drawable, drawing_area->style->white_gc, TRUE, 0, 0,
		       drawing_area->allocation.width,
		       drawing_area->allocation.height);
    WCU->needredraw = FALSE;
    gotit = FALSE;


    if (gotit)
	adjustclip(widget);
    return 0;
}


int GCgetgfxattr(Gwidget_t * widget, Ggattr_t * ap)
{

    if ((ap->flags & G_GATTRCOLOR))
	ap->color = WCU->gattr.color;
    if ((ap->flags & G_GATTRWIDTH))
	ap->width = WCU->gattr.width;
    if ((ap->flags & G_GATTRMODE))
	ap->mode = WCU->gattr.mode;
    if ((ap->flags & G_GATTRFILL))
	ap->fill = WCU->gattr.fill;
    if ((ap->flags & G_GATTRSTYLE))
	ap->style = WCU->gattr.style;
    return 0;
}


gint Gcwbutaction(GtkWidget * w, GdkEvent * event, gpointer data)
{
    Gwidget_t *widget;
    Gevent_t gev;
    int xtype, bn, wi;
    PIXpoint_t pp;

    widget = findwidget((unsigned long) w, G_CANVASWIDGET);
    switch ((xtype = event->type)) {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:

	gev.type = G_MOUSE;
	gev.code = (xtype == GDK_BUTTON_PRESS) ? G_DOWN : G_UP;

	gev.data = event->button.button - 1;
	pp.x = event->button.x, pp.y = event->button.y;
	gev.p = Gppixtodraw(widget, pp);
	bn = WCU->bstate[gev.data];
	WCU->bstate[gev.data] = (xtype == GDK_BUTTON_PRESS) ? 1 : 0;
	bn = WCU->bstate[gev.data] - bn;
	WCU->buttonsdown += bn;
	Gbuttonsdown += bn;
	break;
    default:
	break;
    }

    wi = gev.wi = widget - &Gwidgets[0];
    Gpopdownflag = FALSE;
    if (WCU->func) {
	(*WCU->func) (&gev);
    }

    if (Gpopdownflag) {
	Gpopdownflag = FALSE;
	if (gev.code == G_DOWN) {

	    gev.code = G_UP;
	    widget = &Gwidgets[wi];
	    WCU->bstate[gev.data] = 0;
	    WCU->buttonsdown--;
	    Gbuttonsdown--;
	    if (widget->inuse && WCU->func)
		(*WCU->func) (&gev);
	}
    }
    return TRUE;
}


void Gcwkeyaction(GtkWidget * w, GdkEventKey * event, gpointer data)
{
    Gwidget_t *widget;
    Gevent_t gev;
    int xtype, bn, wi, state;
    PIXpoint_t pp;
    unsigned int mask;
    char c;

    widget = findwidget((unsigned long) canvas->w, G_CANVASWIDGET);
    switch ((xtype = event->type)) {
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
	gdk_window_get_pointer(event->window, &pp.x, &pp.y, &state);
	gev.type = G_KEYBD;
	gev.code = (xtype == GDK_KEY_PRESS) ? G_DOWN : G_UP;
	gev.data = event->keyval;
	gev.p = Gppixtodraw(widget, pp);
	break;
    default:
	return;
    }
    gev.wi = widget - &Gwidgets[0];
    Gpopdownflag = FALSE;
    if (WCU->func)
	(*WCU->func) (&gev);
    if (Gpopdownflag)
	Gpopdownflag = FALSE;

}


gint exposeeventhandler(GtkWidget * w, GdkEvent * event, gpointer data)
{
    Gwidget_t *widget;

    widget = findwidget((unsigned long) w, G_CANVASWIDGET);
    gdk_draw_rectangle(widget->w->window, widget->w->style->white_gc, TRUE,
		       0, 0, widget->w->allocation.width,
		       widget->w->allocation.height);
}


gint cweventhandler(GtkWidget * w, GdkEvent * event, gpointer data)
{
    Gwidget_t *widget;

    widget = findwidget((unsigned long) w, G_CANVASWIDGET);
    Gneedredraw = WCU->needredraw = TRUE;
    adjustclip(widget);

    gtk_signal_connect(G_OBJECT(w), "visibility_notify_event",
		       GTK_SIGNAL_FUNC(cweventhandler), NULL);
    gtk_signal_connect(G_OBJECT(w), "motion_notify_event",
		       GTK_SIGNAL_FUNC(cweventhandler), NULL);
}


static void setgattr(Gwidget_t * widget, Ggattr_t * ap)
{
    GdkGCValues gcv;
    GdkColor c;
    int color, width, mode, style, pati;
    double intens;

    if (!(ap->flags & G_GATTRCOLOR))
	ap->color = WCU->defgattr.color;
    if (!(ap->flags & G_GATTRWIDTH))
	ap->width = WCU->defgattr.width;
    if (!(ap->flags & G_GATTRMODE))
	ap->mode = WCU->defgattr.mode;
    if (!(ap->flags & G_GATTRFILL))
	ap->fill = WCU->defgattr.fill;
    if (!(ap->flags & G_GATTRSTYLE))
	ap->style = WCU->defgattr.style;
    color = ap->color;

    if (color >= G_MAXCOLORS || !(WCU->colors[color].inuse))
	color = 1;

    if (color != WCU->gattr.color) {

	WCU->gattr.color = color;
	/*if (ap->mode == GDK_XOR) {
	   gdk_gc_set_foreground (GC, widget->w->style->white_gc);
	   }
	   else {
	 */
	gdk_gc_set_foreground(GC, &WCU->colors[WCU->gattr.color].color);
	/* } */

/*	    if (Gdepth == 1) {
	        cp = &WCU->colors[color].color;
	        intens = (0.3 * cp->blue + 0.59 * cp->red +
	                        0.11 * cp->green) / 65535.0;
	        pati = (intens <= 0.0625) ? 16 :
	                    -16.0 * (log (intens) / 2.7725887222);
	        XSetTile (Gdisplay, GC, WCU->grays[pati]);
            }
*/
    }
    mode = ap->mode;
    if (mode != WCU->gattr.mode) {
	WCU->gattr.mode = mode;
/*	    XSetFunction (Gdisplay, GC, WCU->gattr.mode);
	    if (mode == GDK_XOR)
            	gdk_gc_set_foreground (GC, &WCU->colors[0].color);
            else
*/
	gdk_gc_set_foreground(GC, &WCU->colors[WCU->gattr.color].color);
    }
    width = ap->width;
    if (width != WCU->gattr.width) {
	gdk_gc_set_line_attributes(GC, width, GDK_LINE_SOLID,
				   GDK_CAP_NOT_LAST, GDK_JOIN_BEVEL);
    }

    WCU->gattr.fill = ap->fill;
    style = ap->style;
    if (style != WCU->gattr.style) {
	WCU->gattr.style = style;
	if (style == G_SOLID) {
	    gdk_gc_set_line_attributes(GC, width, GDK_LINE_SOLID,
				       GDK_CAP_NOT_LAST, GDK_JOIN_BEVEL);
	} else {
	    gdk_gc_set_dashes(GC, 0, gstyles[style], 2);
	    gdk_gc_set_line_attributes(GC, width, GDK_LINE_ON_OFF_DASH,
				       GDK_CAP_NOT_LAST, GDK_JOIN_BEVEL);
	}
    }
}


static PIXrect_t rdrawtopix(Gwidget_t * widget, Grect_t gr)
{
    PIXrect_t pr;
    double tvx, tvy, twx, twy;

    tvx = WCU->vsize.x - 1, tvy = WCU->vsize.y - 1;
    twx = WCU->wrect.c.x - WCU->wrect.o.x;
    twy = WCU->wrect.c.y - WCU->wrect.o.y;
    pr.o.x = tvx * (gr.o.x - WCU->wrect.o.x) / twx + 0.5;
    pr.o.y = tvy * (1.0 - (gr.c.y - WCU->wrect.o.y) / twy) + 0.5;
    pr.c.x = tvx * (gr.c.x - WCU->wrect.o.x) / twx + 0.5;
    pr.c.y = tvy * (1.0 - (gr.o.y - WCU->wrect.o.y) / twy) + 0.5;
    return pr;
}


static PIXpoint_t pdrawtopix(Gwidget_t * widget, Gpoint_t gp)
{
    PIXpoint_t pp;
    double tvx, tvy, twx, twy;

    tvx = WCU->vsize.x - 1, tvy = WCU->vsize.y - 1;
    twx = WCU->wrect.c.x - WCU->wrect.o.x;
    twy = WCU->wrect.c.y - WCU->wrect.o.y;

    pp.x = tvx * (gp.x - WCU->wrect.o.x) / twx + 0.5;
    pp.y = tvy * (1.0 - (gp.y - WCU->wrect.o.y) / twy) + 0.5;
    return pp;
}


static PIXsize_t sdrawtopix(Gwidget_t * widget, Gsize_t gs)
{
    PIXsize_t ps;
    double tvx, tvy, twx, twy;

    tvx = WCU->vsize.x - 1, tvy = WCU->vsize.y - 1;
    twx = WCU->wrect.c.x - WCU->wrect.o.x;
    twy = WCU->wrect.c.y - WCU->wrect.o.y;
    ps.x = tvx * (gs.x - 1) / twx + 1.5;
    ps.y = tvy * (gs.y - 1) / twy + 1.5;

    return ps;
}


static Gpoint_t Gppixtodraw(Gwidget_t * widget, PIXpoint_t pp)
{
    Gpoint_t gp;
    double tvx, tvy, twx, twy;

    tvx = WCU->vsize.x - 1, tvy = WCU->vsize.y - 1;
    twx = WCU->wrect.c.x - WCU->wrect.o.x;
    twy = WCU->wrect.c.y - WCU->wrect.o.y;

    gp.x = (pp.x / tvx) * twx + WCU->wrect.o.x;
    gp.y = (1.0 - pp.y / tvy) * twy + WCU->wrect.o.y;
    return gp;
}


static Gsize_t spixtodraw(Gwidget_t * widget, PIXsize_t ps)
{
    Gsize_t gs;
    double tvx, tvy, twx, twy;

    tvx = WCU->vsize.x - 1, tvy = WCU->vsize.y - 1;
    twx = WCU->wrect.c.x - WCU->wrect.o.x;
    twy = WCU->wrect.c.y - WCU->wrect.o.y;
    gs.x = ((ps.x - 1) / tvx) * twx + 1;
    gs.y = ((ps.y - 1) / tvy) * twy + 1;
    return gs;
}


static Grect_t rpixtodraw(Gwidget_t * widget, PIXrect_t pr)
{
    Grect_t gr;
    double tvx, tvy, twx, twy, n;

    tvx = WCU->vsize.x - 1, tvy = WCU->vsize.y - 1;
    twx = WCU->wrect.c.x - WCU->wrect.o.x;
    twy = WCU->wrect.c.y - WCU->wrect.o.y;

    gr.o.x = (pr.o.x / tvx) * twx + WCU->wrect.o.x;
    gr.o.y = (1.0 - pr.c.y / tvy) * twy + WCU->wrect.o.y;
    gr.c.x = (pr.c.x / tvx) * twx + WCU->wrect.o.x;
    gr.c.y = (1.0 - pr.o.y / tvy) * twy + WCU->wrect.o.y;

    if (gr.o.x > gr.c.x)
	n = gr.o.x, gr.o.x = gr.c.x, gr.c.x = n;
    if (gr.o.y > gr.c.y)
	n = gr.o.y, gr.o.y = gr.c.y, gr.c.y = n;
    return gr;
}


static PIXrect_t rdrawtobpix(Gbitmap_t * bitmap, Grect_t gr)
{
    PIXrect_t pr;
    double tvy;

    tvy = (bitmap->size.y - 1) * bitmap->scale.y;
    pr.o.x = gr.o.x + 0.5;
    pr.o.y = tvy - gr.c.y + 0.5;
    pr.c.x = gr.c.x + 0.5;
    pr.c.y = tvy - gr.o.y + 0.5;
    return pr;
}


static PIXpoint_t pdrawtobpix(Gbitmap_t * bitmap, Gpoint_t gp)
{
    PIXpoint_t pp;
    double tvy;

    tvy = (bitmap->size.y - 1) * bitmap->scale.y;
    pp.x = gp.x + 0.5;
    pp.y = tvy - gp.y + 0.5;
    return pp;
}


static void adjustclip(Gwidget_t * widget)
{
    Gwidget_t *parent;
    PIXrect_t pr;
    int width, height;

}
