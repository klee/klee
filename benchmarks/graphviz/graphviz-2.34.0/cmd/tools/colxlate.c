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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_SEARCH_H
#include <search.h>
#endif
#include <ctype.h>
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif
#ifndef NOT
#define NOT(x)                  (!(x))
#endif
#ifndef NIL
#define NIL(type)               ((type)0)
#endif
typedef struct hsbcolor_t {
    char *name;
    unsigned char h, s, b;
} hsbcolor_t;


#ifndef NOCOLORNAMES
#include "colortbl.h"

/*
	char *bsearch ((char *) key, (char *) base, nel, sizeof (*key), compar)
	unsigned nel;
	int (*compar)( );
*/

static unsigned char *canoncolor(char *orig, unsigned char *out)
{
    unsigned char c;
    unsigned char *p = out;
    while ((c = *(unsigned char *) orig++)) {
	if (isalnum(c) == FALSE)
	    continue;
	if (isupper(c))
	    c = tolower(c);
	*p++ = c;
    }
    *p = '\0';
    return out;
}

static int colorcmpf(const void *a0, const void *a1)
{
    hsbcolor_t *p0 = (hsbcolor_t *) a0;
    hsbcolor_t *p1 = (hsbcolor_t *) a1;
    int i = (p0->name[0] - p1->name[0]);
    return (i ? i : strcmp(p0->name, p1->name));
}

char *colorxlate(char *str, char *buf)
{
    static hsbcolor_t *last;
    unsigned char canon[128];
    char *p;
    hsbcolor_t fake;

    if ((last == NULL) || (last->name[0] != str[0])
	|| (strcmp(last->name, str))) {
	fake.name = (char *) canoncolor(str, canon);
	last =
	    (hsbcolor_t *) bsearch(&fake, color_lib,
				   sizeof(color_lib) / sizeof(hsbcolor_t),
				   sizeof(fake), colorcmpf);
    }
    if (last == NULL) {
	if (isdigit(canon[0]) == FALSE) {
	    fprintf(stderr, "warning: %s is not a known color\n", str);
	    strcpy(buf, str);
	} else
	    for (p = buf; (*p = *str++); p++)
		if (*p == ',')
		    *p = ' ';
    } else
	sprintf(buf, "%.3f %.3f %.3f", ((double) last->h) / 255,
		((double) last->s) / 255, ((double) last->b) / 255);
    return buf;
}
#else
char *colorxlate(char *str, char *buf)
{
    return str;
}
#endif
