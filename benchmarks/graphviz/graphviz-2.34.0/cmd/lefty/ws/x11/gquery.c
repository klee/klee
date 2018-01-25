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
#include "SF.h"

#define WQU widget->u.q

static void qwcallback (Widget, XtPointer, XtPointer);
static void qbwcallback (Widget, XtPointer, XtPointer);

int GQcreatewidget (
    Gwidget_t *parent, Gwidget_t *widget, int attrn, Gwattr_t *attrp
) {
    Widget w;
    int ai;

    WQU->mode = G_QWSTRING;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRMODE:
            if (strcmp ("string", attrp[ai].u.t) == 0)
                WQU->mode = G_QWSTRING;
            else if (strcmp ("file", attrp[ai].u.t) == 0)
                WQU->mode = G_QWFILE;
            else if (strcmp ("choice", attrp[ai].u.t) == 0)
                WQU->mode = G_QWCHOICE;
            else {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    switch (WQU->mode) {
    case G_QWSTRING:
        if (!(widget->w = XtCreatePopupShell (
            "popup", transientShellWidgetClass, Groot, NULL, 0
        ))) {
            Gerr (POS, G_ERRCANNOTCREATEWIDGET);
            return -1;
        }
        XtAddCallback (widget->w, XtNpopdownCallback, qwcallback, NULL);
        RESETARGS;
        ADD2ARGS (XtNlabel, "abcdefghijklmno");
        ADD2ARGS (XtNvalue, "");
        if (!(WQU->w = XtCreateManagedWidget (
            "dialog", dialogWidgetClass, widget->w, argp, argn
        ))) {
            Gerr (POS, G_ERRCANNOTCREATEWIDGET);
            return -1;
        }
        XawDialogAddButton (WQU->w, "Cancel", qbwcallback, (XtPointer) 1);
        XawDialogAddButton (WQU->w, "OK", qbwcallback, (XtPointer) 2);
        if (!(w = XtNameToWidget (WQU->w, "value"))) {
            Gerr (POS, G_ERRCANNOTCREATEWIDGET);
            return -1;
        }
        XtOverrideTranslations (w, Gqwpoptable);
        XtOverrideTranslations (widget->w, Gqwdeltable);
        break;
    case G_QWFILE:
        widget->w = 0;
        break;
    case G_QWCHOICE:
        if (!(widget->w = XtCreatePopupShell (
            "popup", transientShellWidgetClass, Groot, NULL, 0
        ))) {
            Gerr (POS, G_ERRCANNOTCREATEWIDGET);
            return -1;
        }
        XtAddCallback (widget->w, XtNpopdownCallback, qwcallback, NULL);
        RESETARGS;
        ADD2ARGS (XtNlabel, "abcdefghijklmno");
        if (!(WQU->w = XtCreateManagedWidget (
            "dialog", dialogWidgetClass, widget->w, argp, argn
        ))) {
            Gerr (POS, G_ERRCANNOTCREATEWIDGET);
            return -1;
        }
        XtOverrideTranslations (widget->w, Gqwdeltable);
        break;
    }
    return 0;
}

int GQsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    int ai;

    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRMODE:
            Gerr (POS, G_ERRCANNOTSETATTR2, "mode");
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

int GQgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    int ai;

    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRMODE:
            switch (WQU->mode) {
            case G_QWSTRING:
                attrp[ai].u.t = "string";
                break;
            case G_QWFILE:
                attrp[ai].u.t = "file";
                break;
            case G_QWCHOICE:
                attrp[ai].u.t = "choice";
                break;
            }
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

int GQdestroywidget (Gwidget_t *widget) {
    switch (WQU->mode) {
    case G_QWSTRING:
        XtDestroyWidget (widget->w);
        break;
    case G_QWFILE:
        break;
    case G_QWCHOICE:
        XtDestroyWidget (widget->w);
        break;
    }
    return 0;
}

int GQqueryask (
    Gwidget_t *widget, char *prompt, char *args,
    char *responsep, int responsen
) {
    Window rwin, cwin;
    Dimension width, height;
    int rx, ry, x, y;
    long i;
    unsigned int mask;
    Widget widgets[20];
    char buttons[20][40];
    char *s1, *s2;
    char c;

    switch (WQU->mode) {
    case G_QWSTRING:
        RESETARGS;
        ADD2ARGS (XtNlabel, prompt);
        ADD2ARGS (XtNvalue, args ? args : "");
        XtSetValues (WQU->w, argp, argn);
        XtRealizeWidget (widget->w);
        XSetWMProtocols (Gdisplay, XtWindow (widget->w), &Gqwdelatom, 1);
        XQueryPointer (
            Gdisplay, XtWindow (widget->w),
            &rwin, &cwin, &rx, &ry, &x, &y, &mask
        );
        RESETARGS;
        ADD2ARGS (XtNwidth, &width);
        ADD2ARGS (XtNheight, &height);
        XtGetValues (widget->w, argp, argn);
        rx -= (width / 2), ry -= (height / 2);
        if (rx + width > DisplayWidth (Gdisplay, Gscreenn))
            rx = DisplayWidth (Gdisplay, Gscreenn) - width;
        if (ry + height > DisplayHeight (Gdisplay, Gscreenn))
            ry = DisplayHeight (Gdisplay, Gscreenn) - height;
        if (rx < 0)
            rx = 0;
        if (ry < 0)
            ry = 0;
        RESETARGS;
        ADD2ARGS (XtNx, rx);
        ADD2ARGS (XtNy, ry);
        XtSetValues (widget->w, argp, argn);
        WQU->state = 2;
        WQU->button = 0;
        XtPopup (widget->w, XtGrabExclusive);
        while (WQU->state) {
            if (WQU->state == 1) {
                if (WQU->button != 1) {
                    strncpy (
                        responsep, XawDialogGetValueString (WQU->w), responsen
                    );
                }
                XtPopdown (widget->w);
            }
            Gprocessevents (TRUE, G_ONEEVENT);
        }
        XtUnrealizeWidget (widget->w);
        Gpopdownflag = TRUE;
        if (WQU->button == 1)
            return -1;
        break;
    case G_QWFILE:
        if (!XsraSelFile (
            Groot, prompt, "OK", "Cancel", "FAIL",
            (args ? args : "/"), "r", NULL, responsep, responsen
        )) {
            Gpopdownflag = TRUE;
            return -1;
        }
        Gpopdownflag = TRUE;
        break;
    case G_QWCHOICE:
        if (!args)
            return -1;
        RESETARGS;
        ADD2ARGS (XtNlabel, prompt);
        XtSetValues (WQU->w, argp, argn);
        for (s1 = args, i = 1; *s1; i++) {
            s2 = s1;
            while (*s2 && *s2 != '|')
                s2++;
            c = *s2, *s2 = 0;
            strcpy (buttons[i], s1);
            widgets[i] = XtCreateManagedWidget (
                s1, commandWidgetClass, WQU->w, NULL, 0
            );
            XtAddCallback (widgets[i], XtNcallback, qbwcallback, (XtPointer) i);
            *s2 = c;
            s1 = s2;
            if (*s1)
                s1++;
        }
        XtRealizeWidget (widget->w);
        XSetWMProtocols (Gdisplay, XtWindow (widget->w), &Gqwdelatom, 1);
        XQueryPointer (
            Gdisplay, XtWindow (widget->w),
            &rwin, &cwin, &rx, &ry, &x, &y, &mask
        );
        RESETARGS;
        ADD2ARGS (XtNwidth, &width);
        ADD2ARGS (XtNheight, &height);
        XtGetValues (widget->w, argp, argn);
        rx -= (width / 2), ry -= (height / 2);
        if (rx + width > DisplayWidth (Gdisplay, Gscreenn))
            rx = DisplayWidth (Gdisplay, Gscreenn) - width;
        if (ry + height > DisplayHeight (Gdisplay, Gscreenn))
            ry = DisplayHeight (Gdisplay, Gscreenn) - height;
        if (rx < 0)
            rx = 0;
        if (ry < 0)
            ry = 0;
        RESETARGS;
        ADD2ARGS (XtNx, rx);
        ADD2ARGS (XtNy, ry);
        XtSetValues (widget->w, argp, argn);
        WQU->state = 2;
        WQU->button = 0;
        XtPopup (widget->w, XtGrabExclusive);
        while (WQU->state) {
            if (WQU->state == 1) {
                if (WQU->button > 0)
                    strncpy (responsep, buttons[WQU->button], responsen);
                XtPopdown (widget->w);
            }
            Gprocessevents (TRUE, G_ONEEVENT);
        }
        XtUnrealizeWidget (widget->w);
        for (i--; i > 0; i--)
            XtDestroyWidget (widgets[i]);
        Gpopdownflag = TRUE;
        break;
    }
    if (responsep[0] && responsep[strlen(responsep) - 1] == '\n')
        responsep[strlen(responsep) - 1] = 0;
    return 0;
}

void Gqwpopaction (Widget w, XEvent *evp, char **app, unsigned int *anp) {
    Gwidget_t *widget;
    char c;

    if (evp->type == KeyPress || evp->type == KeyRelease)
        XLookupString ((XKeyEvent *) evp, &c, 1, NULL, NULL);
    if (c != 13)
        return;
    widget = findwidget (
        (unsigned long) XtParent (XtParent (w)), G_QUERYWIDGET
    );
    WQU->state = 1;
}

void Gqwdelaction (Widget w, XEvent *evp, char **app, unsigned int *anp) {
    Gwidget_t *widget;

    widget = findwidget ((unsigned long) w, G_QUERYWIDGET);
    WQU->state = 1;
}

static void qwcallback (Widget w, XtPointer clientdata, XtPointer calldata) {
    Gwidget_t *widget;

    widget = findwidget ((unsigned long) w, G_QUERYWIDGET);
    WQU->state = 0;
}

static void qbwcallback (Widget w, XtPointer clientdata, XtPointer calldata) {
    Gwidget_t *widget;

    widget = findwidget (
        (unsigned long) XtParent (XtParent (w)), G_QUERYWIDGET
    );
    WQU->button = (long) clientdata;
    WQU->state = 1;
}
