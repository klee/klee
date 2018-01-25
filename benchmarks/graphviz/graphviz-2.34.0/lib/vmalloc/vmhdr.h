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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _VMHDR_H
#define _VMHDR_H	1
#ifndef _BLD_vmalloc
#define _BLD_vmalloc	1
#endif
#ifdef WIN32
#include <io.h>
#endif

/*	Common types, and macros for vmalloc functions.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/


#if _PACKAGE_ast

#if defined(__STDPP__directive) && defined(__STDPP__hide)
    __STDPP__directive pragma pp:hide getpagesize
#else
#define getpagesize	______getpagesize
#endif

#include	<ast.h>

#if defined(__STDPP__directive) && defined(__STDPP__hide)
    __STDPP__directive pragma pp:nohide getpagesize
#else
#undef	getpagesize
#endif

#else

#include	<ast_common.h>
#include	"FEATURE/vmalloc"

#endif /*_PACKAGE_ast*/

#undef free
#undef malloc
#undef realloc
#undef BITS

    typedef unsigned char Vmuchar_t;
    typedef unsigned long Vmulong_t;

    typedef union _head_u Head_t;
    typedef union _body_u Body_t;
    typedef struct _block_s Block_t;
    typedef struct _seg_s Seg_t;
    typedef struct _pfobj_s Pfobj_t;

#define NIL(t)		((t)0)
#define reg		register
#if __STD_C
#define NOTUSED(x)	(void)(x)
#else
#define NOTUSED(x)	(&x,1)
#endif

/* convert an address to an integral value */
#define VLONG(addr)	((Vmulong_t)((char*)(addr) - (char*)0) )

/* Round x up to a multiple of y. ROUND2 does powers-of-2 and ROUNDX does others */
#define ROUND2(x,y)	(((x) + ((y)-1)) & ~((y)-1))
#define ROUNDX(x,y)	((((x) + ((y)-1)) / (y)) * (y))
#define ROUND(x,y)	(((y)&((y)-1)) ? ROUNDX((x),(y)) : ROUND2((x),(y)) )

/* compute a value that is a common multiple of x and y */
#define MULTIPLE(x,y)	((x)%(y) == 0 ? (x) : (y)%(x) == 0 ? (y) : (y)*(x))

#ifndef DEBUG
#define ASSERT(p)
#define COUNT(n)
#else
    extern int printf _ARG_((const char *, ...));
#if defined(__LINE__) && defined(__FILE__)
#define PRFILELINE	printf("Assertion failed at %s:%d\n",__FILE__,__LINE__)
#else
#define PRFILELINE	0
#endif
#define ASSERT(p)	((p) ? 0 : (PRFILELINE, abort(), 0) )
#define COUNT(n)	((n) += 1)
#endif /*DEBUG*/
#define VMPAGESIZE	8192
#if _lib_getpagesize
#define GETPAGESIZE(x)	((x) ? (x) : \
			 (((x)=getpagesize()) < VMPAGESIZE ? ((x)=VMPAGESIZE) : (x)) )
