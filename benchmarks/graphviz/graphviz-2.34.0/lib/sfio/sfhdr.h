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

#ifndef _SFHDR_H
#define _SFHDR_H	1
#if !defined(_BLD_sfio) && !defined(_BLD_stdio)
#define _BLD_sfio	1
#endif

/*	Internal definitions for sfio.
**	Written by Kiem-Phong Vo
*/

#include	"FEATURE/sfio"
#include	"sfio_t.h"

/* note that the macro vt_threaded has effect on vthread.h */
#include	<vthread.h>

/* file system info */
#if _PACKAGE_ast

#include	<ast.h>
#include	<ast_time.h>
#include	<ast_tty.h>
#include	<ls.h>

#if _mem_st_blksize_stat
#define _stat_blksize	1
#endif

#if _lib_localeconv && _hdr_locale
#define _lib_locale	1
#endif

#else				/*!_PACKAGE_ast */

#if __mips == 2 && !defined(_NO_LARGEFILE64_SOURCE)
#define _NO_LARGEFILE64_SOURCE  1
#endif
#if !defined(_NO_LARGEFILE64_SOURCE) && \
	_lib_lseek64 && _lib_stat64 && _lib_mmap64 && _typ_off64_t && _typ_struct_stat64
#	if !defined(_LARGEFILE64_SOURCE)
#	define _LARGEFILE64_SOURCE     1
#	endif
#else
#	undef  _LARGEFILE64_SOURCE
#endif

/* when building the binary compatibility package, a number of header files
   are not needed and they may get in the way so we remove them here.
*/
#if _SFBINARY_H
#undef  _hdr_time
#undef  _sys_time
#undef  _sys_stat
#undef  _hdr_stat
#undef  _hdr_filio
#undef  _sys_filio
#undef  _lib_poll
#undef  _stream_peek
#undef  _socket_peek
#undef  _hdr_vfork
#undef  _sys_vfork
#undef  _lib_vfork
#undef  _hdr_floatingpoint
#undef  _hdr_float
#undef  _hdr_values
#undef  _hdr_limits
#undef  _hdr_math
#undef  _sys_mman
#undef  _hdr_mman
#undef  _sys_ioctl
#endif

#if _hdr_stdlib
#include	<stdlib.h>
#endif

#if _hdr_string
#include	<string.h>
#endif

#if _hdr_stdint
#include	<stdint.h>
#endif

#if _hdr_stddef
#include	<stddef.h>
#endif

#if _hdr_time
#include	<time.h>
#endif
#if _sys_time
#include	<sys/time.h>
#endif

#if _sys_stat
#include	<sys/stat.h>
#else
#if _hdr_stat
#include	<stat.h>
#ifndef _sys_stat
#define	_sys_stat	1
#endif
#endif
#endif /*_sys_stat*/

#ifndef _sys_stat
#define _sys_stat	0
#endif

#include	<fcntl.h>

#ifndef F_SETFD
#ifndef FIOCLEX
#if _hdr_filio
#include	<filio.h>
#else
#if _sys_filio
#include	<sys/filio.h>
#endif /*_sys_filio*/
#endif /*_hdr_filio*/
#endif /*_FIOCLEX*/
#endif				/*F_SETFD */

#if _hdr_unistd
#include	<unistd.h>
#endif

#endif /*_PACKAGE_ast*/

#include	<errno.h>
#include	<ctype.h>

#if vt_threaded

/* initialization */
#define SFONCE()	(_Sfdone ? 0 : vtonce(_Sfonce,_Sfoncef))

/* to lock/unlock a stream on entering and returning from some function */
#define SFMTXLOCK(f)	 (((f)->flags&SF_MTSAFE) ? sfmutex(f,SFMTX_LOCK) : 0)
#define SFMTXUNLOCK(f)	 (((f)->flags&SF_MTSAFE) ? sfmutex(f,SFMTX_UNLOCK) : 0)
#define SFMTXSTART(f,v)  { if(!f || SFMTXLOCK(f) != 0) return(v); }
#define SFMTXRETURN(f,v) { SFMTXUNLOCK(f); return(v); }

/* start and end critical region for a pool */
#define POOLMTXLOCK(p)		( vtmtxlock(&(p)->mutex) )
#define POOLMTXUNLOCK(p)	( vtmtxunlock(&(p)->mutex) )
#define POOLMTXSTART(p)		{ POOLMTXLOCK(p); }
#define POOLMTXRETURN(p,v)	{ POOLMTXUNLOCK(p); return(v); }

#else				/*!vt_threaded */

#undef SF_MTSAFE		/* no need to worry about thread-safety */
#define SF_MTSAFE		0

#define SFONCE()		(void)(0)

#define SFMTXLOCK(f)		(void)(0)
#define SFMTXUNLOCK(f)		(void)(0)
#define SFMTXSTART(f,v)		{ if(!f) return(v); }
#define SFMTXRETURN(f,v)	{ return(v); }

#define POOLMTXLOCK(p)
#define POOLMTXUNLOCK(p)
#define POOLMTXSTART(p)
#define POOLMTXRETURN(p,v)	{ return(v); }

#endif				/*vt_threaded */


/* functions for polling readiness of streams */
#if _lib_select
#undef _lib_poll
#else
#if _lib_poll_fd_1 || _lib_poll_fd_2
#define _lib_poll	1
#endif
#endif /*_lib_select_*/

