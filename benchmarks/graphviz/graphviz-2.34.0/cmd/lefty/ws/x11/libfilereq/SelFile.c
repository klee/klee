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

#ifdef FEATURE_CS
#include <ast.h>
#endif

/*
 * Copyright 1989 Software Research Associates, Inc., Tokyo, Japan
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Software Research Associates not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Software Research Associates
 * makes no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * SOFTWARE RESEARCH ASSOCIATES DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL SOFTWARE RESEARCH ASSOCIATES BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Erik M. van der Poel
 *         Software Research Associates, Inc., Tokyo, Japan
 *         erik@sra.co.jp
 */

/*
 * Author's address:
 *
 *     erik@sra.co.jp
 *                                            OR
 *     erik%sra.co.jp@uunet.uu.net
 *                                            OR
 *     erik%sra.co.jp@mcvax.uucp
 *                                            OR
 *     try junet instead of co.jp
 *                                            OR
 *     Erik M. van der Poel
 *     Software Research Associates, Inc.
 *     1-1-1 Hirakawa-cho, Chiyoda-ku
 *     Tokyo 102 Japan. TEL +81-3-234-2692
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifndef HAVE_STRERROR
#  ifndef HAVE_ERRNO_DECL
extern int errno;
extern int sys_nerr;
#    if (linux)
extern const char *const sys_errlist[];
#    else
extern char *sys_errlist[];
#    endif
#  endif
#endif

#include <sys/param.h>
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Composite.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Cardinals.h>

#include "SFinternal.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif /* ndef MAXPATHLEN */

#include "SFDecls.h"

#if !defined (SVR4) && !defined (SYSV) && !defined (USG)
extern char *getwd (char *);
#endif /* !defined (SVR4) && !defined (SYSV) && !defined (USG) */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_INTPTR_T
#define INT2PTR(t,v) ((t)(intptr_t)(v))
#define PTR2INT(v) ((Sflong_t)(intptr_t)(v))
#else
#define INT2PTR(t,v) ((t)(v))
#define PTR2INT(v) ((Sflong_t)(v))
#endif

int SFstatus = SEL_FILE_NULL;

char SFstartDir[MAXPATHLEN], SFcurrentPath[MAXPATHLEN];
char SFcurrentDir[MAXPATHLEN];

Widget selFile, selFileCancel, selFileField, selFileForm, selFileHScroll;
Widget selFileHScrolls[3], selFileLists[3], selFileOK, selFilePrompt;
Widget selFileVScrolls[3];

Display *SFdisplay;

Pixel SFfore, SFback;

Atom SFwmDeleteWindow;

XSegment SFsegs[2], SFcompletionSegs[2];

XawTextPosition SFtextPos;

int SFupperX, SFlowerY, SFupperY;

int SFtextX, SFtextYoffset;

int SFentryWidth, SFentryHeight;

int SFlineToTextH = 3;

int SFlineToTextV = 3;

int SFbesideText = 3;

int SFaboveAndBelowText = 2;

int SFcharsPerEntry = 15;

int SFlistSize = 10;

int SFworkProcAdded = 0;
XtWorkProcId SFworkProcId;

XtAppContext SFapp;

int SFpathScrollWidth, SFvScrollHeight, SFhScrollWidth;

char SFtextBuffer[MAXPATHLEN];

XtIntervalId SFdirModTimerId;

int (*SFfunc) (char *, char **, struct stat *);

static char *oneLineTextEditTranslations = "\
    <Key>Return: redraw-display()\n\
    Ctrl<Key>M:  redraw-display()\n\
";

static void SFexposeList (Widget w, XtPointer n, XEvent *event, Boolean *cont) {
    if ((event->type == NoExpose) || event->xexpose.count) {
        return;
    }

    SFdrawList ((intptr_t) n, SF_DO_NOT_SCROLL);
}

static void SFmodVerifyCallback (
    Widget w, XtPointer client_data, XEvent *event, Boolean *cont
) {
    char buf[2];

    if (
        XLookupString (&event->xkey, buf, 2, NULL, NULL) == 1 && *buf == '\r'
    ) {
        SFstatus = SEL_FILE_OK;
    } else {
        SFstatus = SEL_FILE_TEXT;
    }
}

