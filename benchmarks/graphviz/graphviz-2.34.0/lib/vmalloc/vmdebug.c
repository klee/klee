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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_INTPTR_T
#define INT2PTR(t,v) ((t)(intptr_t)(v))
#else
#define INT2PTR(t,v) ((t)(v))
#endif

#include	"vmhdr.h"

/*	Method to help with debugging. This does rigorous checks on
**	addresses and arena integrity.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/

/* structure to keep track of file names */
typedef struct _dbfile_s Dbfile_t;
struct _dbfile_s {
    Dbfile_t *next;
    char file[1];
};
static Dbfile_t *Dbfile;

/* global watch list */
#define S_WATCH	32
static int Dbnwatch;
static Void_t *Dbwatch[S_WATCH];

/* types of warnings reported by dbwarn() */
#define	DB_CHECK	0
#define DB_ALLOC	1
#define DB_FREE		2
#define DB_RESIZE	3
#define DB_WATCH	4
#define DB_RESIZED	5

static int Dbinit = 0;
#define DBINIT()	(Dbinit ? 0 : (dbinit(), Dbinit=1) )
static void dbinit(void)
{
    int fd;
    if ((fd = vmtrace(-1)) >= 0)
	vmtrace(fd);
}

/* just an entry point to make it easy to set break point */
#if __STD_C
static void vmdbwarn(Vmalloc_t * vm, char *mesg, int n)
#else
static void vmdbwarn(vm, mesg, n)
Vmalloc_t *vm;
char *mesg;
int n;
#endif
{
    reg Vmdata_t *vd = vm->data;

    write(2, mesg, n);
    if (vd->mode & VM_DBABORT)
	abort();
}

/* issue a warning of some type */
#if __STD_C
static void dbwarn(Vmalloc_t * vm, Void_t * data, int where, char *file,
		   int line, int type)
#else
static void dbwarn(vm, data, where, file, line, type)
Vmalloc_t *vm;			/* region holding the block     */
Void_t *data;			/* data block                   */
int where;			/* byte that was corrupted      */
char *file;			/* file where call originates   */
int line;			/* line number of call          */
int type;			/* operation being done         */
#endif
{
    char buf[1024], *bufp, *endbuf, *s;
#define SLOP	64		/* enough for a message and an int */

    DBINIT();

    bufp = buf;
    endbuf = buf + sizeof(buf);

    if (type == DB_ALLOC)
	bufp = (*_Vmstrcpy) (bufp, "alloc error", ':');
    else if (type == DB_FREE)
	bufp = (*_Vmstrcpy) (bufp, "free error", ':');
    else if (type == DB_RESIZE)
	bufp = (*_Vmstrcpy) (bufp, "resize error", ':');
    else if (type == DB_CHECK)
	bufp = (*_Vmstrcpy) (bufp, "corrupted data", ':');
    else if (type == DB_WATCH)
	bufp = (*_Vmstrcpy) (bufp, "alert", ':');

    /* region info */
    bufp = (*_Vmstrcpy) (bufp, "region", '=');
    bufp = (*_Vmstrcpy) (bufp, (*_Vmitoa) (VLONG(vm), 0), ':');

    if (data) {
	bufp = (*_Vmstrcpy) (bufp, "block", '=');
	bufp = (*_Vmstrcpy) (bufp, (*_Vmitoa) (VLONG(data), 0), ':');
    }

    if (!data) {
	if (where == DB_ALLOC)
	    bufp = (*_Vmstrcpy) (bufp, "can't get memory", ':');
	else
	    bufp = (*_Vmstrcpy) (bufp, "region is locked", ':');
    } else if (type == DB_FREE || type == DB_RESIZE) {
	if (where == 0)
	    bufp = (*_Vmstrcpy) (bufp, "unallocated block", ':');
	else
	    bufp = (*_Vmstrcpy) (bufp, "already freed", ':');
    } else if (type == DB_WATCH) {
	bufp = (*_Vmstrcpy) (bufp, "size", '=');
	bufp = (*_Vmstrcpy) (bufp, (*_Vmitoa) (DBSIZE(data), -1), ':');
	if (where == DB_ALLOC)
	    bufp = (*_Vmstrcpy) (bufp, "just allocated", ':');
	else if (where == DB_FREE)
	    bufp = (*_Vmstrcpy) (bufp, "being freed", ':');
	else if (where == DB_RESIZE)
	    bufp = (*_Vmstrcpy) (bufp, "being resized", ':');
	else if (where == DB_RESIZED)
	    bufp = (*_Vmstrcpy) (bufp, "just resized", ':');
    } else if (type == DB_CHECK) {
	bufp = (*_Vmstrcpy) (bufp, "bad byte at", '=');
	bufp =
	    (*_Vmstrcpy) (bufp,
			  (*_Vmitoa) (VLONG(INT2PTR(char *, where)), -1),
			  ':');
	if ((s = DBFILE(data)) && (bufp + strlen(s) + SLOP) < endbuf) {
	    bufp = (*_Vmstrcpy) (bufp, "allocated at", '=');
	    bufp = (*_Vmstrcpy) (bufp, s, ',');
	    bufp =
		(*_Vmstrcpy) (bufp,
			      (*_Vmitoa) (VLONG
					  (INT2PTR(char *, DBLINE(data))),
					  -1), ':');
	}
    }

    /* location where offending call originates from */
    if (file && file[0] && line > 0
	&& (bufp + strlen(file) + SLOP) < endbuf) {
	bufp = (*_Vmstrcpy) (bufp, "detected at", '=');
	bufp = (*_Vmstrcpy) (bufp, file, ',');
	bufp =
	    (*_Vmstrcpy) (bufp,
			  (*_Vmitoa) (VLONG(INT2PTR(char *, where)), -1),
			  ':');
    }

    *bufp++ = '\n';
    *bufp = '\0';

    vmdbwarn(vm, buf, (bufp - buf));
}

