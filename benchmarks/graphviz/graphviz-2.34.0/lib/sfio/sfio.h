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

#ifndef _SFIO_H
#define _SFIO_H	1

#define SFIO_VERSION	20010201L

/*	Public header file for the sfio library
**
**	Written by Kiem-Phong Vo
*/

#if _PACKAGE_ast
#include	<ast_std.h>
#else
#include	<ast_common.h>

#include <stdarg.h>

#ifdef UNUSED
/* to prevent stdio.h from being included */
#ifndef __stdio_h__
#define __stdio_h__	1
#endif
#ifndef _stdio_h_
#define _stdio_h_	1
#endif
#ifndef _stdio_h
#define _stdio_h	1
#endif
#ifndef __h_stdio__
#define __h_stdio__	1
#endif
#ifndef _h_stdio_
#define _h_stdio_	1
#endif
#ifndef _h_stdio
#define _h_stdio	1
#endif
#ifndef __STDIO_H__
#define __STDIO_H__	1
#endif
#ifndef _STDIO_H_
#define _STDIO_H_	1
#endif
#ifndef _STDIO_H
#define _STDIO_H	1
#endif
#ifndef __H_STDIO__
#define __H_STDIO__	1
#endif
#ifndef _H_STDIO_
#define _H_STDIO_	1
#endif
#ifndef _H_STDIO
#define _H_STDIO	1
#endif
#ifndef _stdio_included
#define _stdio_included	1
#endif
#ifndef _included_stdio
#define _included_stdio	1
#endif
#ifndef _INCLUDED_STDIO
#define _INCLUDED_STDIO	1
#endif
#ifndef _STDIO_INCLUDED
#define _STDIO_INCLUDED	1
#endif
#ifndef _INC_STDIO
#define _INC_STDIO	1
#endif

#define _FILE_DEFINED	1	/* stop Windows from defining FILE      */
#define _FILEDEFED	1	/* stop SUNOS5.8 from defining FILE     */
#ifndef FILE
#define FILE	struct _sfio_s	/* because certain stdarg.h needs FILE  */
#endif
#endif

#endif				/* _PACKAGE_ast */

/* Sfoff_t should be large enough for largest file address */


#define Sfoff_t		_ast_intmax_t
#define Sflong_t	_ast_intmax_t
#define Sfulong_t	unsigned _ast_intmax_t
#define Sfdouble_t	_ast_fltmax_t

	typedef struct _sfio_s Sfio_t;

    typedef struct _sfdisc_s Sfdisc_t;
    typedef ssize_t(*Sfread_f)
	_ARG_((Sfio_t *, Void_t *, size_t, Sfdisc_t *));
    typedef ssize_t(*Sfwrite_f)
	_ARG_((Sfio_t *, const Void_t *, size_t, Sfdisc_t *));
    typedef Sfoff_t(*Sfseek_f) _ARG_((Sfio_t *, Sfoff_t, int, Sfdisc_t *));
    typedef int (*Sfexcept_f) _ARG_((Sfio_t *, int, Void_t *, Sfdisc_t *));

/* discipline structure */
    struct _sfdisc_s {
	Sfread_f readf;		/* read function                */
	Sfwrite_f writef;	/* write function               */
	Sfseek_f seekf;		/* seek function                */
	Sfexcept_f exceptf;	/* to handle exceptions         */
	Sfdisc_t *disc;		/* the continuing discipline    */
    };

/* a file structure */
    struct _sfio_s {
	unsigned char *next;	/* next position to read/write from     */
	unsigned char *endw;	/* end of write buffer                  */
	unsigned char *endr;	/* end of read buffer                   */
	unsigned char *endb;	/* end of buffer                        */
	Sfio_t *push;		/* the stream that was pushed on        */
	unsigned short flags;	/* type of stream                       */
	short file;		/* file descriptor                      */
	unsigned char *data;	/* base of data buffer                  */
	ssize_t size;		/* buffer size                          */
	ssize_t val;		/* values or string lengths             */
#ifdef _SFIO_PRIVATE
	 _SFIO_PRIVATE
#endif
    };

/* formatting environment */
    typedef struct _sffmt_s Sffmt_t;
    typedef int (*Sffmtext_f) _ARG_((Sfio_t *, Void_t *, Sffmt_t *));
    typedef int (*Sffmtevent_f)
	_ARG_((Sfio_t *, int, Void_t *, Sffmt_t *));
    struct _sffmt_s {
	long version;		/* version of this structure            */
	Sffmtext_f extf;	/* function to process arguments        */
	Sffmtevent_f eventf;	/* process events                       */

	char *form;		/* format string to stack               */
	va_list args;		/* corresponding arg list               */

	int fmt;		/* format character                     */
	ssize_t size;		/* object size                          */
	int flags;		/* formatting flags                     */
	int width;		/* width of field                       */
	int precis;		/* precision required                   */
	int base;		/* conversion base                      */

	char *t_str;		/* type string                          */
	ssize_t n_str;		/* length of t_str                      */

	Void_t *noop;		/* as yet unused                        */
    };
