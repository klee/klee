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

int Gxfd;
Widget Groot;
Display *Gdisplay;
int Gpopdownflag;
int Gscreenn;
int Gdepth;
Glazyq_t Glazyq;

PIXpoint_t *Gppp;
int Gppn, Gppi;

char *Gbufp = NULL;
int Gbufn = 0, Gbufi = 0;

Gfont_t *Gfontp;
int Gfontn;

/* Xt[GS]etValues arguments */
Arg argp[MAXARGS];
int argn;

/* action and translation tables */
static XtActionsRec actiontable[] = {
    { "cwbut", Gcwbutaction },
    { "cwkey", Gcwkeyaction },
    { "lwbut", Glwbutaction },
    { "lwkey", Glwkeyaction },
    { "tweol", Gtweolaction },
    { "qwpop", Gqwpopaction },
    { "qwdel", Gqwdelaction },
    { "wmdel", Gwmdelaction },
};
static char defcwanytrans[] = "\
    <BtnDown>: cwbut()\n\
    <BtnUp>: cwbut()\n\
    <KeyDown>: cwkey()\n\
    <KeyUp>: cwkey()";
static char deflwanytrans[] = "\
    <BtnDown>: lwbut()\n\
    <BtnUp>: lwbut()\n\
    <KeyDown>: lwkey()\n\
    <KeyUp>: lwkey()";
static char deftweoltrans[] = "<Key>Return: newline()\n<KeyUp>Return: tweol()";
static char defqwpoptrans[] = "<KeyDown>Return:\n<KeyUp>Return: qwpop()";
static char defqwdeltrans[] = "<Message>WM_PROTOCOLS: qwdel()\n";
static char defwmdeltrans[] = "<Message>WM_PROTOCOLS: wmdel()\n";
XtTranslations Gtweoltable;
XtTranslations Gqwpoptable;
XtTranslations Glwanytable;
XtTranslations Gcwanytable;
XtTranslations Gqwdeltable;
XtTranslations Gwmdeltable;

Atom Gqwdelatom, Gwmdelatom;

static XtAppContext appcontext;
static XFontStruct *deffont;

#ifdef FEATURE_NEXTAW
static char *props[] = {
    "*Font: -*-lucidatypewriter-*-r-*-*-*-120-*-*-*-*-iso8859-1",
    "*ShapeStyle: Rectangle",
    "*Command.ShapeStyle: Rectangle",
    "*Toggle.ShapeStyle: Rectangle",
    "*ShadowWidth: 2",
    "*Form*Text*Background: white",
    "*Form*Text*Scrollbar*Background: gray",
    "*Text*Background: white",
    "*Text*Foreground: black",
    "*Text*ThreeD*Background: gray",
    "*Text*Scrollbar*Background: gray",
    "*Viewport.background: gray",
    "*BeNiceToColormap: 0",
    "*Toggle.Font: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
    "*Command.Font: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
    "*Command.Background: gray",
    "*Command.highlightThickness: 0",
    "*Toggle.highlightThickness: 0",
    "*Toggle.Background: gray",
    "*MenuButton.Background: gray",
    "*SimpleMenu.Background: gray",
    "*List*Background: gray",
    "*Box.Background: gray",
    "*Label.Background: gray",
    "*Label.ShadowWidth: 0",
    "*Label*shadowWidth: 2",
    "*Scrollbar.Background: gray",
    "*SimpleMenu*MenuLabel*Background: black",
    "*SimpleMenu*MenuLabel*Font: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*",
    "*SimpleMenu*Font: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
    "*Dialog.Background: gray",
    "*Form*Background: gray",
    "*Form*Viewport*Background: white",
    "*Form*Viewport*Scrollbar*Background: grey",
    "*MenuButton.translations: Any<BtnDown>: PopupMenu()",
    NULL
};
#else
#ifdef FEATURE_XAW3D
static char *props[] = {
    "*Font: -*-lucidatypewriter-*-r-*-*-*-120-*-*-*-*-iso8859-1",
    "*ShapeStyle: Rectangle",
    "*Command.ShapeStyle: Rectangle",
    "*Toggle.ShapeStyle: Rectangle",
    "*ShadowWidth: 2",
    "*Form*Text*Background: white",
    "*Form*Text*Scrollbar*Background: gray",
    "*Text*Background: white",
    "*Text*Foreground: black",
    "*Text*ThreeD*Background: gray",
    "*Text*Scrollbar*Background: gray",
    "*Viewport.background: gray",
    "*BeNiceToColormap: 0",
    "*Toggle.Font: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
    "*Command.Font: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
    "*Command.Background: gray",
    "*Command.highlightThickness: 0",
    "*Toggle.highlightThickness: 0",
    "*Toggle.Background: gray",
    "*MenuButton.Background: gray",
    "*SimpleMenu.Background: gray",
    "*List*Background: gray",
    "*Box.Background: gray",
    "*Label.Background: gray",
    "*Label.ShadowWidth: 0",
    "*Label*shadowWidth: 2",
    "*Scrollbar.Background: gray",
    "*SimpleMenu*MenuLabel*Background: black",
    "*SimpleMenu*MenuLabel*Font: -adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*",
    "*SimpleMenu*Font: -adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
    "*Dialog.Background: gray",
    "*Form*Background: gray",
    "*Form*Viewport*Background: white",
    "*Form*Viewport*Scrollbar*Background: grey",
    "*MenuButton.translations: Any<BtnDown>: PopupMenu()",
    NULL
};
#endif
#endif

