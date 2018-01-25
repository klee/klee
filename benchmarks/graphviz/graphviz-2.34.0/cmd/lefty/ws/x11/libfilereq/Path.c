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

#include <X11/Xos.h>
#include <pwd.h>
#include "SFinternal.h"
#include "xstat.h"
#include <X11/Xaw/Scrollbar.h>

#if defined (SVR4) || defined (SYSV) || defined (USG)
extern uid_t getuid ();
extern void qsort ();
#endif /* defined (SVR4) || defined (SYSV) || defined (USG) */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "SFDecls.h"

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

typedef struct {
    char *name;
    char *dir;
} SFLogin;

SFDir *SFdirs = NULL;

int SFdirEnd;

int SFdirPtr;

int SFbuttonPressed = 0;

static int SFdoNotTouchDirPtr = 0;

static int SFdoNotTouchVorigin = 0;

static SFDir SFrootDir, SFhomeDir;

static SFLogin *SFlogins;

static int SFtwiddle = 0;

void SFsetText (char *path);

int SFchdir (char *path) {
    int result;

    result = 0;
    if (strcmp (path, SFcurrentDir)) {
        result = chdir (path);
        if (!result) {
            strcpy (SFcurrentDir, path);
        }
    }
    return result;
}

static void SFfree (int i) {
    SFDir *dir;
    int   j;

    dir = &(SFdirs[i]);
    for (j = dir->nEntries - 1; j >= 0; j--) {
        if (dir->entries[j].shown != dir->entries[j].real) {
            XtFree (dir->entries[j].shown);
        }
        XtFree (dir->entries[j].real);
    }
    XtFree ((char *) dir->entries);
    XtFree (dir->dir);
    dir->dir = NULL;
}

static void SFstrdup (char **s1, char *s2) {
    *s1 = strcpy (XtMalloc ((unsigned) (strlen (s2) + 1)), s2);
}

static void SFunreadableDir (SFDir *dir) {
    char *cannotOpen = "<cannot open> ";

    dir->entries = (SFEntry *) XtMalloc (sizeof (SFEntry));
    dir->entries[0].statDone = 1;
    SFstrdup (&dir->entries[0].real, cannotOpen);
    dir->entries[0].shown = dir->entries[0].real;
    dir->nEntries = 1;
    dir->nChars = strlen (cannotOpen);
}

#ifdef SEL_FILE_IGNORE_CASE
static int SFstrncmp (char *p, char *q, int n) {
    char c1, c2;
    char *psave, *qsave;
    int  nsave;

    psave = p;
    qsave = q;
    nsave = n;
    c1 = *p++;
    if (islower (c1)) {
        c1 = toupper (c1);
    }
    c2 = *q++;
    if (islower (c2)) {
        c2 = toupper (c2);
    }
    while ((--n >= 0) && (c1 == c2)) {
        if (!c1) {
            return strncmp (psave, qsave, nsave);
        }
        c1 = *p++;
        if (islower (c1)) {
            c1 = toupper (c1);
        }
        c2 = *q++;
        if (islower (c2)) {
            c2 = toupper (c2);
        }
    }
    if (n < 0) {
        return strncmp (psave, qsave, nsave);
    }
    return c1 - c2;
}
#endif /* def SEL_FILE_IGNORE_CASE */

static void SFreplaceText (SFDir *dir, char *str) {
    int len;

    *(dir->path) = 0;
    len = strlen (str);
    if (str[len - 1] == '/') {
        strcat (SFcurrentPath, str);
    } else {
        strncat (SFcurrentPath, str, len - 1);
    }
    if (strncmp (SFcurrentPath, SFstartDir, strlen (SFstartDir))) {
        SFsetText (SFcurrentPath);
    } else {
        SFsetText (& (SFcurrentPath[strlen (SFstartDir)]));
    }
    SFtextChanged ();
}

static void SFexpand (char *str) {
    int     len;
    int     cmp;
    char    *name, *growing;
    SFDir   *dir;
    SFEntry *entry, *max;

    len = strlen (str);
    dir = &(SFdirs[SFdirEnd - 1]);
    if (dir->beginSelection == -1) {
        SFstrdup (&str, str);
        SFreplaceText (dir, str);
        XtFree (str);
        return;
    } else if (dir->beginSelection == dir->endSelection) {
        SFreplaceText (dir, dir->entries[dir->beginSelection].shown);
        return;
    }
    max = &(dir->entries[dir->endSelection + 1]);
    name = dir->entries[dir->beginSelection].shown;
    SFstrdup (&growing, name);
    cmp = 0;
    while (!cmp) {
        entry = & (dir->entries[dir->beginSelection]);
        while (entry < max) {
            if ((cmp = strncmp (growing, entry->shown, len))) {
                break;
            }
            entry++;
        }
        len++;
    }

    /* SFreplaceText () expects filename */
    growing[len - 2] = ' ';
    growing[len - 1] = 0;
    SFreplaceText (dir, growing);
    XtFree (growing);
}

