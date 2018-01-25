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

#undef sfputm

#if __STD_C
int sfputm(reg Sfio_t * f, Sfulong_t u, Sfulong_t m)
#else
int sfputm(f, u, m)
reg Sfio_t *f;
reg Sfulong_t u;
reg Sfulong_t m;
#endif
{
    return __sf_putm(f, u, m);
}