static void SFokCallback (Widget w, XtPointer cl, XtPointer cd) {
    SFstatus = SEL_FILE_OK;
}

static XtCallbackRec SFokSelect[] = {
    { SFokCallback, (XtPointer) NULL },
    { NULL,         (XtPointer) NULL },
};

static void SFcancelCallback (Widget w, XtPointer cl, XtPointer cd) {
    SFstatus = SEL_FILE_CANCEL;
}

static XtCallbackRec SFcancelSelect[] = {
    { SFcancelCallback, (XtPointer) NULL },
    { NULL,             (XtPointer) NULL },
};

static void SFdismissAction (
    Widget w, XEvent *event, String *params, Cardinal *num_params
) {
    if (
        event->type == ClientMessage &&
        event->xclient.data.l[0] != SFwmDeleteWindow
    )
        return;

    SFstatus = SEL_FILE_CANCEL;
}

static char *wmDeleteWindowTranslation = "\
    <Message>WM_PROTOCOLS: SelFileDismiss()\n\
";

static XtActionsRec actions[] = {
    { "SelFileDismiss", SFdismissAction },
};

static void SFcreateWidgets (
    Widget toplevel, char *prompt, char *ok, char *cancel
) {
    Cardinal i;
    intptr_t n;
    int      listWidth, listHeight;
    int      listSpacing = 10;
    int      scrollThickness = 15;
    int      hScrollX, hScrollY;
    int      vScrollX, vScrollY;
    Cursor   xtermCursor, sbRightArrowCursor, dotCursor;
    Arg      arglist[20];

    i = 0;
    XtSetArg (arglist[i], XtNtransientFor, toplevel); i++;

    selFile = XtAppCreateShell (
        "selFile", "SelFile", transientShellWidgetClass, SFdisplay, arglist, i
    );

    /* Add WM_DELETE_WINDOW protocol */
    XtAppAddActions (
        XtWidgetToApplicationContext (selFile), actions, XtNumber (actions)
    );
    XtOverrideTranslations (
        selFile, XtParseTranslationTable (wmDeleteWindowTranslation)
    );

    i = 0;
    XtSetArg (arglist[i], XtNdefaultDistance, 30); i++;
    selFileForm = XtCreateManagedWidget (
        "selFileForm", formWidgetClass, selFile, arglist, i
    );

    i = 0;
    XtSetArg (arglist[i], XtNlabel, prompt); i++;
    XtSetArg (arglist[i], XtNresizable, True); i++;
    XtSetArg (arglist[i], XtNtop, XtChainTop); i++;
    XtSetArg (arglist[i], XtNbottom, XtChainTop); i++;
    XtSetArg (arglist[i], XtNleft, XtChainLeft); i++;
    XtSetArg (arglist[i], XtNright, XtChainLeft); i++;
    XtSetArg (arglist[i], XtNborderWidth, 0); i++;
    selFilePrompt = XtCreateManagedWidget (
        "selFilePrompt", labelWidgetClass, selFileForm, arglist, i
    );

    i = 0;
    XtSetArg (arglist[i], XtNforeground, &SFfore); i++;
    XtSetArg (arglist[i], XtNbackground, &SFback); i++;
    XtGetValues (selFilePrompt, arglist, i);

    SFinitFont ();

    SFentryWidth = SFbesideText + SFcharsPerEntry * SFcharWidth + SFbesideText;
    SFentryHeight = SFaboveAndBelowText + SFcharHeight + SFaboveAndBelowText;

    listWidth = (
        SFlineToTextH + SFentryWidth + SFlineToTextH + 1 + scrollThickness
    );
    listHeight = (
        SFlineToTextV + SFentryHeight + SFlineToTextV + 1 +
        SFlineToTextV + SFlistSize * SFentryHeight +
        SFlineToTextV + 1 + scrollThickness
    );

    SFpathScrollWidth = 3 * listWidth + 2 * listSpacing + 4;

    hScrollX = -1;
    hScrollY = (
        SFlineToTextV + SFentryHeight + SFlineToTextV + 1 +
        SFlineToTextV + SFlistSize * SFentryHeight + SFlineToTextV
    );
    SFhScrollWidth = SFlineToTextH + SFentryWidth + SFlineToTextH;

    vScrollX = SFlineToTextH + SFentryWidth + SFlineToTextH;
    vScrollY = SFlineToTextV + SFentryHeight + SFlineToTextV;
    SFvScrollHeight = (
        SFlineToTextV + SFlistSize * SFentryHeight + SFlineToTextV
    );

    SFupperX = SFlineToTextH + SFentryWidth + SFlineToTextH - 1;
    SFlowerY = (
        SFlineToTextV + SFentryHeight + SFlineToTextV + 1 + SFlineToTextV
    );
    SFupperY = (
        SFlineToTextV + SFentryHeight + SFlineToTextV + 1 +
        SFlineToTextV + SFlistSize * SFentryHeight - 1
    );

    SFtextX = SFlineToTextH + SFbesideText;
    SFtextYoffset = SFlowerY + SFaboveAndBelowText + SFcharAscent;

    SFsegs[0].x1 = 0;
    SFsegs[0].y1 = vScrollY;
    SFsegs[0].x2 = vScrollX - 1;
    SFsegs[0].y2 = vScrollY;
    SFsegs[1].x1 = vScrollX;
    SFsegs[1].y1 = 0;
    SFsegs[1].x2 = vScrollX;
    SFsegs[1].y2 = vScrollY - 1;

    SFcompletionSegs[0].x1 = SFcompletionSegs[0].x2 = SFlineToTextH;
    SFcompletionSegs[1].x1 = SFcompletionSegs[1].x2 = (
        SFlineToTextH + SFentryWidth - 1
    );

    i = 0;
    XtSetArg (arglist[i], XtNwidth, 3 * listWidth + 2 * listSpacing + 4); i++;
    XtSetArg (arglist[i], XtNborderColor, SFfore); i++;

    XtSetArg (arglist[i], XtNfromVert, selFilePrompt); i++;
    XtSetArg (arglist[i], XtNvertDistance, 10); i++;
    XtSetArg (arglist[i], XtNresizable, True); i++;
    XtSetArg (arglist[i], XtNtop, XtChainTop); i++;
    XtSetArg (arglist[i], XtNbottom, XtChainTop); i++;
    XtSetArg (arglist[i], XtNleft, XtChainLeft); i++;
    XtSetArg (arglist[i], XtNright, XtChainLeft); i++;
    XtSetArg (arglist[i], XtNstring, SFtextBuffer); i++;
    XtSetArg (arglist[i], XtNlength, MAXPATHLEN); i++;
    XtSetArg (arglist[i], XtNeditType, XawtextEdit); i++;
    XtSetArg (arglist[i], XtNwrap, XawtextWrapWord); i++;
    XtSetArg (arglist[i], XtNresize, XawtextResizeHeight); i++;
    XtSetArg (arglist[i], XtNuseStringInPlace, True); i++;
    selFileField = XtCreateManagedWidget (
        "selFileField", asciiTextWidgetClass, selFileForm, arglist, i
    );

    XtOverrideTranslations (
        selFileField, XtParseTranslationTable (oneLineTextEditTranslations)
    );
    XtSetKeyboardFocus (selFileForm, selFileField);

    i = 0;
    XtSetArg (arglist[i], XtNorientation, XtorientHorizontal); i++;
    XtSetArg (arglist[i], XtNwidth, SFpathScrollWidth); i++;
    XtSetArg (arglist[i], XtNheight, scrollThickness); i++;
    XtSetArg (arglist[i], XtNborderColor, SFfore); i++;
    XtSetArg (arglist[i], XtNfromVert, selFileField); i++;
    XtSetArg (arglist[i], XtNvertDistance, 30); i++;
    XtSetArg (arglist[i], XtNtop, XtChainTop); i++;
    XtSetArg (arglist[i], XtNbottom, XtChainTop); i++;
    XtSetArg (arglist[i], XtNleft, XtChainLeft); i++;
    XtSetArg (arglist[i], XtNright, XtChainLeft); i++;
    selFileHScroll = XtCreateManagedWidget (
        "selFileHScroll", scrollbarWidgetClass, selFileForm, arglist, i
    );

    XtAddCallback (
        selFileHScroll, XtNjumpProc, SFpathSliderMovedCallback, (XtPointer) NULL
    );
    XtAddCallback (
        selFileHScroll, XtNscrollProc, SFpathAreaSelectedCallback,
        (XtPointer) NULL
    );

    i = 0;
    XtSetArg (arglist[i], XtNwidth, listWidth); i++;
    XtSetArg (arglist[i], XtNheight, listHeight); i++;
    XtSetArg (arglist[i], XtNborderColor, SFfore); i++;
    XtSetArg (arglist[i], XtNfromVert, selFileHScroll); i++;
    XtSetArg (arglist[i], XtNvertDistance, 10); i++;
    XtSetArg (arglist[i], XtNtop, XtChainTop); i++;
    XtSetArg (arglist[i], XtNbottom, XtChainTop); i++;
    XtSetArg (arglist[i], XtNleft, XtChainLeft); i++;
    XtSetArg (arglist[i], XtNright, XtChainLeft); i++;
    selFileLists[0] = XtCreateManagedWidget (
        "selFileList1", compositeWidgetClass, selFileForm, arglist, i
    );

    i = 0;
    XtSetArg (arglist[i], XtNwidth, listWidth); i++;
    XtSetArg (arglist[i], XtNheight, listHeight); i++;
    XtSetArg (arglist[i], XtNborderColor, SFfore); i++;
    XtSetArg (arglist[i], XtNfromHoriz, selFileLists[0]); i++;
    XtSetArg (arglist[i], XtNfromVert, selFileHScroll); i++;
    XtSetArg (arglist[i], XtNhorizDistance, listSpacing); i++;
    XtSetArg (arglist[i], XtNvertDistance, 10); i++;
    XtSetArg (arglist[i], XtNtop, XtChainTop); i++;
    XtSetArg (arglist[i], XtNbottom, XtChainTop); i++;
    XtSetArg (arglist[i], XtNleft, XtChainLeft); i++;
    XtSetArg (arglist[i], XtNright, XtChainLeft); i++;
    selFileLists[1] = XtCreateManagedWidget (
        "selFileList2", compositeWidgetClass, selFileForm, arglist, i
    );

    i = 0;
    XtSetArg (arglist[i], XtNwidth, listWidth); i++;
    XtSetArg (arglist[i], XtNheight, listHeight); i++;
    XtSetArg (arglist[i], XtNborderColor, SFfore); i++;
    XtSetArg (arglist[i], XtNfromHoriz, selFileLists[1]); i++;
    XtSetArg (arglist[i], XtNfromVert, selFileHScroll); i++;
    XtSetArg (arglist[i], XtNhorizDistance, listSpacing); i++;
    XtSetArg (arglist[i], XtNvertDistance, 10); i++;
    XtSetArg (arglist[i], XtNtop, XtChainTop); i++;
    XtSetArg (arglist[i], XtNbottom, XtChainTop); i++;
    XtSetArg (arglist[i], XtNleft, XtChainLeft); i++;
    XtSetArg (arglist[i], XtNright, XtChainLeft); i++;
    selFileLists[2] = XtCreateManagedWidget (
        "selFileList3", compositeWidgetClass, selFileForm, arglist, i
    );

    for (n = 0; n < 3; n++) {

        i = 0;
        XtSetArg (arglist[i], XtNx, vScrollX); i++;
        XtSetArg (arglist[i], XtNy, vScrollY); i++;
        XtSetArg (arglist[i], XtNwidth, scrollThickness); i++;
        XtSetArg (arglist[i], XtNheight, SFvScrollHeight); i++;
        XtSetArg (arglist[i], XtNborderColor, SFfore); i++;
        selFileVScrolls[n] = XtCreateManagedWidget (
            "selFileVScroll", scrollbarWidgetClass, selFileLists[n], arglist, i
        );

        XtAddCallback (
            selFileVScrolls[n], XtNjumpProc,
            SFvFloatSliderMovedCallback, (XtPointer) n
        );
        XtAddCallback (
            selFileVScrolls[n], XtNscrollProc,
            SFvAreaSelectedCallback, (XtPointer) n
        );

        i = 0;

        XtSetArg (arglist[i], XtNorientation, XtorientHorizontal); i++;
        XtSetArg (arglist[i], XtNx, hScrollX); i++;
        XtSetArg (arglist[i], XtNy, hScrollY); i++;
        XtSetArg (arglist[i], XtNwidth, SFhScrollWidth); i++;
        XtSetArg (arglist[i], XtNheight, scrollThickness); i++;
        XtSetArg (arglist[i], XtNborderColor, SFfore); i++;
        selFileHScrolls[n] = XtCreateManagedWidget (
            "selFileHScroll", scrollbarWidgetClass, selFileLists[n], arglist, i
        );

        XtAddCallback (
            selFileHScrolls[n], XtNjumpProc,
            SFhSliderMovedCallback, (XtPointer) n
        );
        XtAddCallback (
            selFileHScrolls[n], XtNscrollProc,
            SFhAreaSelectedCallback, (XtPointer) n
        );
    }

    i = 0;
    XtSetArg (arglist[i], XtNlabel, ok); i++;
    XtSetArg (arglist[i], XtNresizable, True); i++;
    XtSetArg (arglist[i], XtNcallback, SFokSelect); i++;
    XtSetArg (arglist[i], XtNborderColor, SFfore); i++;
    XtSetArg (arglist[i], XtNfromVert, selFileLists[0]); i++;
    XtSetArg (arglist[i], XtNvertDistance, 30); i++;
    XtSetArg (arglist[i], XtNtop, XtChainTop); i++;
    XtSetArg (arglist[i], XtNbottom, XtChainTop); i++;
    XtSetArg (arglist[i], XtNleft, XtChainLeft); i++;
    XtSetArg (arglist[i], XtNright, XtChainLeft); i++;
    selFileOK = XtCreateManagedWidget (
        "selFileOK", commandWidgetClass, selFileForm, arglist, i
    );

    i = 0;
    XtSetArg (arglist[i], XtNlabel, cancel); i++;
    XtSetArg (arglist[i], XtNresizable, True); i++;
    XtSetArg (arglist[i], XtNcallback, SFcancelSelect); i++;
    XtSetArg (arglist[i], XtNborderColor, SFfore); i++;
    XtSetArg (arglist[i], XtNfromHoriz, selFileOK); i++;
    XtSetArg (arglist[i], XtNfromVert, selFileLists[0]); i++;
    XtSetArg (arglist[i], XtNhorizDistance, 30); i++;
    XtSetArg (arglist[i], XtNvertDistance, 30); i++;
    XtSetArg (arglist[i], XtNtop, XtChainTop); i++;
    XtSetArg (arglist[i], XtNbottom, XtChainTop); i++;
    XtSetArg (arglist[i], XtNleft, XtChainLeft); i++;
    XtSetArg (arglist[i], XtNright, XtChainLeft); i++;
    selFileCancel = XtCreateManagedWidget (
        "selFileCancel", commandWidgetClass, selFileForm, arglist, i
    );

    XtSetMappedWhenManaged (selFile, False);
    XtRealizeWidget (selFile);

    /* Add WM_DELETE_WINDOW protocol */
    SFwmDeleteWindow = XInternAtom (SFdisplay, "WM_DELETE_WINDOW", False);
    XSetWMProtocols (SFdisplay, XtWindow (selFile), &SFwmDeleteWindow, 1);

    SFcreateGC ();

    xtermCursor = XCreateFontCursor (SFdisplay, XC_xterm);

    sbRightArrowCursor = XCreateFontCursor (SFdisplay, XC_sb_right_arrow);
    dotCursor = XCreateFontCursor (SFdisplay, XC_dot);

    XDefineCursor (SFdisplay, XtWindow (selFileForm), xtermCursor);
    XDefineCursor (SFdisplay, XtWindow (selFileField), xtermCursor);

    for (n = 0; n < 3; n++) {
        XDefineCursor (
            SFdisplay, XtWindow (selFileLists[n]), sbRightArrowCursor
        );
    }
    XDefineCursor (SFdisplay, XtWindow (selFileOK), dotCursor);
    XDefineCursor (SFdisplay, XtWindow (selFileCancel), dotCursor);

    for (n = 0; n < 3; n++) {
        XtAddEventHandler (
            selFileLists[n], ExposureMask, True,
            SFexposeList, (XtPointer) n
        );
        XtAddEventHandler (
            selFileLists[n], EnterWindowMask, False,
            SFenterList, (XtPointer) n
        );
        XtAddEventHandler (
            selFileLists[n], LeaveWindowMask, False,
            SFleaveList, (XtPointer) n
        );
        XtAddEventHandler (
            selFileLists[n], PointerMotionMask, False,
            SFmotionList, (XtPointer) n
        );
        XtAddEventHandler (
            selFileLists[n], ButtonPressMask, False,
            SFbuttonPressList, (XtPointer) n
        );
        XtAddEventHandler (
            selFileLists[n], ButtonReleaseMask, False,
            SFbuttonReleaseList, (XtPointer) n
        );
    }

    XtAddEventHandler (
        selFileField, KeyPressMask, False,
        SFmodVerifyCallback, (XtPointer) NULL
    );

    SFapp = XtWidgetToApplicationContext (selFile);

}