static int SFfindFile (SFDir *dir, char *str) {
    int     i, last, max;
    char    *name, save;
    SFEntry *entries;
    int     len;
    int     begin, end;
    int     result;

    len = strlen (str);
    if (str[len - 1] == ' ') {
        SFexpand (str);
        return 1;
    } else if (str[len - 1] == '/') {
        len--;
    }
    max = dir->nEntries;
    entries = dir->entries;
    i = 0;
    while (i < max) {
        name = entries[i].shown;
        last = strlen (name) - 1;
        save = name[last];
        name[last] = 0;

#ifdef SEL_FILE_IGNORE_CASE
        result = SFstrncmp (str, name, len);
#else /* def SEL_FILE_IGNORE_CASE */
        result = strncmp (str, name, len);
#endif /* def SEL_FILE_IGNORE_CASE */

        name[last] = save;
        if (result <= 0) {
            break;
        }
        i++;
    }
    begin = i;
    while (i < max) {
        name = entries[i].shown;
        last = strlen (name) - 1;
        save = name[last];
        name[last] = 0;

#ifdef SEL_FILE_IGNORE_CASE
        result = SFstrncmp (str, name, len);
#else /* def SEL_FILE_IGNORE_CASE */
        result = strncmp (str, name, len);
#endif /* def SEL_FILE_IGNORE_CASE */

        name[last] = save;
        if (result) {
            break;
        }
        i++;
    }
    end = i;

    if (begin != end) {
        if ((dir->beginSelection != begin) || (dir->endSelection != end - 1)) {
            dir->changed = 1;
            dir->beginSelection = begin;
            if (str[strlen (str) - 1] == '/') {
                dir->endSelection = begin;
            } else {
                dir->endSelection = end - 1;
            }
        }
    } else {
        if (dir->beginSelection != -1) {
            dir->changed = 1;
            dir->beginSelection = -1;
            dir->endSelection = -1;
        }
    }
    if (
        SFdoNotTouchVorigin ||
        ((begin > dir->vOrigin) && (end < dir->vOrigin + SFlistSize))
    ) {
        SFdoNotTouchVorigin = 0;
        return 0;
    }
    i = begin - 1;
    if (i > max - SFlistSize) {
        i = max - SFlistSize;
    }
    if (i < 0) {
        i = 0;
    }
    if (dir->vOrigin != i) {
        dir->vOrigin = i;
        dir->changed = 1;
    }
    return 0;
}

static void SFunselect (void) {
    SFDir *dir;

    dir = &(SFdirs[SFdirEnd - 1]);
    if (dir->beginSelection != -1) {
        dir->changed = 1;
    }
    dir->beginSelection = -1;
    dir->endSelection = -1;
}

static int SFcompareLogins (const void *vp, const void *vq) {
    SFLogin *p = (SFLogin *) vp, *q = (SFLogin *) vq;

    return strcmp (p->name, q->name);
}