#define sffmtversion(fe,type) \
		(type ? ((fe)->version = SFIO_VERSION) : (fe)->version)

#define SFFMT_SSHORT	00000010	/* 'hh' flag, char                     */
#define SFFMT_TFLAG	00000020	/* 't' flag, ptrdiff_t                 */
#define SFFMT_ZFLAG	00000040	/* 'z' flag, size_t                    */

#define SFFMT_LEFT	00000100	/* left-justification                  */
#define SFFMT_SIGN	00000200	/* must have a sign                    */
#define SFFMT_BLANK	00000400	/* if not signed, prepend a blank      */
#define SFFMT_ZERO	00001000	/* zero-padding on the left            */
#define SFFMT_ALTER	00002000	/* alternate formatting                */
#define SFFMT_THOUSAND	00004000	/* thousand grouping                   */
#define SFFMT_SKIP	00010000	/* skip assignment in scanf()          */
#define SFFMT_SHORT	00020000	/* 'h' flag                            */
#define SFFMT_LONG	00040000	/* 'l' flag                            */
#define SFFMT_LLONG	00100000	/* 'll' flag                           */
#define SFFMT_LDOUBLE	00200000	/* 'L' flag                            */
#define SFFMT_VALUE	00400000	/* value is returned                   */
#define SFFMT_ARGPOS	01000000	/* getting arg for $ patterns          */
#define SFFMT_IFLAG	02000000	/* 'I' flag                            */
#define SFFMT_JFLAG	04000000	/* 'j' flag, intmax_t                  */
#define SFFMT_SET	07777770	/* flags settable on calling extf      */

/* for sfmutex() call */
#define SFMTX_LOCK	0	/* up mutex count                       */
#define SFMTX_TRYLOCK	1	/* try to up mutex count                */
#define SFMTX_UNLOCK	2	/* down mutex count                     */
#define SFMTX_CLRLOCK	3	/* clear mutex count                    */

/* various constants */
#ifndef NULL
#define NULL		0
#endif
#ifndef EOF
#define EOF		(-1)
#endif
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif


/* bits for various types of files */
#define	SF_READ		0000001	/* open for reading                     */
#define SF_WRITE	0000002	/* open for writing                     */
#define SF_STRING	0000004	/* a string stream                      */

#define SF_APPENDWR	0000010	/* file is in append mode only.         */
#if !_mac_SF_APPEND
#define SF_APPEND	SF_APPENDWR	/* this was the original append bit */
    /* but BSDI stat.h now uses this symbol. */
    /* So we leave it out in such cases.    */
#endif

#define SF_MALLOC	0000020	/* buffer is malloc-ed                  */
#define SF_LINE		0000040	/* line buffering                       */
#define SF_SHARE	0000100	/* stream with shared file descriptor   */
#define SF_EOF		0000200	/* eof was detected                     */
#define SF_ERROR	0000400	/* an error happened                    */
#define SF_STATIC	0001000	/* a stream that cannot be freed        */
#define SF_IOCHECK	0002000	/* call exceptf before doing IO         */
#define SF_PUBLIC	0004000	/* SF_SHARE and follow physical seek    */
#define SF_MTSAFE	0010000	/* need thread safety                   */
#define SF_WHOLE	0020000	/* preserve wholeness of sfwrite/sfputr */

#define SF_FLAGS	0077177	/* PUBLIC FLAGS PASSABLE TO SFNEW()     */
#define SF_SETS		0027163	/* flags passable to sfset()            */

#ifndef KPVDEL
#define SF_BUFCONST	0400000	/* unused flag - for compatibility only */
#endif

/* for sfgetr/sfreserve to hold a record */
#define SF_LOCKR	0000010	/* lock record, stop access to stream   */
#define SF_LASTR	0000020	/* get the last incomplete record       */

/* exception events: SF_NEW(0), SF_READ(1), SF_WRITE(2) and the below 	*/
#define SF_SEEK		3	/* seek error                           */

#define SF_CLOSING	4	/* stream is about to be closed.        */
#if !_mac_SF_CLOSE
#define SF_CLOSE	SF_CLOSING	/* this was the original close event */
    /* but AIX now uses this symbol. So we  */
    /* avoid defining it in such cases.     */
