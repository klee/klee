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
#include "mem.h"

#define WLU widget->u.l

int GLcreatewidget (
    Gwidget_t *parent, Gwidget_t *widget, int attrn, Gwattr_t *attrp
) {
    PIXsize_t ps;
    DWORD wflags;
    char *s;
    int ai;

    if (!parent) {
        Gerr (POS, G_ERRNOPARENTWIDGET);
        return -1;
    }
    wflags = WS_CHILDWINDOW;
    WLU->func = NULL;
    ps.x = ps.y = MINLWSIZE;
    s = "";
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINLWSIZE);
            break;
        case G_ATTRBORDERWIDTH:
            wflags |= WS_BORDER;
            break;
        case G_ATTRTEXT:
            s = attrp[ai].u.t;
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR1, "windowid");
            return -1;
        case G_ATTREVENTCB:
            WLU->func = attrp[ai].u.func;
            break;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    Gadjustwrect (parent, &ps);
    if (!(widget->w = CreateWindow (
        "LabelClass", s, wflags, 0, 0, ps.x, ps.y,
        parent->w, (HMENU) (widget - &Gwidgets[0]), hinstance, NULL
    ))) {
        Gerr (POS, G_ERRCANNOTCREATEWIDGET);
        return -1;
    }
    ShowWindow (widget->w, SW_SHOW);
    UpdateWindow (widget->w);
    if (parent && parent->type == G_ARRAYWIDGET)
        Gawinsertchild (parent, widget);
    return 0;
}

int GLsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    Gwidget_t *parent;
    PIXsize_t ps;
    RECT r;
    DWORD wflags1;
    int ai;

    parent = (widget->pwi == -1) ? NULL : &Gwidgets[widget->pwi];
    wflags1 = SWP_NOMOVE | SWP_NOZORDER;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINLWSIZE);
            Gadjustwrect (parent, &ps);
            SetWindowPos (widget->w, (HWND) NULL, 0, 0, ps.x, ps.y, wflags1);
            r.top = r.left = 0;
            r.bottom = ps.y, r.right = ps.x;
            InvalidateRect (widget->w, NULL, FALSE);
            break;
        case G_ATTRBORDERWIDTH:
            Gerr (POS, G_ERRCANNOTSETATTR2, "borderwidth");
            return -1;
        case G_ATTRTEXT:
            SetWindowText (widget->w, attrp[ai].u.t);
            GetClientRect (widget->w, &r);
            InvalidateRect (widget->w, &r, TRUE);
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR2, "windowid");
            return -1;
        case G_ATTREVENTCB:
            attrp[ai].u.func = WLU->func;
            break;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    return 0;
}

int GLgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    RECT r;
    int ai;

    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GetWindowRect (widget->w, &r);
            attrp[ai].u.s.x = r.right - r.left;
            attrp[ai].u.s.y = r.bottom - r.top;
            break;
        case G_ATTRBORDERWIDTH:
            Gerr (POS, G_ERRCANNOTGETATTR, "borderwidth");
            return -1;
        case G_ATTRTEXT:
            GetWindowText (widget->w, &Gbufp[0], Gbufn);
            attrp[ai].u.t = &Gbufp[0];
            break;
        case G_ATTRWINDOWID:
            sprintf (&Gbufp[0], "0x%lx", widget->w);
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
    Gwidget_t *parent;

    parent = (widget->pwi == -1) ? NULL : &Gwidgets[widget->pwi];
    if (parent && parent->type == G_ARRAYWIDGET)
        Gawdeletechild (parent, widget);
    DestroyWindow (widget->w);
    return 0;
}