static void SFgetHomeDirs (void) {
    struct passwd *pw;
    int           alloc;
    int           i;
    SFEntry       *entries = NULL;
    int           len;
    int           maxChars;

    alloc = 1;
    i = 1;
    entries = (SFEntry *) XtMalloc (sizeof (SFEntry));
    SFlogins = (SFLogin *) XtMalloc (sizeof (SFLogin));
    entries[0].real = XtMalloc (3);
    strcpy (entries[0].real, "~");
    entries[0].shown = entries[0].real;
    entries[0].statDone = 1;
    SFlogins[0].name = "";
    pw = getpwuid ((int) getuid ());
    SFstrdup (&SFlogins[0].dir, pw ? pw->pw_dir : "/");
    maxChars = 0;

    setpwent ();
    while ((pw = getpwent ()) && (*(pw->pw_name))) {
        if (i >= alloc) {
            alloc *= 2;
            entries = (SFEntry *) XtRealloc (
                (char *) entries, (unsigned) (alloc * sizeof (SFEntry))
            );
            SFlogins = (SFLogin *) XtRealloc (
                (char *) SFlogins, (unsigned) (alloc * sizeof (SFLogin))
            );
        }
        len = strlen (pw->pw_name);
        entries[i].real = XtMalloc ((unsigned) (len + 3));
        strcat (strcpy (entries[i].real, "~"), pw->pw_name);
        entries[i].shown = entries[i].real;
        entries[i].statDone = 1;
        if (len > maxChars) {
            maxChars = len;
        }
        SFstrdup (&SFlogins[i].name, pw->pw_name);
        SFstrdup (&SFlogins[i].dir, pw->pw_dir);
        i++;
    }
    SFhomeDir.dir            = XtMalloc (1);
    SFhomeDir.dir[0]         = 0;
    SFhomeDir.path           = SFcurrentPath;
    SFhomeDir.entries        = entries;
    SFhomeDir.nEntries       = i;
    SFhomeDir.vOrigin        = 0;  /* :-) */
    SFhomeDir.nChars         = maxChars + 2;
    SFhomeDir.hOrigin        = 0;
    SFhomeDir.changed        = 1;
    SFhomeDir.beginSelection = -1;
    SFhomeDir.endSelection   = -1;

#if defined (SVR4) || defined (SYSV) || defined (USG)
    qsort ((char *) entries, (unsigned)i, sizeof (SFEntry), SFcompareEntries);
    qsort ((char *) SFlogins, (unsigned)i, sizeof (SFLogin), SFcompareLogins);
#else /* defined (SVR4) || defined (SYSV) || defined (USG) */
    qsort ((char *) entries, i, sizeof (SFEntry), SFcompareEntries);
    qsort ((char *) SFlogins, i, sizeof (SFLogin), SFcompareLogins);
#endif /* defined (SVR4) || defined (SYSV) || defined (USG) */

    for (i--; i >= 0; i--) {
        strcat (entries[i].real, "/");
    }
}

static int SFfindHomeDir (char *begin, char *end) {
    char save;
    char *theRest;
    int  i;

    save = *end;
    *end = 0;
    for (i = SFhomeDir.nEntries - 1; i >= 0; i--) {
        if (!strcmp (SFhomeDir.entries[i].real, begin)) {
            *end = save;
            SFstrdup (&theRest, end);
            strcat (strcat (strcpy (
                SFcurrentPath, SFlogins[i].dir
            ), "/"), theRest);
            XtFree (theRest);
            SFsetText (SFcurrentPath);
            SFtextChanged ();
            return 1;
        }
    }
    *end = save;
    return 0;
}