#else
#define GETPAGESIZE(x)	((x) = VMPAGESIZE)
#endif
/* Blocks are allocated such that their sizes are 0%(BITS+1)
** This frees up enough low order bits to store state information
*/
#define BUSY		(01)	/* block is busy                                */
#define PFREE		(02)	/* preceding block is free                      */
#define JUNK		(04)	/* marked as freed but not yet processed        */
#define BITS		(07)	/* (BUSY|PFREE|JUNK)                            */
#define ALIGNB		(8)	/* size must be a multiple of BITS+1            */
#define ISBITS(w)	((w) & BITS)
#define CLRBITS(w)	((w) &= ~BITS)
#define CPYBITS(w,f)	((w) |= ((f)&BITS) )
#define ISBUSY(w)	((w) & BUSY)
#define SETBUSY(w)	((w) |= BUSY)
#define CLRBUSY(w)	((w) &= ~BUSY)
#define ISPFREE(w)	((w) & PFREE)
#define SETPFREE(w)	((w) |= PFREE)
#define CLRPFREE(w)	((w) &= ~PFREE)
#define ISJUNK(w)	((w) & JUNK)
#define SETJUNK(w)	((w) |= JUNK)
#define CLRJUNK(w)	((w) &= ~JUNK)
#define OFFSET(t,e)	((size_t)(&(((t*)0)->e)) )
/* these bits share the "mode" field with the public bits */
#define VM_AGAIN	0010000	/* research the arena for space */
#define VM_LOCK		0020000	/* region is locked             */
#define VM_LOCAL	0040000	/* local call, bypass lock      */
#define VM_UNUSED	0104060
#define VMETHOD(vd)	((vd)->mode&VM_METHODS)
/* test/set/clear lock state */
#define SETLOCAL(vd)	((vd)->mode |= VM_LOCAL)
#define GETLOCAL(vd,l)	(((l) = (vd)->mode&VM_LOCAL), ((vd)->mode &= ~VM_LOCAL) )
#define ISLOCK(vd,l)	((l) ? 0 : ((vd)->mode &  VM_LOCK) )
#define SETLOCK(vd,l)	((l) ? 0 : ((vd)->mode |= VM_LOCK) )
#define CLRLOCK(vd,l)	((l) ? 0 : ((vd)->mode &= ~VM_LOCK) )
/* local calls */
#define KPVALLOC(vm,sz,func)		(SETLOCAL((vm)->data), func((vm),(sz)) )
#define KPVALIGN(vm,sz,al,func)		(SETLOCAL((vm)->data), func((vm),(sz),(al)) )
#define KPVFREE(vm,d,func)		(SETLOCAL((vm)->data), func((vm),(d)) )
#define KPVRESIZE(vm,d,sz,mv,func)	(SETLOCAL((vm)->data), func((vm),(d),(sz),(mv)) )
#define KPVADDR(vm,addr,func)		(SETLOCAL((vm)->data), func((vm),(addr)) )
/* ALIGN is chosen so that a block can store all primitive types.
** It should also be a multiple of ALIGNB==(BITS+1) so the size field
** of Block_t will always be 0%(BITS+1) as noted above.
** Of paramount importance is the ALIGNA macro below. If the local compile
** environment is strange enough that the below method does not calculate
** ALIGNA right, then the code below should be commented out and ALIGNA
** redefined to the appropriate requirement.
*/
	union _align_u {
	char c, *cp;
	int i, *ip;
	long l, *lp;
	double d, *dp, ***dppp[8];
	size_t s, *sp;
	void (*fn) (void);
	union _align_u *align;
	Head_t *head;
	Body_t *body;
	Block_t *block;
	Vmuchar_t a[ALIGNB];
#if _long_double
	long double ld, *ldp;
#endif
    };
    struct _a_s {
	char c;
	union _align_u a;
    };
#define ALIGNA	(sizeof(struct _a_s) - sizeof(union _align_u))
    struct _align_s {
	char data[MULTIPLE(ALIGNA, ALIGNB)];
    };
#define ALIGN	sizeof(struct _align_s)

/* make sure that the head of a block is a multiple of ALIGN */
    struct _head_s {
	union {
	    Seg_t *seg;		/* the containing segment       */
	    Block_t *link;	/* possible link list usage     */
	    Pfobj_t *pf;	/* profile structure pointer    */
	    char *file;		/* for file name in Vmdebug     */
	} seg;
	union {
	    size_t size;	/* size of data area in bytes   */
	    Block_t *link;	/* possible link list usage     */
	    int line;		/* for line number in Vmdebug   */
	} size;
    };
#define HEADSIZE	ROUND(sizeof(struct _head_s),ALIGN)
    union _head_u {
	Vmuchar_t data[HEADSIZE];	/* to standardize size          */
	struct _head_s head;
    };

/* now make sure that the body of a block is a multiple of ALIGN */
    struct _body_s {
	Block_t *link;		/* next in link list            */
	Block_t *left;		/* left child in free tree      */
	Block_t *right;		/* right child in free tree     */
	Block_t **self;		/* self pointer when free       */
    };
#define BODYSIZE	ROUND(sizeof(struct _body_s),ALIGN)
    union _body_u {
	Vmuchar_t data[BODYSIZE];	/* to standardize size          */
	struct _body_s body;
    };

/* After all the songs and dances, we should now have:
**	sizeof(Head_t)%ALIGN == 0
**	sizeof(Body_t)%ALIGN == 0
** and	sizeof(Block_t) = sizeof(Head_t)+sizeof(Body_t)
*/
    struct _block_s {
	Head_t head;
	Body_t body;
    };

