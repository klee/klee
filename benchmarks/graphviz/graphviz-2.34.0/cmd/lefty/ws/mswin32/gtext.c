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

#define WTU widget->u.t

int GTcreatewidget (
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
    WTU->func = NULL;
    ps.x = ps.y = MINTWSIZE;
    s = "";
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINTWSIZE);
            break;
        case G_ATTRBORDERWIDTH:
            wflags |= WS_BORDER;
            break;
        case G_ATTRTEXT:
            s = attrp[ai].u.t;
            break;
        case G_ATTRAPPENDTEXT:
            Gerr (POS, G_ERRCANNOTSETATTR1, "appendtext");
            return -1;
        case G_ATTRMODE:
            if (
                strcmp ("oneline", attrp[ai].u.t) != 0 &&
                strcmp ("input", attrp[ai].u.t) != 0 &&
                strcmp ("output", attrp[ai].u.t) != 0
            ) {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR1, "windowid");
            return -1;
        case G_ATTRNEWLINECB:
            WTU->func = attrp[ai].u.func;
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
    wflags |= (ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL);
    if (ps.y > 40)
        wflags |= (WS_HSCROLL | WS_VSCROLL);
    if (!(widget->w = CreateWindow (
        "EDIT", s, wflags, 0, 0, ps.x, ps.y,
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

int GTsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    Gwidget_t *parent;
    PIXsize_t ps;
    DWORD wflags1;
    int ai;

    parent = (widget->pwi == -1) ? NULL : &Gwidgets[widget->pwi];
    wflags1 = SWP_NOMOVE | SWP_NOZORDER;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINTWSIZE);
            Gadjustwrect (parent, &ps);
            SetWindowPos (widget->w, (HWND) NULL, 0, 0, ps.x, ps.y, wflags1);
            break;
        case G_ATTRBORDERWIDTH:
            Gerr (POS, G_ERRCANNOTSETATTR2, "borderwidth");
            return -1;
        case G_ATTRTEXT:
            Gnocallbacks = TRUE;
            SendMessage (widget->w, WM_SETTEXT, 0, "");
            SendMessage (widget->w, EM_SETSEL, -1, 32760);
            SendMessage (widget->w, EM_REPLACESEL, 0, (LPARAM) attrp[ai].u.t);
            SendMessage (widget->w, EM_SETSEL, -1, 32760);
#if 0
            SendMessage (widget->w, EM_REPLACESEL, 0, (LPARAM) "\r\n");
            SendMessage (widget->w, EM_SETSEL, -1, 32760);
#endif
            Gnocallbacks = FALSE;
            break;
        case G_ATTRAPPENDTEXT:
            Gnocallbacks = TRUE;
            SendMessage (widget->w, EM_SETSEL, -1, 32760);
            SendMessage (widget->w, EM_REPLACESEL, 0, (LPARAM) attrp[ai].u.t);
            SendMessage (widget->w, EM_SETSEL, -1, 32760);
            SendMessage (widget->w, EM_REPLACESEL, 0, (LPARAM) "\r\n");
            SendMessage (widget->w, EM_SETSEL, -1, 32760);
            Gnocallbacks = FALSE;
            break;
        case G_ATTRMODE:
            if (
                strcmp ("oneline", attrp[ai].u.t) != 0 &&
                strcmp ("input", attrp[ai].u.t) != 0 &&
                strcmp ("output", attrp[ai].u.t) != 0
            ) {
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

int GTgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    RECT r;
    int rtn, ai, i, j;

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
            if ((rtn = GetWindowTextLength (widget->w)) + 1 > Gbufn) {
                Gbufp = Marraygrow (Gbufp, (long) (rtn + 1) * BUFSIZE);
                Gbufn = rtn + 1;
            }
            GetWindowText (widget->w, &Gbufp[0], Gbufn - 1);
            for (i = 0, j = 0; Gbufp[i]; i++)
                if (Gbufp[i] != '\r')
                    Gbufp[j++] = Gbufp[i];
            Gbufp[j++] = 0;
            attrp[ai].u.t = &Gbufp[0];
            break;
        case G_ATTRAPPENDTEXT:
            Gerr (POS, G_ERRCANNOTGETATTR, "appendtext");
            return -1;
        case G_ATTRMODE:
            attrp[ai].u.t = "oneline";
            break;
        case G_ATTRWINDOWID:
            sprintf (&Gbufp[0], "0x%lx", widget->w);
            attrp[ai].u.t = &Gbufp[0];
            break;
        case G_ATTRNEWLINECB:
            attrp[ai].u.func = WTU->func;
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

int GTdestroywidget (Gwidget_t *widget) {
    Gwidget_t *parent;

    parent = (widget->pwi == -1) ? NULL : &Gwidgets[widget->pwi];
    if (parent && parent->type == G_ARRAYWIDGET)
        Gawdeletechild (parent, widget);
    DestroyWindow (widget->w);
    return 0;
}
