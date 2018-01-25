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
    char *s;
    int ai;
    XColor c;
    int color;

    static XawTextSelectType sarray[] = {
        XawselectLine, XawselectNull
    };

    if (!parent) {
        Gerr (POS, G_ERRNOPARENTWIDGET);
        return -1;
    }
    WTU->func = NULL;
    ps.x = ps.y = MINTWSIZE;
    s = "oneline";
    RESETARGS;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINTWSIZE);
            break;
        case G_ATTRBORDERWIDTH:
            ADD2ARGS (XtNborderWidth, attrp[ai].u.i);
            break;
        case G_ATTRTEXT:
            ADD2ARGS (XtNstring, attrp[ai].u.t);
            break;
        case G_ATTRAPPENDTEXT:
            Gerr (POS, G_ERRCANNOTSETATTR1, "appendtext");
            return -1;
        case G_ATTRMODE:
            s = attrp[ai].u.t;
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
        case G_ATTRNEWLINECB:
            WTU->func = (Gtwnewlinecb) attrp[ai].u.func;
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
    if (strcmp ("oneline", s) == 0)
        ADD2ARGS (XtNeditType, XawtextAppend);
    else if (strcmp ("input", s) == 0 || strcmp ("select", s) == 0)
        ADD2ARGS (XtNeditType, XawtextEdit);
    else if (strcmp ("output", s) == 0)
        ADD2ARGS (XtNeditType, XawtextRead);
    else {
        Gerr (POS, G_ERRBADATTRVALUE, s);
        return -1;
    }
    ADD2ARGS (XtNscrollHorizontal, XawtextScrollWhenNeeded);
    ADD2ARGS (XtNscrollVertical, XawtextScrollWhenNeeded);
    if (!(widget->w = XtCreateWidget (
        "ascii", asciiTextWidgetClass, parent->w, argp, argn
    ))) {
        Gerr (POS, G_ERRCANNOTCREATEWIDGET);
        return -1;
    }
    if (strcmp ("oneline", s) == 0 || strcmp ("input", s) == 0)
        XtOverrideTranslations (widget->w, Gtweoltable);
    else if (strcmp ("select", s) == 0)
        XawTextSetSelectionArray (widget->w, sarray);
    Glazymanage (widget->w);
    return 0;
}

int GTsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    PIXsize_t ps;
    XawTextBlock tb;
    int ai, li;
    XColor c;
    int color;

    RESETARGS;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINTWSIZE);
            ADD2ARGS (XtNwidth, ps.x);
            ADD2ARGS (XtNheight, ps.y);
            break;
        case G_ATTRBORDERWIDTH:
            ADD2ARGS (XtNborderWidth, attrp[ai].u.i);
            break;
        case G_ATTRTEXT:
            ADD2ARGS (XtNstring, attrp[ai].u.t);
            break;
        case G_ATTRAPPENDTEXT:
            XawTextSetInsertionPoint (widget->w, 327670000);
            li = XawTextGetInsertionPoint (widget->w);
            tb.firstPos = 0, tb.length = strlen (attrp[ai].u.t);
            tb.ptr = attrp[ai].u.t, tb.format = FMT8BIT;
            XawTextReplace (widget->w, li, li, &tb);
            li = XawTextGetInsertionPoint (widget->w);
            tb.firstPos = 0, tb.length = 1;
            tb.ptr = "\n", tb.format = FMT8BIT;
            XawTextReplace (widget->w, li, li, &tb);
            break;
        case G_ATTRMODE:
            if (strcmp ("oneline", attrp[ai].u.t) == 0)
                ADD2ARGS (XtNeditType, XawtextAppend);
            else if (strcmp ("input", attrp[ai].u.t) == 0)
                ADD2ARGS (XtNeditType, XawtextEdit);
            else if (strcmp ("output", attrp[ai].u.t) == 0)
                ADD2ARGS (XtNeditType, XawtextRead);
            else {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
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
        case G_ATTRNEWLINECB:
            WTU->func = (Gtwnewlinecb) attrp[ai].u.func;
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

int GTgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    Dimension width, height;
    XawTextEditType mode;
    XawTextBlock tb;
    Widget w;
    int rtn, ai;
    long fi, li;

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
            w = XawTextGetSource (widget->w);
            tb.firstPos = 0, tb.ptr = NULL, tb.format = FMT8BIT, tb.length = 0;
            rtn = XawTextSourceRead (w, 0, &tb, 30000);
            if (rtn > Gbufn + 1) {
                Gbufp = Marraygrow (Gbufp, (long) (rtn + 1) * BUFSIZE);
                Gbufn = rtn + 1;
            }
            for (Gbufi = 0; Gbufi < rtn; Gbufi++)
                Gbufp[Gbufi] = tb.ptr[Gbufi];
            Gbufp[Gbufi++] = '\000';
            attrp[ai].u.t = &Gbufp[0];
            break;
        case G_ATTRAPPENDTEXT:
            Gerr (POS, G_ERRCANNOTGETATTR, "appendtext");
            return -1;
        case G_ATTRSELECTION:
            w = XawTextGetSource (widget->w);
            XawTextGetSelectionPos (widget->w, &fi, &li);
            tb.firstPos = 0, tb.ptr = NULL, tb.format = FMT8BIT, tb.length = 0;
            rtn = XawTextSourceRead (w, fi, &tb, li - fi);
            if (li - fi > Gbufn + 1) {
                Gbufp = Marraygrow (Gbufp, (long) (rtn + 1) * BUFSIZE);
                Gbufn = rtn + 1;
            }
            for (Gbufi = 0; Gbufi < li - fi; Gbufi++)
                Gbufp[Gbufi] = tb.ptr[Gbufi];
            Gbufp[Gbufi++] = '\000';
            attrp[ai].u.t = &Gbufp[0];
            break;
        case G_ATTRMODE:
            ADD2ARGS (XtNeditType, &mode);
            XtGetValues (widget->w, argp, argn);
            if (mode == XawtextAppend)
                attrp[ai].u.t = "oneline";
            else if (mode == XawtextEdit)
                attrp[ai].u.t = "input";
            else if (mode == XawtextRead)
                attrp[ai].u.t = "output";
            else {
                panic1 (POS, "GTgetwidgetattr", "unexpected text mode");
                return -1;
            }
            break;
        case G_ATTRWINDOWID:
            sprintf (&Gbufp[0], "0x%lx", XtWindow (widget->w));
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
    XtDestroyWidget (widget->w);
    return 0;
}

/* used for one line text input widgets */
void Gtweolaction (Widget w, XEvent *evp, char **app, unsigned int *anp) {
    Gwidget_t *widget;
    Widget ww;
    XawTextBlock tb;
    int ret, fi, li, n, i;

    widget = findwidget ((unsigned long) w, G_TEXTWIDGET);
    li = XawTextGetInsertionPoint (w) - 1;
    ww = XawTextGetSource (w);
    tb.firstPos = 0, tb.ptr = "\n", tb.format = FMT8BIT, tb.length = 1;
    fi = XawTextSourceSearch (ww, li, XawsdLeft, &tb);
    if (fi == XawTextSearchError)
        fi = 0;
    else
        fi++;
    n = li - fi;
    Gbufp[(Gbufi = 0)] = '\000';
    while (Gbufi != n) {
        ret = XawTextSourceRead (ww, fi, &tb, n - Gbufi) - fi;
        for (i = 0; i < ret; i++)
            Gbufp[Gbufi++] = tb.ptr[i];
    }
    Gbufp[Gbufi] = '\000';
    if (n >= 0 && WTU->func)
        (*WTU->func) (widget - &Gwidgets[0], Gbufp);
}
