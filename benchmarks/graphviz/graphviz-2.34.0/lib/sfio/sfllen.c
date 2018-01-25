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

/*	Get size of a long value coded in a portable format
**
**	Written by Kiem-Phong Vo
*/
#if __STD_C
int _sfllen(Sflong_t v)
#else
int _sfllen(v)
Sflong_t v;
#endif
{
    if (v < 0)
	v = -(v + 1);
    v = (Sfulong_t) v >> SF_SBITS;
    return 1 + (v > 0 ? sfulen(v) : 0);
}
