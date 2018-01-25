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

/*	Allocation with freeing and reallocing of last allocated block only.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/

#if __STD_C
static Void_t *lastalloc(Vmalloc_t * vm, size_t size)
#else
static Void_t *lastalloc(vm, size)
Vmalloc_t *vm;
size_t size;
#endif
{
    reg Block_t *tp, *next;
    reg Seg_t *seg, *last;
    reg size_t s;
    reg Vmdata_t *vd = vm->data;
    reg int local;
    size_t orgsize = 0;

    if (!(local = vd->mode & VM_TRUST)) {
	GETLOCAL(vd, local);
	if (ISLOCK(vd, local))
	    return NIL(Void_t *);
	SETLOCK(vd, local);
	orgsize = size;
    }

    size = size < ALIGN ? ALIGN : ROUND(size, ALIGN);
    for (;;) {
	for (last = NIL(Seg_t *), seg = vd->seg; seg;
	     last = seg, seg = seg->next) {
	    if (!(tp = seg->free) || (SIZE(tp) + sizeof(Head_t)) < size)
		continue;
	    if (last) {
		last->next = seg->next;
		seg->next = vd->seg;
		vd->seg = seg;
	    }
	    goto got_block;
	}

	/* there is no usable free space in region, try extending */
	if ((tp = (*_Vmextend) (vm, size, NIL(Vmsearch_f)))) {
	    seg = SEG(tp);
	    goto got_block;
	} else if (vd->mode & VM_AGAIN)
	    vd->mode &= ~VM_AGAIN;
	else
	    goto done;
    }

  got_block:
    if ((s = SIZE(tp)) >= size) {
	next = (Block_t *) ((Vmuchar_t *) tp + size);
	SIZE(next) = s - size;
	SEG(next) = seg;
	seg->free = next;
    } else
	seg->free = NIL(Block_t *);

    vd->free = seg->last = tp;

    if (!local && (vd->mode & VM_TRACE) && _Vmtrace)
	(*_Vmtrace) (vm, NIL(Vmuchar_t *), (Vmuchar_t *) tp, orgsize, 0);

  done:
    CLRLOCK(vd, local);
    return (Void_t *) tp;
}

#if __STD_C
static int lastfree(Vmalloc_t * vm, reg Void_t * data)
#else
static int lastfree(vm, data)
Vmalloc_t *vm;
reg Void_t *data;
#endif
{
    reg Seg_t *seg;
    reg Block_t *fp;
    reg size_t s;
    reg Vmdata_t *vd = vm->data;
    reg int local;

    if (!data)
	return 0;
    if (!(local = vd->mode & VM_TRUST)) {
	if (ISLOCK(vd, 0))
	    return -1;
	SETLOCK(vd, 0);
    }
    if (data != (Void_t *) vd->free) {
	if (!local && vm->disc->exceptf)
	    (void) (*vm->disc->exceptf) (vm, VM_BADADDR, data, vm->disc);
	CLRLOCK(vd, 0);
	return -1;
    }

    seg = vd->seg;
    if (!local && (vd->mode & VM_TRACE) && _Vmtrace) {
	if (seg->free)
	    s = (Vmuchar_t *) (seg->free) - (Vmuchar_t *) data;
	else
	    s = (Vmuchar_t *) BLOCK(seg->baddr) - (Vmuchar_t *) data;
	(*_Vmtrace) (vm, (Vmuchar_t *) data, NIL(Vmuchar_t *), s, 0);
    }

    vd->free = NIL(Block_t *);
    fp = (Block_t *) data;
    SEG(fp) = seg;
    SIZE(fp) =
	((Vmuchar_t *) BLOCK(seg->baddr) - (Vmuchar_t *) data) -
	sizeof(Head_t);
    seg->free = fp;
    seg->last = NIL(Block_t *);

    CLRLOCK(vd, 0);
    return 0;
}

#if __STD_C
static Void_t *lastresize(Vmalloc_t * vm, reg Void_t * data, size_t size,
			  int type)
