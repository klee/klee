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

/* Lefteris Koutsofios - AT&T Labs Research */

#include "common.h"
#include "g.h"
#include "gcommon.h"

#define WLU widget->u.l

int GLcreatewidget (
    Gwidget_t *parent, Gwidget_t *widget, int attrn, Gwattr_t *attrp
) {
    PIXsize_t ps;
    int ai;
    XColor c;
    int color;

    if (!parent) {
        Gerr (POS, G_ERRNOPARENTWIDGET);
        return -1;
    }
    WLU->func = NULL;
    ps.x = ps.y = MINLWSIZE;
    RESETARGS;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINLWSIZE);
            break;
        case G_ATTRBORDERWIDTH:
            ADD2ARGS (XtNborderWidth, attrp[ai].u.i);
            break;
        case G_ATTRTEXT:
            ADD2ARGS (XtNlabel, attrp[ai].u.t);
            break;
        case G_ATTRCOLOR:
            color = attrp[ai].u.c.index;
            if (color != 0 && color != 1) {
                Gerr (POS, G_ERRBADCOLORINDEX, color);
                return -1;
            }
            c.red = attrp[ai].u.c.r * 257;
            c.green = attrp[ai].u.c.g * 257;
            c.blue = attrp[ai].u.c.b * 257;
            if (XAllocColor (
                Gdisplay, DefaultColormap (Gdisplay, Gscreenn), &c
            )) {
                if (color == 0)
                    ADD2ARGS (XtNbackground, c.pixel);
                else
                    ADD2ARGS (XtNforeground, c.pixel);
	    }
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR1, "windowid");
            return -1;
        case G_ATTREVENTCB:
            WLU->func = (Glabelcb) attrp[ai].u.func;
            break;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    ADD2ARGS (XtNwidth, ps.x);
    ADD2ARGS (XtNheight, ps.y);
    ADD2ARGS (XtNhighlightThickness, 0);
    ADD2ARGS (XtNinternalHeight, 0);
    ADD2ARGS (XtNinternalWidth, 0);
    ADD2ARGS (XtNjustify, XtJustifyLeft);
    if (!(widget->w = XtCreateWidget (
        "label", labelWidgetClass, parent->w, argp, argn
    ))) {
        Gerr (POS, G_ERRCANNOTCREATEWIDGET);
        return -1;
    }
    XtOverrideTranslations (widget->w, Glwanytable);
    Glazymanage (widget->w);
    return 0;
}

int GLsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    PIXsize_t ps;
    int ai;
    XColor c;
    int color;

    RESETARGS;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINLWSIZE);
            ADD2ARGS (XtNwidth, ps.x);
            ADD2ARGS (XtNheight, ps.y);
            break;
        case G_ATTRBORDERWIDTH:
            ADD2ARGS (XtNborderWidth, attrp[ai].u.i);
            break;
        case G_ATTRTEXT:
            ADD2ARGS (XtNlabel, attrp[ai].u.t);
            break;
        case G_ATTRCOLOR:
            color = attrp[ai].u.c.index;
            if (color != 0 && color != 1) {
                Gerr (POS, G_ERRBADCOLORINDEX, color);
                return -1;
            }
            c.red = attrp[ai].u.c.r * 257;
            c.green = attrp[ai].u.c.g * 257;
            c.blue = attrp[ai].u.c.b * 257;
            if (XAllocColor (
                Gdisplay, DefaultColormap (Gdisplay, Gscreenn), &c
            )) {
                if (color == 0)
                    ADD2ARGS (XtNbackground, c.pixel);
                else
                    ADD2ARGS (XtNforeground, c.pixel);
	    }
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR2, "windowid");
            return -1;
        case G_ATTREVENTCB:
            WLU->func = (Glabelcb) attrp[ai].u.func;
            break;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    XtSetValues (widget->w, argp, argn);
    return 0;
}

int GLgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    Dimension width, height;
    char *s;
    int ai;

    for (ai = 0; ai < attrn; ai++) {
        RESETARGS;
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            ADD2ARGS (XtNwidth, &width);
            ADD2ARGS (XtNheight, &height);
            XtGetValues (widget->w, argp, argn);
            attrp[ai].u.s.x = width, attrp[ai].u.s.y = height;
            break;
        case G_ATTRBORDERWIDTH:
            ADD2ARGS (XtNborderWidth, &width);
            XtGetValues (widget->w, argp, argn);
            attrp[ai].u.i = width;
            break;
        case G_ATTRTEXT:
            ADD2ARGS (XtNlabel, &s);
            XtGetValues (widget->w, argp, argn);
            attrp[ai].u.t = s;
            break;
        case G_ATTRWINDOWID:
            sprintf (&Gbufp[0], "0x%lx", XtWindow (widget->w));
            attrp[ai].u.t = &Gbufp[0];
            break;
        case G_ATTREVENTCB:
            attrp[ai].u.func = WLU->func;
            break;
        case G_ATTRUSERDATA:
            attrp[ai].u.u = widget->udata;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    return 0;
}

int GLdestroywidget (Gwidget_t *widget) {
    XtDestroyWidget (widget->w);
    return 0;
}

void Glwbutaction (Widget w, XEvent *evp, char **app, unsigned int *anp) {
    Gwidget_t *widget;
    Gevent_t gev;
    int wi, xtype;

    widget = findwidget ((unsigned long) w, G_LABELWIDGET);
    switch ((xtype = evp->type)) {
    case ButtonPress:
    case ButtonRelease:
        gev.type = G_MOUSE;
        gev.code = (xtype == ButtonPress) ? G_DOWN : G_UP;
        if ((gev.data = evp->xbutton.button - Button1) > 4)
            return;
        break;
    default:
        return;
    }
    wi = gev.wi = widget - &Gwidgets[0];
    if (WLU->func)
        (*WLU->func) (&gev);
    if (Gpopdownflag) {
        Gpopdownflag = FALSE;
        if (gev.code == G_DOWN) {
            gev.code = G_UP;
            widget = &Gwidgets[wi];
            if (widget->inuse && WLU->func)
                (*WLU->func) (&gev);
        }
    }
}

void Glwkeyaction (Widget w, XEvent *evp, char **app, unsigned int *anp) {
    Gwidget_t *widget;
    Gevent_t gev;
    int xtype;
    char c;

    widget = findwidget ((unsigned long) w, G_LABELWIDGET);
    switch ((xtype = evp->type)) {
    case KeyPress:
    case KeyRelease:
        if (!XLookupString ((XKeyEvent *) evp, &c, 1, NULL, NULL))
            return;
        gev.type = G_KEYBD;
        gev.code = (xtype == KeyPress) ? G_DOWN : G_UP;
        gev.data = c;
        break;
    default:
        return;
    }
    gev.wi = widget - &Gwidgets[0];
    if (WLU->func)
        (*WLU->func) (&gev);
    if (Gpopdownflag)
        Gpopdownflag = FALSE;
}
