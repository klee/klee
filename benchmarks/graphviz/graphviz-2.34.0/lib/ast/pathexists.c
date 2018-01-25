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

/*
 * Glenn Fowler
 * AT&T Research
 *
 * return 1 if path exisis
 * maintains a cache to minimize stat(2) calls
 * path is modified in-place but restored on return
 * path components checked in pairs to cut stat()'s
 * in half by checking ENOTDIR vs. ENOENT
 */

#include <ast.h>
#include <error.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <compat_unistd.h>
#endif
/* #include "lclib.h" */
/* #include <ls.h> */

typedef struct Tree_s {
    struct Tree_s *next;
    struct Tree_s *tree;
    int mode;
    char name[1];
} Tree_t;

int pathexists(char *path, int mode)
{
    register char *s;
    register char *e;
    register Tree_t *p;
    register Tree_t *t;
    register int c;
    char *ee;
    int cc = 0;
    int x;
    struct stat st;

    static Tree_t tree;

    t = &tree;
    e = path + 1;
    c = *path;
    while (c) {
	p = t;
	for (s = e; *e && *e != '/'; e++);
	c = *e;
	*e = 0;
	for (t = p->tree; t && !streq(s, t->name); t = t->next);
	if (!t) {
	    if (!(t = newof(0, Tree_t, 1, strlen(s)))) {
		*e = c;
		return 0;
	    }
	    strcpy(t->name, s);
	    t->next = p->tree;
	    p->tree = t;
	    if (c) {
		*e = c;
		for (s = ee = e + 1; *ee && *ee != '/'; ee++);
		cc = *ee;
		*ee = 0;
	    } else
		ee = 0;
	    x = stat(path, &st);
	    if (ee) {
		e = ee;
		c = cc;
		if (!x || errno == ENOENT)
		    t->mode = PATH_READ | PATH_EXECUTE;
		if (!(p = newof(0, Tree_t, 1, strlen(s)))) {
		    *e = c;
		    return 0;
		}
		strcpy(p->name, s);
		p->next = t->tree;
		t->tree = p;
		t = p;
	    }
	    if (x) {
		*e = c;
		return 0;
	    }
	    if (st.st_mode & (S_IRUSR | S_IRGRP | S_IROTH))
		t->mode |= PATH_READ;
	    if (st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH))
		t->mode |= PATH_WRITE;
	    if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
		t->mode |= PATH_EXECUTE;
	    if (!S_ISDIR(st.st_mode))
		t->mode |= PATH_REGULAR;
	}
	*e++ = c;
	if (!t->mode || (c && (t->mode & PATH_REGULAR)))
	    return 0;
    }
    mode &= (PATH_READ | PATH_WRITE | PATH_EXECUTE | PATH_REGULAR);
    return (t->mode & mode) == mode;
}
