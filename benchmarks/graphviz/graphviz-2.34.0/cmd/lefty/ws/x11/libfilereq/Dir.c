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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>

#ifdef SEL_FILE_IGNORE_CASE
#include <ctype.h>
#endif /* def SEL_FILE_IGNORE_CASE */

#include "SFinternal.h"

#if HAVE_DIRENT_H
#include <dirent.h>
#define DIRENT_DONE
#else
#define dirent direct
#  if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
#  define DIRENT_DONE
#  endif
#  if HAVE_NDIR_H
#  include <ndir.h>
#  define DIRENT_DONE
#  endif
#endif

#ifndef DIRENT_DONE
#if defined (SVR4) || defined (SYSV) || defined (USG) || defined (__osf__) || defined (__svr4__) || defined (__FreeBSD__) || defined (SCO)
#include <dirent.h>
#else /* defined (SVR4) || defined (SYSV) || defined (USG) */
#include <sys/dir.h>
#define dirent direct
#endif /* defined (SVR4) || defined (SYSV) || defined (USG) */
#endif

#include <sys/stat.h>

#if defined (SVR4) || defined (SYSV) || defined (USG)
extern void qsort ();
#endif /* defined (SVR4) || defined (SYSV) || defined (USG) */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#include "SFDecls.h"

#ifdef SEL_FILE_IGNORE_CASE
int SFcompareEntries (const void *vp, const void *vq) {
    SFEntry *p = (SFEntry *) vp, *q = (SFEntry *) vq;
    char *r, *s;
    char c1, c2;

    r = p->real;
    s = q->real;
    c1 = *r++;
    if (islower (c1)) {
        c1 = toupper (c1);
    }
    c2 = *s++;
    if (islower (c2)) {
        c2 = toupper (c2);
    }
    while (c1 == c2) {
        if (!c1) {
            return strcmp (p->real, q->real);
        }
        c1 = *r++;
        if (islower (c1)) {
            c1 = toupper (c1);
        }
        c2 = *s++;
        if (islower (c2)) {
            c2 = toupper (c2);
        }
    }
    return c1 - c2;
}
#else /* def SEL_FILE_IGNORE_CASE */
int SFcompareEntries (const void *vp, const void *vq) {
    SFEntry *p = (SFEntry *) vp, *q = (SFEntry *) vq;

    return strcmp (p->real, q->real);
}
#endif /* def SEL_FILE_IGNORE_CASE */

int SFgetDir (SFDir *dir) {
    SFEntry       *result = NULL;
    int           alloc = 0;
    int           i;
    DIR           *dirp;
    struct dirent *dp;
    char          *str;
    int           len;
    int           maxChars;
    struct stat   statBuf;

    maxChars = strlen (dir->dir) - 1;
    dir->entries = NULL;
    dir->nEntries = 0;
    dir->nChars = 0;
    result = NULL;
    i = 0;
    dirp = opendir (".");
    if (!dirp) {
        return 1;
    }
    stat (".", &statBuf);
    dir->mtime = statBuf.st_mtime;
    readdir (dirp);    /* throw away "." */

#ifndef S_IFLNK
    readdir (dirp);    /* throw away ".." */
#endif /* ndef S_IFLNK */

    while ((dp = readdir (dirp))) {
        if (i >= alloc) {
            alloc = 2 * (alloc + 1);
            result = (SFEntry *) XtRealloc (
                (char *) result, (unsigned) (alloc * sizeof (SFEntry))
            );
        }
        result[i].statDone = 0;
        str = dp->d_name;
        len = strlen (str);
        result[i].real = XtMalloc ((unsigned) (len + 2));
        strcat (strcpy (result[i].real, str), " ");
        if (len > maxChars) {
            maxChars = len;
        }
        result[i].shown = result[i].real;
        i++;
    }

#if defined (SVR4) || defined (SYSV) || defined (USG)
    qsort ((char *) result, (unsigned) i, sizeof (SFEntry), SFcompareEntries);
#else /* defined (SVR4) || defined (SYSV) || defined (USG) */
    qsort ((char *) result, i, sizeof (SFEntry), SFcompareEntries);
#endif /* defined (SVR4) || defined (SYSV) || defined (USG) */

    dir->entries = result;
    dir->nEntries = i;
    dir->nChars = maxChars + 1;
    closedir (dirp);
    return 0;
}
