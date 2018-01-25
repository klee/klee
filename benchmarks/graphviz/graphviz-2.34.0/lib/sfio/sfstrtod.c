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

/*	Convert a Sfdouble_t value represented in an ASCII format into
**	the internal Sfdouble_t representation.
**
**	Written by Kiem-Phong Vo.
*/

#define BATCH	(2*sizeof(int))	/* accumulate this many digits at a time */
#define IPART		0	/* doing integer part */
#define FPART		1	/* doing fractional part */
#define EPART		2	/* doing exponent part */

#if __STD_C
static Sfdouble_t sfpow10(reg int n)
#else
static Sfdouble_t sfpow10(n)
reg int n;
#endif
{
    Sfdouble_t dval;

    switch (n) {
    case -3:
	return .001;
    case -2:
	return .01;
    case -1:
	return .1;
    case 0:
	return 1.;
    case 1:
	return 10.;
    case 2:
	return 100.;
    case 3:
	return 1000.;
    }

    if (n < 0) {
	dval = .0001;
	for (n += 4; n < 0; n += 1)
	    dval /= 10.;
    } else {
	dval = 10000.;
	for (n -= 4; n > 0; n -= 1)
	    dval *= 10.;
    }

    return dval;
}

#if __STD_C
Sfdouble_t _sfstrtod(reg const char *s, char **retp)
#else
Sfdouble_t _sfstrtod(s, retp)
reg char *s;			/* string to convert */
char **retp;			/* to return the remainder of string */
#endif
{
    reg int n, c, m;
    reg int mode, fexp, sign, expsign;
    Sfdouble_t dval;
#if _lib_locale
    int decpoint = 0;
    SFSETLOCALE(&decpoint, NIL(int *));
#else
#define decpoint	'.'
#endif

    /* skip initial blanks */
    while (isspace(*s))
	++s;

    /* get the sign */
    if ((sign = (*s == '-')) || *s == '+')
	s += 1;

    mode = IPART;
    fexp = expsign = 0;
    dval = 0.;
    while (*s) {		/* accumulate a handful of the digits */
	for (m = BATCH, n = 0; m > 0; --m, ++s) {	/* get and process a char */
	    c = *s;
	    if (isdigit(c))
		n = 10 * n + (c - '0');
	    else
		break;
	}

	/* number of digits accumulated */
	m = BATCH - m;

	if (mode == IPART) {	/* doing the integer part */
	    if (dval == 0.)
		dval = (Sfdouble_t) n;
	    else
		dval = dval * sfpow10(m) + (Sfdouble_t) n;
	} else if (mode == FPART) {	/* doing the fractional part */
	    fexp -= m;
	    if (n > 0)
		dval += n * sfpow10(fexp);
	} else if (n) {		/* doing the exponent part */
	    if (expsign)
		n = -n;
	    dval *= sfpow10(n);
	}

	if (!c)
	    break;

	if (m < BATCH) {	/* detected a non-digit */
	    if (c == decpoint) {	/* start the fractional part or no match */
		if (mode != IPART)
		    break;
		mode = FPART;
		s += 1;
	    } else if (c == 'e' || c == 'E') {
		if (mode == EPART)
		    break;
		mode = EPART;
		c = *++s;
		if ((expsign = (c == '-')) || c == '+')
		    s += 1;
	    } else
		break;
	}
    }

    if (retp)
	*retp = (char *) s;
    return sign ? -dval : dval;
}
