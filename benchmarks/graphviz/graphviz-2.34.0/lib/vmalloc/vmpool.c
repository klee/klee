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

#define POOLFREE	0x55555555L	/* block free indicator  */

/*	Method for pool allocation.
**	All elements in a pool have the same size.
**	The following fields of Vmdata_t are used as:
**		pool:	size of a block.
**		free:	list of free blocks.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/

#if __STD_C
static Void_t *poolalloc(Vmalloc_t * vm, reg size_t size)
#else
static Void_t *poolalloc(vm, size)
Vmalloc_t *vm;
reg size_t size;
#endif
{
    reg Vmdata_t *vd = vm->data;
    reg Block_t *tp, *next;
    reg size_t s;
    reg Seg_t *seg;
    reg int local;

    if (size <= 0)
	return NIL(Void_t *);
    else if (size != vd->pool) {
	if (vd->pool <= 0)
	    vd->pool = size;
	else
	    return NIL(Void_t *);
    }

    if (!(local = vd->mode & VM_TRUST)) {
	if (ISLOCK(vd, 0))
	    return NIL(Void_t *);
	SETLOCK(vd, 0);
    }

    if ((tp = vd->free)) {	/* there is a ready free block */
	vd->free = SEGLINK(tp);
	goto done;
    }

    size = ROUND(size, ALIGN);

    /* look thru all segments for a suitable free block */
    for (tp = NIL(Block_t *), seg = vd->seg; seg; seg = seg->next) {
	if ((tp = seg->free) &&
	    (s = (SIZE(tp) & ~BITS) + sizeof(Head_t)) >= size)
	    goto has_blk;
    }

    for (;;) {			/* must extend region */
	if ((tp =
	     (*_Vmextend) (vm, ROUND(size, vd->incr), NIL(Vmsearch_f)))) {
	    s = (SIZE(tp) & ~BITS) + sizeof(Head_t);
	    seg = SEG(tp);
	    goto has_blk;
	} else if (vd->mode & VM_AGAIN)
	    vd->mode &= ~VM_AGAIN;
	else
	    goto done;
    }

  has_blk:			/* if get here, (tp, s, seg) must be well-defined */
    next = (Block_t *) ((Vmuchar_t *) tp + size);
    if ((s -= size) <= (size + sizeof(Head_t))) {
	for (; s >= size; s -= size) {
	    SIZE(next) = POOLFREE;
	    SEGLINK(next) = vd->free;
	    vd->free = next;
	    next = (Block_t *) ((Vmuchar_t *) next + size);
	}
	seg->free = NIL(Block_t *);
    } else {
	SIZE(next) = s - sizeof(Head_t);
	SEG(next) = seg;
	seg->free = next;
    }

  done:
    if (!local && (vd->mode & VM_TRACE) && _Vmtrace && tp)
	(*_Vmtrace) (vm, NIL(Vmuchar_t *), (Vmuchar_t *) tp, vd->pool, 0);

    CLRLOCK(vd, 0);
    return (Void_t *) tp;
}

#if __STD_C
static long pooladdr(Vmalloc_t * vm, reg Void_t * addr)
#else
static long pooladdr(vm, addr)
Vmalloc_t *vm;
reg Void_t *addr;
#endif
{
    reg Block_t *bp, *tp;
    reg Vmuchar_t *laddr, *baddr;
    reg size_t size;
    reg Seg_t *seg;
    reg long offset;
    reg Vmdata_t *vd = vm->data;
    reg int local;

    if (!(local = vd->mode & VM_TRUST)) {
	GETLOCAL(vd, local);
	if (ISLOCK(vd, local))
	    return -1L;
	SETLOCK(vd, local);
    }

    offset = -1L;
    for (seg = vd->seg; seg; seg = seg->next) {
	laddr = (Vmuchar_t *) SEGBLOCK(seg);
	baddr = seg->baddr - sizeof(Head_t);
	if ((Vmuchar_t *) addr < laddr || (Vmuchar_t *) addr >= baddr)
	    continue;

	/* the block that has this address */
	size = ROUND(vd->pool, ALIGN);
	tp = (Block_t *) (laddr +
			  (((Vmuchar_t *) addr - laddr) / size) * size);

	/* see if this block has been freed */
	if (SIZE(tp) == POOLFREE)	/* may be a coincidence - make sure */
	    for (bp = vd->free; bp; bp = SEGLINK(bp))
		if (bp == tp)
		    goto done;

	offset = (Vmuchar_t *) addr - (Vmuchar_t *) tp;
	goto done;
    }

  done:
    CLRLOCK(vd, local);
    return offset;
}