#else
static Void_t *lastresize(vm, data, size, type)
Vmalloc_t *vm;
reg Void_t *data;
size_t size;
int type;
#endif
{
    reg Block_t *tp;
    reg Seg_t *seg;
    reg int *d, *ed;
    reg size_t oldsize;
    reg ssize_t s, ds;
    reg Vmdata_t *vd = vm->data;
    reg int local;
    reg Void_t *addr;
    Void_t *orgdata = 0;
    size_t orgsize = 0;

    if (!data) {
	oldsize = 0;
	data = lastalloc(vm, size);
	goto done;
    }
    if (size <= 0) {
	(void) lastfree(vm, data);
	return NIL(Void_t *);
    }

    if (!(local = vd->mode & VM_TRUST)) {
	if (ISLOCK(vd, 0))
	    return NIL(Void_t *);
	SETLOCK(vd, 0);
	orgdata = data;
	orgsize = size;
    }

    if (data == (Void_t *) vd->free)
	seg = vd->seg;
    else {			/* see if it was one of ours */
	for (seg = vd->seg; seg; seg = seg->next)
	    if (data >= seg->addr && data < (Void_t *) seg->baddr)
		break;
	if (!seg || (VLONG(data) % ALIGN) != 0 ||
	    (seg->last && (Vmuchar_t *) data > (Vmuchar_t *) seg->last)) {
	    CLRLOCK(vd, 0);
	    return NIL(Void_t *);
	}
    }

    /* set 's' to be the current available space */
    if (data != seg->last) {
	if (seg->last && (Vmuchar_t *) data < (Vmuchar_t *) seg->last)
	    oldsize = (Vmuchar_t *) seg->last - (Vmuchar_t *) data;
	else
	    oldsize = (Vmuchar_t *) BLOCK(seg->baddr) - (Vmuchar_t *) data;
	s = -1;
    } else {
	s = (Vmuchar_t *) BLOCK(seg->baddr) - (Vmuchar_t *) data;
	if (!(tp = seg->free))
	    oldsize = s;
	else {
	    oldsize = (Vmuchar_t *) tp - (Vmuchar_t *) data;
	    seg->free = NIL(Block_t *);
	}
    }

    size = size < ALIGN ? ALIGN : ROUND(size, ALIGN);
    if (s < 0 || (ssize_t) size > s) {
	if (s >= 0) {		/* amount to extend */
	    ds = size - s;
	    ds = ROUND(ds, vd->incr);
	    addr = (*vm->disc->memoryf) (vm, seg->addr, seg->extent,
					 seg->extent + ds, vm->disc);
	    if (addr == seg->addr) {
		s += ds;
		seg->size += ds;
		seg->extent += ds;
		seg->baddr += ds;
		SIZE(BLOCK(seg->baddr)) = BUSY;
	    } else
		goto do_alloc;
	} else {
	  do_alloc:
	    if (!(type & (VM_RSMOVE | VM_RSCOPY)))
		data = NIL(Void_t *);
	    else {
		tp = vd->free;
		if (!(addr = KPVALLOC(vm, size, lastalloc))) {
		    vd->free = tp;
		    data = NIL(Void_t *);
		} else {
		    if (type & VM_RSCOPY) {
			ed = (int *) data;
			d = (int *) addr;
			ds = oldsize < size ? oldsize : size;
			INTCOPY(d, ed, ds);
		    }

		    if (s >= 0 && seg != vd->seg) {
			tp = (Block_t *) data;
			SEG(tp) = seg;
			SIZE(tp) = s - sizeof(Head_t);
			seg->free = tp;
		    }

		    /* new block and size */
		    data = addr;
		    seg = vd->seg;
		    s = (Vmuchar_t *) BLOCK(seg->baddr) -
			(Vmuchar_t *) data;
		    seg->free = NIL(Block_t *);
		}
	    }
	}
    }

    if (data) {
	if (s >= (ssize_t) (size + sizeof(Head_t))) {
	    tp = (Block_t *) ((Vmuchar_t *) data + size);
	    SEG(tp) = seg;
	    SIZE(tp) = (s - size) - sizeof(Head_t);
	    seg->free = tp;
	}

	vd->free = seg->last = (Block_t *) data;

	if (!local && (vd->mode & VM_TRACE) && _Vmtrace)
	    (*_Vmtrace) (vm, (Vmuchar_t *) orgdata, (Vmuchar_t *) data,
			 orgsize, 0);
    }

    CLRLOCK(vd, 0);

  done:if (data && (type & VM_RSZERO) && size > oldsize) {
	d = (int *) ((char *) data + oldsize);
	size -= oldsize;
	INTZERO(d, size);
    }

    return data;
}


#if __STD_C
static long lastaddr(Vmalloc_t * vm, Void_t * addr)
#else
static long lastaddr(vm, addr)
Vmalloc_t *vm;
Void_t *addr;
#endif
{
    reg Vmdata_t *vd = vm->data;

    if (!(vd->mode & VM_TRUST) && ISLOCK(vd, 0))
	return -1L;
    if (!vd->free || addr < (Void_t *) vd->free
	|| addr >= (Void_t *) vd->seg->baddr)
	return -1L;
    else
	return (Vmuchar_t *) addr - (Vmuchar_t *) vd->free;
}