/* check for watched address and issue warnings */
#if __STD_C
static void dbwatch(Vmalloc_t * vm, Void_t * data, char *file, int line,
		    int type)
#else
static void dbwatch(vm, data, file, line, type)
Vmalloc_t *vm;
Void_t *data;
char *file;
int line;
int type;
#endif
{
    reg int n;

    for (n = Dbnwatch; n >= 0; --n) {
	if (Dbwatch[n] == data) {
	    dbwarn(vm, data, type, file, line, DB_WATCH);
	    return;
	}
    }
}

/* record information about the block */
#if __STD_C
static void dbsetinfo(Vmuchar_t * data, size_t size, char *file, int line)
#else
static void dbsetinfo(data, size, file, line)
Vmuchar_t *data;		/* real address not the one from Vmbest */
size_t size;			/* the actual requested size            */
char *file;			/* file where the request came from     */
int line;			/* and line number                      */
#endif
{
    reg Vmuchar_t *begp, *endp;
    reg Dbfile_t *last, *db;

    DBINIT();

    /* find the file structure */
    if (!file || !file[0])
	db = NIL(Dbfile_t *);
    else {
	for (last = NIL(Dbfile_t *), db = Dbfile; db;
	     last = db, db = db->next)
	    if (strcmp(db->file, file) == 0)
		break;
	if (!db) {
	    db = (Dbfile_t *) vmalloc(Vmheap,
				      sizeof(Dbfile_t) + strlen(file));
	    if (db) {
		(*_Vmstrcpy) (db->file, file, 0);
		db->next = Dbfile;
		Dbfile = db->next;
	    }
	} else if (last) {	/* move-to-front heuristic */
	    last->next = db->next;
	    db->next = Dbfile;
	    Dbfile = db->next;
	}
    }

    DBSETFL(data, (db ? db->file : NIL(char *)), line);
    DBSIZE(data) = size;
    DBSEG(data) = SEG(DBBLOCK(data));

    DBHEAD(data, begp, endp);
    while (begp < endp)
	*begp++ = DB_MAGIC;
    DBTAIL(data, begp, endp);
    while (begp < endp)
	*begp++ = DB_MAGIC;
}

