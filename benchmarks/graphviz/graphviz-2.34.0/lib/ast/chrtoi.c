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
 * convert a 0 terminated character constant string to an int
 */

#include <ast.h>

int chrtoi(register const char *s)
{
    register int c;
    register int n;
    register int x;
    char *p;

    c = 0;
    for (n = 0; n < sizeof(int) * CHAR_BIT; n += CHAR_BIT) {
	switch (x = *((unsigned char *) s++)) {
	case '\\':
	    x = chresc(s - 1, &p);
	    s = (const char *) p;
	    break;
	case 0:
	    return (c);
	}
	c = (c << CHAR_BIT) | x;
    }
    return (c);
}
