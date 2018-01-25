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

/*	Walks all segments created in region(s)
**
**	Written by Kiem-Phong Vo, kpv@research.att.com (02/08/96)
*/

#if __STD_C
int vmwalk(Vmalloc_t * vm,
	   int (*segf) (Vmalloc_t *, Void_t *, size_t, Vmdisc_t *))
#else
int vmwalk(vm, segf)
Vmalloc_t *vm;
int (*segf) ( /* Vmalloc_t*, Void_t*, size_t, Vmdisc_t* */ );
#endif
{
    reg Seg_t *seg;
    reg int rv;

    if (!vm) {
	for (vm = Vmheap; vm; vm = vm->next) {
	    if (!(vm->data->mode & VM_TRUST) && ISLOCK(vm->data, 0))
		continue;

	    SETLOCK(vm->data, 0);
	    for (seg = vm->data->seg; seg; seg = seg->next) {
		rv = (*segf) (vm, seg->addr, seg->extent, vm->disc);
		if (rv < 0)
		    return rv;
	    }
	    CLRLOCK(vm->data, 0);
	}
    } else {
	if (!(vm->data->mode & VM_TRUST) && ISLOCK(vm->data, 0))
	    return -1;

	SETLOCK(vm->data, 0);
	for (seg = vm->data->seg; seg; seg = seg->next) {
	    rv = (*segf) (vm, seg->addr, seg->extent, vm->disc);
	    if (rv < 0)
		return rv;
	}
	CLRLOCK(vm->data, 0);
    }

    return 0;
}
