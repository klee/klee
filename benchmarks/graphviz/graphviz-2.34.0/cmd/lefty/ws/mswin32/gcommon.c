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

#define WCU widget->u.c
#define WVU widget->u.v

FILE *Gxfp;
int Gxfd;
int Gpopdownflag;
int Gdepth;
int Gnocallbacks;
int menuselected;
int menupoped;

char *Gbufp = NULL;
int Gbufn = 0, Gbufi = 0;

PIXpoint_t *Gppp;
int Gppn, Gppi;

Gfont_t *Gfontp;
int Gfontn;

static HFONT deffont;
static int twobmouse;
static HWND palettechanged;

LRESULT CALLBACK LeftyWndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ArrayWndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CanvasWndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LabelWndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ScrollWndProc (HWND, UINT, WPARAM, LPARAM);

static void processcommand (Gwidget_t *, WPARAM, LPARAM);
static void handleresize (Gwidget_t *);

int Ginitgraphics (void) {
    WNDCLASS wc;
    HDC hdc;
    ATOM rtn;

    if (!hprevinstance) {
        wc.style = NULL;
        wc.lpfnWndProc = LeftyWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hinstance;
        wc.hIcon = LoadIcon ((HINSTANCE) NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor ((HINSTANCE) NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject (WHITE_BRUSH);
        wc.lpszMenuName = 0;
        wc.lpszClassName = "LeftyClass";
        if (!(rtn = RegisterClass (&wc)))
            panic1 (POS, "GXinit", "register class rtn = %d", (int) rtn);

        wc.style = NULL;
        wc.lpfnWndProc = ArrayWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hinstance;
        wc.hIcon = LoadIcon ((HINSTANCE) NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor ((HINSTANCE) NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject (WHITE_BRUSH);
        wc.lpszMenuName = 0;
        wc.lpszClassName = "ArrayClass";
        if (!(rtn = RegisterClass (&wc)))
            panic1 (POS, "GXinit", "register class rtn = %d", (int) rtn);

        wc.style = CS_OWNDC;
        wc.lpfnWndProc = CanvasWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hinstance;
        wc.hIcon = LoadIcon ((HINSTANCE) NULL, IDI_APPLICATION);
        wc.hCursor = NULL;
        wc.hbrBackground = GetStockObject (WHITE_BRUSH);
        wc.lpszMenuName = 0;
        wc.lpszClassName = "CanvasClass";
        if (!(rtn = RegisterClass (&wc)))
            panic1 (POS, "GXinit", "register class rtn = %d", (int) rtn);

        wc.style = NULL;
        wc.lpfnWndProc = ScrollWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hinstance;
        wc.hIcon = LoadIcon ((HINSTANCE) NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor ((HINSTANCE) NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject (WHITE_BRUSH);
        wc.lpszMenuName = 0;
        wc.lpszClassName = "ScrollClass";
        if (!(rtn = RegisterClass (&wc)))
            panic1 (POS, "GXinit", "register class rtn = %d", (int) rtn);

        wc.style = NULL;
        wc.lpfnWndProc = LabelWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hinstance;
        wc.hIcon = LoadIcon ((HINSTANCE) NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor ((HINSTANCE) NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject (WHITE_BRUSH);
        wc.lpszMenuName = 0;
        wc.lpszClassName = "LabelClass";
        if (!(rtn = RegisterClass (&wc)))
            panic1 (POS, "GXinit", "register class rtn = %d", (int) rtn);
    }
    if (getenv ("LEFTY3BMOUSE"))
        twobmouse = FALSE;
    else
        twobmouse = TRUE;
    hdc = GetDC ((HWND) NULL);
    Gdepth = GetDeviceCaps (hdc, BITSPIXEL);
    deffont = GetStockObject (SYSTEM_FONT);
#ifndef FEATURE_MS
    if (!(Gxfp = fopen ("/dev/windows", "r")))
        panic1 (POS, "GXinit", "cannot open windows device");
    Gxfd = fileno (Gxfp);
#endif
    Gpopdownflag = FALSE;
    Gbufp = Marrayalloc ((long) BUFINCR * BUFSIZE);
    Gbufn = BUFINCR;
    Gppp = Marrayalloc ((long) PPINCR * PPSIZE);
    Gppn = PPINCR;
    Gfontp = Marrayalloc ((long) FONTSIZE);
    Gfontn = 1;
    Gfontp[0].name = strdup ("default");
    if (!Gdefaultfont)
        Gfontp[0].font = deffont;
    else if (Gdefaultfont[0] != '\000')
        Gfontp[0].font = CreateFont (
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Gdefaultfont
        );
    else
        Gfontp[0].font = NULL;
    ReleaseDC ((HWND) NULL, hdc);
    Gnocallbacks = FALSE;
    return 0;
}

int Gtermgraphics (void) {
    int fi;

    for (fi = 0; fi < Gfontn; fi++)
        free (Gfontp[fi].name);
    Marrayfree (Gfontp), Gfontp = NULL, Gfontn = 0;
    Marrayfree (Gppp), Gppp = NULL, Gppn = 0;
    Marrayfree (Gbufp), Gbufp = NULL, Gbufn = 0;
    return 0;
}

int Gsync (void) {
    return 0;
}

int Gresetbstate (int wi) {
    Gcw_t *cw;
    int bn;

    cw = Gwidgets[wi].u.c;
    bn = cw->bstate[0] + cw->bstate[1] + cw->bstate[2];
    cw->bstate[0] = cw->bstate[1] = cw->bstate[2] = 0;
    cw->buttonsdown -= bn;
    Gbuttonsdown -= bn;
    return 0;
}

int Gprocessevents (int waitflag, int mode) {
    MSG msg;
    int rtn;

    rtn = 0;
    switch (waitflag) {
    case TRUE:
        if (!GetMessage(&msg, (HWND) NULL, (UINT) NULL, (UINT) NULL))
            exit (msg.wParam);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (mode == G_ONEEVENT)
            return 1;
        rtn = 1;
        /* FALL THROUGH */
    case FALSE:
        while (PeekMessage(&msg, (HWND) 0, (UINT) 0, (UINT) 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                exit (msg.wParam);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (mode == G_ONEEVENT)
                return 1;
            rtn = 1;
        }
        break;
    }
    return rtn;
}

LRESULT CALLBACK LeftyWndProc (
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam
) {
    Gwidget_t *widget;
    WINDOWPOS *wpos;
    Gevent_t gev;

    widget = findwidget (hwnd, G_VIEWWIDGET);
    switch (message) {
    case WM_WINDOWPOSCHANGED:
        if (Gnocallbacks || !widget)
            return (DefWindowProc(hwnd, message, wparam, lparam));
        wpos = (WINDOWPOS *) lparam;
        if (!(wpos->flags & SWP_NOSIZE))
            handleresize (widget);
        break;
    case WM_COMMAND:
        if (Gnocallbacks || !widget)
            return (DefWindowProc(hwnd, message, wparam, lparam));
        processcommand (widget, wparam, lparam);
        break;
    case WM_CLOSE:
        if (!widget)
            exit (0);
        if (WVU->closing)
            DestroyWindow (hwnd);
        if (Gnocallbacks)
            exit (0);
        gev.type = 0, gev.code = 0, gev.data = 0;
        gev.wi = widget - &Gwidgets[0];
        if (WVU->func)
            (*WVU->func) (&gev);
        else
            exit (0);
        break;
    case WM_PALETTECHANGED:
        palettechanged = (HWND) wparam;
        break;
    default:
        return (DefWindowProc(hwnd, message, wparam, lparam));
    }
    return 0;
}

LRESULT CALLBACK ArrayWndProc (
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam
) {
    Gwidget_t *widget;
    WINDOWPOS *wpos;

    if (Gnocallbacks || !(widget = findwidget (hwnd, G_ARRAYWIDGET)))
        return (DefWindowProc(hwnd, message, wparam, lparam));
    switch (message) {
    case WM_WINDOWPOSCHANGED:
        wpos = (WINDOWPOS *) lparam;
        if (!(wpos->flags & SWP_NOSIZE))
            handleresize (widget);
        break;
    case WM_COMMAND:
        processcommand (widget, wparam, lparam);
        break;
    default:
        return (DefWindowProc (hwnd, message, wparam, lparam));
    }
    return 0;
}

LRESULT CALLBACK CanvasWndProc (
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam
) {
    Gwidget_t *widget;
    WINDOWPOS *wpos;
    PIXpoint_t pp;
    Gevent_t gev;
    POINT p;
    int wi, bn;

    static int cntlflag;

    if (Gnocallbacks || !(widget = findwidget (hwnd, G_CANVASWIDGET)))
        return (DefWindowProc(hwnd, message, wparam, lparam));
    Gpopdownflag = FALSE;
    switch (message) {
    case WM_PAINT:
        if (palettechanged != hwnd)
            RealizePalette (widget->u.c->gc);
        Gneedredraw = widget->u.c->needredraw = TRUE;
        Gadjustclip (widget);
        return (DefWindowProc(hwnd, message, wparam, lparam));
    case WM_WINDOWPOSCHANGED:
        wpos = (WINDOWPOS *) lparam;
        if (!(wpos->flags & SWP_NOSIZE))
            handleresize (widget);
        return 0;
    case WM_MOUSEACTIVATE:
        SetFocus (widget->w);
        return (DefWindowProc(hwnd, message, wparam, lparam));
    case WM_COMMAND:
        processcommand (widget, wparam, lparam);
        return 0;
    case WM_CHAR:
        gev.type = G_KEYBD;
        gev.code = G_DOWN;   /* I don't know how to get up events so I make */
        Gpopdownflag = TRUE; /* the code after this switch send the up event */
        gev.data = wparam;
        GetCursorPos (&p);
        ScreenToClient (widget->w, &p);
        pp.x = p.x, pp.y = p.y;
        gev.p = ppixtodraw (widget, pp);
        /* continues after the end of this switch */
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        gev.type = G_MOUSE;
        if (twobmouse) {
            if (message == WM_LBUTTONDOWN && (wparam & MK_CONTROL))
                message = WM_MBUTTONDOWN, cntlflag = TRUE;
            if (message == WM_LBUTTONUP && cntlflag)
                message = WM_MBUTTONUP, cntlflag = FALSE;
        }
        switch (message) {
        case WM_LBUTTONDOWN: gev.code = G_DOWN, gev.data = G_LEFT;   break;
        case WM_LBUTTONUP:   gev.code = G_UP,   gev.data = G_LEFT;   break;
        case WM_MBUTTONDOWN: gev.code = G_DOWN, gev.data = G_MIDDLE; break;
        case WM_MBUTTONUP:   gev.code = G_UP,   gev.data = G_MIDDLE; break;
        case WM_RBUTTONDOWN: gev.code = G_DOWN, gev.data = G_RIGHT;  break;
        case WM_RBUTTONUP:   gev.code = G_UP,   gev.data = G_RIGHT;  break;
        }
        pp.x = LOWORD (lparam), pp.y = HIWORD (lparam);
        gev.p = ppixtodraw (widget, pp);
        bn = WCU->bstate[gev.data];
        WCU->bstate[gev.data] = (gev.code == G_DOWN) ? 1 : 0;
        bn = WCU->bstate[gev.data] - bn;
        widget->u.c->buttonsdown += bn;
        Gbuttonsdown += bn;
        /* continues after the end of this switch */
        break;
    default:
        return (DefWindowProc(hwnd, message, wparam, lparam));
    }
    wi = gev.wi = widget - &Gwidgets[0];
    if (widget->u.c->func)
        (*widget->u.c->func) (&gev);
    if (Gpopdownflag) {
        Gpopdownflag = FALSE;
        if (gev.code == G_DOWN) {
            gev.code = G_UP;
            widget = &Gwidgets[wi];
            WCU->bstate[gev.data] = 0;
            widget->u.c->buttonsdown--;
            Gbuttonsdown--;
            if (widget->inuse && widget->u.c->func)
                (*widget->u.c->func) (&gev);
        }
    }
    return 0;
}

LRESULT CALLBACK LabelWndProc (
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam
) {
    Gwidget_t *widget;
    PAINTSTRUCT paintstruct;
    WINDOWPOS *wpos;
    Gevent_t gev;
    RECT r;
    HDC hdc;
    int wi;

    if (Gnocallbacks || !(widget = findwidget (hwnd, G_LABELWIDGET)))
        return (DefWindowProc(hwnd, message, wparam, lparam));
    switch (message) {
    case WM_PAINT:
        hdc = BeginPaint (widget->w, &paintstruct);
        GetWindowText (widget->w, &Gbufp[0], Gbufn);
        GetClientRect (widget->w, &r);
        DrawText (hdc, (LPCSTR) &Gbufp[0], strlen (Gbufp), &r, DT_LEFT);
        EndPaint (widget->w, &paintstruct);
        return (DefWindowProc(hwnd, message, wparam, lparam));
    case WM_WINDOWPOSCHANGED:
        wpos = (WINDOWPOS *) lparam;
        if (!(wpos->flags & SWP_NOSIZE))
            handleresize (widget);
        return 0;
    case WM_COMMAND:
        processcommand (widget, wparam, lparam);
        return 0;
    case WM_KEYDOWN:
    case WM_KEYUP:
        gev.type = G_KEYBD;
        gev.code = (message == WM_KEYDOWN) ? G_DOWN : G_UP;
        gev.data = wparam;
        /* continues after the end of this switch */
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        gev.type = G_MOUSE;
        if (wparam & MK_CONTROL) {
            if (message == WM_LBUTTONDOWN)
                message = WM_MBUTTONDOWN;
            else if (message == WM_LBUTTONUP)
                message = WM_MBUTTONUP;
        }
        switch (message) {
        case WM_LBUTTONDOWN: gev.code = G_DOWN, gev.data = G_LEFT;   break;
        case WM_LBUTTONUP:   gev.code = G_UP,   gev.data = G_LEFT;   break;
        case WM_MBUTTONDOWN: gev.code = G_DOWN, gev.data = G_MIDDLE; break;
        case WM_MBUTTONUP:   gev.code = G_UP,   gev.data = G_MIDDLE; break;
        case WM_RBUTTONDOWN: gev.code = G_DOWN, gev.data = G_RIGHT;  break;
        case WM_RBUTTONUP:   gev.code = G_UP,   gev.data = G_RIGHT;  break;
        }
        /* continues after the end of this switch */
        break;
    default:
        return (DefWindowProc(hwnd, message, wparam, lparam));
    }
    wi = gev.wi = widget - &Gwidgets[0];
    if (widget->u.l->func)
        (*widget->u.l->func) (&gev);
    if (Gpopdownflag) {
        Gpopdownflag = FALSE;
        if (gev.type == G_MOUSE && gev.code == G_DOWN) {
            gev.code = G_UP;
            widget = &Gwidgets[wi];
            if (widget->inuse && widget->u.l->func)
                (*widget->u.l->func) (&gev);
        }
    }
    return 0;
}

LRESULT CALLBACK ScrollWndProc (
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam
) {
    Gwidget_t *widget, *child;
    WINDOWPOS *wpos;
    PIXpoint_t po;
    RECT r;
    int dummy, dx, dy, wi;

    if (Gnocallbacks || !(widget = findwidget (hwnd, G_SCROLLWIDGET)))
        return (DefWindowProc(hwnd, message, wparam, lparam));
    switch (message) {
    case WM_WINDOWPOSCHANGED:
        wpos = (WINDOWPOS *) lparam;
        if (!(wpos->flags & SWP_NOSIZE))
            handleresize (widget);
        break;
    case WM_HSCROLL:
    case WM_VSCROLL:
        for (wi = 0; wi < Gwidgetn; wi++) {
            child = &Gwidgets[wi];
            if (child->inuse && child->pwi == widget - &Gwidgets[0])
                break;
        }
        if (wi == Gwidgetn)
            return (DefWindowProc(hwnd, message, wparam, lparam));
        GetClientRect (widget->w, &r);
        GetScrollRange (widget->w, SB_HORZ, &dummy, &dx);
        GetScrollRange (widget->w, SB_VERT, &dummy, &dy);
        po.x = GetScrollPos (widget->w, SB_HORZ);
        po.y = GetScrollPos (widget->w, SB_VERT);
        switch (message) {
        case WM_HSCROLL:
            switch (LOWORD (wparam)) {
            case SB_BOTTOM:        po.x = dx;                  break;
            case SB_LINEDOWN:      po.x += 10;                 break;
            case SB_LINEUP:        po.x -= 10;                 break;
            case SB_PAGEDOWN:      po.x += (r.right - r.left); break;
            case SB_PAGEUP:        po.x -= (r.right - r.left); break;
            case SB_THUMBPOSITION: po.x = HIWORD (wparam);     break;
            case SB_THUMBTRACK:    po.x = HIWORD (wparam);     break;
            case SB_TOP:           po.x = 0;                   break;
            }
            po.x = min (po.x, dx);
            po.x = max (po.x, 0);
            SetScrollPos (widget->w, SB_HORZ, po.x, TRUE);
            SetWindowPos (
                child->w, (HWND) NULL, -po.x, -po.y, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER
            );
            break;
        case WM_VSCROLL:
            switch (LOWORD (wparam)) {
            case SB_BOTTOM:        po.y = dy;                  break;
            case SB_LINEDOWN:      po.y += 10;                 break;
            case SB_LINEUP:        po.y -= 10;                 break;
            case SB_PAGEDOWN:      po.y += (r.bottom - r.top); break;
            case SB_PAGEUP:        po.y -= (r.bottom - r.top); break;
            case SB_THUMBPOSITION: po.y = HIWORD (wparam);     break;
            case SB_THUMBTRACK:    po.y = HIWORD (wparam);     break;
            case SB_TOP:           po.y = 0;                   break;
            }
            po.y = min (po.y, dy);
            po.y = max (po.y, 0);
            SetScrollPos (widget->w, SB_VERT, po.y, TRUE);
            SetWindowPos (
                child->w, (HWND) NULL, -po.x, -po.y, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER
            );
            break;
        }
        break;
    default:
        return (DefWindowProc (hwnd, message, wparam, lparam));
    }
    return 0;
}

static void processcommand (Gwidget_t *widget, WPARAM wparam, LPARAM lparam) {
    Gwidget_t *child;
    WORD l;
    int n;

    if (lparam == 0) { /* it's a menu */
        if (LOWORD (wparam) != 999)
            menuselected = LOWORD (wparam);
        menupoped = FALSE;
        return;
    }
    if (!(LOWORD (wparam) > 0 && LOWORD (wparam) < Gwidgetn))
        return;
    child = &Gwidgets[LOWORD (wparam)];
    if (!child->inuse)
        return;

    switch (child->type) {
    case G_TEXTWIDGET:
        if (HIWORD (wparam) == EN_CHANGE) { /* it's a text widget message */
            if ((n = SendMessage (child->w, EM_GETLINECOUNT, 0, 0L)) < 2)
                return;
            *((WORD *) &Gbufp[0]) = Gbufn - 1;
            l = SendMessage (
                child->w, EM_GETLINE, n - 1, (LPARAM) (LPSTR) &Gbufp[0]
            );
            if (l != 0)
                return; /* no carriage return yet */
            *((WORD *) &Gbufp[0]) = Gbufn - 1;
            l = SendMessage (
                child->w, EM_GETLINE, n - 2, (LPARAM) (LPSTR) &Gbufp[0]
            );
            Gbufp[l] = 0;
            if (l > 0 && child->u.t->func)
                (*child->u.t->func) (child - &Gwidgets[0], &Gbufp[0]);
        }
        break;
    case G_BUTTONWIDGET:
        if (child->u.b->func)
            (*child->u.b->func) (child - &Gwidgets[0], child->udata);
        break;
    }
}

void Gadjustwrect (Gwidget_t *parent, PIXsize_t *psp) {
    RECT r;

    GetClientRect (parent->w, &r);
    switch (parent->type) {
    case G_ARRAYWIDGET:
        if (parent->u.a->data.type == G_AWHARRAY)
            psp->y = r.bottom - r.top;
        else
            psp->x = r.right - r.left;
        break;
    case G_SCROLLWIDGET:
        psp->x = max (psp->x, r.right - r.left);
        psp->y = max (psp->y, r.bottom - r.top);
        break;
    case G_VIEWWIDGET:
    case G_QUERYWIDGET:
        psp->x = r.right - r.left;
        psp->y = r.bottom - r.top;
        break;
    }
}

static void handleresize (Gwidget_t *widget) {
    Gwidget_t *parent, *child;
    PIXsize_t ps1, ps2;
    PIXpoint_t po;
    DWORD wflags1, wflags2;
    RECT r;
    int dx, dy, wi, i;

    wflags1 = SWP_NOMOVE | SWP_NOZORDER;
    wflags2 = SWP_NOSIZE | SWP_NOZORDER;
    GetWindowRect (widget->w, &r);
    ps1.x = r.right - r.left, ps1.y = r.bottom - r.top;
    ps2 = ps1;
    /* first, take care of parent */
    parent = (widget->pwi == -1) ? NULL : &Gwidgets[widget->pwi];
    if (!parent)
        goto handlechildren;
    switch (parent->type) {
    case G_VIEWWIDGET:
        Gadjustwrect (parent, &ps1);
        if (ps1.x != ps2.x || ps1.y != ps2.y) {
            Gnocallbacks = TRUE;
            SetWindowPos (widget->w, (HWND) NULL, 0, 0, ps1.x, ps1.y, wflags1);
            Gnocallbacks = FALSE;
        }
        break;
    case G_ARRAYWIDGET:
        Gnocallbacks = TRUE;
        Gawresize (parent);
        Gnocallbacks = FALSE;
        break;
    case G_SCROLLWIDGET:
        Gnocallbacks = TRUE;
        for (i = 0; i  < 2; i++) {
            Gadjustwrect (parent, &ps1);
            if (ps1.x > ps2.x || ps1.y > ps2.y)
                SetWindowPos (
                    widget->w, (HWND) NULL, 0, 0, ps1.x, ps1.y, wflags1
                );
            GetClientRect (parent->w, &r);
            ps2.x = r.right - r.left, ps2.y = r.bottom - r.top;
            dx = max (0, ps1.x - ps2.x);
            dy = max (0, ps1.y - ps2.y);
            SetScrollRange (parent->w, SB_HORZ, 0, dx, TRUE);
            SetScrollRange (parent->w, SB_VERT, 0, dy, TRUE);
            po.x = GetScrollPos (parent->w, SB_HORZ);
            po.y = GetScrollPos (parent->w, SB_VERT);
            po.x = min (po.x, dx);
            po.x = max (po.x, 0);
            SetScrollPos (parent->w, SB_HORZ, po.x, TRUE);
            po.y = min (po.y, dy);
            po.y = max (po.y, 0);
            SetScrollPos (parent->w, SB_VERT, po.y, TRUE);
            SetWindowPos (widget->w, (HWND) NULL, -po.x, -po.y, 0, 0, wflags2);
            ps2 = ps1;
        }
        Gnocallbacks = FALSE;
        break;
    }

handlechildren:
    for (wi = 0; wi < Gwidgetn; wi++) {
        child = &Gwidgets[wi];
        if (child->inuse && child->pwi == widget - &Gwidgets[0])
            break;
    }
    if (wi == Gwidgetn)
        return;
    GetWindowRect (child->w, &r);
    ps1.x = r.right - r.left, ps1.y = r.bottom - r.top;
    ps2 = ps1;
    switch (widget->type) {
    case G_VIEWWIDGET:
        Gadjustwrect (widget, &ps1);
        if (ps1.x != ps2.x || ps1.y != ps2.y)
            SetWindowPos (child->w, (HWND) NULL, 0, 0, ps1.x, ps1.y, wflags1);
        break;
    case G_ARRAYWIDGET:
        Gawresize (widget);
        break;
    case G_SCROLLWIDGET:
        Gadjustwrect (widget, &ps1);
        if (ps1.x > ps2.x || ps1.y > ps2.y)
            SetWindowPos (child->w, (HWND) NULL, 0, 0, ps1.x, ps1.y, wflags1);
        GetClientRect (widget->w, &r);
        ps2.x = r.right - r.left, ps2.y = r.bottom - r.top;
        dx = max (0, ps1.x - ps2.x);
        dy = max (0, ps1.y - ps2.y);
        SetScrollRange (widget->w, SB_HORZ, 0, dx, TRUE);
        SetScrollRange (widget->w, SB_VERT, 0, dy, TRUE);
        po.x = GetScrollPos (widget->w, SB_HORZ);
        po.y = GetScrollPos (widget->w, SB_VERT);
        po.x = min (po.x, dx);
        po.x = max (po.x, 0);
        SetScrollPos (widget->w, SB_HORZ, po.x, TRUE);
        po.y = min (po.y, dy);
        po.y = max (po.y, 0);
        SetScrollPos (widget->w, SB_VERT, po.y, TRUE);
        SetWindowPos (child->w, (HWND) NULL, -po.x, -po.y, 0, 0, wflags2);
        break;
    }
}
