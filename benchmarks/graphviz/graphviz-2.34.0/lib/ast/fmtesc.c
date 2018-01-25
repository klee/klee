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
 * return string with expanded escape chars
 */

#include <ast.h>
/* #include <ccode.h> */
#include <string.h>
#include <ctype.h>

/*
 * quote string as of length n with qb...qe
 * (flags&FMT_ALWAYS) always quotes, otherwise quote output only if necessary
 * qe and the usual suspects are \... escaped
 * (flags&FMT_WIDE) doesn't escape 8 bit chars
 * (flags&FMT_ESCAPED) doesn't \... escape the usual suspects
 * (flags&FMT_SHELL) escape $`"#;~&|()<>[]*?
 */

char *fmtquote(const char *as, const char *qb, const char *qe, size_t n,
	       int flags)
{
    register unsigned char *s = (unsigned char *) as;
    register unsigned char *e = s + n;
    register char *b;
    register int c;
    register int escaped;
    register int spaced;
    int shell;
    char *f;
    char *buf;

    c = 4 * (n + 1);
    if (qb)
	c += strlen((char *) qb);
    if (qe)
	c += strlen((char *) qe);
    b = buf = fmtbuf(c);
    shell = 0;
    if (qb) {
	if (qb[0] == '$' && qb[1] == '\'' && qb[2] == 0)
	    shell = 1;
	while ((*b = *qb++))
	    b++;
    }
    f = b;
    escaped = spaced = !!(flags & FMT_ALWAYS);
    while (s < e) {
#ifdef UNUSED
	if ((c = mbsize(s)) > 1) {
	    while (c-- && s < e)
		*b++ = *s++;
	} else {
#endif
	    c = *s++;
	    if (!(flags & FMT_ESCAPED)
		&& (iscntrl(c) || !isprint(c) || c == '\\')) {
		escaped = 1;
		*b++ = '\\';
		switch (c) {
		case CC_bel:
		    c = 'a';
		    break;
		case '\b':
		    c = 'b';
		    break;
		case '\f':
		    c = 'f';
		    break;
		case '\n':
		    c = 'n';
		    break;
		case '\r':
		    c = 'r';
		    break;
		case '\t':
		    c = 't';
		    break;
		case CC_vt:
		    c = 'v';
		    break;
		case CC_esc:
		    c = 'E';
		    break;
		case '\\':
		    break;
		default:
		    if (!(flags & FMT_WIDE) || !(c & 0200)) {
			*b++ = '0' + ((c >> 6) & 07);
			*b++ = '0' + ((c >> 3) & 07);
			c = '0' + (c & 07);
		    } else
			b--;
		    break;
		}
	    } else if (c == '\\') {
		escaped = 1;
		*b++ = c;
		if (*s)
		    c = *s++;
	    } else if ((qe && strchr(qe, c)) ||
		       ((flags & FMT_SHELL) && !shell
			&& (c == '$' || c == '`'))) {
		escaped = 1;
		*b++ = '\\';
	    } else if (!spaced &&
		       !escaped &&
		       (isspace(c) ||
			(((flags & FMT_SHELL) || shell) &&
			 (strchr("\";~&|()<>[]*?", c) ||
			  (c == '#' && (b == f || isspace(*(b - 1)))
			  )
			 )
			)
		       )
		)
		spaced = 1;
	    *b++ = c;
#ifdef UNUSED
	}
#endif
    }
    if (qb) {
	if (!escaped)
	    buf += shell + !spaced;
	if (qe && (escaped || spaced))
	    while ((*b = *qe++))
		b++;
    }
    *b = 0;
    return buf;
}

/*
 * escape the usual suspects and quote chars in qs
 * in length n string as
 */

char *fmtnesq(const char *as, const char *qs, size_t n)
{
    return fmtquote(as, NiL, qs, n, 0);
}

/*
 * escape the usual suspects and quote chars in qs
 */

char *fmtesq(const char *as, const char *qs)
{
    return fmtquote(as, NiL, qs, strlen((char *) as), 0);
}

/*
 * escape the usual suspects
 */

char *fmtesc(const char *as)
{
    return fmtquote(as, NiL, NiL, strlen((char *) as), 0);
}