/* requirements for smallest block type */
    struct _tiny_s {
	Block_t *link;
	Block_t *self;
    };
#define TINYSIZE	ROUND(sizeof(struct _tiny_s),ALIGN)
#define S_TINY		7	/* # of tiny blocks     */
#define MAXTINY		(S_TINY*ALIGN + TINYSIZE)
#define TLEFT(b)	((b)->head.head.seg.link)	/* instead of LEFT      */
#define TINIEST(b)	(SIZE(b) == TINYSIZE)	/* this type uses TLEFT */

#define DIV(x,y)	((y) == 8 ? ((x)>>3) : (x)/(y) )
#define INDEX(s)	DIV((s)-TINYSIZE,ALIGN)

/* number of small block types that can be cached after free */
#define S_CACHE		7
#define MAXCACHE	(S_CACHE*ALIGN + TINYSIZE)
#define C_INDEX(s)	(s < MAXCACHE ? INDEX(s) : S_CACHE)

#define TINY(vd)	((vd)->tiny)
#define CACHE(vd)	((vd)->cache)

    typedef struct _vmdata_s {
	int mode;		/* current mode for region              */
	size_t incr;		/* allocate in multiple of this         */
	size_t pool;		/* size of an elt in a Vmpool region    */
	Seg_t *seg;		/* list of segments                     */
	Block_t *free;		/* most recent free block               */
	Block_t *wild;		/* wilderness block                     */
	Block_t *root;		/* root of free tree                    */
	Block_t *tiny[S_TINY];	/* small blocks                         */
	Block_t *cache[S_CACHE + 1];	/* delayed free blocks                */
    } Vmdata_t;

/* private parts of Vmalloc_t */
#define _VM_PRIVATE_ \
	Vmdisc_t*	disc;		/* discipline to get space		*/ \
	Vmdata_t*	data;		/* the real region data			*/ \
	Vmalloc_t*	next;	/* linked list of regions               */

#include	"vmalloc.h"

/* we don't use these here and they interfere with some local names */
#undef malloc
#undef free
#undef realloc

/* segment structure */
    struct _seg_s {
	Vmalloc_t *vm;		/* the region that holds this   */
	Seg_t *next;		/* next segment                 */
	Void_t *addr;		/* starting segment address     */
	size_t extent;		/* extent of segment            */
	Vmuchar_t *baddr;	/* bottom of usable memory      */
	size_t size;		/* allocable size               */
	Block_t *free;		/* recent free blocks           */
	Block_t *last;		/* Vmlast last-allocated block  */
    };

/* starting block of a segment */
#define SEGBLOCK(s)	((Block_t*)(((Vmuchar_t*)(s)) + ROUND(sizeof(Seg_t),ALIGN)))

/* short-hands for block data */
#define SEG(b)		((b)->head.head.seg.seg)
#define SEGLINK(b)	((b)->head.head.seg.link)
#define	SIZE(b)		((b)->head.head.size.size)
#define SIZELINK(b)	((b)->head.head.size.link)
#define LINK(b)		((b)->body.body.link)
#define LEFT(b)		((b)->body.body.left)
#define RIGHT(b)	((b)->body.body.right)
#define VM(b)		(SEG(b)->vm)

#define DATA(b)		((Void_t*)((b)->body.data) )
#define BLOCK(d)	((Block_t*)((char*)(d) - sizeof(Head_t)) )
#define SELF(b)		((Block_t**)((b)->body.data + SIZE(b) - sizeof(Block_t*)) )
#define LAST(b)		(*((Block_t**)(((char*)(b)) - sizeof(Block_t*)) ) )
#define NEXT(b)		((Block_t*)((b)->body.data + SIZE(b)) )

/* functions to manipulate link lists of elts of the same size */
#define SETLINK(b)	(RIGHT(b) =  (b) )
#define ISLINK(b)	(RIGHT(b) == (b) )
#define UNLINK(vd,b,i,t) \
		((((t) = LINK(b)) ? (LEFT(t) = LEFT(b)) : NIL(Block_t*) ), \
		 (((t) = LEFT(b)) ? (LINK(t) = LINK(b)) : (TINY(vd)[i] = LINK(b)) ) )

