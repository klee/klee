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

#define WAU widget->u.a

int GAcreatewidget (
    Gwidget_t *parent, Gwidget_t *widget, int attrn, Gwattr_t *attrp
) {
    PIXsize_t ps;
    DWORD wflags;
    int mode, ai;

    if (!parent) {
        Gerr (POS, G_ERRNOPARENTWIDGET);
        return -1;
    }
    wflags = WS_CHILDWINDOW;
    mode = G_AWVARRAY;
    WAU->func = NULL;
    ps.x = ps.y = MINAWSIZE;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINAWSIZE);
            break;
        case G_ATTRBORDERWIDTH:
            wflags |= WS_BORDER;
            break;
        case G_ATTRMODE:
            if (strcmp ("horizontal", attrp[ai].u.t) == 0)
                mode = G_AWHARRAY;
            else if (strcmp ("vertical", attrp[ai].u.t) == 0)
                mode = G_AWVARRAY;
            else {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRLAYOUT:
            if (strcmp ("on", attrp[ai].u.t) == 0) {
                Gawsetmode (widget, FALSE);
                WAU->mode = G_AWHARRAY;
            } else if (strcmp ("off", attrp[ai].u.t) == 0) {
                Gawsetmode (widget, TRUE);
                WAU->mode = G_AWHARRAY;
            } else {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR1, "windowid");
            return -1;
        case G_ATTRRESIZECB:
            WAU->func = attrp[ai].u.func;
            break;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    Gawinitialize (widget, mode);
    Gadjustwrect (parent, &ps);
    if (!(widget->w = CreateWindow (
        "ArrayClass", "array", wflags, 0, 0, ps.x,
        ps.y, parent->w, (HMENU) (widget - &Gwidgets[0]),
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

int GAsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    Gwidget_t *parent;
    PIXsize_t ps;
    DWORD wflags1;
    int ai;

    parent = (widget->pwi == -1) ? NULL : &Gwidgets[widget->pwi];
    wflags1 = SWP_NOMOVE | SWP_NOZORDER;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINAWSIZE);
/*            Gadjustwrect (parent, &ps);*/
            SetWindowPos (widget->w, (HWND) NULL, 0, 0, ps.x, ps.y, wflags1);
            break;
        case G_ATTRBORDERWIDTH:
            Gerr (POS, G_ERRCANNOTSETATTR2, "borderwidth");
            return -1;
        case G_ATTRMODE:
            Gerr (POS, G_ERRCANNOTSETATTR2, "mode");
            return -1;
        case G_ATTRLAYOUT:
            if (strcmp ("on", attrp[ai].u.t) == 0)
                Gawsetmode (widget, FALSE);
            else if (strcmp ("off", attrp[ai].u.t) == 0)
                Gawsetmode (widget, TRUE);
            else {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR2, "windowid");
            return -1;
        case G_ATTRRESIZECB:
            WAU->func = attrp[ai].u.func;
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

int GAgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
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
        case G_ATTRMODE:
            attrp[ai].u.t = (
                WAU->mode == G_AWHARRAY
            ) ? "horizontal" : "vertical";
            break;
        case G_ATTRLAYOUT:
            attrp[ai].u.t = (Gawgetmode (widget)) ? "off" : "on";
            break;
        case G_ATTRWINDOWID:
            sprintf (&Gbufp[0], "0x%lx", widget->w);
            attrp[ai].u.t = &Gbufp[0];
            break;
        case G_ATTRRESIZECB:
            attrp[ai].u.func = WAU->func;
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

int GAdestroywidget (Gwidget_t *widget) {
    Gwidget_t *parent;

    parent = (widget->pwi == -1) ? NULL : &Gwidgets[widget->pwi];
    if (parent && parent->type == G_ARRAYWIDGET)
        Gawdeletechild (parent, widget);
    Gawdestroy (widget);
    DestroyWindow (widget->w);
    return 0;
}

/* the rest of this file contains the implementation of the array widget */

static void dolayout (Gwidget_t *, int);

int Gaworder (Gwidget_t *widget, void *data, Gawordercb func) {
    (*func) (data, &WAU->data);
    dolayout (widget, TRUE);
    return 0;
}

int Gawsetmode (Gwidget_t *widget, int mode) {
    WAU->data.batchmode = mode;
    dolayout (widget, TRUE);
    return 0;
}

int Gawgetmode (Gwidget_t *widget) {
    return WAU->data.batchmode;
}

void Gawdefcoordscb (int wi, Gawdata_t *dp) {
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

void Gawinitialize (Gwidget_t *widget, int mode) {
    WAU->data.type = mode;
    if (!(WAU->data.carray = Marrayalloc ((long) AWCARRAYINCR * AWCARRAYSIZE)))
        panic1 (POS, "Gawinitialize", "cannot allocate carray");
    WAU->data.cn = AWCARRAYINCR;
    WAU->data.cj = 0;
    WAU->data.batchmode = FALSE;
    WAU->data.working = FALSE;
}

void Gawdestroy (Gwidget_t *widget) {
    Marrayfree (WAU->data.carray);
    WAU->data.cn = WAU->data.cj = 0;
}

void Gawresize (Gwidget_t *widget) {
    dolayout (widget, FALSE);
}

void Gawinsertchild (Gwidget_t *parent, Gwidget_t *child) {
    if (parent->u.a->data.cj == parent->u.a->data.cn) {
        parent->u.a->data.carray = Marraygrow (
            parent->u.a->data.carray,
            (long) (parent->u.a->data.cn + AWCARRAYINCR) * AWCARRAYSIZE
        );
        parent->u.a->data.cn += AWCARRAYINCR;
    }
    parent->u.a->data.carray[parent->u.a->data.cj++].w = child->w;
    dolayout (parent, TRUE);
}

void Gawdeletechild (Gwidget_t *parent, Gwidget_t *child) {
    int ci;

    for (ci = 0; ci < parent->u.a->data.cj; ci++)
        if (parent->u.a->data.carray[ci].w == child->w)
            break;
    if (ci < parent->u.a->data.cj) {
        for (; ci + 1 < parent->u.a->data.cj; ci++)
            parent->u.a->data.carray[ci].w = parent->u.a->data.carray[ci + 1].w;
        parent->u.a->data.cj--;
        dolayout (parent, TRUE);
    }
}

static void dolayout (Gwidget_t *widget, int flag) {
    Gawdata_t *dp;
    Gawcarray_t *cp;
    RECT r;
    int sx, sy, ci;

    if (WAU->data.batchmode || WAU->data.working)
        return;
    WAU->data.working = TRUE;
    dp = &WAU->data;
    for (ci = 0; ci < dp->cj; ci++) {
        cp = &dp->carray[ci];
        GetWindowRect (cp->w, &r);
        cp->flag = 1;
        cp->ox = 0;
        cp->oy = 0;
        cp->sx = r.right - r.left;
        cp->sy = r.bottom - r.top;
        cp->bs = 0;
    }
    GetClientRect (widget->w, &r);
    dp->sx = r.right - r.left, dp->sy = r.bottom - r.top;
    if (WAU->func)
        (*WAU->func) (widget - &Gwidgets[0], dp);
    else
        Gawdefcoordscb (widget - &Gwidgets[0], dp);
    if ((sx = dp->sx) < MINAWSIZE)
        sx = MINAWSIZE;
    if ((sy = dp->sy) < MINAWSIZE)
        sy = MINAWSIZE;
    if (flag && (r.right - r.left != sx || r.bottom - r.top != sy)) {
        sx -= (r.right - r.left);
        sy -= (r.bottom - r.top);
        GetWindowRect (widget->w, &r);
        sx += (r.right - r.left);
        sy += (r.bottom - r.top);
        SetWindowPos (
            widget->w, (HWND) NULL, 0, 0, sx, sy, SWP_NOMOVE | SWP_NOZORDER
        );
    }
    for (ci = 0; ci < dp->cj; ci++) {
        cp = &dp->carray[ci];
        SetWindowPos (
            cp->w, (HWND) NULL, cp->ox, cp->oy, cp->sx, cp->sy, SWP_NOZORDER
        );
    }
    WAU->data.working = FALSE;
}
