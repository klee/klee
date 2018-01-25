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
#include "resource.h"

#define WQU widget->u.q

BOOL CALLBACK stringproc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK choiceproc (HWND, UINT, WPARAM, LPARAM);

int GQcreatewidget (
    Gwidget_t *parent, Gwidget_t *widget, int attrn, Gwattr_t *attrp
) {
    DWORD wflags;
    int ai;

    WQU->mode = G_QWSTRING;
    wflags = WS_OVERLAPPEDWINDOW;
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
        widget->w = 0;
        break;
    case G_QWFILE:
        widget->w = 0;
        break;
    case G_QWCHOICE:
        widget->w = 0;
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
    return 0;
}

static Gwidget_t *choicew;
static char *buttons[20];
static char *stringp;
static int stringn;

int GQqueryask (
    Gwidget_t *widget, char *prompt, char *args,
    char *responsep, int responsen
) {
    OPENFILENAME ofn;
    char buf[256];
    char *s1, *s2;
    char c;
    int i;

    switch (WQU->mode) {
    case G_QWSTRING:
        stringp = responsep;
        stringn = responsen;
        buttons[0] = prompt;
        buttons[1] = args;
        DialogBox (hinstance, (LPCSTR) "STRINGDIALOG", (HWND) NULL, stringproc);
        Gpopdownflag = TRUE;
        if (stringp)
            return 0;
        return -1;
        break;
    case G_QWFILE:
        strcpy (buf, args);
        ofn.lStructSize       = sizeof (OPENFILENAME);
        ofn.hwndOwner         = (HWND) NULL;
        ofn.lpstrFilter       = "All Files (*.*)\0*.*\0";
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter    = 0;
        ofn.nFilterIndex      = 1;
        ofn.lpstrFile         = buf;
        ofn.nMaxFile          = 256;
        ofn.lpstrFileTitle    = NULL;
        ofn.nMaxFileTitle     = 0;
        ofn.lpstrInitialDir   = NULL;
        ofn.lpstrTitle        = prompt;
        ofn.Flags             = 0;
        ofn.lpstrDefExt       = NULL;
        if (!GetOpenFileName (&ofn)) {
            Gpopdownflag = TRUE;
            return -1;
        }
        strncpy (responsep, buf, responsen);
        Gpopdownflag = TRUE;
        break;
    case G_QWCHOICE:
        if (!args)
            return -1;
        WQU->button = 0;
        choicew = widget;
        buttons[0] = prompt;
        for (s1 = args, i = 1; *s1; i++) {
            buttons[i] = s1;
            s2 = s1;
            while (*s2 && *s2 != '|')
                s2++;
            c = *s2, *s2 = 0;
            s1 = s2;
            if (c)
                s1++;
        }
        buttons[i] = NULL;
        DialogBox (hinstance, "CHOICEDIALOG", (HWND) NULL, choiceproc);
        if (WQU->button > 0)
            strncpy (responsep, buttons[WQU->button], responsen);
        for (s2 = args; s2 < s1; s2++)
            if (!*s2)
                *s2 = '|';
        Gpopdownflag = TRUE;
        if (WQU->button > 0)
            return 0;
        return -1;
        break;
    }
    if (responsep[0] && responsep[strlen(responsep) - 1] == '\n')
        responsep[strlen(responsep) - 1] = 0;
    return 0;
}

BOOL CALLBACK stringproc (
    HWND hdlg, UINT message, WPARAM wparam, LPARAM lparam
) {
    switch (message) {
    case WM_INITDIALOG:
        SetDlgItemText (hdlg, IDC_STATIC1, (LPCSTR) buttons[0]);
        if (buttons[1])
            SetDlgItemText (hdlg, IDC_EDIT1, (LPCSTR) buttons[1]);
        return TRUE;
        break;
    case WM_COMMAND:
        switch (wparam) {
        case IDOK:
            GetDlgItemText (hdlg, IDC_EDIT1, stringp, stringn);
            EndDialog (hdlg, TRUE);
            return TRUE;
        case IDCANCEL:
            stringp = NULL;
            EndDialog(hdlg, TRUE);
            return TRUE;
            break;
        }
    }
    return FALSE;
}

BOOL CALLBACK choiceproc (HWND hdlg, UINT message,
        WPARAM wparam, LPARAM lparam) {
    int sel, i;

    switch (message) {
    case WM_INITDIALOG:
        SetDlgItemText (hdlg, IDC_STATIC1, (LPCSTR) buttons[0]);
        for (i = 1; buttons[i]; i++)
            SendDlgItemMessage (
                hdlg, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) buttons[i]
            );
        return TRUE;
        break;
    case WM_COMMAND:
        if ((sel = (int) SendDlgItemMessage (
            hdlg, IDC_LIST1, LB_GETCURSEL, 0,  0
        )) >= 0) {
            choicew->u.q->button = sel + 1;
            EndDialog (hdlg, TRUE);
        }
        return TRUE;
        break;
    }
    return FALSE;
}
