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

#undef sfputd

#if __STD_C
int sfputd(reg Sfio_t * f, Sfdouble_t d)
#else
int sfputd(f, d)
reg Sfio_t *f;
reg Sfdouble_t d;
#endif
{
    return __sf_putd(f, d);
}
