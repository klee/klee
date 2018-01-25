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

#include	"sfhdr.h"

/*	Print data with a given format
**
**	Written by Kiem-Phong Vo.
*/

#if __STD_C
int sfprintf(Sfio_t * f, const char *form, ...)
#else
int sfprintf(va_alist)
va_dcl
#endif
{
    va_list args;
    reg int rv;

#if __STD_C
    va_start(args, form);
#else
    reg Sfio_t *f;
    reg char *form;
    va_start(args);
    f = va_arg(args, Sfio_t *);
    form = va_arg(args, char *);
#endif
    rv = sfvprintf(f, form, args);

    va_end(args);
    return rv;
}

#if __STD_C
int sfvsprintf(char *s, int n, const char *form, va_list args)
#else
int sfvsprintf(s, n, form, args)
char *s;
int n;
char *form;
va_list args;
#endif
{
    Sfio_t f;
    reg int rv;

    if (!s || n <= 0)
	return -1;

    /* make a fake stream */
    SFCLEAR(&f, NIL(Vtmutex_t *));
    f.flags = SF_STRING | SF_WRITE;
    f.bits = SF_PRIVATE;
    f.mode = SF_WRITE;
    f.size = n - 1;
    f.data = f.next = f.endr = (uchar *) s;
    f.endb = f.endw = f.data + f.size;

    rv = sfvprintf(&f, form, args);
    *f.next = '\0';
    _Sfi = f.next - f.data;

    return rv;
}

#if __STD_C
int sfsprintf(char *s, int n, const char *form, ...)
#else
int sfsprintf(va_alist)
va_dcl
#endif
{
    va_list args;
    reg int rv;

#if __STD_C
    va_start(args, form);
#else
    reg char *s;
    reg int n;
    reg char *form;
    va_start(args);
    s = va_arg(args, char *);
    n = va_arg(args, int);
    form = va_arg(args, char *);
#endif

    rv = sfvsprintf(s, n, form, args);
    va_end(args);

    return rv;
}
