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
 * AT&T Bell Laboratories
 *
 * return path to file a/b with access mode using : separated dirs
 * both a and b may be 0
 * if (mode&PATH_REGULAR) then path must not be a directory
 * if (mode&PATH_ABSOLUTE) then path must be rooted
 * path returned in path buffer
 */

#include <ast.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include <compat_unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

/* #include <ls.h> */

char *pathcanon(char *path, int flags);

char *pathaccess(register char *path, register const char *dirs,
		 const char *a, const char *b, register int mode)
{
    register int m = 0;
    int sep = ':';
    char cwd[PATH_MAX];
    struct stat st;

#ifdef EFF_ONLY_OK
    m |= EFF_ONLY_OK;
#endif
#ifdef EX_OK
    if (mode == (PATH_EXECUTE | PATH_REGULAR)) {
	mode &= ~PATH_REGULAR;
	m |= EX_OK;
    } else
#endif
    {
	if (mode & PATH_READ)
	    m |= R_OK;
	if (mode & PATH_WRITE)
	    m |= W_OK;
	if (mode & PATH_EXECUTE)
	    m |= X_OK;
    }
    do {
	dirs = pathcat(path, dirs, sep, a, b);
	pathcanon(path, 0);
	if (!access(path, m)) {
	    if ((mode & PATH_REGULAR)
		&& (stat(path, &st) || S_ISDIR(st.st_mode)))
		continue;
	    if (*path == '/' || !(mode & PATH_ABSOLUTE))
		return (path);
	    dirs = getcwd(cwd, sizeof(cwd));
	    sep = 0;
	}
    } while (dirs);
    return (0);
}