/* Check to see if an address is in some data block of a region.
** This returns -(offset+1) if block is already freed, +(offset+1)
** if block is live, 0 if no match.
*/
#if __STD_C
static long dbaddr(Vmalloc_t * vm, Void_t * addr)
#else
static long dbaddr(vm, addr)
Vmalloc_t *vm;
Void_t *addr;
#endif
{
    reg Block_t *b = NULL, *endb = NULL;
    reg Seg_t *seg;
    reg Vmuchar_t *data;
    reg long offset = -1L;
    reg Vmdata_t *vd = vm->data;
    reg int local;

    GETLOCAL(vd, local);
    if (ISLOCK(vd, local) || !addr)
	return -1L;
    SETLOCK(vd, local);

    for (seg = vd->seg; seg; seg = seg->next) {
	b = SEGBLOCK(seg);
	endb = (Block_t *) (seg->baddr - sizeof(Head_t));
	if ((Vmuchar_t *) addr > (Vmuchar_t *) b &&
	    (Vmuchar_t *) addr < (Vmuchar_t *) endb)
	    break;
    }
    if (!seg)
	goto done;

    if (local) {		/* must be vmfree or vmresize checking address */
	if (DBSEG(addr) == seg) {
	    b = DBBLOCK(addr);
	    if (ISBUSY(SIZE(b)) && !ISJUNK(SIZE(b)))
		offset = 0;
	    else
		offset = -2L;
	}
	goto done;
    }

    while (b < endb) {
	data = (Vmuchar_t *) DATA(b);
	if ((Vmuchar_t *) addr >= data
	    && (Vmuchar_t *) addr < data + SIZE(b)) {
	    if (ISBUSY(SIZE(b)) && !ISJUNK(SIZE(b))) {
		data = DB2DEBUG(data);
		if ((Vmuchar_t *) addr >= data &&
		    (Vmuchar_t *) addr < data + DBSIZE(data))
		    offset = (Vmuchar_t *) addr - data;
	    }
	    goto done;
	}

	b = (Block_t *) ((Vmuchar_t *) DATA(b) + (SIZE(b) & ~BITS));
    }

  done:
    CLRLOCK(vd, local);
    return offset;
}


#if __STD_C
static long dbsize(Vmalloc_t * vm, Void_t * addr)
#else
static long dbsize(vm, addr)
Vmalloc_t *vm;
Void_t *addr;
#endif
{
    reg Block_t *b, *endb;
    reg Seg_t *seg;
    reg long size;
    reg Vmdata_t *vd = vm->data;

    if (ISLOCK(vd, 0))
	return -1L;
    SETLOCK(vd, 0);

    size = -1L;
    for (seg = vd->seg; seg; seg = seg->next) {
	b = SEGBLOCK(seg);
	endb = (Block_t *) (seg->baddr - sizeof(Head_t));
	if ((Vmuchar_t *) addr <= (Vmuchar_t *) b ||
	    (Vmuchar_t *) addr >= (Vmuchar_t *) endb)
	    continue;
	while (b < endb) {
	    if (addr == (Void_t *) DB2DEBUG(DATA(b))) {
		if (ISBUSY(SIZE(b)) && !ISJUNK(SIZE(b)))
		    size = (long) DBSIZE(addr);
		goto done;
	    }

	    b = (Block_t *) ((Vmuchar_t *) DATA(b) + (SIZE(b) & ~BITS));
	}
    }
  done:
    CLRLOCK(vd, 0);
    return size;
}

#if __STD_C
static Void_t *dballoc(Vmalloc_t * vm, size_t size)
#else
static Void_t *dballoc(vm, size)
Vmalloc_t *vm;
size_t size;
#endif
{
    reg size_t s;
    reg Vmuchar_t *data;
    reg char *file;
    reg int line;
    reg Vmdata_t *vd = vm->data;

    VMFILELINE(vm, file, line);

    if (ISLOCK(vd, 0)) {
	dbwarn(vm, NIL(Vmuchar_t *), 0, file, line, DB_ALLOC);
	return NIL(Void_t *);
    }
    SETLOCK(vd, 0);

    if (vd->mode & VM_DBCHECK)
	vmdbcheck(vm);

    s = ROUND(size, ALIGN) + DB_EXTRA;
    if (s < sizeof(Body_t))	/* no tiny blocks during Vmdebug */
	s = sizeof(Body_t);

    if (!(data = (Vmuchar_t *) KPVALLOC(vm, s, (*(Vmbest->allocf))))) {
	dbwarn(vm, NIL(Vmuchar_t *), DB_ALLOC, file, line, DB_ALLOC);
	goto done;
    }

    data = DB2DEBUG(data);
    dbsetinfo(data, size, file, line);

    if ((vd->mode & VM_TRACE) && _Vmtrace) {
	vm->file = file;
	vm->line = line;
	(*_Vmtrace) (vm, NIL(Vmuchar_t *), data, size, 0);
    }

    if (Dbnwatch > 0)
	dbwatch(vm, data, file, line, DB_ALLOC);

  done:
    CLRLOCK(vd, 0);
    return (Void_t *) data;
}