int Ginitgraphics (void) {
    argn = 0;
#if defined(FEATURE_NEXTAW) || defined(FEATURE_XAW3D)
    if (!(Groot = XtAppInitialize (
        &appcontext, "LEFTY", NULL, 0, &argn, NULL, props, NULL, 0
    )))
#else
    if (!(Groot = XtAppInitialize (
        &appcontext, "LEFTY", NULL, 0, &argn, NULL, NULL, NULL, 0
    )))
#endif
        Gerr (POS, G_ERRINITFAILED);
    XtAppAddActions (appcontext, actiontable, XtNumber (actiontable));
    Gtweoltable = XtParseTranslationTable (deftweoltrans);
    Gqwpoptable = XtParseTranslationTable (defqwpoptrans);
    Glwanytable = XtParseTranslationTable (deflwanytrans);
    Gcwanytable = XtParseTranslationTable (defcwanytrans);
    Gqwdeltable = XtParseTranslationTable (defqwdeltrans);
    Gwmdeltable = XtParseTranslationTable (defwmdeltrans);
    XtRegisterGrabAction (
        Glwbutaction, True,
        ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync
    );
    XtRegisterGrabAction (
        Gcwbutaction, True,
        ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync
    );
    Gdisplay = XtDisplay (Groot);
    Gscreenn = DefaultScreen (Gdisplay);
    Gdepth = DefaultDepth (Gdisplay, Gscreenn);
    deffont = XLoadQueryFont (Gdisplay, "fixed");
    Gxfd = ConnectionNumber (Gdisplay);
    Gqwdelatom = XInternAtom (Gdisplay, "WM_DELETE_WINDOW", False);
    Gwmdelatom = XInternAtom (Gdisplay, "WM_DELETE_WINDOW", False);
    Gpopdownflag = FALSE;
    Glazyq.flag = 0;
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
        Gfontp[0].font = XLoadQueryFont (Gdisplay, Gdefaultfont);
    else
        Gfontp[0].font = NULL;
    return 0;
}

int Gtermgraphics (void) {
    int fi;

    for (fi = 0; fi < Gfontn; fi++)
        free (Gfontp[fi].name);
    Marrayfree (Gfontp), Gfontp = NULL, Gfontn = 0;
    Marrayfree (Gppp), Gppp = NULL, Gppn = 0;
    Marrayfree (Gbufp), Gbufp = NULL, Gbufn = 0;
    XtDestroyWidget (Groot);
    return 0;
}

void Gflushlazyq (void) {
    if (Glazyq.flag & LAZYMANAGE) {
        XtManageChildren (Glazyq.mws, Glazyq.mwn);
        Glazyq.flag &= ~LAZYMANAGE;
    }
    if (Glazyq.flag & LAZYREALIZE) {
        XtRealizeWidget (Glazyq.rw);
        if (Glazyq.flag & LAZYRHINTS)
            XSetWMNormalHints (Gdisplay, XtWindow (Glazyq.rw), &Glazyq.hints);
        XSetWMProtocols (Gdisplay, XtWindow (Glazyq.rw), &Gwmdelatom, 1);
        XtOverrideTranslations (Glazyq.rw, Gwmdeltable);
        Glazyq.flag &= ~LAZYRHINTS;
        Glazyq.flag &= ~LAZYREALIZE;
    }
}

void Glazyrealize (Widget w, int hintsflag, XSizeHints *hintsp) {
    if (Glazyq.flag & LAZYREALIZE) {
        XtRealizeWidget (Glazyq.rw);
        if (Glazyq.flag & LAZYRHINTS)
            XSetWMNormalHints (Gdisplay, XtWindow (Glazyq.rw), &Glazyq.hints);
        XSetWMProtocols (Gdisplay, XtWindow (Glazyq.rw), &Gwmdelatom, 1);
        XtOverrideTranslations (Glazyq.rw, Gwmdeltable);
    } else
        Glazyq.flag |= LAZYREALIZE;
    Glazyq.rw = w;
    if (hintsflag) {
        Glazyq.flag |= LAZYRHINTS;
        Glazyq.hints = *hintsp;
    } else
        Glazyq.flag &= ~LAZYRHINTS;
}

void Glazymanage (Widget w) {
    if (Glazyq.flag & LAZYMANAGE) {
        if (
            XtParent (Glazyq.mws[Glazyq.mwn - 1]) != XtParent (w) ||
            Glazyq.mwn >= LAZYQNUM
        ) {
            XtManageChildren (Glazyq.mws, Glazyq.mwn);
            Glazyq.mwn = 0;
        }
    } else {
        Glazyq.flag |= LAZYMANAGE;
        Glazyq.mwn = 0;
    }
    Glazyq.mws[Glazyq.mwn++] = w;
}

int Gsync (void) {
    if (Glazyq.flag)
        Gflushlazyq ();
    XFlush (Gdisplay);
    return 0;
}

int Gresetbstate (int wi) {
    Gcw_t *cw;
    int bn;

    cw = Gwidgets[wi].u.c;
    bn = (
        cw->bstate[0] + cw->bstate[1] + cw->bstate[2] +
        cw->bstate[3] + cw->bstate[4]
    );
    cw->bstate[0] = cw->bstate[1] = cw->bstate[2] = 0;
    cw->bstate[3] = cw->bstate[4] = 0;
    cw->buttonsdown -= bn;
    Gbuttonsdown -= bn;
    return 0;
}

int Gprocessevents (int waitflag, int mode) {
    int rtn;

    if (Glazyq.flag)
        Gflushlazyq ();
    rtn = 0;
    switch (waitflag) {
    case TRUE:
        XtAppProcessEvent (appcontext, XtIMAll);
        if (mode == G_ONEEVENT)
            return 1;
        rtn = 1;
        /* FALL THROUGH */
    case FALSE:
        while (XtAppPending (appcontext)) {
            XtAppProcessEvent (appcontext, XtIMAll);
            if (mode == G_ONEEVENT)
                return 1;
            rtn = 1;
        }
        break;
    }
    return rtn;
}
