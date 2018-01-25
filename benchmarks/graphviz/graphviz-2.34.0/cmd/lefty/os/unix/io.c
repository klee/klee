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
#include "io.h"
#ifdef FEATURE_CS
#include "cs2l.h"
#endif
#include "mem.h"
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#ifndef HAVE_TERMIOS_H
#include <termio.h>
#else
#include <termios.h>
#endif
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef FEATURE_VFORK
#define FORK vfork
#else
#define FORK fork
#endif
#include <netdb.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

io_t *iop;
int ion;

static char *shell;
static char *shbname;

static FILE *serverconnect (char *);
static void ptyopen (char *, FILE **, FILE **, int *);
static int findpty (int *);
static void pipeopen (char *, FILE **, FILE **, int *);
static void socketopen (char *, int, FILE **, FILE **, int *);

static void sigchldhandler (int);

void IOinit (void) {
    struct stat statbuf;
    int ioi;

    if (!(shell = getenv ("SHELL")))
        shell = "/bin/sh";
    if (shell[0] != '/' && shell[0] != '.') {
        if (!(shell = buildpath (shell, TRUE)))
            shell = "/bin/sh";
        else
            shell = strdup (shell);
    }
    shbname = shell + strlen (shell) - 1;
    while (shbname >= shell && *shbname != '/')
        shbname--;
    if (*shbname == '/')
        shbname++;
    ion = IOINCR;
    for (ioi = FD_SETSIZE - 1; ioi >= 0; ioi--) {
        if (fstat (ioi, &statbuf) == 0) {
            ion = (ioi / IOINCR + 1) * IOINCR;
            break;
        }
    }
    iop = Marrayalloc ((long) ion * IOSIZE);
    for (ioi = 0; ioi < ion; ioi++)
        iop[ioi].inuse = FALSE;
    for (ioi = 0; ioi < ion; ioi++) {
        if (fstat (ioi, &statbuf) == 0) {
            if ((iop[ioi].ifp = iop[ioi].ofp = fdopen (
                ioi, (ioi == 0 ? "r" : "w")
            ))) {
                iop[ioi].inuse = TRUE;
                iop[ioi].type = IO_FILE;
                iop[ioi].ismonitored = FALSE;
                iop[ioi].pid = -1;
                iop[ioi].buf = NULL;
            }
        }
    }
    signal (SIGCHLD, sigchldhandler);
    signal (SIGPIPE, SIG_IGN);
}

void IOterm (void) {
    int ioi;

    for (ioi = 3; ioi < ion; ioi++)
        if (iop[ioi].inuse)
            IOclose (ioi, NULL);
    Marrayfree (iop), iop = NULL, ion = 0;
}