void SFupdatePath (void) {
    static int alloc;
    static int wasTwiddle = 0;
    char       *begin, *end;
    int        i, j;
    int        prevChange;
    int        SFdirPtrSave, SFdirEndSave;
    SFDir      *dir;

    if (!SFdirs) {
        SFdirs = (SFDir *) XtMalloc ((alloc = 10) * sizeof (SFDir));
        dir = &(SFdirs[0]);
        SFstrdup (&dir->dir, "/");
        SFchdir ("/");
        SFgetDir (dir);
        for (j = 1; j < alloc; j++) {
            SFdirs[j].dir = NULL;
        }
        dir->path = SFcurrentPath + 1;
        dir->vOrigin = 0;
        dir->hOrigin = 0;
        dir->changed = 1;
        dir->beginSelection = -1;
        dir->endSelection = -1;
        SFhomeDir.dir = NULL;
    }
    SFdirEndSave = SFdirEnd;
    SFdirEnd = 1;
    SFdirPtrSave = SFdirPtr;
    SFdirPtr = 0;
    begin = NULL;
    if (SFcurrentPath[0] == '~') {
        if (!SFtwiddle) {
            SFtwiddle = 1;
            dir = & (SFdirs[0]);
            SFrootDir = *dir;
            if (!SFhomeDir.dir) {
                SFgetHomeDirs ();
            }
            *dir = SFhomeDir;
            dir->changed = 1;
        }
        end = SFcurrentPath;
        SFdoNotTouchDirPtr = 1;
        wasTwiddle = 1;
    } else {
        if (SFtwiddle) {
            SFtwiddle = 0;
            dir = & (SFdirs[0]);
            *dir = SFrootDir;
            dir->changed = 1;
        }
        end = SFcurrentPath + 1;
    }
    i = 0;
    prevChange = 0;
    while (*end) {
        while (*end++ == '/') {
            ;
        }
        end--;
        begin = end;
        while ((*end) && (*end++ != '/')) {
            ;
        }
        if ((end - SFcurrentPath <= SFtextPos) && (* (end - 1) == '/')) {
            SFdirPtr = i - 1;
            if (SFdirPtr < 0) {
                SFdirPtr = 0;
            }
        }
        if (*begin) {
            if (*(end - 1) == '/') {
                char save = *end;

                if (SFtwiddle) {
                    if (SFfindHomeDir (begin, end)) {
                        return;
                    }
                }
                *end = 0;
                i++;
                SFdirEnd++;
                if (i >= alloc) {
                    SFdirs = (SFDir *) XtRealloc (
                        (char *) SFdirs,
                        (unsigned) ((alloc *= 2) * sizeof (SFDir))
                    );
                    for (j = alloc / 2; j < alloc; j++) {
                        SFdirs[j].dir = NULL;
                    }
                }
                dir = &(SFdirs[i]);
                if ((!(dir->dir)) || prevChange || strcmp (dir->dir, begin)) {
                    if (dir->dir) {
                        SFfree (i);
                    }
                    prevChange = 1;
                    SFstrdup (&dir->dir, begin);
                    dir->path = end;
                    dir->vOrigin = 0;
                    dir->hOrigin = 0;
                    dir->changed = 1;
                    dir->beginSelection = -1;
                    dir->endSelection = -1;
                    SFfindFile (dir - 1, begin);
                    if (SFchdir (SFcurrentPath) || SFgetDir (dir)) {
                        SFunreadableDir (dir);
                        break;
                    }
                }
                *end = save;
                if (!save) {
                    SFunselect ();
                }
            } else {
                if (SFfindFile (& (SFdirs[SFdirEnd-1]), begin)) {
                    return;
                }
            }
        } else {
            SFunselect ();
        }
    }
    if ((end == SFcurrentPath + 1) && (!SFtwiddle)) {
        SFunselect ();
    }
    for (i = SFdirEnd; i < alloc; i++) {
        if (SFdirs[i].dir) {
            SFfree (i);
        }
    }
    if (SFdoNotTouchDirPtr) {
        if (wasTwiddle) {
            wasTwiddle = 0;
            SFdirPtr = SFdirEnd - 2;
            if (SFdirPtr < 0) {
                SFdirPtr = 0;
            }
        } else {
            SFdirPtr = SFdirPtrSave;
        }
        SFdoNotTouchDirPtr = 0;
    }
    if ((SFdirPtr != SFdirPtrSave) || (SFdirEnd != SFdirEndSave)) {
        XawScrollbarSetThumb (
            selFileHScroll,
            (float) (((double) SFdirPtr) / SFdirEnd),
            (float) (((double) ((SFdirEnd < 3) ? SFdirEnd : 3)) / SFdirEnd)
        );
    }
    if (SFdirPtr != SFdirPtrSave) {
        SFdrawLists (SF_DO_SCROLL);
    } else {
        for (i = 0; i < 3; i++) {
            if (SFdirPtr + i < SFdirEnd) {
                if (SFdirs[SFdirPtr + i].changed) {
                    SFdirs[SFdirPtr + i].changed = 0;
                    SFdrawList (i, SF_DO_SCROLL);
                }
            } else {
                SFclearList (i, SF_DO_SCROLL);
            }
        }
    }
}

void SFsetText (char *path) {
    XawTextBlock text;

    text.firstPos = 0;
    text.length = strlen (path);
    text.ptr = path;
    text.format = FMT8BIT;
    XawTextReplace (selFileField, 0, strlen (SFtextBuffer), &text);
    XawTextSetInsertionPoint (selFileField, strlen (SFtextBuffer));
}


void SFbuttonPressList (Widget w, XtPointer cl, XEvent *ev, Boolean *b) {
    SFbuttonPressed = 1;
}

void SFbuttonReleaseList (Widget w, XtPointer cl, XEvent *ev, Boolean *b) {
    SFDir *dir;
    intptr_t n;
    XButtonReleasedEvent *event;

    n = (intptr_t) cl;
    event = (XButtonReleasedEvent *) ev;
    SFbuttonPressed = 0;
    if (SFcurrentInvert[n] != -1) {
        if (n < 2) {
            SFdoNotTouchDirPtr = 1;
        }
        SFdoNotTouchVorigin = 1;
        dir = &(SFdirs[SFdirPtr + n]);
        SFreplaceText (
            dir, dir->entries[dir->vOrigin + SFcurrentInvert[n]].shown
        );
        SFmotionList (w, (XtPointer) n, (XEvent *) event, NULL);
    }
}

