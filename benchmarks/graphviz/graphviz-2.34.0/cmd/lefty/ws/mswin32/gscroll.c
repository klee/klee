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

int GScreatewidget (
    Gwidget_t *parent, Gwidget_t *widget, int attrn, Gwattr_t *attrp
) {
    PIXsize_t ps;
    DWORD wflags;
    int ai;

    if (!parent) {
        Gerr (POS, G_ERRNOPARENTWIDGET);
        return -1;
    }
    wflags = WS_CHILDWINDOW | WS_HSCROLL | WS_VSCROLL;
    ps.x = ps.y = MINSWSIZE;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINSWSIZE);
            break;
        case G_ATTRBORDERWIDTH:
            wflags |= WS_BORDER;
            break;
        case G_ATTRCHILDCENTER:
            Gerr (POS, G_ERRCANNOTSETATTR1, "childcenter");
            return -1;
        case G_ATTRMODE:
            if (strcmp ("forcebars", attrp[ai].u.t) != 0) {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR1, "windowid");
            return -1;
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
        "ScrollClass", "scroll", wflags, 0, 0,
        ps.x, ps.y, parent->w, (HMENU) (widget - &Gwidgets[0]),
        hinstance, NULL
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

int GSsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    Gwidget_t *parent, *child;
    PIXpoint_t po;
    PIXsize_t ps, pps, cps;
    RECT r;
    DWORD wflags1, wflags2;
    int ai, wi;

    parent = (widget->pwi == -1) ? NULL : &Gwidgets[widget->pwi];
    wflags1 = SWP_NOMOVE | SWP_NOZORDER;
    wflags2 = SWP_NOSIZE | SWP_NOZORDER;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINSWSIZE);
            Gadjustwrect (parent, &ps);
            SetWindowPos (widget->w, (HWND) NULL, 0, 0, ps.x, ps.y, wflags1);
            break;
        case G_ATTRBORDERWIDTH:
            Gerr (POS, G_ERRCANNOTSETATTR2, "borderwidth");
            return -1;
        case G_ATTRCHILDCENTER:
            for (wi = 0; wi < Gwidgetn; wi++) {
                child = &Gwidgets[wi];
                if (child->inuse && child->pwi == widget - &Gwidgets[0])
                    break;
            }
            if (wi == Gwidgetn)
                return 0;
            GETORIGIN (attrp[ai].u.p, po);
            GetClientRect (widget->w, &r);
            pps.x = r.right - r.left, pps.y = r.bottom - r.top;
            po.x -= pps.x / 2, po.y -= pps.y / 2;
            GetWindowRect (child->w, &r);
            cps.x = r.right - r.left, cps.y = r.bottom - r.top;
            if (po.x < 0)
                po.x = 0;
            if (po.y < 0)
                po.y = 0;
            if (po.x > cps.x - pps.x)
                po.x = cps.x - pps.x;
            if (po.y > cps.y - pps.y)
                po.y = cps.y - pps.y;
            SetWindowPos (child->w, (HWND) NULL, -po.x, -po.y, 0, 0, wflags2);
            SetScrollPos (widget->w, SB_HORZ, po.x, TRUE);
            SetScrollPos (widget->w, SB_VERT, po.y, TRUE);
            break;
        case G_ATTRMODE:
            if (strcmp ("forcebars", attrp[ai].u.t) != 0) {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR2, "windowid");
            return -1;
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

int GSgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    Gwidget_t *child;
    RECT r;
    int width, height, ai, wi;

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
        case G_ATTRCHILDCENTER:
            for (wi = 0; wi < Gwidgetn; wi++) {
                child = &Gwidgets[wi];
                if (child->inuse && child->pwi == widget - &Gwidgets[0])
                    break;
            }
            if (wi == Gwidgetn) {
                Gerr (POS, G_ERRNOCHILDWIDGET);
                return -1;
            }
            GetWindowRect (widget->w, &r);
            width = r.right - r.left;
            height = r.bottom - r.top;
            GetWindowRect (widget->w, &r);
            attrp[ai].u.p.x = width / 2 - r.left;
            attrp[ai].u.p.y = height / 2 - r.top;
            break;
        case G_ATTRMODE:
            attrp[ai].u.t = "forcebars";
            break;
        case G_ATTRWINDOWID:
            sprintf (&Gbufp[0], "0x%lx", widget->w);
            attrp[ai].u.t = &Gbufp[0];
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

int GSdestroywidget (Gwidget_t *widget) {
    Gwidget_t *parent;

    parent = (widget->pwi == -1) ? NULL : &Gwidgets[widget->pwi];
    if (parent && parent->type == G_ARRAYWIDGET)
        Gawdeletechild (parent, widget);
    DestroyWindow (widget->w);
    return 0;
}
