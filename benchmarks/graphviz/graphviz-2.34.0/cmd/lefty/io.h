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

#ifdef __cplusplus
extern "C" {
#endif

/* Lefteris Koutsofios - AT&T Labs Research */

#ifndef _IO_H
#define _IO_H

#define IO_FILE   0
#define IO_PTY    1
#define IO_PIPE   2
#define IO_SOCKET 3
#ifdef FEATURE_CS
#define IO_CS     4
#define IO_SIZE   5
#else
#define IO_SIZE   4
#endif

typedef struct io_t {
    int inuse, ismonitored;
    int type;
    FILE *ifp, *ofp;
    int pid;
    char *buf;
} io_t;

#ifdef FEATURE_MS
#define IOmonitor(ioi, set) do { \
    iop[ioi].ismonitored = TRUE; \
} while (0)
#define IOunmonitor(ioi, set) do { \
    iop[ioi].ismonitored = FALSE; \
} while (0)
#else
#define IOmonitor(ioi, set) do { \
    iop[ioi].ismonitored = TRUE; \
    FD_SET (fileno (iop[ioi].ifp), &set); \
} while (0)
#define IOunmonitor(ioi, set) do { \
    iop[ioi].ismonitored = FALSE; \
    FD_CLR (fileno (iop[ioi].ifp), &set); \
} while (0)
#endif

#define IOismonitored(ioi) (iop[ioi].ismonitored == TRUE)
#define IOINCR 5
#define IOSIZE sizeof (io_t)
#define IOBUFSIZE 2048

extern io_t *iop;
extern int ion;

void IOinit (void);
void IOterm (void);
int IOopen (char *, char *, char *, char *);
int IOclose (int, char *);
int IOreadline (int, char *, int);
int IOread (int, char *, int);
int IOwriteline (int, char *);

#endif /* _IO_H */

#ifdef __cplusplus
}
#endif

