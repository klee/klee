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

int warnflag;
char *leftypath, *leftyoptions, *shellpath;
jmp_buf exitljbuf;
int idlerunmode;
fd_set inputfds;
static int innetscape;
static char *nswin;

#ifndef FEATURE_MS
#define PATHDEL '/'
#define PATHSEP ':'
#define PATHSEPSTR ":"
#define PATHLEFTY "/../lib/lefty"
#else
#define PATHDEL '\\'
#define PATHSEP ';'
#define PATHSEPSTR ";"
#define PATHLEFTY "\\..\\lib\\lefty"
#endif
#ifdef FEATURE_X11
#define WINSYS "LEFTYWINSYS=X11"
#else
#define WINSYS "LEFTYWINSYS=mswin"
#endif
#include "g.h"
#include "gcommon.h"

#ifdef FEATURE_MS
extern int Gnocallbacks;
#else
static int Gnocallbacks;
#endif

static char *pathp;
#define PATHINCR 10240
#define PATHSIZE sizeof (char)
static char *cmdp;
#define CMDINCR 4096
#define CMDSIZE sizeof (char)

static char *lpathp;

int init (char *aout) {
    char *s1, *s2, c;
    int k;
#ifdef FEATURE_WIN32
        extern HANDLE hinstance;
        char buf[260];
#endif
#ifdef LEFTYDATADIR
    char *leftdatadir = LEFTYDATADIR;
#else
    char *leftdatadir = NULL;
#endif

    c = 0;
    if (getenv ("INNETSCAPE"))
        innetscape = TRUE, nswin = getenv ("NSWIN");
    if (!(pathp = malloc (PATHINCR * PATHSIZE)))
        panic1 (POS, "init", "pathp malloc failed");
    if (!(cmdp = malloc (CMDINCR * CMDSIZE)))
        panic1 (POS, "init", "cmdp malloc failed");
    shellpath = getenv ("PATH");
#ifdef FEATURE_WIN32
    GetModuleFileName (hinstance, buf, 260);
    aout = buf;
#else
    if (!strchr (aout, PATHDEL)) {
        leftypath = "";
        if ((s1 = buildpath (aout, TRUE)))
            aout = strdup (s1);
    } else
        aout = strdup (aout);
#endif
    if (!(s1 = strrchr (aout, PATHDEL)))
        s1 = aout;
    *s1 = 0;
    if (!(leftypath = malloc (PATHINCR * PATHSIZE)))
        panic1 (POS, "init", "leftypath malloc failed");
    leftypath[0] = 0;
    if ((s1 = getenv ("LEFTYPATH"))) {
        strcat (leftypath, s1);
	strcat (leftypath, PATHSEPSTR);
    }
    if (*aout) {
        strcat (leftypath, aout);
        strcat (leftypath, PATHSEPSTR);
    }
    for (k = 0; k < 2; k++) {
        if (k == 0)
            s1 = aout;
        else
            s1 = shellpath;
        while (s1) {
            if ((s2 = strchr (s1, PATHSEP)))
                c = *s2, *s2 = 0;
            strcat (leftypath, s1);
            strcat (leftypath, PATHLEFTY);
            if (s2) {
                *s2 = c, s1 = s2 + 1;
                strcat (leftypath, PATHSEPSTR);
            } else
                s1 = NULL;
        }
        if (leftypath[0])
            strcat (leftypath, PATHSEPSTR);
    }
    if (leftdatadir) {    /* support a compile-time path as last resort */
	strcat (leftypath, leftdatadir);
	strcat (leftypath, PATHSEPSTR);
    }
    if (!(leftyoptions = getenv ("LEFTYOPTIONS")))
        leftyoptions = "";
    putenv (WINSYS);
    return 0;
}

void term (void) {
    if (lpathp)
        free (lpathp);
    if (pathp)
        free (pathp);
    if (cmdp)
        free (cmdp);
}