#if _lib_poll
#include	<poll.h>

#if _lib_poll_fd_1
#define SFPOLL(pfd,n,tm)	poll((pfd),(ulong)(n),(tm))
#else
#define SFPOLL(pfd,n,tm)	poll((ulong)(n),(pfd),(tm))
#endif
#endif /*_lib_poll*/

#if _stream_peek
#include	<stropts.h>
#endif

#if _socket_peek
#include	<sys/socket.h>
#endif

/* to test for executable access mode of a file */
#ifndef X_OK
#define X_OK	01
#endif

/* alternative process forking */
#if _lib_vfork && !defined(fork) && !defined(sparc) && !defined(__sparc)
#if _hdr_vfork
#include	<vfork.h>
#endif
#if _sys_vfork
#include	<sys/vfork.h>
#endif
#define fork	vfork
#endif

#if _lib_unlink
#define remove	unlink
#endif

#if _hdr_math
#include	<math.h>
#if !defined(SF_MAXDOUBLE) && defined(MAXDOUBLE)
#define SF_MAXDOUBLE	MAXDOUBLE
#endif
#if !defined(SF_MAXDOUBLE) && defined(DBL_MAX)
#define SF_MAXDOUBLE	DBL_MAX
#endif
#endif

#ifdef MAXFLOAT			/* on some platforms, these are defined in both values.h and math.h */
#undef MAXFLOAT			/* we don't need them so we zap them here to avoid compiler warnings */
#endif
#ifdef MAXSHORT
#undef MAXSHORT
#endif
#ifdef MAXINT
#undef MAXINT
#endif
#ifdef MAXLONG
#undef MAXLONG
#endif

#if _hdr_limits
#include	<limits.h>
#else
#if _hdr_values
#include	<values.h>
#if !defined(SF_MAXDOUBLE) && defined(MAXDOUBLE)
#define SF_MAXDOUBLE	MAXDOUBLE
#endif
#if !defined(SF_MAXDOUBLE) && defined(DBL_MAX)
#define SF_MAXDOUBLE	DBL_MAX
#endif
#endif
#endif

#if !defined(SF_MAXDOUBLE) && _hdr_floatingpoint
#include	<floatingpoint.h>
#if !defined(SF_MAXDOUBLE) && defined(MAXDOUBLE)
#define SF_MAXDOUBLE	MAXDOUBLE
#endif
#if !defined(SF_MAXDOUBLE) && defined(DBL_MAX)
#define SF_MAXDOUBLE	DBL_MAX
#endif
#endif

#if !defined(SF_MAXDOUBLE) && _hdr_float
#include	<float.h>
#if !defined(SF_MAXDOUBLE) && defined(MAXDOUBLE)
#define SF_MAXDOUBLE	MAXDOUBLE
#endif
#if !defined(SF_MAXDOUBLE) && defined(DBL_MAX)
#define SF_MAXDOUBLE	DBL_MAX
#endif
#endif

#if !_ast_fltmax_double

#if !defined(SF_MAXDOUBLE)
#define SF_MAXDOUBLE	1.79769313486231570e+308
#endif

#if _lib_qfrexp && _lib_qldexp
#define _has_expfuncs	1
#define frexp		qfrexp
#define ldexp		qldexp
#else
#define _has_expfuncs	0
#endif

#endif/*_ast_fltmax_double*/

/* 64-bit vs 32-bit file stuff */
#if _sys_stat
#ifdef _LARGEFILE64_SOURCE
    typedef struct stat64 Stat_t;
#define	lseek		lseek64
#define stat		stat64
#define fstat		fstat64
#define off_t		off64_t
#else
    typedef struct stat Stat_t;
#endif
#endif

/* to get rid of pesky compiler warnings */
#if __STD_C
#define NOTUSED(x)	(void)(x)
#else
#define NOTUSED(x)	(&x,1)
#endif

/* Private flags in the "bits" field */
#define SF_MMAP		00000001	/* in memory mapping mode               */
#define SF_BOTH		00000002	/* both read/write                      */
#define SF_HOLE		00000004	/* a hole of zero's was created         */
#define SF_NULL		00000010	/* stream is /dev/null                  */
#define SF_SEQUENTIAL	00000020	/* sequential access                    */
#define SF_JUSTSEEK	00000040	/* just did a sfseek                    */

/* this bit signals sfmutex() not to create a mutex for a private stream */
#define SF_PRIVATE	00000200	/* private stream to Sfio               */

/* on closing, don't be a hero about reread/rewrite on interrupts */
#define SF_ENDING	00000400

/* private flags that must be cleared in sfclrlock */
#define SF_DCDOWN	00001000	/* recurse down the discipline stack    */
#define SF_MVSIZE	00002000
#define SFMVSET(f)	(((f)->size *= SF_NMAP), ((f)->bits |= SF_MVSIZE) )
#define SFMVUNSET(f)	(!((f)->bits&SF_MVSIZE) ? 0 : \
				(((f)->bits &= ~SF_MVSIZE), ((f)->size /= SF_NMAP)) )
#define SFCLRBITS(f)	(SFMVUNSET(f), ((f)->bits &= ~(SF_DCDOWN|SF_MVSIZE)) )

