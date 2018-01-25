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

/* Lefteris Koutsofios - AT&T Bell Laboratories */

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

static char *shellpath;

static char *buildpath (char *);
static void panic (char *, int, char *, char *, ...);

static char* lneato =
#ifdef LNEATO
  "dotty.protogt.lserver='neato';";
#else
  "";
#endif

int APIENTRY WinMain (
    HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow
) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char cmd[256];
    char *path;
    char *s;

    ZeroMemory (&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory (&pi, sizeof(pi));

    shellpath = getenv ("PATH");
    if (!shellpath || !(path = buildpath ("lefty"))) {
        if (!GetModuleFileName (hInstance, cmd, 256) ||
                !(s = strrchr (cmd, '\\')))
            exit (1);
        *s = 0;
        shellpath = &cmd[0];
        if (!(path = buildpath ("lefty")))
            exit (1);
    }
    if (lpCmdLine[0] == 0)
        sprintf (
	    cmd, "%s -e \"load('dotty.lefty');%sdotty.init();dotty.createviewandgraph(null,'file',null,null);txtview('off');\"",
	    path, lneato
        );
    else
        sprintf (
	    cmd, "%s -e \"load('dotty.lefty');%sdotty.init();dotty.createviewandgraph('%Ns','file',null,null);txtview('off');\"",
            path, lneato, lpCmdLine
        );

    CreateProcess (
        NULL,   // No module name (use command line). 
        cmd, // Command line. 
        NULL,             // Process handle not inheritable. 
        NULL,             // Thread handle not inheritable. 
        FALSE,            // Set handle inheritance to FALSE. 
        0,                // No creation flags. 
        NULL,             // Use parent's environment block. 
        NULL,             // Use parent's starting directory. 
        &si,              // Pointer to STARTUPINFO structure.
        &pi             // Pointer to PROCESS_INFORMATION structure.
    );
    exit (0);
}

#define PATHDEL '\\'
#define PATHSEP ';'

static char pathbuf[1024];
static char commandbuf[1024];

static char *buildpath (char *file) {
    struct stat statbuf;
    char *s1, *s2;
    int mode, pathi;

    if (file && file[0] && (file[0] == '.' || file[0] == PATHDEL))
        return file;
    mode = ~0;
    s1 = shellpath;
    while (*s1) {
        pathi = 0;
        while (*s1 && *s1 != PATHSEP)
            pathbuf[pathi++] = *s1++;
        if (*s1)
            s1++;
        pathbuf[pathi++] = PATHDEL;
        for (s2 = file; *s2; s2++)
            pathbuf[pathi++] = *s2;
        pathbuf[pathi++] = '.';
        pathbuf[pathi++] = 'e';
        pathbuf[pathi++] = 'x';
        pathbuf[pathi++] = 'e';
        pathbuf[pathi] = '\000';
        if (stat (pathbuf, &statbuf) == 0 && (statbuf.st_mode & mode))
            return pathbuf;
    }
    return NULL;
}

static void panic (char *file, int line, char *func, char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    {
        char buf[256];
        vsprintf (buf, fmt, args);
        MessageBox ((HWND) NULL, buf, "dotty PANIC", MB_APPLMODAL);
    }
    abort ();
}