/*  given a file name, it looks for this file in LEFTYPATH
    (and if flag == TRUE in PATH)

    returns the first occurance of that file or NULL
*/
char *buildpath (char *file, int flag) {
    struct stat statbuf;
    char *s1, *s2;
    int mode, pathi, i;

#ifdef FEATURE_NETSCAPE
    if (flag == FALSE && innetscape) {
#ifdef FEATURE_WIN32
        HWND hwnd;
        char *s;

        if (!nswin) {
            MessageBox (
                (HWND) NULL, "error: no peer window",
                "Lefty Warning", MB_APPLMODAL
            );
            return NULL;
        }
        hwnd = atol (nswin);
        fprintf (stdout, "file %s\n", file);
        if (fflush (stdout) == -1) {
            MessageBox (
                (HWND) NULL, "Lost Connection to Netscape",
                "Lefty Warning", MB_APPLMODAL
            );
            return NULL;
        }
        SendMessage (hwnd, WM_USER, 12, 34);
        fgets (pathp, PATHINCR - 1, stdin);
        pathp[strlen (pathp) - 1] = 0;
        if (pathp[0] == 0)
            return NULL;
        return pathp;
#else
        Window window;
        static XButtonEvent ev;

        if (!nswin) {
            fprintf (stderr, "error: no peer window\n");
            return NULL;
        }
        window = strtol (nswin, NULL, 16);
        fprintf (stdout, "file %s\n", file);
        if (fflush (stdout) == -1) {
            fprintf (stderr, "Lost Connection to Netscape\n");
            return NULL;
        }
        ev.type = ButtonPress;
        ev.window = window;
        ev.x = -123, ev.y = -123;
        XSendEvent (Gdisplay, window, False, 0, (XEvent *) &ev);
        XFlush (Gdisplay);
        fgets (pathp, PATHINCR - 1, stdin);
        pathp[strlen (pathp) - 1] = 0;
        if (pathp[0] == 0)
            return NULL;
        return pathp;
#endif
    }
#endif

#ifndef FEATURE_MS
    if (file && file[0] && strchr (file, PATHDEL))
        return file;
    mode = S_IRUSR | (flag ? S_IXUSR : 0);
    for (i = 0; i < 2; i++) {
        if (i == 0)
            s1 = leftypath;
        else
            s1 = shellpath;
        while (*s1) {
            pathi = 0;
            while (*s1 && *s1 != PATHSEP)
                if (pathi < PATHINCR)
                    pathp[pathi++] = *s1++;
            if (*s1)
                s1++;
            if (pathi + 3 + strlen (file) >= PATHINCR)
                continue;
            pathp[pathi++] = PATHDEL;
            for (s2 = file; *s2; s2++)
                pathp[pathi++] = *s2;
            pathp[pathi] = '\000';
            if (stat (pathp, &statbuf) == 0 && (statbuf.st_mode & mode))
                return pathp;
        }
    }
#else
    if (file && file[0] && strchr (file, PATHDEL))
        return file;
    mode = ~0;
    for (i = 0; i < 2; i++) {
        if (i == 0)
            s1 = leftypath;
        else
            s1 = shellpath;
        while (*s1) {
            pathi = 0;
            while (*s1 && *s1 != PATHSEP)
                if (pathi < PATHINCR)
                    pathp[pathi++] = *s1++;
            if (*s1)
                s1++;
            if (pathi + 7 + strlen (file) >= PATHINCR)
                continue;
            pathp[pathi++] = PATHDEL;
            for (s2 = file; *s2; s2++)
                pathp[pathi++] = *s2;
            if (flag) {
                pathp[pathi++] = '.';
                pathp[pathi++] = 'e';
                pathp[pathi++] = 'x';
                pathp[pathi++] = 'e';
            }
            pathp[pathi] = '\000';
            if (stat (pathp, &statbuf) == 0 && (statbuf.st_mode & mode))
                return pathp;
        }
    }
#endif
    return NULL;
}