/* delete a block from a link list or the free tree.
** The test in the below macro is worth scratching your head a bit.
** Even though tiny blocks (size < BODYSIZE) are kept in separate lists,
** only the TINIEST ones require TLEFT(b) for the back link. Since this
** destroys the SEG(b) pointer, it must be carefully restored in bestsearch().
** Other tiny blocks have enough space to use the usual LEFT(b).
** In this case, I have also carefully arranged so that RIGHT(b) and
** SELF(b) can be overlapped and the test ISLINK() will go through.
*/
#define REMOVE(vd,b,i,t,func) \
		((!TINIEST(b) && ISLINK(b)) ? UNLINK((vd),(b),(i),(t)) : \
	 		func((vd),SIZE(b),(b)) )

/* see if a block is the wilderness block */
#define SEGWILD(b)	(((b)->body.data+SIZE(b)+sizeof(Head_t)) >= SEG(b)->baddr)
#define VMWILD(vd,b)	(((b)->body.data+SIZE(b)+sizeof(Head_t)) >= vd->seg->baddr)

#define VMFILELINE(vm,f,l)	((f) = (vm)->file, (vm)->file = NIL(char*), \
		 		 (l) = (vm)->line, (vm)->line = 0 )

/* The lay-out of a Vmprofile block is this:
**	seg_ size ----data---- _pf_ size
**	_________ ____________ _________
**	seg_, size: header required by Vmbest.
**	data:	actual data block.
**	_pf_:	pointer to the corresponding Pfobj_t struct
**	size:	the true size of the block.
** So each block requires an extra Head_t.
*/
#define PF_EXTRA   sizeof(Head_t)
#define PFDATA(d)  ((Head_t*)((Vmuchar_t*)(d)+(SIZE(BLOCK(d))&~BITS)-sizeof(Head_t)) )
#define PFOBJ(d)   (PFDATA(d)->head.seg.pf)
#define PFSIZE(d)  (PFDATA(d)->head.size.size)

/* The lay-out of a block allocated by Vmdebug is this:
**	seg_ size file size seg_ magi ----data---- --magi-- magi line
**	--------- --------- --------- ------------ -------- ---------
**	seg_,size: header required by Vmbest management.
**	file:	the file where it was created.
**	size:	the true byte count of the block
**	seg_:	should be the same as the previous seg_.
**		This allows the function vmregion() to work.
**	magi:	magic bytes to detect overwrites.
**	data:	the actual data block.
**	magi:	more magic bytes.
**	line:	the line number in the file where it was created.
** So for each allocated block, we'll need 3 extra Head_t.
*/

/* convenient macros for accessing the above fields */
#define DB_HEAD		(2*sizeof(Head_t))
#define DB_TAIL		(2*sizeof(Head_t))
#define DB_EXTRA	(DB_HEAD+DB_TAIL)
#define DBBLOCK(d)	((Block_t*)((Vmuchar_t*)(d) - 3*sizeof(Head_t)) )
#define DBBSIZE(d)	(SIZE(DBBLOCK(d)) & ~BITS)
#define DBSEG(d)	(((Head_t*)((Vmuchar_t*)(d) - sizeof(Head_t)))->head.seg.seg )
#define DBSIZE(d)	(((Head_t*)((Vmuchar_t*)(d) - 2*sizeof(Head_t)))->head.size.size )
#define DBFILE(d)	(((Head_t*)((Vmuchar_t*)(d) - 2*sizeof(Head_t)))->head.seg.file )
#define DBLN(d)		(((Head_t*)((Vmuchar_t*)DBBLOCK(d)+DBBSIZE(d)))->head.size.line )
#define DBLINE(d)	(DBLN(d) < 0 ? -DBLN(d) : DBLN(d))

/* forward/backward translation for addresses between Vmbest and Vmdebug */
#define DB2BEST(d)	((Vmuchar_t*)(d) - 2*sizeof(Head_t))
#define DB2DEBUG(b)	((Vmuchar_t*)(b) + 2*sizeof(Head_t))

/* set file and line number, note that DBLN > 0 so that DBISBAD will work  */
#define DBSETFL(d,f,l)	(DBFILE(d) = (f), DBLN(d) = (f) ? (l) : 1)

/* set and test the state of known to be corrupted */
#define DBSETBAD(d)	(DBLN(d) > 0 ? (DBLN(d) = -DBLN(d)) : -1)
#define DBISBAD(d)	(DBLN(d) <= 0)

#define DB_MAGIC	0255	/* 10101101     */

