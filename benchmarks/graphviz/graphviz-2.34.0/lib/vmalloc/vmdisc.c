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

#include	"vmhdr.h"

/*	Change the discipline for a region.  The old discipline
**	is returned.  If the new discipline is NIL then the
**	discipline is not changed.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/
#if __STD_C
Vmdisc_t *vmdisc(Vmalloc_t * vm, Vmdisc_t * disc)
#else
Vmdisc_t *vmdisc(vm, disc)
Vmalloc_t *vm;
Vmdisc_t *disc;
#endif
{
    Vmdisc_t *old = vm->disc;

    if (disc) {
	if (disc->memoryf != old->memoryf)
	    return NIL(Vmdisc_t *);
	if (old->exceptf &&
	    (*old->exceptf) (vm, VM_DISC, (Void_t *) disc, old) != 0)
	    return NIL(Vmdisc_t *);
	vm->disc = disc;
    }
    return old;
}
