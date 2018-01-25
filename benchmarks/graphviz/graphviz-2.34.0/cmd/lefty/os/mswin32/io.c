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
#include "io.h"
#include "mem.h"

io_t *iop;
int ion;

static void pipeopen (char *, FILE **, FILE **, int *);

void IOinit (void) {
    struct stat statbuf;
    int ioi;

    iop = Marrayalloc ((long) IOINCR * IOSIZE);
    ion = IOINCR;
    for (ioi = 0; ioi < ion; ioi++)
        iop[ioi].inuse = FALSE;
    for (ioi = 0; ioi < 3; ioi++)
        if (fstat (ioi, &statbuf) == 0) {
            iop[ioi].inuse = TRUE;
            iop[ioi].type = IO_FILE;
            iop[ioi].ifp = iop[ioi].ofp = fdopen (ioi, "r+");
            if (!(iop[ioi].buf = malloc (IOBUFSIZE)))
                panic1 (POS, "IOinit", "malloc failed");
            iop[ioi].buf[0] = 0;
        }
}

void IOterm (void) {
    int ioi;

    for (ioi = 0; ioi < ion; ioi++)
        if (iop[ioi].inuse)
            IOclose (ioi, NULL);
    Marrayfree (iop), iop = NULL, ion = 0;
}

int IOopen (char *kind, char *name, char *mode, char *fmt) {
    io_t *p;
    int type;
    char *path, *command;
    int i;
    int count = 0;

    if (strcmp (kind, "file") == 0)
        type = IO_FILE;
    else if (strcmp (kind, "pipe") == 0)
        type = IO_PIPE;
    else
        return -1;

    for (i = 0; i < ion; i++)
        if (!iop[i].inuse)
            break;
    if (i == ion) {
        iop = Marraygrow (iop, (long) (ion + IOINCR) * IOSIZE);
        for (i = ion + IOINCR - 1; i >= ion; i--)
            iop[i].inuse = FALSE;
        i++, ion += IOINCR;
    }
    p = &iop[i];
    p->type = type;
    if (!(p->buf = malloc (IOBUFSIZE)))
        panic1 (POS, "IOopen", "malloc failed");
    p->buf[0] = 0;
    switch (type) {
    case IO_FILE:
        if (!(p->ifp = p->ofp = fopen (name, mode))) {
            path = buildpath (name, FALSE);
            if (!path || !(p->ifp = p->ofp = fopen (path, mode)))
                return -1;
        }
        break;
    case IO_PIPE:
        if (!fmt)
            fmt = "%e";
        if (
            !(path = buildpath (name, TRUE)) ||
            !(command = buildcommand (path, NULL, -1, -1, fmt))
        )
            return -1;
        pipeopen (command, &p->ifp, &p->ofp, &p->pid);
        if (!p->ifp || !p->ofp)
            return -1;
        break;
    }
    p->inuse = TRUE;
    return i;
}

int IOclose (int ioi, char *action) {
    io_t *p;

    if (ioi < 0 || ioi >= ion || !iop[ioi].inuse)
        return -1;

    p = &iop[ioi];
    free (p->buf);
    switch (p->type) {
    case IO_FILE:
        fclose (p->ifp);
        fclose (p->ofp);
        break;
    case IO_PIPE:
        CloseHandle (p->ifp);
        CloseHandle (p->ofp);
        break;
    }
    p->inuse = FALSE;
    return 0;
}

int IOreadline (int ioi, char *bufp, int bufn) {
    io_t *p;
    DWORD n;
    int l, i, m;

    if (ioi < 0 || ioi >= ion || !iop[ioi].inuse)
        return -1;

    p = &iop[ioi];
    switch (p->type) {
    case IO_FILE:
        if (fgets (bufp, bufn, p->ifp) == NULL)
            return -1;
        break;
    case IO_PIPE:
        l = 0;
        if (p->buf[0]) {
            for (l = 0; l < bufn - 1 && p->buf[l]; l++)
                if ((bufp[l] = p->buf[l]) == '\n') {
                    l++;
                    break;
                }
            for (i = l; p->buf[i]; i++)
                p->buf[i - l] = p->buf[i];
            p->buf[i - l] = 0;
            bufp[l] = 0;
            if (bufp[l - 1] == '\n')
                break; /* exit switch */
        }
        while (l < bufn - 1) {
            m = bufn - l - 1;
            if (m > IOBUFSIZE - 1)
                m = IOBUFSIZE - 1;
            if (!ReadFile (p->ifp, bufp + l, m, &n, NULL))
                return -1;
            for (m = l; m < l + n; m++)
                if (bufp[m] == '\n') {
                    m++;
                    break;
                }
            for (i = m; i < l + n; i++)
                p->buf[i - m] = bufp[i];
            bufp[m] = 0;
            p->buf[i - m] = 0;
            if (bufp[m - 1] == '\n')
                break;
            l += n;
        }
        break;
    }
    l = strlen (bufp) - 1;
    while (bufp[l] == '\n' || bufp[l] == '\r')
        bufp[l--] = '\000';
    return 0;
}

