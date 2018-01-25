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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "errno.h"

#ifdef UNIV_MAX

#include <ctype.h>

#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include <compat_unistd.h>
#endif

/*
 * return external representation for symbolic link text of name in buf
 * the link text string length is returned
 */

int pathgetlink(const char *name, char *buf, int siz)
{
#ifdef WIN32
	return (-1);
#else
    int n;

    if ((n = readlink(name, buf, siz)) < 0)
	return (-1);
    if (n >= siz) {
	errno = EINVAL;
	return (-1);
    }
    buf[n] = 0;
#ifdef UNIV_MAX
    if (isspace(*buf)) {
	register char *s;
	register char *t;
	register char *u;
	register char *v;
	int match = 0;
	char tmp[PATH_MAX];

	s = buf;
	t = tmp;
	while (isalnum(*++s) || *s == '_' || *s == '.');
	if (*s++) {
	    for (;;) {
		if (!*s || isspace(*s)) {
		    if (match) {
			*t = 0;
			n = t - tmp;
			strcpy(buf, tmp);
		    }
		    break;
		}
		if (t >= &tmp[sizeof(tmp)])
		    break;
		*t++ = *s++;
		if (!match && t < &tmp[sizeof(tmp) - univ_size + 1])
		    for (n = 0; n < UNIV_MAX; n++) {
			if (*(v = s - 1) == *(u = univ_name[n])) {
			    while (*u && *v++ == *u)
				u++;
			    if (!*u) {
				match = 1;
				strcpy(t - 1, univ_cond);
				t += univ_size - 1;
				s = v;
				break;
			    }
			}
		    }
	    }
	}
    }
#endif
    return (n);
#endif
}