/*  given a file name (path) and an optional format (fmt)
    it builds a shell command.

    %e is replaced by the command path
    %i      ...       the input channel descriptor
    %o      ...       the output channel descriptor
    %h      ...       the hostname

    returns the complete command string or NULL
*/
char *buildcommand (char *path, char *host, int infd, int outfd, char *fmt) {
    char buf[10];
    char *s1, *s2;
    int bufi;

    cmdp[0] = '\000';
    for (bufi = 0, s1 = fmt; *s1; s1++) {
        if (*s1 == '%') {
            if (*(s1 + 1) == 'e') {
                s1++;
                if (bufi + strlen (path) >= CMDINCR)
                    return NULL;
                for (s2 = path; *s2; s2++)
                    cmdp[bufi++] = *s2;
                continue;
            } else if (*(s1 + 1) == 'i') {
                if (infd == -1)
                    buf[0] = '%', buf[1] = 'd', buf[2] = '\000';
                else
                    sprintf (buf, "%d", infd);
                s1++;
                if (bufi + strlen (buf) >= CMDINCR)
                    return NULL;
                for (s2 = buf; *s2; s2++)
                    cmdp[bufi++] = *s2;
                continue;
            } else if (*(s1 + 1) == 'o') {
                if (outfd == -1)
                    buf[0] = '%', buf[1] = 'd', buf[2] = '\000';
                else
                    sprintf (buf, "%d", outfd);
                s1++;
                if (bufi + strlen (buf) >= CMDINCR)
                    return NULL;
                for (s2 = buf; *s2; s2++)
                    cmdp[bufi++] = *s2;
                continue;
            } else if (*(s1 + 1) == 'h') {
                s1++;
                if (bufi + strlen (host) >= CMDINCR)
                    return NULL;
                for (s2 = host; *s2; s2++)
                    cmdp[bufi++] = *s2;
                continue;
            }
        }
        if (bufi + 1 >= CMDINCR)
            return NULL;
        cmdp[bufi++] = *s1;
    }
    if (bufi + 1 >= CMDINCR)
        return NULL;
    cmdp[bufi] = '\000';
    return &cmdp[0];
}

/* varargs function for printing a warning */
void warning (char *file, int line, char *func, char *fmt, ...) {
    va_list args;

#ifndef FEATURE_MS
    if (!warnflag)
        return;

    va_start(args, fmt);
    Gnocallbacks = TRUE;
    fprintf (stderr, "warning: (file %s, line %d, func %s) ", file, line, func);
    vfprintf (stderr, fmt, args);
    fprintf (stderr, "\n");
    Gnocallbacks = FALSE;
    va_end(args);
#else
    char buf[256];

    if (!warnflag)
        return;

    va_start(args, fmt);
    vsprintf (buf, fmt, args);
    Gnocallbacks = TRUE;
    MessageBox ((HWND) NULL, buf, "Lefty Warning", MB_APPLMODAL);
    Gnocallbacks = FALSE;
    va_end(args);
#endif
}

/* varargs function for printing an error message and aborting */
void panic1 (char *file, int line, char *func, char *fmt, ...) {
    va_list args;

#ifndef FEATURE_MS
    va_start(args, fmt);
    Gnocallbacks = TRUE;
    fprintf (stderr, "panic: (file %s, line %d, func %s) ", file, line, func);
    vfprintf (stderr, fmt, args);
    fprintf (stderr, "\n");
    fflush (stdout);
    Gnocallbacks = FALSE;
    va_end(args);
#else
    char buf[256];

    va_start(args, fmt);
    vsprintf (buf, fmt, args);
    Gnocallbacks = TRUE;
    MessageBox ((HWND) NULL, buf, "Lefty PANIC", MB_APPLMODAL);
    Gnocallbacks = FALSE;
    va_end(args);
#endif
    abort ();
}

/*  varargs function for printing an error message, and the
    error message corresponding to errno and aborting
*/
void panic2 (char *file, int line, char *func, char *fmt, ...) {
    va_list args;

#ifndef FEATURE_MS
    va_start(args, fmt);
    Gnocallbacks = TRUE;
    fprintf (stderr, "panic: (file %s, line %d, func %s) ", file, line, func);
    vfprintf (stderr, fmt, args);
    fprintf (stderr, "\n");
    perror ("");
    fflush (stdout);
    Gnocallbacks = FALSE;
    va_end(args);
#else
    char buf[256];

    va_start(args, fmt);
    vsprintf (buf, fmt, args);
    Gnocallbacks = TRUE;
    MessageBox ((HWND) NULL, buf, "Lefty PANIC", MB_APPLMODAL);
    Gnocallbacks = FALSE;
    va_end(args);
#endif
    abort ();
}

#ifdef FEATURE_MS
int gprintf (const char *fmt, ...) {
    va_list args;
    char buf[10240];
    int l;

    va_start(args, fmt);
    vsprintf (buf, fmt, args);
    l = strlen (buf);
    if (buf[l - 1] == '\n')
        buf[l - 1] = 0;
    if (buf[0]) {
        Gnocallbacks = TRUE;
        MessageBox ((HWND) NULL, buf, "Lefty printf", MB_APPLMODAL);
        Gnocallbacks = FALSE;
    }
    va_end(args);
}
#endif