int IOread (int ioi, char *bufp, int bufn) {
    io_t *p;
    DWORD l;

    if (ioi < 0 || ioi >= ion || !iop[ioi].inuse)
        return -1;

    p = &iop[ioi];
    switch (p->type) {
    case IO_FILE:
        if ((l = read (fileno (p->ifp), bufp, bufn - 1)) == -1)
            return -1;
        else if (l == 0)
            return 0;
        break;
    case IO_PIPE:
        if (!ReadFile (p->ifp, bufp, bufn - 1, &l, NULL))
            return -1;
        if (l == 0)
            return 0;
        break;
    }
    bufp[l] = '\000';
    return l;
}

int IOwriteline (int ioi, char *bufp) {
    io_t *p;
    DWORD l;

    if (ioi < 0 || ioi >= ion || !iop[ioi].inuse)
        return -1;

    p = &iop[ioi];
    switch (p->type) {
    case IO_FILE:
        if (fputs (bufp, p->ofp) == EOF || fputs ("\n", p->ofp) == EOF)
            return -1;
        fflush (p->ofp);
        break;
    case IO_PIPE:
        if (
            !WriteFile (p->ofp, bufp, strlen (bufp), &l, NULL) ||
            !WriteFile (p->ofp, "\n", 1, &l, NULL)
        )
            return -1;
        break;
    }
    return 0;
}

static void pipeopen (char *cmd, FILE **ifp, FILE **ofp, int *pidp) {
    PROCESS_INFORMATION pinfo;
    STARTUPINFO sinfo;
    SECURITY_ATTRIBUTES sattr;
    HANDLE h, p1[2], p2[2], save[2];

    sattr.nLength = sizeof (SECURITY_ATTRIBUTES);
    sattr.bInheritHandle = TRUE;
    sattr.lpSecurityDescriptor = NULL;

    if (
        !CreatePipe (&p1[0], &p1[1], &sattr, 0) ||
        !CreatePipe (&p2[0], &p2[1], &sattr, 0)
    ) {
        *ifp = NULL;
        return;
    }
    save[0] = GetStdHandle (STD_INPUT_HANDLE);
    save[1] = GetStdHandle (STD_OUTPUT_HANDLE);
    if (!SetStdHandle (STD_OUTPUT_HANDLE, p1[1]))
        panic1 (POS, "pipeopen", "cannot set stdout handle");
    if (!SetStdHandle (STD_INPUT_HANDLE, p2[0]))
        panic1 (POS, "pipeopen", "cannot set stdin handle");
    h = p1[0];
    if (!DuplicateHandle (
        GetCurrentProcess (), h, GetCurrentProcess (), &p1[0], 0, FALSE,
        DUPLICATE_SAME_ACCESS
    ))
        panic1 (POS, "pipeopen", "cannot dup input handle");
    CloseHandle (h);
    h = p2[1];
    if (!DuplicateHandle (
        GetCurrentProcess(), h, GetCurrentProcess(), &p2[1], 0, FALSE,
        DUPLICATE_SAME_ACCESS
    ))
        panic1 (POS, "pipeopen", "cannot dup output handle");
    CloseHandle (h);
    sinfo.cb = sizeof (STARTUPINFO);
    sinfo.lpReserved = NULL;
    sinfo.lpDesktop = NULL;
    sinfo.lpTitle = NULL;
    sinfo.cbReserved2 = 0;
    sinfo.lpReserved2 = NULL;
    sinfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    sinfo.hStdInput = p2[0];
    sinfo.hStdOutput = p1[1];
    sinfo.wShowWindow = SW_HIDE;
    if (!CreateProcess (
        NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &sinfo, &pinfo
    ))
        panic1 (POS, "pipeopen", "cannot create child process");
    *pidp = pinfo.hProcess;
    if (!SetStdHandle (STD_INPUT_HANDLE, save[0]))
        panic1 (POS, "pipeopen", "cannot restore stdin");
    if (!SetStdHandle (STD_OUTPUT_HANDLE, save[1]))
        panic1 (POS, "pipeopen", "cannot restore stdout");
    CloseHandle (p1[1]);
    CloseHandle (p2[0]);
    *ifp = p1[0], *ofp = p2[1];
    return;
}
