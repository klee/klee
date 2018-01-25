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

#define WVU widget->u.v

int GVcreatewidget (
    Gwidget_t *parent, Gwidget_t *widget, int attrn, Gwattr_t *attrp
) {
    PIXpoint_t po;
    PIXsize_t ps;
    DWORD wflags;
    char *s;
    int ai;

    WVU->func = NULL;
    WVU->closing = FALSE;
    wflags = WS_OVERLAPPEDWINDOW;
    s = "LEFTY";
    po.x = po.y = CW_USEDEFAULT;
    ps.x = ps.y = MINVWSIZE;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRORIGIN:
            GETORIGIN (attrp[ai].u.p, po);
            break;
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINVWSIZE);
            break;
        case G_ATTRNAME:
            s = attrp[ai].u.t;
            break;
        case G_ATTRZORDER:
            Gerr (POS, G_ERRCANNOTSETATTR1, "zorder");
            return -1;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR1, "windowid");
            return -1;
        case G_ATTREVENTCB:
            WVU->func = attrp[ai].u.func;
            break;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    if (!(widget->w = CreateWindow (
        "LeftyClass", s, wflags, po.x, po.y,
        ps.x, ps.y, (HWND) 0, (HMENU) 0, hinstance, NULL
    ))) {
        Gerr (POS, G_ERRCANNOTCREATEWIDGET);
        return -1;
    }
    ShowWindow (widget->w, SW_SHOW);
    UpdateWindow (widget->w);
    return 0;
}

int GVsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    PIXpoint_t po;
    PIXsize_t ps;
    DWORD wflags1, wflags2, wflags3, wflags4;
    int ai;

    wflags1 = SWP_NOMOVE | SWP_NOZORDER;
    wflags2 = SWP_NOSIZE | SWP_NOZORDER;
    wflags3 = SWP_NOSIZE | SWP_NOMOVE;
    wflags4 = SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRORIGIN:
            GETORIGIN (attrp[ai].u.p, po);
            SetWindowPos (widget->w, (HWND) NULL, po.x, po.y, 0, 0, wflags2);
            break;
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINVWSIZE);
            SetWindowPos (widget->w, (HWND) NULL, 0, 0, ps.x, ps.y, wflags1);
            break;
        case G_ATTRNAME:
            SetWindowText (widget->w, attrp[ai].u.t);
            return -1;
        case G_ATTRZORDER:
            if (strcmp (attrp[ai].u.t, "top") == 0)
                SetWindowPos (widget->w, (HWND) HWND_TOP, 0, 0, 0, 0, wflags3);
            else if (strcmp (attrp[ai].u.t, "bottom") == 0)
                SetWindowPos (
                    widget->w, (HWND) HWND_BOTTOM, 0, 0, 0, 0, wflags4
                );
            else {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR1, "windowid");
            return -1;
        case G_ATTREVENTCB:
            WVU->func = attrp[ai].u.func;
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

int GVgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    RECT r;
    int ai;

    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRORIGIN:
            GetWindowRect (widget->w, &r);
            attrp[ai].u.p.x = r.left, attrp[ai].u.p.y = r.top;
            break;
        case G_ATTRSIZE:
            GetWindowRect (widget->w, &r);
            attrp[ai].u.s.x = r.right - r.left;
            attrp[ai].u.s.y = r.bottom - r.top;
            break;
        case G_ATTRNAME:
            Gerr (POS, G_ERRNOTIMPLEMENTED);
            return -1;
        case G_ATTRZORDER:
            Gerr (POS, G_ERRNOTIMPLEMENTED);
            return -1;
        case G_ATTRWINDOWID:
            sprintf (&Gbufp[0], "0x%lx", widget->w);
            attrp[ai].u.t = &Gbufp[0];
            break;
        case G_ATTREVENTCB:
            attrp[ai].u.func = WVU->func;
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

int GVdestroywidget (Gwidget_t *widget) {
    WVU->closing = TRUE;
    DestroyWindow (widget->w);
    return 0;
}