int IOopen (char *kind, char *name, char *mode, char *fmt) {
    io_t *p;
    int type;
    char *path, *command;
    char hname[200];
    int sfd;
    unsigned int slen;
    int i;
    struct sockaddr_in sname;

    if (strcmp (kind, "file") == 0)
        type = IO_FILE;
    else if (strcmp (kind, "pty") == 0)
        type = IO_PTY;
    else if (strcmp (kind, "pipe") == 0)
        type = IO_PIPE;
    else if (strcmp (kind, "socket") == 0)
        type = IO_SOCKET;
#ifdef FEATURE_CS
    else if (strcmp (kind, "cs") == 0)
        type = IO_CS;
#endif
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
    p->pid = -1;
    switch (type) {
    case IO_FILE:
        if (!(p->ifp = p->ofp = fopen (name, mode))) {
            if (strncmp (name, "/dev/tcp/", 9) == 0) {
                if (!(p->ifp = p->ofp = serverconnect (name)))
                    return -1;
                break;
            }
            path = buildpath (name, FALSE);
            if (!path || !(p->ifp = p->ofp = fopen (path, mode)))
                return -1;
        }
        break;
    case IO_PTY:
        if (!fmt)
            fmt = "%e";
        if (
            !(path = buildpath (name, TRUE)) ||
            !(command = buildcommand (path, NULL, -1, -1, fmt))
        )
            return -1;
        ptyopen (command, &p->ifp, &p->ofp, &p->pid);
        if (!p->ifp || !p->ofp)
            return -1;
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
    case IO_SOCKET:
        if (!fmt)
            fmt = "%e";
        if ((sfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
            return -1;
        sname.sin_family = AF_INET;
        sname.sin_port = 0;
        sname.sin_addr.s_addr = htonl (INADDR_ANY);
        slen = sizeof (sname);
        if (
            bind (sfd, (struct sockaddr *) &sname, slen) < 0 ||
            getsockname (sfd, (struct sockaddr *) &sname, &slen) < 0
        )
            return -1;
        if (listen (sfd, 5) < 0)
            return -1;
        gethostname (hname, sizeof (hname));
        if (!(path = buildpath (name, TRUE)) || !(command = buildcommand (
            path, hname, (int) ntohs (sname.sin_port),
            (int) ntohs (sname.sin_port), fmt
        )))
            return -1;
        socketopen (command, sfd, &p->ifp, &p->ofp, &p->pid);
        if (!p->ifp || !p->ofp)
            return -1;
        close (sfd);
        break;
#ifdef FEATURE_CS
    case IO_CS:
        if (C2Lopen (name, mode, &p->ifp, &p->ofp) == -1)
            return -1;
        break;
#endif
    }
    p->inuse = TRUE;
    FD_CLR (fileno (p->ifp), &inputfds);
    FD_CLR (fileno (p->ofp), &inputfds);
    return i;
}

int IOclose (int ioi, char *action) {
    io_t *p;

    if (ioi < 0 || ioi >= ion || !iop[ioi].inuse)
        return -1;
    p = &iop[ioi];
    FD_CLR (fileno (p->ifp), &inputfds);
    FD_CLR (fileno (p->ofp), &inputfds);
    if (p->ifp != p->ofp)
        fclose (p->ifp);
    fclose (p->ofp);
    p->inuse = FALSE;
    if (action && strcmp (action, "kill") == 0 && p->pid != -1)
        kill (p->pid, 15);
    return 0;
}

int IOreadline (int ioi, char *bufp, int bufn) {
    io_t *p;
    int l;

    if (ioi < 0 || ioi >= ion || !iop[ioi].inuse)
        return -1;
    p = &iop[ioi];
    fseek (p->ofp, 0L, 1);
    if (fgets (bufp, bufn, p->ifp) == NULL)
        return -1;
    l = strlen (bufp) - 1;
    while (bufp[l] == '\n' || bufp[l] == '\r')
        bufp[l--] = '\000';
    return l + 1;
}

int IOread (int ioi, char *bufp, int bufn) {
    io_t *p;
    int l;

    if (ioi < 0 || ioi >= ion || !iop[ioi].inuse)
        return -1;
    p = &iop[ioi];
    if ((l = read (fileno (p->ifp), bufp, bufn - 1)) == -1)
        return -1;
    else if (l == 0)
        return 0;
    bufp[l] = '\000';
    return l;
}

int IOwriteline (int ioi, char *bufp) {
    io_t *p;

    if (ioi < 0 || ioi >= ion || !iop[ioi].inuse)
        return -1;
    p = &iop[ioi];
    fseek (p->ofp, 0L, 1);
    if (fputs (bufp, p->ofp) == EOF || fputs ("\n", p->ofp) == EOF)
        return -1;
    fflush (p->ofp);
    fseek (p->ofp, 0L, 1);
    return 0;
}

static FILE *serverconnect (char *name) {
    char *host, *portp, buf[1024];
    int port;
    struct hostent *hp;
    struct sockaddr_in sin;
    int cfd;

    strcpy (buf, name);
    host = buf + 9;
    portp = strchr (host, '/');
    if (*host == 0 || !portp)
        return NULL;
    *portp++ = 0, port = atoi (portp);
    if (!(hp = gethostbyname (host)))
        return NULL;
    memset ((char *) &sin, 1, sizeof (sin));
    memcpy ((char *) &sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons (port);
    if ((cfd = socket (hp->h_addrtype, SOCK_STREAM, 0)) < 0)
        return NULL;
    if (connect (cfd, (struct sockaddr *) &sin, sizeof (sin)) < 0)
        return NULL;
    return fdopen (cfd, "w+");
}

static void ptyopen (char *cmd, FILE **ifp, FILE **ofp, int *pidp) {
    int fd[2];

    if (findpty (fd) == -1) {
        *ifp = NULL;
        return;
    }
    switch ((*pidp = FORK ())) {
    case -1:
        panic2 (POS, "ptyopen", "cannot fork");
    case 0:
        close (fd[0]), close (0), dup (fd[1]);
        close (1), dup (fd[1]), close (fd[1]);
        execl (shell, shbname, "-c", cmd, NULL);
        panic2 (POS, "ptyopen", "child cannot exec: %s\n", cmd);
    default:
        close (fd[1]);
    }
    fcntl (fd[0], F_SETFD, FD_CLOEXEC);
    *ifp = fdopen (fd[0], "r"), *ofp = fdopen (fd[0], "a+");
    return;
}

static int findpty (int *fd) {
    char *majorp, *minorp;
    char pty[32], tty[32];
#ifndef HAVE_TERMIOS_H
    struct termio tio;
#else
    struct termios tio;
#endif
    static char ptymajor[] = "pqrs";
    static char ptyminor[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    for (majorp = ptymajor; *majorp; majorp++) {
        for (minorp = ptyminor; *minorp; minorp++) {
            sprintf (pty, "/dev/pty%c%c", *majorp, *minorp);
            if ((fd[0] = open (pty, O_RDWR)) >= 0) {
                sprintf (tty, "/dev/tty%c%c", *majorp, *minorp);
                if ((fd[1] = open (tty, O_RDWR)) >= 0) {
#ifndef HAVE_TERMIOS_H
                    ioctl (fd[1], TCGETA, &tio);
                    tio.c_lflag &= ~ECHO;
                    ioctl (fd[1], TCSETA, &tio);
#else
                    tcgetattr (fd[1], &tio);
                    tio.c_lflag &= ~ECHO;
                    tcsetattr (fd[1], TCSANOW, &tio);
#endif
                    return 0;
                }
                close (fd[0]);
            }
        }
    }
    return -1;
}

static void pipeopen (char *cmd, FILE **ifp, FILE **ofp, int *pidp) {
    int p1[2], p2[2];
    char cmd2[1024];
    char *s;

    if (pipe (p1) == -1 || pipe (p2) == -1) {
        *ifp = NULL;
        return;
    }
    switch ((*pidp = FORK ())) {
    case -1:
        panic2 (POS, "pipeopen", "cannot fork");
    case 0:
        close (p1[0]), close (p2[1]);
        for (s = cmd; *s; s++)
            if (*s == '%' && *(s + 1) && *(s + 1) == 'd') {
                sprintf (cmd2, cmd, p2[0], p1[1]);
                execl (shell, shbname, "-c", cmd2, NULL);
                panic2 (POS, "pipeopen", "child cannot exec: %s\n", cmd2);
            }
        close (1), dup (p1[1]), close (p1[1]);
        close (0), dup (p2[0]), close (p2[0]);
        execl (shell, shbname, "-c", cmd, NULL);
        panic2 (POS, "pipeopen", "child cannot exec: %s\n", cmd);
    default:
        close (p1[1]), close (p2[0]);
    }
    fcntl (p1[0], F_SETFD, FD_CLOEXEC);
    fcntl (p2[1], F_SETFD, FD_CLOEXEC);
    *ifp = fdopen (p1[0], "r"), *ofp = fdopen (p2[1], "a");
    return;
}

static void socketopen (char *cmd, int sfd, FILE **ifp, FILE **ofp, int *pidp) {
    int fd;

    switch ((*pidp = FORK ())) {
    case -1:
        panic2 (POS, "socketopen", "cannot fork");
    case 0:
        execl (shell, shbname, "-c", cmd, NULL);
        panic2 (POS, "socketopen", "child cannot exec: %s\n", cmd);
    default:
        if ((fd = accept (sfd, NULL, NULL)) < 0) {
            *ifp = NULL;
            return;
        }
    }
    fcntl (fd, F_SETFD, FD_CLOEXEC);
    *ifp = fdopen (fd, "r"), *ofp = fdopen (fd, "a+");
    return;
}

static void sigchldhandler (int data) {
    while (waitpid (-1, NULL, WNOHANG) > 0)
        ;
    signal (SIGCHLD, sigchldhandler);
}