/* bits for the mode field, SF_INIT defined in sfio_t.h */
#define SF_RC		00000010	/* peeking for a record                 */
#define SF_RV		00000020	/* reserve without read or most write   */
#define SF_LOCK		00000040	/* stream is locked for io op           */
#define SF_PUSH		00000100	/* stream has been pushed               */
#define SF_POOL		00000200	/* stream is in a pool but not current  */
#define SF_PEEK		00000400	/* there is a pending peek              */
#define SF_PKRD		00001000	/* did a peek read                      */
#define SF_GETR		00002000	/* did a getr on this stream            */
#define SF_SYNCED	00004000	/* stream was synced                    */
#define SF_STDIO	00010000	/* given up the buffer to stdio         */
#define SF_AVAIL	00020000	/* was closed, available for reuse      */
#define SF_LOCAL	00100000	/* sentinel for a local call            */

#ifdef DEBUG
#define ASSERT(p)	((p) ? 0 : (abort(),0) )
#else
#define ASSERT(p)
#endif

/* short-hands */
#define NIL(t)		((t)0)
#define reg		register
#ifndef uchar
#define uchar		unsigned char
#endif
#ifndef ulong
#define ulong		unsigned long
#endif
#ifndef uint
#define uint		unsigned int
#endif
#ifndef ushort
#define ushort		unsigned short
#endif

#define SECOND		1000	/* millisecond units */

/* macros do determine stream types from Stat_t data */
#ifndef S_IFMT
#define S_IFMT	0
#endif
#ifndef S_IFDIR
#define S_IFDIR	0
#endif
#ifndef S_IFREG
#define S_IFREG	0
#endif
#ifndef S_IFCHR
#define S_IFCHR	0
#endif
#ifndef S_IFIFO
#define S_IFIFO	0
#endif

#ifndef S_ISDIR
#define S_ISDIR(m)	(((m)&S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m)	(((m)&S_IFMT) == S_IFREG)
#endif
#ifndef S_ISCHR
#define S_ISCHR(m)	(((m)&S_IFMT) == S_IFCHR)
#endif

#ifndef S_ISFIFO
#	ifdef S_IFIFO
#		define S_ISFIFO(m)	(((m)&S_IFMT) == S_IFIFO)
#	else
#		define S_ISFIFO(m)	(0)
#	endif
#endif

#if defined(S_IRUSR) && defined(S_IWUSR) && defined(S_IRGRP) && defined(S_IWGRP) && defined(S_IROTH) && defined(S_IWOTH)
#define SF_CREATMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#else
#define SF_CREATMODE	0666
#endif

/* set close-on-exec */
#ifdef F_SETFD
#	ifndef FD_CLOEXEC
#		define FD_CLOEXEC	1
#	endif			/*FD_CLOEXEC */
#	define SETCLOEXEC(fd)		((void)fcntl((fd),F_SETFD,FD_CLOEXEC))
#else
#	ifdef FIOCLEX
#		define SETCLOEXEC(fd)	((void)ioctl((fd),FIOCLEX,0))
#	else
#		define SETCLOEXEC(fd)
#	endif /*FIOCLEX*/
#endif				/*F_SETFD */
/* a couple of error number that we use, default values are like Linux */
#ifndef EINTR
#define EINTR	4
#endif
#ifndef EBADF
#define EBADF	9
#endif
#ifndef EAGAIN
#define EAGAIN	11
#endif
#ifndef ENOMEM
#define ENOMEM	12
#endif
#ifndef EINVAL
#define EINVAL	22
#endif
#ifndef ESPIPE
#define ESPIPE	29
#endif
/* see if we can use memory mapping for io */
#if !_PACKAGE_ast && _mmap_worthy
#	ifdef _LARGEFILE64_SOURCE
#		undef	mmap
#	endif
#	if _hdr_mman
#		include	<mman.h>
#	endif
#	if _sys_mman
#		include	<sys/mman.h>
#	endif
#	ifdef _LARGEFILE64_SOURCE
#		ifndef off_t
#			define off_t		off64_t
#		endif
#		define mmap		mmap64
#	endif
#endif
/* function to get the decimal point for local environment */
#if _lib_locale
#ifdef MAXFLOAT			/* we don't need these, so we zap them to avoid compiler warnings */
#undef MAXFLOAT
#endif
#ifdef MAXSHORT
#undef MAXSHORT
#endif
#ifdef MAXINT
#undef MAXINT
#endif
#ifdef MAXLONG
#undef MAXLONG
#endif
#include	<locale.h>
#define SFSETLOCALE(decimal,thousand) \
	{ struct lconv*	lv; \
	  if(*(decimal) == 0) \
	  { *(decimal) = '.'; \
	    if((lv = localeconv())) \
	    { if(lv->decimal_point && lv->decimal_point[0]) \
	    	*(decimal) = lv->decimal_point[0]; \
	      if(thousand != NULL && lv->thousands_sep && lv->thousands_sep[0]) \
	    	*(thousand) = lv->thousands_sep[0]; \
	    } \
	  } \
	}
#else
#define SFSETLOCALE(decimal,thousand)
#endif
/* stream pool structure. */
    typedef struct _sfpool_s Sfpool_t;
    struct _sfpool_s {
	Sfpool_t *next;
	int mode;		/* type of pool                 */
	int s_sf;		/* size of pool array           */
	int n_sf;		/* number currently in pool     */
	Sfio_t **sf;		/* array of streams             */
	Sfio_t *array[3];	/* start with 3                 */
	Vtmutex_t mutex;	/* mutex lock object            */
    };

