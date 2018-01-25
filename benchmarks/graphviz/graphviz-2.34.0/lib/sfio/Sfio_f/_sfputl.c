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

#undef sfputl

#if __STD_C
int sfputl(reg Sfio_t * f, Sflong_t l)
#else
int sfputl(f, l)
reg Sfio_t *f;
reg Sflong_t l;
#endif
{
    return __sf_putl(f, l);
}