#if __STD_C
static int poolfree(reg Vmalloc_t * vm, reg Void_t * data)
#else
static int poolfree(vm, data)
reg Vmalloc_t *vm;
reg Void_t *data;
#endif
{
    reg Block_t *bp;
    reg Vmdata_t *vd = vm->data;
    reg int local;

    if (!data)
	return 0;

    if (!(local = vd->mode & VM_TRUST)) {
	if (ISLOCK(vd, 0) || vd->pool <= 0)
	    return -1;

	if (KPVADDR(vm, data, pooladdr) != 0) {
	    if (vm->disc->exceptf)
		(void) (*vm->disc->exceptf) (vm, VM_BADADDR, data,
					     vm->disc);
	    return -1;
	}

	SETLOCK(vd, 0);
    }

    bp = (Block_t *) data;
    SIZE(bp) = POOLFREE;
    SEGLINK(bp) = vd->free;
    vd->free = bp;

    if (!local && (vd->mode & VM_TRACE) && _Vmtrace)
	(*_Vmtrace) (vm, (Vmuchar_t *) data, NIL(Vmuchar_t *), vd->pool,
		     0);

    CLRLOCK(vd, local);
    return 0;
}

#if __STD_C
static Void_t *poolresize(Vmalloc_t * vm, Void_t * data, size_t size,
			  int type)
#else
static Void_t *poolresize(vm, data, size, type)
Vmalloc_t *vm;
Void_t *data;
size_t size;
int type;
#endif
{
    reg Vmdata_t *vd = vm->data;

    NOTUSED(type);

    if (!data) {
	if ((data = poolalloc(vm, size)) && (type & VM_RSZERO)) {
	    reg int *d = (int *) data, *ed =
		(int *) ((char *) data + size);
	    do {
		*d++ = 0;
	    } while (d < ed);
	}
	return data;
    }
    if (size == 0) {
	(void) poolfree(vm, data);
	return NIL(Void_t *);
    }

    if (!(vd->mode & VM_TRUST)) {
	if (ISLOCK(vd, 0))
	    return NIL(Void_t *);

	if (size != vd->pool || KPVADDR(vm, data, pooladdr) != 0) {
	    if (vm->disc->exceptf)
		(void) (*vm->disc->exceptf) (vm, VM_BADADDR, data,
					     vm->disc);
	    return NIL(Void_t *);
	}

	if ((vd->mode & VM_TRACE) && _Vmtrace)
	    (*_Vmtrace) (vm, (Vmuchar_t *) data, (Vmuchar_t *) data, size,
			 0);
    }

    return data;
}

#if __STD_C
static long poolsize(Vmalloc_t * vm, Void_t * addr)
#else
static long poolsize(vm, addr)
Vmalloc_t *vm;
Void_t *addr;
#endif
{
    return pooladdr(vm, addr) == 0 ? (long) vm->data->pool : -1L;
}

#if __STD_C
static int poolcompact(Vmalloc_t * vm)
#else
static int poolcompact(vm)
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
static Void_t *poolalign(Vmalloc_t * vm, size_t size, size_t align)
#else
static Void_t *poolalign(vm, size, align)
Vmalloc_t *vm;
size_t size;
size_t align;
#endif
{
    NOTUSED(vm);
    NOTUSED(size);
    NOTUSED(align);
    return NIL(Void_t *);
}

/* Public interface */
static Vmethod_t _Vmpool = {
    poolalloc,
    poolresize,
    poolfree,
    pooladdr,
    poolsize,
    poolcompact,
    poolalign,
    VM_MTPOOL
};

__DEFINE__(Vmethod_t *, Vmpool, &_Vmpool);