/* position widget under the cursor */
static void SFpositionWidget (Widget w) {
    Arg args[3];
    Cardinal num_args;
    Dimension width, height, b_width;
    int x, y, max_x, max_y;
    Window root, child;
    int dummyx, dummyy;
    unsigned int dummymask;

    XQueryPointer (
        XtDisplay (w), XtWindow (w), &root, &child, &x, &y,
        &dummyx, &dummyy, &dummymask
    );
    num_args = 0;
    XtSetArg (args[num_args], XtNwidth, &width); num_args++;
    XtSetArg (args[num_args], XtNheight, &height); num_args++;
    XtSetArg (args[num_args], XtNborderWidth, &b_width); num_args++;
    XtGetValues (w, args, num_args);

    width += 2 * b_width;
    height += 2 * b_width;

    x -= ((Position) width / 2);
    if (x < 0)
        x = 0;
    if (x > (max_x = (Position) (XtScreen (w)->width - width)))
        x = max_x;

    y -= ((Position) height / 2);
    if (y < 0)
        y = 0;
    if (y > (max_y = (Position) (XtScreen (w)->height - height)))
        y = max_y;

    num_args = 0;
    XtSetArg (args[num_args], XtNx, x); num_args++;
    XtSetArg (args[num_args], XtNy, y); num_args++;
    XtSetValues (w, args, num_args);
}

