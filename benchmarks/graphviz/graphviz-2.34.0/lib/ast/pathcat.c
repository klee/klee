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
 * single dir support for pathaccess()
 */

#include <ast.h>

char *pathcat(char *path, register const char *dirs, int sep,
	      const char *a, register const char *b)
{
    register char *s;

    s = path;
    while (*dirs && *dirs != sep)
	*s++ = *dirs++;
    if (s != path)
	*s++ = '/';
    if (a) {
	while ((*s = *a++))
	    s++;
	if (b)
	    *s++ = '/';
    } else if (!b)
	b = ".";
    if (b)
	while ((*s++ = *b++));
    return (*dirs ? (char *) ++dirs : 0);
}