/* reserve buffer structure */
    typedef struct _sfrsrv_s Sfrsrv_t;
    struct _sfrsrv_s {
	ssize_t slen;		/* last string length           */
	ssize_t size;		/* buffer size                  */
	uchar data[1];		/* data buffer                  */
    };

/* co-process structure */
    typedef struct _sfproc_s Sfproc_t;
    struct _sfproc_s {
	int pid;		/* process id                   */
	uchar *rdata;		/* read data being cached       */
	int ndata;		/* size of cached data          */
	int size;		/* buffer size                  */
	int file;		/* saved file descriptor        */
	int sigp;		/* sigpipe protection needed    */
    };

/* extensions to sfvprintf/sfvscanf */
#define FP_SET(fp,fn)	(fp < 0 ? (fn += 1) : (fn = fp) )
#define FP_WIDTH	0
#define FP_PRECIS	1
#define FP_BASE		2
#define FP_STR		3
#define FP_SIZE		4
#define FP_INDEX	5	/* index size   */

    typedef struct _fmt_s Fmt_t;
    typedef struct _fmtpos_s Fmtpos_t;
    typedef union {
	int i, *ip;
	long l, *lp;
	short h, *hp;
	uint ui;
	ulong ul;
	ushort uh;
	Sflong_t ll, *llp;
	Sfulong_t lu;
	Sfdouble_t ld;
	double d;
	float f;
	char c, *s, **sp;
	Void_t *vp;
	Sffmt_t *ft;
    } Argv_t;

    struct _fmt_s {
	char *form;		/* format string                */
	va_list args;		/* corresponding arglist        */

	char *oform;		/* original format string       */
	va_list oargs;		/* original arg list            */
	int argn;		/* number of args already used  */
	Fmtpos_t *fp;		/* position list                */

	Sffmt_t *ft;		/* formatting environment       */
	Sffmtevent_f eventf;	/* event function               */
	Fmt_t *next;		/* stack frame pointer          */
    };

    struct _fmtpos_s {
	Sffmt_t ft;		/* environment                  */
	Argv_t argv;		/* argument value               */
	int fmt;		/* original format              */
	int need[FP_INDEX];	/* positions depending on       */
    };

#define LEFTP		'('
#define RIGHTP		')'
#define QUOTE		'\''

#ifndef CHAR_BIT
#define CHAR_BIT	8
#endif

#define FMTSET(ft, frm,ags, fv, sz, flgs, wid,pr,bs, ts,ns) \
	((ft->form = (char*)frm), va_copy(ft->args,ags), \
	 (ft->fmt = fv), (ft->size = sz), \
	 (ft->flags = (flgs&SFFMT_SET)), \
	 (ft->width = wid), (ft->precis = pr), (ft->base = bs), \
	 (ft->t_str = ts), (ft->n_str = ns) )
#define FMTGET(ft, frm,ags, fv, sz, flgs, wid,pr,bs) \
	((frm = ft->form), va_copy(ags,ft->args), (fv = ft->fmt), (sz = ft->size), \
	 (flgs = (flgs&~(SFFMT_SET))|(ft->flags&SFFMT_SET)), \
	 (wid = ft->width), (pr = ft->precis), (bs = ft->base) )
#define FMTCMP(sz, type, maxtype) \
	(sz == sizeof(type) || (sz == 0 && sizeof(type) == sizeof(maxtype)) || \
	 (sz == 64 && sz == sizeof(type)*CHAR_BIT) )

/* format flags&types, must coexist with those in sfio.h */
#define SFFMT_FORBIDDEN 00077777777	/* for sfio.h only      */
#define SFFMT_EFORMAT	01000000000	/* sfcvt converting %e  */
#define SFFMT_MINUS	02000000000	/* minus sign           */

#define SFFMT_TYPES	(SFFMT_SHORT|SFFMT_SSHORT | SFFMT_LONG|SFFMT_LLONG|\
			 SFFMT_LDOUBLE | SFFMT_IFLAG|SFFMT_JFLAG| \
			 SFFMT_TFLAG | SFFMT_ZFLAG )

/* type of elements to be converted */
#define SFFMT_INT	001	/* %d,%i                */
#define SFFMT_UINT	002	/* %u,o,x etc.          */
#define SFFMT_FLOAT	004	/* %f,e,g etc.          */
#define SFFMT_BYTE	010	/* %c                   */
#define SFFMT_POINTER	020	/* %p, %n               */
#define SFFMT_CLASS	040	/* %[                   */