FILE *SFopenFile (char *name, char *mode, char *prompt, char *failed) {
    Arg args[1];
    FILE *fp;

    SFchdir (SFstartDir);
    if ((fp = fopen (name, mode)) == NULL) {
        char *buf;
#ifdef HAVE_STRERROR
        char *errormsg = strerror (errno);
        if (errormsg) {
            buf = XtMalloc (
                strlen (failed) + strlen (errormsg) + strlen (prompt) + 2
            );
            strcpy (buf, failed);
            strcat (buf, errormsg);
            strcat (buf, "\n");
#else
        if (errno < sys_nerr) {
            buf = XtMalloc (
                strlen (failed) + strlen (sys_errlist[errno]) +
                strlen (prompt) + 2
            );
            strcpy (buf, failed);
            strcat (buf, sys_errlist[errno]);
            strcat (buf, "\n");
            strcat (buf, prompt);
#endif
        } else {
            buf = XtMalloc (strlen (failed) + strlen (prompt) + 2);
            strcpy (buf, failed);
            strcat (buf, "\n");
            strcat (buf, prompt);
        }
        XtSetArg (args[0], XtNlabel, buf);
        XtSetValues (selFilePrompt, args, ONE);
        XtFree (buf);
        return NULL;
    }
    return fp;
}

void SFtextChanged (void) {
    if ((SFtextBuffer[0] == '/') || (SFtextBuffer[0] == '~')) {
        strcpy (SFcurrentPath, SFtextBuffer);
        SFtextPos = XawTextGetInsertionPoint (selFileField);
    } else {
        strcat (strcpy (SFcurrentPath, SFstartDir), SFtextBuffer);
        SFtextPos = XawTextGetInsertionPoint (
            selFileField
        ) + strlen (SFstartDir);
    }
    if (!SFworkProcAdded) {
        SFworkProcId = XtAppAddWorkProc (SFapp, SFworkProc, NULL);
        SFworkProcAdded = 1;
    }
    SFupdatePath ();
}

static void SFprepareToReturn (void) {
    SFstatus = SEL_FILE_NULL;
    XtRemoveGrab (selFile);
    XtUnmapWidget (selFile);
    XtRemoveTimeOut (SFdirModTimerId);
    if (SFchdir (SFstartDir)) {
        XtAppError (SFapp, "XsraSelFile: can't return to current directory");
    }
}

int
XsraSelFile (
    Widget toplevel, char *prompt, char *ok, char *cancel, char *failed,
    char *init_path, char *mode,
    int (*show_entry) (char *, char **, struct stat *),
    char *name_return, int name_size
) {
    static int firstTime = 1;
    Cardinal   i;
    Arg        arglist[20];
    XEvent     event;

    if (!prompt) {
        prompt = "Pathname:";
    }
    if (!ok) {
        ok = "OK";
    }
    if (!cancel) {
        cancel = "Cancel";
    }
    if (firstTime) {
        firstTime = 0;
        SFdisplay = XtDisplay (toplevel);
        SFcreateWidgets (toplevel, prompt, ok, cancel);
    } else {
        i = 0;
        XtSetArg (arglist[i], XtNlabel, prompt); i++;
        XtSetValues (selFilePrompt, arglist, i);

        i = 0;
        XtSetArg (arglist[i], XtNlabel, ok); i++;
        XtSetValues (selFileOK, arglist, i);

        i = 0;
        XtSetArg (arglist[i], XtNlabel, cancel); i++;
        XtSetValues (selFileCancel, arglist, i);
    }
    SFpositionWidget (selFile);
    XtMapWidget (selFile);

    if (!getcwd (SFstartDir, MAXPATHLEN)) {
        XtAppError (SFapp, "XsraSelFile: can't get current directory");
    }
    strcat (SFstartDir, "/");
    strcpy (SFcurrentDir, SFstartDir);

    if (init_path) {
        if (init_path[0] == '/') {
            strcpy (SFcurrentPath, init_path);
            if (strncmp (SFcurrentPath, SFstartDir, strlen (SFstartDir))) {
                SFsetText (SFcurrentPath);
            } else {
                SFsetText (& (SFcurrentPath[strlen (SFstartDir)]));
            }
        } else {
            strcat (strcpy (SFcurrentPath, SFstartDir), init_path);
            SFsetText (& (SFcurrentPath[strlen (SFstartDir)]));
        }
    } else {
        strcpy (SFcurrentPath, SFstartDir);
    }

    SFfunc = show_entry;

    SFtextChanged ();

    XtAddGrab (selFile, True, True);

    SFdirModTimerId = XtAppAddTimeOut (
        SFapp, (unsigned long) 1000, SFdirModTimer, (XtPointer) NULL
    );

    while (1) {
        XtAppNextEvent (SFapp, &event);
        XtDispatchEvent (&event);
        switch (SFstatus) {
        case SEL_FILE_TEXT:
            SFstatus = SEL_FILE_NULL;
            SFtextChanged ();
            break;
        case SEL_FILE_OK:
            strncpy (name_return, SFtextBuffer, name_size);
            SFprepareToReturn ();
            if (SFworkProcAdded) {
                XtRemoveWorkProc (SFworkProcId);
                SFworkProcAdded = 0;
            }
            return 1;
        case SEL_FILE_CANCEL:
            SFprepareToReturn ();
            return 0;
        case SEL_FILE_NULL:
            break;
        }
    }
    return 0;
}