#if __STD_C
static int dbfree(Vmalloc_t * vm, Void_t * data)
#else
static int dbfree(vm, data)
Vmalloc_t *vm;
Void_t *data;
#endif
{
    char *file;
    int line;
    reg long offset;
    reg int *ip, *endip;
    reg Vmdata_t *vd = vm->data;

    VMFILELINE(vm, file, line);

    if (!data)
	return 0;

    if (ISLOCK(vd, 0)) {
	dbwarn(vm, NIL(Vmuchar_t *), 0, file, line, DB_FREE);
	return -1;
    }
    SETLOCK(vd, 0);

    if (vd->mode & VM_DBCHECK)
	vmdbcheck(vm);

    if ((offset = KPVADDR(vm, data, dbaddr)) != 0) {
	if (vm->disc->exceptf)
	    (void) (*vm->disc->exceptf) (vm, VM_BADADDR, data, vm->disc);
	dbwarn(vm, (Vmuchar_t *) data, offset == -1L ? 0 : 1, file, line,
	       DB_FREE);
	CLRLOCK(vd, 0);
	return -1;
    }

    if (Dbnwatch > 0)
	dbwatch(vm, data, file, line, DB_FREE);

    if ((vd->mode & VM_TRACE) && _Vmtrace) {
	vm->file = file;
	vm->line = line;
	(*_Vmtrace) (vm, (Vmuchar_t *) data, NIL(Vmuchar_t *),
		     DBSIZE(data), 0);
    }

    /* clear free space */
    ip = (int *) data;
    endip = ip + (DBSIZE(data) + sizeof(int) - 1) / sizeof(int);
    while (ip < endip)
	*ip++ = 0;

    CLRLOCK(vd, 0);
    return (*(Vmbest->freef)) (vm, (Void_t *) DB2BEST(data));
}

/*	Resizing an existing block */
#if __STD_C
static Void_t *dbresize(Vmalloc_t * vm, Void_t * addr, reg size_t size,
			int type)
#else
static Void_t *dbresize(vm, addr, size, type)
Vmalloc_t *vm;			/* region allocating from       */
Void_t *addr;			/* old block of data            */
reg size_t size;		/* new size                     */
int type;			/* !=0 for movable, >0 for copy */
#endif
{
    reg Vmuchar_t *data;
    reg size_t s, oldsize;
    reg long offset;
    char *file, *oldfile;
    int line, oldline;
    reg Vmdata_t *vd = vm->data;

    if (!addr) {
	oldsize = 0;
	data = (Vmuchar_t *) dballoc(vm, size);
	goto done;
    }
    if (size == 0) {
	(void) dbfree(vm, addr);
	return NIL(Void_t *);
    }

    VMFILELINE(vm, file, line);

    if (ISLOCK(vd, 0)) {
	dbwarn(vm, NIL(Vmuchar_t *), 0, file, line, DB_RESIZE);
	return NIL(Void_t *);
    }
    SETLOCK(vd, 0);

    if (vd->mode & VM_DBCHECK)
	vmdbcheck(vm);

    if ((offset = KPVADDR(vm, addr, dbaddr)) != 0) {
	if (vm->disc->exceptf)
	    (void) (*vm->disc->exceptf) (vm, VM_BADADDR, addr, vm->disc);
	dbwarn(vm, (Vmuchar_t *) addr, offset == -1L ? 0 : 1, file, line,
	       DB_RESIZE);
	CLRLOCK(vd, 0);
	return NIL(Void_t *);
    }

    if (Dbnwatch > 0)
	dbwatch(vm, addr, file, line, DB_RESIZE);

    /* Vmbest data block */
    data = DB2BEST(addr);
    oldsize = DBSIZE(addr);
    oldfile = DBFILE(addr);
    oldline = DBLINE(addr);

    /* do the resize */
    s = ROUND(size, ALIGN) + DB_EXTRA;
    if (s < sizeof(Body_t))
	s = sizeof(Body_t);
    data = (Vmuchar_t *) KPVRESIZE(vm, (Void_t *) data, s,
				   (type & ~VM_RSZERO),
				   (*(Vmbest->resizef)));
    if (!data) {		/* failed, reset data for old block */
	dbwarn(vm, NIL(Vmuchar_t *), DB_ALLOC, file, line, DB_RESIZE);
	dbsetinfo((Vmuchar_t *) addr, oldsize, oldfile, oldline);
    } else {
	data = DB2DEBUG(data);
	dbsetinfo(data, size, file, line);

	if ((vd->mode & VM_TRACE) && _Vmtrace) {
	    vm->file = file;
	    vm->line = line;
	    (*_Vmtrace) (vm, (Vmuchar_t *) addr, data, size, 0);
	}
	if (Dbnwatch > 0)
	    dbwatch(vm, data, file, line, DB_RESIZED);
    }

    CLRLOCK(vd, 0);

  done:if (data && (type & VM_RSZERO) && size > oldsize) {
	reg Vmuchar_t *d = data + oldsize, *ed = data + size;
	do {
	    *d++ = 0;
	} while (d < ed);
    }
    return (Void_t *) data;
}