/* local variables used across sf-functions */
#define _Sfpage		(_Sfextern.sf_page)
#define _Sfpool		(_Sfextern.sf_pool)
#define _Sfpmove	(_Sfextern.sf_pmove)
#define _Sfstack	(_Sfextern.sf_stack)
#define _Sfnotify	(_Sfextern.sf_notify)
#define _Sfstdsync	(_Sfextern.sf_stdsync)
#define _Sfudisc	(&(_Sfextern.sf_udisc))
#define _Sfcleanup	(_Sfextern.sf_cleanup)
#define _Sfexiting	(_Sfextern.sf_exiting)
#define _Sfdone		(_Sfextern.sf_done)
#define _Sfonce		(_Sfextern.sf_once)
#define _Sfoncef	(_Sfextern.sf_oncef)
#define _Sfmutex	(_Sfextern.sf_mutex)
    typedef struct _sfextern_s {
	ssize_t sf_page;
	struct _sfpool_s sf_pool;
	int (*sf_pmove) _ARG_((Sfio_t *, int));
	Sfio_t *(*sf_stack) _ARG_((Sfio_t *, Sfio_t *));
	void (*sf_notify) _ARG_((Sfio_t *, int, int));
	int (*sf_stdsync) _ARG_((Sfio_t *));
	struct _sfdisc_s sf_udisc;
	void (*sf_cleanup) _ARG_((void));
	int sf_exiting;
	int sf_done;
	Vtonce_t *sf_once;
	void (*sf_oncef) _ARG_((void));
	Vtmutex_t *sf_mutex;
    } Sfextern_t;

/* get the real value of a byte in a coded long or ulong */
#define SFUVALUE(v)	(((ulong)(v))&(SF_MORE-1))
#define SFSVALUE(v)	((( long)(v))&(SF_SIGN-1))
#define SFBVALUE(v)	(((ulong)(v))&(SF_BYTE-1))

/* amount of precision to get in each iteration during coding of doubles */
#define SF_PRECIS	(SF_UBITS-1)

/* grain size for buffer increment */
#define SF_GRAIN	1024
#define SF_PAGE		((ssize_t)(SF_GRAIN*sizeof(int)*2))

/* when the buffer is empty, certain io requests may be better done directly
   on the given application buffers. The below condition determines when.
*/
#define SFDIRECT(f,n)	(((ssize_t)(n) >= (f)->size) || \
			 ((n) >= SF_GRAIN && (ssize_t)(n) >= (f)->size/16 ) )

/* number of pages to memory map at a time */
#define SF_NMAP		8

/* set/unset sequential states for mmap */
#if _lib_madvise && defined(MADV_SEQUENTIAL) && defined(MADV_NORMAL)
#define SFMMSEQON(f,a,s)	(void)(madvise((caddr_t)(a),(size_t)(s),MADV_SEQUENTIAL) )
#define SFMMSEQOFF(f,a,s)	(void)(madvise((caddr_t)(a),(size_t)(s),MADV_NORMAL) )
#else
#define SFMMSEQON(f,a,s)
#define SFMMSEQOFF(f,a,s)
#endif

#define SFMUNMAP(f,a,s)		(munmap((caddr_t)(a),(size_t)(s)), \
				 ((f)->endb = (f)->endr = (f)->endw = (f)->next = \
				  (f)->data = NIL(uchar*)) )

#ifndef MAP_VARIABLE
#define MAP_VARIABLE	0
#endif
#ifndef _mmap_fixed
#define _mmap_fixed	0
#endif

/* the bottomless bit bucket */
#define DEVNULL		"/dev/null"
#define SFSETNULL(f)	((f)->extent = (Sfoff_t)(-1), (f)->bits |= SF_NULL)
#define SFISNULL(f)	((f)->extent < 0 && ((f)->bits&SF_NULL) )

#define SFKILL(f)	((f)->mode = (SF_AVAIL|SF_LOCK) )
#define SFKILLED(f)	(((f)->mode&(SF_AVAIL|SF_LOCK)) == (SF_AVAIL|SF_LOCK) )

/* exception types */
#define SF_EDONE	0	/* stop this operation and return       */
#define SF_EDISC	1	/* discipline says it's ok              */
#define SF_ESTACK	2	/* stack was popped                     */
#define SF_ECONT	3	/* can continue normally                */

#define SETLOCAL(f)	((f)->mode |= SF_LOCAL)
#define GETLOCAL(f,v)	((v) = ((f)->mode&SF_LOCAL), (f)->mode &= ~SF_LOCAL, (v))
#define SFWRALL(f)	((f)->mode |= SF_RV)
#define SFISALL(f,v)	((((v) = (f)->mode&SF_RV) ? ((f)->mode &= ~SF_RV) : 0), \
			 ((v) || (f)->extent < 0 || \
			  ((f)->flags&(SF_SHARE|SF_APPENDWR|SF_WHOLE)) ) )
#define SFSK(f,a,o,d)	(SETLOCAL(f),sfsk(f,(Sfoff_t)a,o,d))
#define SFRD(f,b,n,d)	(SETLOCAL(f),sfrd(f,(Void_t*)b,n,d))
#define SFWR(f,b,n,d)	(SETLOCAL(f),sfwr(f,(Void_t*)b,n,d))
#define SFSYNC(f)	(SETLOCAL(f),sfsync(f))
#define SFCLOSE(f)	(SETLOCAL(f),sfclose(f))
#define SFFLSBUF(f,n)	(SETLOCAL(f),_sfflsbuf(f,n))
#define SFFILBUF(f,n)	(SETLOCAL(f),_sffilbuf(f,n))
#define SFSETBUF(f,s,n)	(SETLOCAL(f),sfsetbuf(f,s,n))
#define SFWRITE(f,s,n)	(SETLOCAL(f),sfwrite(f,s,n))
#define SFREAD(f,s,n)	(SETLOCAL(f),sfread(f,s,n))
#define SFSEEK(f,p,t)	(SETLOCAL(f),sfseek(f,p,t))
#define SFNPUTC(f,c,n)	(SETLOCAL(f),sfnputc(f,c,n))
#define SFRAISE(f,e,d)	(SETLOCAL(f),sfraise(f,e,d))