#if __STD_C
static long lastsize(Vmalloc_t * vm, Void_t * addr)
#else
static long lastsize(vm, addr)
Vmalloc_t *vm;
Void_t *addr;
#endif
{
    reg Vmdata_t *vd = vm->data;

    if (!(vd->mode & VM_TRUST) && ISLOCK(vd, 0))
	return -1L;
    if (!vd->free || addr != (Void_t *) vd->free)
	return -1L;
    else if (vd->seg->free)
	return (Vmuchar_t *) vd->seg->free - (Vmuchar_t *) addr;
    else
	return (Vmuchar_t *) vd->seg->baddr - (Vmuchar_t *) addr -
	    sizeof(Head_t);
}

#if __STD_C
static int lastcompact(Vmalloc_t * vm)
#else
static int lastcompact(vm)
Vmalloc_t *vm;
#endif
{
    reg Block_t *fp;
    reg Seg_t *seg, *next;
    reg size_t s;
    reg Vmdata_t *vd = vm->data;

    if (!(vd->mode & VM_TRUST)) {
	if (ISLOCK(vd, 0))
	    return -1;
	SETLOCK(vd, 0);
    }

    for (seg = vd->seg; seg; seg = next) {
	next = seg->next;

	if (!(fp = seg->free))
	    continue;

	seg->free = NIL(Block_t *);
	if (seg->size == (s = SIZE(fp) & ~BITS))
	    s = seg->extent;
	else
	    s += sizeof(Head_t);

	if ((*_Vmtruncate) (vm, seg, s, 1) < 0)
	    seg->free = fp;
    }

    if ((vd->mode & VM_TRACE) && _Vmtrace)
	(*_Vmtrace) (vm, (Vmuchar_t *) 0, (Vmuchar_t *) 0, 0, 0);

    CLRLOCK(vd, 0);
    return 0;
}

#if __STD_C
static Void_t *lastalign(Vmalloc_t * vm, size_t size, size_t align)
#else
static Void_t *lastalign(vm, size, align)
Vmalloc_t *vm;
size_t size;
size_t align;
#endif
{
    reg Vmuchar_t *data;
    reg size_t s, orgsize = 0, orgalign = 0;
    reg Seg_t *seg;
    reg Block_t *next;
    reg int local;
    reg Vmdata_t *vd = vm->data;

    if (size <= 0 || align <= 0)
	return NIL(Void_t *);

    if (!(local = vd->mode & VM_TRUST)) {
	GETLOCAL(vd, local);
	if (ISLOCK(vd, local))
	    return NIL(Void_t *);
	SETLOCK(vd, local);
	orgsize = size;
	orgalign = align;
    }

    size = size <= TINYSIZE ? TINYSIZE : ROUND(size, ALIGN);
    align = MULTIPLE(align, ALIGN);

    s = size + align;
    if (!(data = (Vmuchar_t *) KPVALLOC(vm, s, lastalloc)))
	goto done;

    /* find the segment containing this block */
    for (seg = vd->seg; seg; seg = seg->next)
	if (seg->last == (Block_t *) data)
	    break;
     /**/ ASSERT(seg);

    /* get a suitably aligned address */
    if ((s = (size_t) (VLONG(data) % align)) != 0)
	data += align - s;
    /**/ ASSERT((VLONG(data) % align) == 0);

    /* free the unused tail */
    next = (Block_t *) (data + size);
    if ((s = (seg->baddr - (Vmuchar_t *) next)) >= sizeof(Block_t)) {
	SEG(next) = seg;
	SIZE(next) = s - sizeof(Head_t);
	seg->free = next;
    }

    vd->free = seg->last = (Block_t *) data;

    if (!local && !(vd->mode & VM_TRUST) && _Vmtrace
	&& (vd->mode & VM_TRACE))
	(*_Vmtrace) (vm, NIL(Vmuchar_t *), data, orgsize, orgalign);

  done:
    CLRLOCK(vd, local);

    return (Void_t *) data;
}

/* Public method for free-1 allocation */
static Vmethod_t _Vmlast = {
    lastalloc,
    lastresize,
    lastfree,
    lastaddr,
    lastsize,
    lastcompact,
    lastalign,
    VM_MTLAST
};

__DEFINE__(Vmethod_t *, Vmlast, &_Vmlast);