/* compute the bounds of the magic areas */
#define DBHEAD(d,begp,endp) \
		(((begp) = (Vmuchar_t*)(&DBSEG(d)) + sizeof(Seg_t*)), ((endp) = (d)) )
#define DBTAIL(d,begp,endp) \
		(((begp) = (Vmuchar_t*)(d)+DBSIZE(d)), ((endp) = (Vmuchar_t*)(&DBLN(d))) )

/* clear and copy functions */
#define INTCOPY(to,fr,n) \
	switch(n/sizeof(int)) \
	{ default: memcpy((Void_t*)to,(Void_t*)fr,n); break; \
	  case 7:	*to++ = *fr++; \
	  case 6:	*to++ = *fr++; \
	  case 5:	*to++ = *fr++; \
	  case 4:	*to++ = *fr++; \
	  case 3:	*to++ = *fr++; \
	  case 2:	*to++ = *fr++; \
	  case 1:	*to++ = *fr++; \
	}
#define INTZERO(d,n) \
	switch(n/sizeof(int)) \
	{ default: memset((Void_t*)d,0,n); break; \
	  case 7:	*d++ = 0; \
	  case 6:	*d++ = 0; \
	  case 5:	*d++ = 0; \
	  case 4:	*d++ = 0; \
	  case 3:	*d++ = 0; \
	  case 2:	*d++ = 0; \
	  case 1:	*d++ = 0; \
	}

/* external symbols for internal use by vmalloc */
    typedef Block_t *(*Vmsearch_f) _ARG_((Vmdata_t *, size_t, Block_t *));
    typedef struct _vmextern_ {
	Block_t *(*vm_extend) _ARG_((Vmalloc_t *, size_t, Vmsearch_f));
	int (*vm_truncate) _ARG_((Vmalloc_t *, Seg_t *, size_t, int));
	size_t vm_pagesize;
	char *(*vm_strcpy) _ARG_((char *, char *, int));
	char *(*vm_itoa) _ARG_((Vmulong_t, int));
	void (*vm_trace) _ARG_((Vmalloc_t *,
				Vmuchar_t *, Vmuchar_t *, size_t, size_t));
	void (*vm_pfclose) _ARG_((Vmalloc_t *));
    } Vmextern_t;

#define _Vmextend	(_Vmextern.vm_extend)
#define _Vmtruncate	(_Vmextern.vm_truncate)
#define _Vmpagesize	(_Vmextern.vm_pagesize)
#define _Vmstrcpy	(_Vmextern.vm_strcpy)
#define _Vmitoa		(_Vmextern.vm_itoa)
#define _Vmtrace	(_Vmextern.vm_trace)
#define _Vmpfclose	(_Vmextern.vm_pfclose)

     _BEGIN_EXTERNS_ extern Vmextern_t _Vmextern;


#if !_PACKAGE_ast

    extern size_t getpagesize _ARG_((void));

#ifndef WIN32
    extern void abort _ARG_((void));
    extern ssize_t write _ARG_((int, const void *, size_t));
#endif

#if !__STDC__ && !_hdr_stdlib
    extern size_t strlen _ARG_((const char *));
    extern char *strcpy _ARG_((char *, const char *));
    extern int strcmp _ARG_((const char *, const char *));
    extern int atexit _ARG_((void (*)(void)));
    extern char *getenv _ARG_((const char *));
    extern Void_t *memcpy _ARG_((Void_t *, const Void_t *, size_t));
    extern Void_t *memset _ARG_((Void_t *, int, size_t));
#else
#ifndef cfree
#define cfree ______cfree
#endif
#include	<stdlib.h>
#undef cfree
#include	<string.h>
#endif

/* for malloc.c */
#ifndef WIN32
    extern int creat _ARG_((const char *, int));
    extern int close _ARG_((int));
#endif
    extern int getpid _ARG_((void));

/* for vmexit.c */
#ifndef WIN32
    extern int onexit _ARG_((void (*)(void)));
    extern void _exit _ARG_((int));
#endif
    extern void _cleanup _ARG_((void));

#endif				/*!PACKAGE_ast */

/* for vmdcsbrk.c */
#if !_typ_ssize_t
    typedef int ssize_t;
#endif
#if !_WIN32
    extern Vmuchar_t *sbrk _ARG_((ssize_t));
#endif

     _END_EXTERNS_
#endif				/* _VMHDR_H */
#ifdef __cplusplus
}
#endif