/* lock/open a stream */
#define SFMODE(f,l)	((f)->mode & ~(SF_RV|SF_RC|((l) ? SF_LOCK : 0)) )
#define SFLOCK(f,l)	(void)((f)->mode |= SF_LOCK, (f)->endr = (f)->endw = (f)->data)
#define _SFOPENRD(f)	((f)->endr = ((f)->flags&SF_MTSAFE) ? (f)->data : (f)->endb)
#define _SFOPENWR(f)	((f)->endw = ((f)->flags&(SF_MTSAFE|SF_LINE)) ? (f)->data : (f)->endb)
#define _SFOPEN(f)	((f)->mode == SF_READ  ? _SFOPENRD(f) : \
			 (f)->mode == SF_WRITE ? _SFOPENWR(f) : \
			 ((f)->endw = (f)->endr = (f)->data) )
#define SFOPEN(f,l)	(void)((l) ? 0 : \
				((f)->mode &= ~(SF_LOCK|SF_RC|SF_RV), _SFOPEN(f), 0) )

/* check to see if the stream can be accessed */
#define SFFROZEN(f)	((f)->mode&(SF_PUSH|SF_LOCK|SF_PEEK) ? 1 : \
			 ((f)->mode&SF_STDIO) ? (*_Sfstdsync)(f) : 0)


/* set discipline code */
#define SFDISC(f,dc,iof) \
	{	Sfdisc_t* d; \
		if(!(dc)) \
			d = (dc) = (f)->disc; \
		else 	d = (f->bits&SF_DCDOWN) ? ((dc) = (dc)->disc) : (dc); \
		while(d && !(d->iof))	d = d->disc; \
		if(d)	(dc) = d; \
	}
#define SFDCRD(f,buf,n,dc,rv) \
	{	int		dcdown = f->bits&SF_DCDOWN; f->bits |= SF_DCDOWN; \
		rv = (*dc->readf)(f,buf,n,dc); \
		if(!dcdown)	f->bits &= ~SF_DCDOWN; \
	}
#define SFDCWR(f,buf,n,dc,rv) \
	{	int		dcdown = f->bits&SF_DCDOWN; f->bits |= SF_DCDOWN; \
		rv = (*dc->writef)(f,buf,n,dc); \
		if(!dcdown)	f->bits &= ~SF_DCDOWN; \
	}
#define SFDCSK(f,addr,type,dc,rv) \
	{	int		dcdown = f->bits&SF_DCDOWN; f->bits |= SF_DCDOWN; \
		rv = (*dc->seekf)(f,addr,type,dc); \
		if(!dcdown)	f->bits &= ~SF_DCDOWN; \
	}

/* fast peek of a stream */
#define _SFAVAIL(f,s,n)	((n) = (f)->endb - ((s) = (f)->next) )
#define SFRPEEK(f,s,n)	(_SFAVAIL(f,s,n) > 0 ? (n) : \
				((n) = SFFILBUF(f,-1), (s) = (f)->next, (n)) )
#define SFWPEEK(f,s,n)	(_SFAVAIL(f,s,n) > 0 ? (n) : \
				((n) = SFFLSBUF(f,-1), (s) = (f)->next, (n)) )

/* more than this for a line buffer, we might as well flush */
#define HIFORLINE	128

/* safe closing function */
#define CLOSE(f)	{ while(close(f) < 0 && errno == EINTR) errno = 0; }

/* string stream extent */
#define SFSTRSIZE(f)	{ Sfoff_t s = (f)->next - (f)->data; \
			  if(s > (f)->here) \
			    { (f)->here = s; if(s > (f)->extent) (f)->extent = s; } \
			}

/* control flags for open() */
#ifdef O_CREAT
#define _has_oflags	1
#else				/* for example, research UNIX */
#define _has_oflags	0
#define O_CREAT		004
#define O_TRUNC		010
#define O_APPEND	020
#define O_EXCL		040

#ifndef O_RDONLY
#define	O_RDONLY	000
#endif
#ifndef O_WRONLY
#define O_WRONLY	001
#endif
#ifndef O_RDWR
#define O_RDWR		002
#endif
#endif				/*O_CREAT */

#ifndef O_BINARY
#define O_BINARY	000
#endif
#ifndef O_TEXT
#define O_TEXT		000
#endif
#ifndef O_TEMPORARY
#define O_TEMPORARY	000
#endif

#define	SF_RADIX	64	/* maximum integer conversion base */

#if _PACKAGE_ast
#define SF_MAXINT	INT_MAX
#define SF_MAXLONG	LONG_MAX
#else
#define SF_MAXINT	((int)(((uint)~0) >> 1))
#define SF_MAXLONG	((long)(((ulong)~0L) >> 1))
#endif

#define SF_MAXCHAR	((uchar)(~0))

/* floating point to ascii conversion */
#define SF_MAXEXP10	6
#define SF_MAXPOW10	(1 << SF_MAXEXP10)
#if !_ast_fltmax_double
#define SF_FDIGITS	1024	/* max allowed fractional digits */
#define SF_IDIGITS	(8*1024)	/* max number of digits in int part */
#else
#define SF_FDIGITS	256	/* max allowed fractional digits */
#define SF_IDIGITS	1024	/* max number of digits in int part */
#endif
#define SF_MAXDIGITS	(((SF_FDIGITS+SF_IDIGITS)/sizeof(int) + 1)*sizeof(int))