#endif

#define SF_DPUSH	5	/* when discipline is being pushed      */
#define SF_DPOP		6	/* when discipline is being popped      */
#define SF_DPOLL	7	/* see if stream is ready for I/O       */
#define SF_DBUFFER	8	/* buffer not empty during push or pop  */
#define SF_SYNC		9	/* announcing start/end synchronization */
#define SF_PURGE	10	/* a sfpurge() call was issued          */
#define SF_FINAL	11	/* closing is done except stream free   */
#define SF_READY	12	/* a polled stream is ready             */
#define SF_LOCKED	13	/* stream is in a locked state          */
#define SF_ATEXIT	14	/* process is exiting                   */
#define SF_EVENT	100	/* start of user-defined events         */

/* for stack and disciplines */
#define SF_POPSTACK	((Sfio_t*)0)	/* pop the stream stack         */
#define SF_POPDISC	((Sfdisc_t*)0)	/* pop the discipline stack     */

/* for the notify function and discipline exception */
#define SF_NEW		0	/* new stream                           */
#define SF_SETFD	(-1)	/* about to set the file descriptor     */

#define SF_BUFSIZE	8192	/* default buffer size                  */
#define SF_UNBOUND	(-1)	/* unbounded buffer size                */

     _BEGIN_EXTERNS_ extern ssize_t _Sfi;

#if _BLD_sfio && defined(GVDLL)
#define extern	__declspec(dllexport)
#endif
/* standard in/out/err streams */
    extern Sfio_t *sfstdin;
    extern Sfio_t *sfstdout;
    extern Sfio_t *sfstderr;
    extern Sfio_t _Sfstdin;
    extern Sfio_t _Sfstdout;
    extern Sfio_t _Sfstderr;
#undef extern

#if _DLL && _DLL_INDIRECT_DATA
/* The Uwin shared library environment requires these to be defined
   in a global structure set up by the Uwin start-up procedure.
*/
#define sfstdin		((Sfio_t*)_ast_dll->_ast_stdin)
#define sfstdout	((Sfio_t*)_ast_dll->_ast_stdout)
#define sfstderr	((Sfio_t*)_ast_dll->_ast_stderr)
#endif

#if _BLD_sfio && defined(__EXPORT__)
#define extern	__EXPORT__
#endif

    extern Sfio_t *sfnew _ARG_((Sfio_t *, Void_t *, size_t, int, int));
    extern Sfio_t *sfopen _ARG_((Sfio_t *, const char *, const char *));
    extern Sfio_t *sfpopen _ARG_((Sfio_t *, const char *, const char *));
    extern Sfio_t *sfstack _ARG_((Sfio_t *, Sfio_t *));
    extern Sfio_t *sfswap _ARG_((Sfio_t *, Sfio_t *));
    extern Sfio_t *sftmp _ARG_((size_t));
    extern int sfpurge _ARG_((Sfio_t *));
    extern int sfpoll _ARG_((Sfio_t **, int, int));
    extern Void_t *sfreserve _ARG_((Sfio_t *, ssize_t, int));
    extern int sfsync _ARG_((Sfio_t *));
    extern int sfclrlock _ARG_((Sfio_t *));
    extern Void_t *sfsetbuf _ARG_((Sfio_t *, Void_t *, size_t));
    extern Sfdisc_t *sfdisc _ARG_((Sfio_t *, Sfdisc_t *));
    extern int sfraise _ARG_((Sfio_t *, int, Void_t *));
    extern int sfnotify _ARG_((void (*)(Sfio_t *, int, int)));
    extern int sfset _ARG_((Sfio_t *, int, int));
    extern int sfsetfd _ARG_((Sfio_t *, int));
    extern Sfio_t *sfpool _ARG_((Sfio_t *, Sfio_t *, int));
    extern ssize_t sfread _ARG_((Sfio_t *, Void_t *, size_t));
    extern ssize_t sfwrite _ARG_((Sfio_t *, const Void_t *, size_t));
    extern Sfoff_t sfmove _ARG_((Sfio_t *, Sfio_t *, Sfoff_t, int));
    extern int sfclose _ARG_((Sfio_t *));
    extern Sfoff_t sftell _ARG_((Sfio_t *));
    extern Sfoff_t sfseek _ARG_((Sfio_t *, Sfoff_t, int));
    extern ssize_t sfputr _ARG_((Sfio_t *, const char *, int));
    extern char *sfgetr _ARG_((Sfio_t *, int, int));
    extern ssize_t sfnputc _ARG_((Sfio_t *, int, size_t));
    extern int sfungetc _ARG_((Sfio_t *, int));
    extern int sfprintf _ARG_((Sfio_t *, const char *, ...));
    extern char *sfprints _ARG_((const char *, ...));
    extern int sfsprintf _ARG_((char *, int, const char *, ...));
    extern int sfvsprintf _ARG_((char *, int, const char *, va_list));
    extern int sfvprintf _ARG_((Sfio_t *, const char *, va_list));
    extern int sfscanf _ARG_((Sfio_t *, const char *, ...));
    extern int sfsscanf _ARG_((const char *, const char *, ...));
    extern int sfvsscanf _ARG_((const char *, const char *, va_list));
    extern int sfvscanf _ARG_((Sfio_t *, const char *, va_list));
    extern int sfresize _ARG_((Sfio_t *, Sfoff_t));