/* compact any residual free space */
#if __STD_C
static int dbcompact(Vmalloc_t * vm)
#else
static int dbcompact(vm)
Vmalloc_t *vm;
#endif
{
    return (*(Vmbest->compactf)) (vm);
}

/* check for memory overwrites over all live blocks */
#if __STD_C
int vmdbcheck(Vmalloc_t * vm)
#else
int vmdbcheck(vm)
Vmalloc_t *vm;
#endif
{
    reg Block_t *b, *endb;
    reg Seg_t *seg;
    int rv;
    reg Vmdata_t *vd = vm->data;

    if (!(vd->mode & VM_MTDEBUG))
	return -1;

    rv = 0;
    for (seg = vd->seg; seg; seg = seg->next) {
	b = SEGBLOCK(seg);
	endb = (Block_t *) (seg->baddr - sizeof(Head_t));
	while (b < endb) {
	    reg Vmuchar_t *data, *begp, *endp;

	    if (ISJUNK(SIZE(b)) || !ISBUSY(SIZE(b)))
		goto next;

	    data = DB2DEBUG(DATA(b));
	    if (DBISBAD(data)) {	/* seen this before */
		rv += 1;
		goto next;
	    }

	    DBHEAD(data, begp, endp);
	    for (; begp < endp; ++begp)
		if (*begp != DB_MAGIC)
		    goto set_bad;

	    DBTAIL(data, begp, endp);
	    for (; begp < endp; ++begp) {
		if (*begp == DB_MAGIC)
		    continue;
	      set_bad:
		dbwarn(vm, data, begp - data, NIL(char *), 0, DB_CHECK);
		DBSETBAD(data);
		rv += 1;
		goto next;
	    }

	  next:b = (Block_t *) ((Vmuchar_t *) DATA(b) +
			     (SIZE(b) & ~BITS));
	}
    }

    return rv;
}

/* set/delete an address to watch */
#if __STD_C
Void_t *vmdbwatch(Void_t * addr)
#else
Void_t *vmdbwatch(addr)
Void_t *addr;			/* address to insert                    */
#endif
{
    reg int n;
    reg Void_t *out;

    out = NIL(Void_t *);
    if (!addr)
	Dbnwatch = 0;
    else {
	for (n = Dbnwatch - 1; n >= 0; --n)
	    if (Dbwatch[n] == addr)
		break;
	if (n < 0) {		/* insert */
	    if (Dbnwatch == S_WATCH) {	/* delete left-most */
		out = Dbwatch[0];
		Dbnwatch -= 1;
		for (n = 0; n < Dbnwatch; ++n)
		    Dbwatch[n] = Dbwatch[n + 1];
	    }
	    Dbwatch[Dbnwatch] = addr;
	    Dbnwatch += 1;
	}
    }
    return out;
}

#if __STD_C
static Void_t *dbalign(Vmalloc_t * vm, size_t size, size_t align)
#else
static Void_t *dbalign(vm, size, align)
Vmalloc_t *vm;
size_t size;
size_t align;
#endif
{
    reg Vmuchar_t *data;
    reg size_t s;
    reg char *file;
    reg int line;
    reg Vmdata_t *vd = vm->data;

    VMFILELINE(vm, file, line);

    if (size <= 0 || align <= 0)
	return NIL(Void_t *);

    if (!(vd->mode & VM_TRUST)) {
	if (ISLOCK(vd, 0))
	    return NIL(Void_t *);
	SETLOCK(vd, 0);
    }

    if ((s = ROUND(size, ALIGN) + DB_EXTRA) < sizeof(Body_t))
	s = sizeof(Body_t);

    if (!
	(data = (Vmuchar_t *) KPVALIGN(vm, s, align, (*(Vmbest->alignf)))))
	goto done;

    data += DB_HEAD;
    dbsetinfo(data, size, file, line);

    if ((vd->mode & VM_TRACE) && _Vmtrace) {
	vm->file = file;
	vm->line = line;
	(*_Vmtrace) (vm, NIL(Vmuchar_t *), data, size, align);
    }

  done:
    CLRLOCK(vd, 0);
    return (Void_t *) data;
}

static Vmethod_t _Vmdebug = {
    dballoc,
    dbresize,
    dbfree,
    dbaddr,
    dbsize,
    dbcompact,
    dbalign,
    VM_MTDEBUG
};

__DEFINE__(Vmethod_t *, Vmdebug, &_Vmdebug);