/* tables for numerical translation */
#define _Sfpos10	(_Sftable.sf_pos10)
#define _Sfneg10	(_Sftable.sf_neg10)
#define _Sfdec		(_Sftable.sf_dec)
#define _Sfdigits	(_Sftable.sf_digits)
#define _Sfcvinitf	(_Sftable.sf_cvinitf)
#define _Sfcvinit	(_Sftable.sf_cvinit)
#define _Sffmtposf	(_Sftable.sf_fmtposf)
#define _Sffmtintf	(_Sftable.sf_fmtintf)
#define _Sfcv36		(_Sftable.sf_cv36)
#define _Sfcv64		(_Sftable.sf_cv64)
#define _Sftype		(_Sftable.sf_type)
    typedef struct _sftab_ {
	Sfdouble_t sf_pos10[SF_MAXEXP10];	/* positive powers of 10        */
	Sfdouble_t sf_neg10[SF_MAXEXP10];	/* negative powers of 10        */
	uchar sf_dec[200];	/* ascii reps of values < 100   */
	char *sf_digits;	/* digits for general bases     */
	int (*sf_cvinitf) (void);	/* initialization function      */
	int sf_cvinit;		/* initialization state         */
	Fmtpos_t *(*sf_fmtposf)
	    _ARG_((Sfio_t *, const char *, va_list, int));
	char *(*sf_fmtintf) _ARG_((const char *, int *));
	uchar sf_cv36[SF_MAXCHAR + 1];	/* conversion for base [2-36]   */
	uchar sf_cv64[SF_MAXCHAR + 1];	/* conversion for base [37-64]  */
	uchar sf_type[SF_MAXCHAR + 1];	/* conversion formats&types     */
    } Sftab_t;

/* thread-safe macro/function to initialize _Sfcv* conversion tables */
#define SFCVINIT()      (_Sfcvinit ? 1 : (_Sfcvinit = (*_Sfcvinitf)()) )

/* sfucvt() converts decimal integers to ASCII */
#define SFDIGIT(v,scale,digit) \
	{ if(v < 5*scale) \
		if(v < 2*scale) \
			if(v < 1*scale) \
				{ digit = '0'; } \
			else	{ digit = '1'; v -= 1*scale; } \
		else	if(v < 3*scale) \
				{ digit = '2'; v -= 2*scale; } \
			else if(v < 4*scale) \
				{ digit = '3'; v -= 3*scale; } \
			else	{ digit = '4'; v -= 4*scale; } \
	  else	if(v < 7*scale) \
			if(v < 6*scale) \
				{ digit = '5'; v -= 5*scale; } \
			else	{ digit = '6'; v -= 6*scale; } \
		else	if(v < 8*scale) \
				{ digit = '7'; v -= 7*scale; } \
			else if(v < 9*scale) \
				{ digit = '8'; v -= 8*scale; } \
			else	{ digit = '9'; v -= 9*scale; } \
	}
#define sfucvt(v,s,n,list,type,utype) \
	{ while((utype)v >= 10000) \
	  {	n = v; v = (type)(((utype)v)/10000); \
		n = (type)((utype)n - ((utype)v)*10000); \
	  	s -= 4; SFDIGIT(n,1000,s[0]); SFDIGIT(n,100,s[1]); \
			s[2] = *(list = (char*)_Sfdec + (n <<= 1)); s[3] = *(list+1); \
	  } \
	  if(v < 100) \
	  { if(v < 10) \
	    { 	s -= 1; s[0] = (char)('0'+v); \
	    } else \
	    { 	s -= 2; s[0] = *(list = (char*)_Sfdec + (v <<= 1)); s[1] = *(list+1); \
	    } \
	  } else \
	  { if(v < 1000) \
	    { 	s -= 3; SFDIGIT(v,100,s[0]); \
			s[1] = *(list = (char*)_Sfdec + (v <<= 1)); s[2] = *(list+1); \
	    } else \
	    {	s -= 4; SFDIGIT(v,1000,s[0]); SFDIGIT(v,100,s[1]); \
			s[2] = *(list = (char*)_Sfdec + (v <<= 1)); s[3] = *(list+1); \
	    } \
	  } \
	}

/* handy functions */
#undef min
#undef max
#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))

/* fast functions for memory copy and memory clear */
#if _PACKAGE_ast
#define memclear(s,n)	memzero(s,n)
#else
#if _lib_bcopy && !_lib_memcpy
#define memcpy(to,fr,n)	bcopy((fr),(to),(n))
#endif
#if _lib_bzero && !_lib_memset
#define memclear(s,n)	bzero((s),(n))
#else
#define memclear(s,n)	memset((s),'\0',(n))
#endif
#endif /*_PACKAGE_ast*/

/* note that MEMCPY advances the associated pointers */
#define MEMCPY(to,fr,n) \
	switch(n) \
	{ default : memcpy((Void_t*)to,(Void_t*)fr,n); to += n; fr += n; break; \
	  case  7 : *to++ = *fr++; \
	  case  6 : *to++ = *fr++; \
	  case  5 : *to++ = *fr++; \
	  case  4 : *to++ = *fr++; \
	  case  3 : *to++ = *fr++; \
	  case  2 : *to++ = *fr++; \
	  case  1 : *to++ = *fr++; \
	}
