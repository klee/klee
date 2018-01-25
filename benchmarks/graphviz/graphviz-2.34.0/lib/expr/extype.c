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
 * expression library C program generator
 */

#include "exlib.h"

#define str(s)		# s
#define xstr(s)		str(s)

/*
 * return C type name for type
 */

char*
extype(int type)
{
	switch (type)
	{
	case FLOATING:
		return "double";
	case STRING:
		return "char*";
	case UNSIGNED:
		return xstr(uintmax_t);
	}
	return xstr(intmax_t);
}