/* mutex locking for thread-safety */
    extern int sfmutex _ARG_((Sfio_t *, int));

/* io functions with discipline continuation */
    extern ssize_t sfrd _ARG_((Sfio_t *, Void_t *, size_t, Sfdisc_t *));
    extern ssize_t sfwr
	_ARG_((Sfio_t *, const Void_t *, size_t, Sfdisc_t *));
    extern Sfoff_t sfsk _ARG_((Sfio_t *, Sfoff_t, int, Sfdisc_t *));
    extern ssize_t sfpkrd _ARG_((int, Void_t *, size_t, int, long, int));

/* portable handling of primitive types */
    extern int sfdlen _ARG_((Sfdouble_t));
    extern int sfllen _ARG_((Sflong_t));
    extern int sfulen _ARG_((Sfulong_t));

    extern int sfputd _ARG_((Sfio_t *, Sfdouble_t));
    extern int sfputl _ARG_((Sfio_t *, Sflong_t));
    extern int sfputu _ARG_((Sfio_t *, Sfulong_t));
    extern int sfputm _ARG_((Sfio_t *, Sfulong_t, Sfulong_t));
    extern int sfputc _ARG_((Sfio_t *, int));

    extern Sfdouble_t sfgetd _ARG_((Sfio_t *));
    extern Sflong_t sfgetl _ARG_((Sfio_t *));
    extern Sfulong_t sfgetu _ARG_((Sfio_t *));
    extern Sfulong_t sfgetm _ARG_((Sfio_t *, Sfulong_t));
    extern int sfgetc _ARG_((Sfio_t *));

    extern int _sfputd _ARG_((Sfio_t *, Sfdouble_t));
    extern int _sfputl _ARG_((Sfio_t *, Sflong_t));
    extern int _sfputu _ARG_((Sfio_t *, Sfulong_t));
    extern int _sfputm _ARG_((Sfio_t *, Sfulong_t, Sfulong_t));
    extern int _sfflsbuf _ARG_((Sfio_t *, int));

    extern int _sffilbuf _ARG_((Sfio_t *, int));

    extern int _sfdlen _ARG_((Sfdouble_t));
    extern int _sfllen _ARG_((Sflong_t));
    extern int _sfulen _ARG_((Sfulong_t));

/* miscellaneous function analogues of fast in-line functions */
    extern Sfoff_t sfsize _ARG_((Sfio_t *));
    extern int sfclrerr _ARG_((Sfio_t *));
    extern int sfeof _ARG_((Sfio_t *));
    extern int sferror _ARG_((Sfio_t *));
    extern int sffileno _ARG_((Sfio_t *));
    extern int sfstacked _ARG_((Sfio_t *));
    extern ssize_t sfvalue _ARG_((Sfio_t *));
    extern ssize_t sfslen _ARG_((void));

#undef extern
     _END_EXTERNS_
/* coding long integers in a portable and compact fashion */
#define SF_SBITS	6
#define SF_UBITS	7
#define SF_BBITS	8
#define SF_SIGN		(1 << SF_SBITS)
#define SF_MORE		(1 << SF_UBITS)
#define SF_BYTE		(1 << SF_BBITS)
#define SF_U1		SF_MORE
#define SF_U2		(SF_U1*SF_U1)
#define SF_U3		(SF_U2*SF_U1)
#define SF_U4		(SF_U3*SF_U1)
#if __cplusplus
#define _SF_(f)		(f)
#else
#define _SF_(f)		((Sfio_t*)(f))
#endif
#define __sf_putd(f,v)		(_sfputd(_SF_(f),(Sfdouble_t)(v)))
#define __sf_putl(f,v)		(_sfputl(_SF_(f),(Sflong_t)(v)))
#define __sf_putu(f,v)		(_sfputu(_SF_(f),(Sfulong_t)(v)))
#define __sf_putm(f,v,m)	(_sfputm(_SF_(f),(Sfulong_t)(v),(Sfulong_t)(m)))
#define __sf_putc(f,c)	(_SF_(f)->next >= _SF_(f)->endw ? \
			 _sfflsbuf(_SF_(f),(int)((unsigned char)(c))) : \
			 (int)(*_SF_(f)->next++ = (unsigned char)(c)) )