#define MEMSET(s,c,n) \
	switch(n) \
	{ default : memset((Void_t*)s,(int)c,n); s += n; break; \
	  case  7 : *s++ = c; \
	  case  6 : *s++ = c; \
	  case  5 : *s++ = c; \
	  case  4 : *s++ = c; \
	  case  3 : *s++ = c; \
	  case  2 : *s++ = c; \
	  case  1 : *s++ = c; \
	}

     _BEGIN_EXTERNS_ extern Sfextern_t _Sfextern;
    extern Sftab_t _Sftable;

    extern int _sfpopen _ARG_((Sfio_t *, int, int, int));
    extern int _sfpclose _ARG_((Sfio_t *));
    extern int _sfmode _ARG_((Sfio_t *, int, int));
    extern int _sftype _ARG_((const char *, int *, int *));
    extern int _sfexcept _ARG_((Sfio_t *, int, ssize_t, Sfdisc_t *));
    extern Sfrsrv_t *_sfrsrv _ARG_((Sfio_t *, ssize_t));
    extern int _sfsetpool _ARG_((Sfio_t *));
    extern char *_sfcvt _ARG_((Void_t *, int, int *, int *, int));
    extern char **_sfgetpath _ARG_((char *));
    extern Sfdouble_t _sfstrtod _ARG_((const char *, char **));

#if !_lib_strtod
#define strtod		_sfstrtod
#endif

#ifndef errno
    _astimport int errno;
#endif

/* for portable encoding of double values */
#if !__STDC__
#ifndef WIN32
    extern double frexp _ARG_((double, int *));
    extern double ldexp _ARG_((double, int));
#endif
#endif

#if !_hdr_mman && !_sys_mman
    extern Void_t *mmap _ARG_((Void_t *, size_t, int, int, int, off_t));
    extern int munmap _ARG_((Void_t *, size_t));
#endif

#if !_PACKAGE_ast

#if /*!__STDC__ &&*/ !_hdr_stdlib
    extern void abort _ARG_((void));
    extern int atexit _ARG_((void (*)(void)));
    extern char *getenv _ARG_((const char *));
    extern void *malloc _ARG_((size_t));
    extern void *realloc _ARG_((void *, size_t));
    extern void free _ARG_((void *));
    extern size_t strlen _ARG_((const char *));
    extern char *strcpy _ARG_((char *, const char *));

    extern Void_t *memset _ARG_((void *, int, size_t));
    extern Void_t *memchr _ARG_((const void *, int, size_t));
    extern Void_t *memccpy _ARG_((void *, const void *, int, size_t));
#ifndef memcpy
    extern Void_t *memcpy _ARG_((void *, const void *, size_t));
#endif
#if !defined(strtod)
    extern double strtod _ARG_((const char *, char **));
#endif
#if !defined(remove)
    extern int remove _ARG_((const char *));
#endif
#endif				/* !__STDC__ && !_hdr_stdlib */

#ifdef WIN32
#undef SF_ERROR
#include <io.h>
#define SF_ERROR	0000400	/* an error happened                    */
#else
#if !_hdr_unistd
    extern int close _ARG_((int));
    extern ssize_t read _ARG_((int, void *, size_t));
    extern ssize_t write _ARG_((int, const void *, size_t));
    extern off_t lseek _ARG_((int, off_t, int));
    extern int dup _ARG_((int));
    extern int isatty _ARG_((int));
    extern int wait _ARG_((int *));
    extern int pipe _ARG_((int *));
    extern int access _ARG_((const char *, int));
    extern uint sleep _ARG_((uint));
    extern int execl _ARG_((const char *, const char *, ...));
    extern int execv _ARG_((const char *, char **));
#if !defined(fork)
    extern int fork _ARG_((void));
#endif
#if _lib_unlink
    extern int unlink _ARG_((const char *));
#endif

#endif /*_hdr_unistd*/
#endif /* WIN32 */

#if _lib_bcopy && !_proto_bcopy
    extern void bcopy _ARG_((const void *, void *, size_t));
#endif
#if _lib_bzero && !_proto_bzero
    extern void bzero _ARG_((void *, size_t));
#endif

    extern time_t time _ARG_((time_t *));
    extern int waitpid _ARG_((int, int *, int));
#ifndef WIN32
    extern void _exit _ARG_((int));
#endif
    typedef int (*Onexit_f) _ARG_((void));
    extern Onexit_f onexit _ARG_((Onexit_f));

#if _sys_stat
    extern int fstat _ARG_((int, Stat_t *));
#endif

#if _lib_vfork && !_hdr_vfork && !_sys_vfork
    extern pid_t vfork _ARG_((void));
#endif /*_lib_vfork*/

#if _lib_poll
#if _lib_poll_fd_1
    extern int poll _ARG_((struct pollfd *, ulong, int));
#else
    extern int poll _ARG_((ulong, struct pollfd *, int));
#endif
#endif /*_lib_poll*/

#if _proto_open && __cplusplus
    extern int open _ARG_((const char *, int, ...));
#endif

#endif				/* _PACKAGE_ast */

     _END_EXTERNS_
#endif /*_SFHDR_H*/
#ifdef __cplusplus
}
#endif