static int SFcheckDir (int n, SFDir *dir) {
    struct stat statBuf;
    int         i;

    if ((!stat (".", &statBuf)) && (statBuf.st_mtime != dir->mtime)) {
        /*  If the pointer is currently in the window that we are about
            to update, we must warp it to prevent the user from
            accidentally selecting the wrong file.
        */
        if (SFcurrentInvert[n] != -1) {
            XWarpPointer (
                SFdisplay, None, XtWindow (selFileLists[n]), 0, 0, 0, 0, 0, 0
            );
        }
        for (i = dir->nEntries - 1; i >= 0; i--) {
            if (dir->entries[i].shown != dir->entries[i].real) {
                XtFree (dir->entries[i].shown);
            }
            XtFree (dir->entries[i].real);
        }
        XtFree ((char *) dir->entries);
        if (SFgetDir (dir)) {
            SFunreadableDir (dir);
        }
        if (dir->vOrigin > dir->nEntries - SFlistSize) {
            dir->vOrigin = dir->nEntries - SFlistSize;
        }
        if (dir->vOrigin < 0) {
            dir->vOrigin = 0;
        }
        if (dir->hOrigin > dir->nChars - SFcharsPerEntry) {
            dir->hOrigin = dir->nChars - SFcharsPerEntry;
        }
        if (dir->hOrigin < 0) {
            dir->hOrigin = 0;
        }
        dir->beginSelection = -1;
        dir->endSelection = -1;
        SFdoNotTouchVorigin = 1;
        if ((dir + 1)->dir) {
            SFfindFile (dir, (dir + 1)->dir);
        } else {
            SFfindFile (dir, dir->path);
        }
        if (!SFworkProcAdded) {
            SFworkProcId = XtAppAddWorkProc (SFapp, SFworkProc, NULL);
            SFworkProcAdded = 1;
        }
        return 1;
    }
    return 0;
}

static int SFcheckFiles (SFDir *dir) {
    int         from, to;
    int         result;
    char        old, new;
    int         i;
    char        *str;
    int         last;
    struct stat statBuf;

    result = 0;
    from = dir->vOrigin;
    to = dir->vOrigin + SFlistSize;
    if (to > dir->nEntries) {
        to = dir->nEntries;
    }
    for (i = from; i < to; i++) {
        str = dir->entries[i].real;
        last = strlen (str) - 1;
        old = str[last];
        str[last] = 0;
        if (stat (str, &statBuf)) {
            new = ' ';
        } else {
            new = SFstatChar (&statBuf);
        }
        str[last] = new;
        if (new != old) {
            result = 1;
        }
    }
    return result;
}

void SFdirModTimer (XtPointer cl, XtIntervalId *id) {
    static int n = -1;
    static int f = 0;
    char       save;
    SFDir      *dir;

    if ((!SFtwiddle) && (SFdirPtr < SFdirEnd)) {
        n++;
        if ((n > 2) || (SFdirPtr + n >= SFdirEnd)) {
            n = 0;
            f++;
            if ((f > 2) || (SFdirPtr + f >= SFdirEnd)) {
                f = 0;
            }
        }
        dir = & (SFdirs[SFdirPtr + n]);
        save = * (dir->path);
        * (dir->path) = 0;
        if (SFchdir (SFcurrentPath)) {
            *(dir->path) = save;
            /* force a re-read */
            *(dir->dir) = 0;
            SFupdatePath ();
        } else {
            *(dir->path) = save;
            if (SFcheckDir (n, dir) || ((f == n) && SFcheckFiles (dir))) {
                SFdrawList (n, SF_DO_SCROLL);
            }
        }
    }
    SFdirModTimerId = XtAppAddTimeOut (
        SFapp, (unsigned long) 1000, SFdirModTimer, (XtPointer) NULL
    );
}

/* Return a single character describing what kind of file STATBUF is. */
char SFstatChar (struct stat *statBuf) {
    if (S_ISDIR (statBuf->st_mode)) {
        return '/';
    } else if (S_ISREG (statBuf->st_mode)) {
        return S_ISXXX (statBuf->st_mode) ? '*' : ' ';
#ifdef S_ISSOCK
    } else if (S_ISSOCK (statBuf->st_mode)) {
        return '=';
#endif /* S_ISSOCK */
    } else {
        return ' ';
    }
}