#define __sf_getc(f)	(_SF_(f)->next >= _SF_(f)->endr ? _sffilbuf(_SF_(f),0) : \
			 (int)(*_SF_(f)->next++) )
#define __sf_dlen(v)	(_sfdlen((Sfdouble_t)(v)) )
#define __sf_llen(v)	(_sfllen((Sflong_t)(v)) )
#define __sf_ulen(v)	((Sfulong_t)(v) < SF_U1 ? 1 : (Sfulong_t)(v) < SF_U2 ? 2 : \
			 (Sfulong_t)(v) < SF_U3 ? 3 : (Sfulong_t)(v) < SF_U4 ? 4 : 5)
#define __sf_fileno(f)	((f) ? _SF_(f)->file : -1)
#define __sf_eof(f)	((f) ? (_SF_(f)->flags&SF_EOF) : 0)
#define __sf_error(f)	((f) ? (_SF_(f)->flags&SF_ERROR) : 0)
#define __sf_clrerr(f)	((f) ? (_SF_(f)->flags &= ~(SF_ERROR|SF_EOF)) : 0)
#define __sf_stacked(f)	((f) ? (_SF_(f)->push != (Sfio_t*)0) : 0)
#define __sf_value(f)	((f) ? (_SF_(f)->val) : 0)
#define __sf_slen()	(_Sfi)
#if defined(__INLINE__) && !_BLD_sfio
     __INLINE__ int sfputd(Sfio_t * f, Sfdouble_t v) {
	return __sf_putd(f, v);
    } __INLINE__ int sfputl(Sfio_t * f, Sflong_t v) {
	return __sf_putl(f, v);
    }
    __INLINE__ int sfputu(Sfio_t * f, Sfulong_t v) {
	return __sf_putu(f, v);
    }
    __INLINE__ int sfputm(Sfio_t * f, Sfulong_t v, Sfulong_t m) {
	return __sf_putu(f, v);
    }

    __INLINE__ int sfputc(Sfio_t * f, int c) {
	return __sf_putc(f, c);
    }
    __INLINE__ int sfgetc(Sfio_t * f) {
	return __sf_getc(f);
    }

    __INLINE__ int sfdlen(Sfdouble_t v) {
	return __sf_dlen(v);
    }
    __INLINE__ int sfllen(Sflong_t v) {
	return __sf_llen(v);
    }
    __INLINE__ int sfulen(Sfulong_t v) {
	return __sf_ulen(v);
    }

    __INLINE__ int sffileno(Sfio_t * f) {
	return __sf_fileno(f);
    }
    __INLINE__ int sfeof(Sfio_t * f) {
	return __sf_eof(f);
    }
    __INLINE__ int sferror(Sfio_t * f) {
	return __sf_error(f);
    }
    __INLINE__ int sfclrerr(Sfio_t * f) {
	return __sf_clrerr(f);
    }
    __INLINE__ int sfstacked(Sfio_t * f) {
	return __sf_stacked(f);
    }
    __INLINE__ ssize_t sfvalue(Sfio_t * f) {
	return __sf_value(f);
    }
#else
#define sfputd(f,v)				( __sf_putd((f),(v))		)
#define sfputl(f,v)				( __sf_putl((f),(v))		)
#define sfputu(f,v)				( __sf_putu((f),(v))		)
#define sfputm(f,v,m)				( __sf_putm((f),(v),(m))	)
#define sfputc(f,c)				( __sf_putc((f),(c))		)
#define sfgetc(f)				( __sf_getc(f)			)
#define sfdlen(v)				( __sf_dlen(v)			)
#define sfllen(v)				( __sf_llen(v)			)
#define sfulen(v)				( __sf_ulen(v)			)
#define sffileno(f)				( __sf_fileno(f)		)
#define sfeof(f)				( __sf_eof(f)			)
#define sferror(f)				( __sf_error(f)			)
#define sfclrerr(f)				( __sf_clrerr(f)		)
#define sfstacked(f)				( __sf_stacked(f)		)
#define sfvalue(f)				( __sf_value(f)			)
#define sfslen()				( __sf_slen()			)
#endif /*__INLINE__*/
#endif				/* _SFIO_H */
#ifdef __cplusplus
}
#endif
