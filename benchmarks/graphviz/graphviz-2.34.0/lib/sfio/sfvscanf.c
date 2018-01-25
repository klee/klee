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

/*	The main engine for reading formatted data
**
**	Written by Kiem-Phong Vo.
*/

#define MAXWIDTH	(int)(((uint)~0)>>1)	/* max amount to scan   */

#if __STD_C
static char *setclass(reg char *form, reg char *accept)
#else
static char *setclass(form, accept)
reg char *form;			/* format string                        */
reg char *accept;		/* accepted characters are set to 1     */
#endif
{
    reg int fmt, c, yes;

    if ((fmt = *form++) == '^') {	/* we want the complement of this set */
	yes = 0;
	fmt = *form++;
    } else
	yes = 1;

    for (c = 0; c <= SF_MAXCHAR; ++c)
	accept[c] = !yes;

    if (fmt == ']' || fmt == '-') {	/* special first char */
	accept[fmt] = yes;
	fmt = *form++;
    }

    for (; fmt != ']'; fmt = *form++) {	/* done */
	if (!fmt)
	    return (form - 1);

	/* interval */
	if (fmt != '-' || form[0] == ']' || form[-2] > form[0])
	    accept[fmt] = yes;
	else
	    for (c = form[-2] + 1; c < form[0]; ++c)
		accept[c] = yes;
    }

    return form;
}

#if __STD_C
static void _sfbuf(Sfio_t * f, int *rs)
#else
static void _sfbuf(f, rs)
Sfio_t *f;
int *rs;
#endif
{
    if (f->next >= f->endb) {
	if (*rs > 0) {		/* try peeking for a share stream if possible */
	    f->mode |= SF_RV;
	    if (SFFILBUF(f, -1) > 0) {
		f->mode |= SF_PEEK;
		return;
	    }
	    *rs = -1;		/* can't peek, back to normal reads */
	}
	(void) SFFILBUF(f, -1);
    }
}

#if __STD_C
int sfvscanf(Sfio_t * f, reg const char *form, va_list args)
#else
int sfvscanf(f, form, args)
Sfio_t *f;			/* file to be scanned */
reg char *form;			/* scanning format */
va_list args;
#endif
{
    reg uchar *d, *endd, *data;
    reg int inp, shift, base, width;
    ssize_t size;
    int fmt, flags, dot, n_assign, v, n, n_input;
    char *sp;
    char accept[SF_MAXDIGITS];

    Argv_t argv;
    Sffmt_t *ft;
    Fmt_t *fm, *fmstk;

    Fmtpos_t *fp;
    char *oform;
    va_list oargs;
    int argp, argn;

    Void_t *value;		/* location to assign scanned value */
    char *t_str;
    ssize_t n_str;
    int rs;

#define SFBUF(f)	(_sfbuf(f,&rs), (data = d = f->next), (endd = f->endb) )
#define SFLEN(f)	(d-data)
#define SFEND(f)	((n_input += d-data), \
			 (rs > 0 ? SFREAD(f,(Void_t*)data,d-data) : ((f->next = d), 0)) )
#define SFGETC(f,c)	((c) = (d < endd || (SFEND(f), SFBUF(f), d < endd)) ? \
				(int)(*d++) : -1 )
#define SFUNGETC(f,c)	(--d)

    SFMTXSTART(f, -1);

    if (!form)
	SFMTXRETURN(f, -1);

    if (f->mode != SF_READ && _sfmode(f, SF_READ, 0) < 0)
	SFMTXRETURN(f, -1);
    SFLOCK(f, 0);

    rs = (f->extent < 0 && (f->flags & SF_SHARE)) ? 1 : 0;

    SFCVINIT();			/* initialize conversion tables */

    SFBUF(f);
    n_assign = n_input = 0;

    inp = -1;

    fmstk = NIL(Fmt_t *);
    ft = NIL(Sffmt_t *);

    fp = NIL(Fmtpos_t *);
    argn = -1;
    oform = (char *) form;
    va_copy(oargs, args);

  loop_fmt:
    while ((fmt = *form++)) {
	if (fmt != '%') {
	    if (isspace(fmt)) {
		if (fmt != '\n' || !(f->flags & SF_LINE))
		    fmt = -1;
		for (;;) {
		    if (SFGETC(f, inp) < 0 || inp == fmt)
			goto loop_fmt;
		    else if (!isspace(inp)) {
			SFUNGETC(f, inp);
			goto loop_fmt;
		    }
		}
	    } else {
	      match_1:
		if (SFGETC(f, inp) != fmt) {
		    if (inp >= 0)
			SFUNGETC(f, inp);
		    goto pop_fmt;
		}
	    }
	    continue;
	}

	if (*form == '%') {
	    form += 1;
	    goto match_1;
	}

	if (*form == '\0')
	    goto pop_fmt;

	if (*form == '*') {
	    flags = SFFMT_SKIP;
	    form += 1;
	} else
	    flags = 0;

	/* matching some pattern */
	base = 10;
	size = -1;
	width = dot = 0;
	t_str = NIL(char *);
	n_str = 0;
	value = NIL(Void_t *);
	argp = -1;

      loop_flags:		/* LOOP FOR FLAGS, WIDTH, BASE, TYPE */
	switch ((fmt = *form++)) {
	case LEFTP:		/* get the type which is enclosed in balanced () */
	    t_str = (char *) form;
	    for (v = 1;;) {
		switch (*form++) {
		case 0:	/* not balanceable, retract */
		    form = t_str;
		    t_str = NIL(char *);
		    n_str = 0;
		    goto loop_flags;
		case LEFTP:	/* increasing nested level */
		    v += 1;
		    continue;
		case RIGHTP:	/* decreasing nested level */
		    if ((v -= 1) != 0)
			continue;
		    if (*t_str != '*')
			n_str = (form - 1) - t_str;
		    else {
			t_str = (*_Sffmtintf) (t_str + 1, &n);
			if (*t_str == '$') {
			    if (!fp && !(fp = (*_Sffmtposf)
					 (f, oform, oargs, 1)))
				goto pop_fmt;
			    n = FP_SET(n, argn);
			} else
			    n = FP_SET(-1, argn);

			if (fp) {
			    t_str = fp[n].argv.s;
			    n_str = fp[n].ft.size;
			} else if (ft && ft->extf) {
			    FMTSET(ft, form, args,
				   LEFTP, 0, 0, 0, 0, 0, NIL(char *), 0);
			    n = (*ft->extf)
				(f, (Void_t *) & argv, ft);
			    if (n < 0)
				goto pop_fmt;
			    if (!(ft->flags & SFFMT_VALUE))
				goto t_arg;
			    if ((t_str = argv.s) &&
				(n_str = (int) ft->size) < 0)
				n_str = strlen(t_str);
			} else {
			  t_arg:
			    if ((t_str = va_arg(args, char *)))
				 n_str = strlen(t_str);
			}
		    }
		    goto loop_flags;
		}
	    }

	case '#':		/* alternative format */
	    flags |= SFFMT_ALTER;
	    goto loop_flags;

	case '.':		/* width & base */
	    dot += 1;
	    if (isdigit(*form)) {
		fmt = *form++;
		goto dot_size;
	    } else if (*form == '*') {
		form = (*_Sffmtintf) (form + 1, &n);
		if (*form == '$') {
		    form += 1;
		    if (!fp && !(fp = (*_Sffmtposf) (f, oform, oargs, 1)))
			goto pop_fmt;
		    n = FP_SET(n, argn);
		} else
		    n = FP_SET(-1, argn);

		if (fp)
		    v = fp[n].argv.i;
		else if (ft && ft->extf) {
		    FMTSET(ft, form, args, '.', dot, 0, 0, 0, 0,
			   NIL(char *), 0);
		    if ((*ft->extf) (f, (Void_t *) (&argv), ft) < 0)
			goto pop_fmt;
		    if (ft->flags & SFFMT_VALUE)
			v = argv.i;
		    else
			v = (dot <= 2) ? va_arg(args, int) : 0;
		} else
		    v = (dot <= 2) ? va_arg(args, int) : 0;
		if (v < 0)
		    v = 0;
		goto dot_set;
	    } else
		goto loop_flags;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  dot_size:
	    for (v = fmt - '0'; isdigit(*form); ++form)
		v = v * 10 + (*form - '0');

	    if (*form == '$') {
		form += 1;
		if (!fp && !(fp = (*_Sffmtposf) (f, oform, oargs, 1)))
		    goto pop_fmt;
		argp = v - 1;
		goto loop_flags;
	    }

	  dot_set:
	    if (dot == 0 || dot == 1)
		width = v;
	    else if (dot == 2)
		base = v;
	    goto loop_flags;

	case 'I':		/* object size */
	    size = 0;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_IFLAG;
	    if (isdigit(*form)) {
		for (n = *form; isdigit(n); n = *++form)
		    size = size * 10 + (n - '0');
	    } else if (*form == '*') {
		form = (*_Sffmtintf) (form + 1, &n);
		if (*form == '$') {
		    form += 1;
		    if (!fp && !(fp = (*_Sffmtposf) (f, oform, oargs, 1)))
			goto pop_fmt;
		    n = FP_SET(n, argn);
		} else
		    n = FP_SET(-1, argn);

		if (fp)		/* use position list */
		    size = fp[n].argv.i;
		else if (ft && ft->extf) {
		    FMTSET(ft, form, args, 'I', sizeof(int), 0, 0, 0, 0,
			   NIL(char *), 0);
		    if ((*ft->extf) (f, (Void_t *) (&argv), ft) < 0)
			goto pop_fmt;
		    if (ft->flags & SFFMT_VALUE)
			size = argv.i;
		    else
			size = va_arg(args, int);
		} else
		    size = va_arg(args, int);
	    }
	    goto loop_flags;

	case 'l':
	    size = -1;
	    flags &= ~SFFMT_TYPES;
	    if (*form == 'l') {
		form += 1;
		flags |= SFFMT_LLONG;
	    } else
		flags |= SFFMT_LONG;
	    goto loop_flags;
	case 'h':
	    size = -1;
	    flags &= ~SFFMT_TYPES;
	    if (*form == 'h') {
		form += 1;
		flags |= SFFMT_SSHORT;
	    } else
		flags |= SFFMT_SHORT;
	    goto loop_flags;
	case 'L':
	    size = -1;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_LDOUBLE;
	    goto loop_flags;
	case 'j':
	    size = -1;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_JFLAG;
	    goto loop_flags;
	case 'z':
	    size = -1;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_ZFLAG;
	    goto loop_flags;
	case 't':
	    size = -1;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_TFLAG;
	    goto loop_flags;
	}

	/* set object size */
	if (flags & (SFFMT_TYPES & ~SFFMT_IFLAG)) {
	    if ((_Sftype[fmt] & (SFFMT_INT | SFFMT_UINT)) || fmt == 'n') {
		size = (flags & SFFMT_LLONG) ? sizeof(Sflong_t) :
		    (flags & SFFMT_LONG) ? sizeof(long) :
		    (flags & SFFMT_SHORT) ? sizeof(short) :
		    (flags & SFFMT_SSHORT) ? sizeof(char) :
		    (flags & SFFMT_JFLAG) ? sizeof(Sflong_t) :
		    (flags & SFFMT_TFLAG) ? sizeof(ptrdiff_t) :
		    (flags & SFFMT_ZFLAG) ? sizeof(size_t) : -1;
	    } else if (_Sftype[fmt] & SFFMT_FLOAT) {
		size = (flags & SFFMT_LDOUBLE) ? sizeof(Sfdouble_t) :
		    (flags & (SFFMT_LONG | SFFMT_LLONG)) ?
		    sizeof(double) : -1;
	    }
	}

	argp = FP_SET(argp, argn);
	if (fp) {
	    if (!(fp[argp].ft.flags & SFFMT_SKIP)) {
		n_assign += 1;
		value = fp[argp].argv.vp;
		size = fp[argp].ft.size;
		if (ft && ft->extf && fp[argp].ft.fmt != fp[argp].fmt)
		    fmt = fp[argp].ft.fmt;
	    } else
		flags |= SFFMT_SKIP;
	} else if (ft && ft->extf) {
	    FMTSET(ft, form, args, fmt, size, flags, width, 0, base, t_str,
		   n_str);
	    SFEND(f);
	    SFOPEN(f, 0);
	    v = (*ft->extf) (f, (Void_t *) & argv, ft);
	    SFLOCK(f, 0);
	    SFBUF(f);

	    if (v < 0)
		goto pop_fmt;
	    else if (v == 0) {	/* extf did not use input stream */
		FMTGET(ft, form, args, fmt, size, flags, width, n, base);
		if ((ft->flags & SFFMT_VALUE) && !(ft->flags & SFFMT_SKIP))
		    value = argv.vp;
	    } else {		/* v > 0: number of input bytes consumed */
		n_input += v;
		if (!(ft->flags & SFFMT_SKIP))
		    n_assign += 1;
		continue;
	    }
	}

	if (_Sftype[fmt] == 0)	/* unknown pattern */
	    continue;

	if (fmt == '!') {
	    if (!fp)
		fp = (*_Sffmtposf) (f, oform, oargs, 1);
	    else
		goto pop_fmt;

	    if (!(argv.ft = va_arg(args, Sffmt_t *)))
		continue;
	    if (!argv.ft->form && ft) {	/* change extension functions */
		if (ft->eventf &&
		    (*ft->eventf) (f, SF_DPOP, (Void_t *) form, ft) < 0)
		    continue;
		fmstk->ft = ft = argv.ft;
	    } else {		/* stack a new environment */
		if (!(fm = (Fmt_t *) malloc(sizeof(Fmt_t))))
		    goto done;

		if (argv.ft->form) {
		    fm->form = (char *) form;
		    va_copy(fm->args, args);

		    fm->oform = oform;
		    va_copy(fm->oargs, oargs);
		    fm->argn = argn;
		    fm->fp = fp;

		    form = argv.ft->form;
		    va_copy(args, argv.ft->args);
		    argn = -1;
		    fp = NIL(Fmtpos_t *);
		} else
		    fm->form = NIL(char *);

		fm->eventf = argv.ft->eventf;
		fm->ft = ft;
		fm->next = fmstk;
		fmstk = fm;
		ft = argv.ft;
	    }
	    continue;
	}

	/* get the address to assign value */
	if (!value && !(flags & SFFMT_SKIP))
	    value = va_arg(args, Void_t *);

	if (fmt == 'n') {	/* return length of consumed input */
#if !_ast_intmax_long
	    if (FMTCMP(size, Sflong_t, Sflong_t))
		*((Sflong_t *) value) = (Sflong_t) (n_input + SFLEN(f));
	    else
#endif
	    if (sizeof(long) > sizeof(int) && FMTCMP(size, long, Sflong_t))
		*((long *) value) = (long) (n_input + SFLEN(f));
	    else if (sizeof(short) < sizeof(int) &&
		     FMTCMP(size, short, Sflong_t))
		*((short *) value) = (short) (n_input + SFLEN(f));
	    else if (size == sizeof(char))
		*((char *) value) = (char) (n_input + SFLEN(f));
	    else
		*((int *) value) = (int) (n_input + SFLEN(f));
	    continue;
	}

	/* if get here, start scanning input */
	if (width == 0)
	    width = fmt == 'c' ? 1 : MAXWIDTH;

	/* define the first input character */
	if (fmt == 'c' || fmt == '[')
	    SFGETC(f, inp);
	else {
	    do {
		SFGETC(f, inp);
	    }
	    while (isspace(inp))	/* skip starting blanks */
	    ;
	}
	if (inp < 0)
	    goto done;

	if (_Sftype[fmt] == SFFMT_FLOAT) {
	    reg char *val;
	    reg int dot, exponent;

	    val = accept;
	    if (width >= SF_MAXDIGITS)
		width = SF_MAXDIGITS - 1;
	    dot = exponent = 0;
	    do {
		if (isdigit(inp))
		    *val++ = inp;
		else if (inp == '.') {	/* too many dots */
		    if (dot++ > 0)
			break;
		    *val++ = '.';
		} else if (inp == 'e' || inp == 'E') {	/* too many e,E */
		    if (exponent++ > 0)
			break;
		    *val++ = inp;
		    if (--width <= 0 || SFGETC(f, inp) < 0 ||
			(inp != '-' && inp != '+' && !isdigit(inp)))
			break;
		    *val++ = inp;
		} else if (inp == '-' || inp == '+') {	/* too many signs */
		    if (val > accept)
			break;
		    *val++ = inp;
		} else
		    break;

	    } while (--width > 0 && SFGETC(f, inp) >= 0);

	    if (value) {
		*val = '\0';
#if !_ast_fltmax_double
		if (FMTCMP(size, Sfdouble_t, Sfdouble_t))
		    argv.ld = _sfstrtod(accept, NIL(char **));
		else
#endif
		    argv.d = (double) strtod(accept, NIL(char **));
	    }

	    if (value) {
		n_assign += 1;
#if !_ast_fltmax_double
		if (FMTCMP(size, Sfdouble_t, Sfdouble_t))
		    *((Sfdouble_t *) value) = argv.ld;
		else
#endif
		if (FMTCMP(size, double, Sfdouble_t))
		    *((double *) value) = argv.d;
		else
		    *((float *) value) = (float) argv.d;
	    }
	} else if (_Sftype[fmt] == SFFMT_UINT || fmt == 'p') {
	    if (inp == '-') {
		SFUNGETC(f, inp);
		goto pop_fmt;
	    } else
		goto int_cvt;
	} else if (_Sftype[fmt] == SFFMT_INT) {
	  int_cvt:
	    if (inp == '-' || inp == '+') {
		if (inp == '-')
		    flags |= SFFMT_MINUS;
		while (--width > 0 && SFGETC(f, inp) >= 0)
		    if (!isspace(inp))
			break;
	    }
	    if (inp < 0)
		goto done;

	    if (fmt == 'o')
		base = 8;
	    else if (fmt == 'x' || fmt == 'p')
		base = 16;
	    else if (fmt == 'i' && inp == '0') {	/* self-described data */
		base = 8;
		if (width > 1) {	/* peek to see if it's a base-16 */
		    if (SFGETC(f, inp) >= 0) {
			if (inp == 'x' || inp == 'X')
			    base = 16;
			SFUNGETC(f, inp);
		    }
		    inp = '0';
		}
	    }

	    /* now convert */
	    argv.lu = 0;
	    if (base == 16) {
		sp = (char *) _Sfcv36;
		shift = 4;
		if (sp[inp] >= 16) {
		    SFUNGETC(f, inp);
		    goto pop_fmt;
		}
		if (inp == '0' && --width > 0) {	/* skip leading 0x or 0X */
		    if (SFGETC(f, inp) >= 0 &&
			(inp == 'x' || inp == 'X') && --width > 0)
			SFGETC(f, inp);
		}
		if (inp >= 0 && sp[inp] < 16)
		    goto base_shift;
	    } else if (base == 10) {	/* fast base 10 conversion */
		if (inp < '0' || inp > '9') {
		    SFUNGETC(f, inp);
		    goto pop_fmt;
		}

		do {
		    argv.lu =
			(argv.lu << 3) + (argv.lu << 1) + (inp - '0');
		} while (--width > 0 && SFGETC(f, inp) >= '0'
			 && inp <= '9');

		if (fmt == 'i' && inp == '#' && !(flags & SFFMT_ALTER)) {
		    base = (int) argv.lu;
		    if (base < 2 || base > SF_RADIX)
			goto pop_fmt;
		    argv.lu = 0;
		    sp = base <= 36 ? (char *) _Sfcv36 : (char *) _Sfcv64;
		    if (--width > 0 &&
			SFGETC(f, inp) >= 0 && sp[inp] < base)
			goto base_conv;
		}
	    } else {		/* other bases */
		sp = base <= 36 ? (char *) _Sfcv36 : (char *) _Sfcv64;
		if (base < 2 || base > SF_RADIX || sp[inp] >= base) {
		    SFUNGETC(f, inp);
		    goto pop_fmt;
		}

	      base_conv:	/* check for power of 2 conversions */
		if ((base & ~(base - 1)) == base) {
		    if (base < 8)
			shift = base < 4 ? 1 : 2;
		    else if (base < 32)
			shift = base < 16 ? 3 : 4;
		    else
			shift = base < 64 ? 5 : 6;

		  base_shift:do {
			argv.lu = (argv.lu << shift) + sp[inp];
		    } while (--width > 0 &&
			     SFGETC(f, inp) >= 0 && sp[inp] < base);
		} else {
		    do {
			argv.lu = (argv.lu * base) + sp[inp];
		    } while (--width > 0 &&
			     SFGETC(f, inp) >= 0 && sp[inp] < base);
		}
	    }

	    if (flags & SFFMT_MINUS)
		argv.ll = -argv.ll;

	    if (value) {
		n_assign += 1;

		if (fmt == 'p')
#if _more_void_int
		    *((Void_t **) value) = (Void_t *) ((ulong) argv.lu);
#else
		    *((Void_t **) value) = (Void_t *) ((uint) argv.lu);
#endif
#if !_ast_intmax_long
		else if (FMTCMP(size, Sflong_t, Sflong_t))
		    *((Sflong_t *) value) = argv.ll;
#endif
		else if (sizeof(long) > sizeof(int) &&
			 FMTCMP(size, long, Sflong_t)) {
		    if (fmt == 'd' || fmt == 'i')
			*((long *) value) = (long) argv.ll;
		    else
			*((ulong *) value) = (ulong) argv.lu;
		} else if (sizeof(short) < sizeof(int) &&
			   FMTCMP(size, short, Sflong_t)) {
		    if (fmt == 'd' || fmt == 'i')
			*((short *) value) = (short) argv.ll;
		    else
			*((ushort *) value) = (ushort) argv.lu;
		} else if (size == sizeof(char)) {
		    if (fmt == 'd' || fmt == 'i')
			*((char *) value) = (char) argv.ll;
		    else
			*((uchar *) value) = (uchar) argv.lu;
		} else {
		    if (fmt == 'd' || fmt == 'i')
			*((int *) value) = (int) argv.ll;
		    else
			*((uint *) value) = (uint) argv.lu;
		}
	    }
	} else if (fmt == 's' || fmt == 'c' || fmt == '[') {
	    if (size < 0)
		size = MAXWIDTH;
	    if (value) {
		argv.s = (char *) value;
		if (fmt != 'c')
		    size -= 1;
	    } else
		size = 0;

	    n = 0;
	    if (fmt == 's') {
		do {
		    if (isspace(inp))
			break;
		    if ((n += 1) <= size)
			*argv.s++ = inp;
		} while (--width > 0 && SFGETC(f, inp) >= 0);
	    } else if (fmt == 'c') {
		do {
		    if ((n += 1) <= size)
			*argv.s++ = inp;
		} while (--width > 0 && SFGETC(f, inp) >= 0);
	    } else {		/* if(fmt == '[') */
		form = setclass((char *) form, accept);
		do {
		    if (!accept[inp]) {
			if (n > 0 || (flags & SFFMT_ALTER))
			    break;
			else {
			    SFUNGETC(f, inp);
			    goto pop_fmt;
			}
		    }
		    if ((n += 1) <= size)
			*argv.s++ = inp;
		} while (--width > 0 && SFGETC(f, inp) >= 0);
	    }

	    if (value && (n > 0 || fmt == '[')) {
		n_assign += 1;
		if (fmt != 'c' && size >= 0)
		    *argv.s = '\0';
	    }
	}

	if (width > 0 && inp >= 0)
	    SFUNGETC(f, inp);
    }

  pop_fmt:
    if (fp) {
	free(fp);
	fp = NIL(Fmtpos_t *);
    }
    while ((fm = fmstk)) {	/* pop the format stack and continue */
	if (fm->eventf) {
	    if (!form || !form[0])
		(*fm->eventf) (f, SF_FINAL, NIL(Void_t *), ft);
	    else if ((*fm->eventf) (f, SF_DPOP, (Void_t *) form, ft) < 0)
		goto loop_fmt;
	}

	fmstk = fm->next;
	if ((form = fm->form)) {
	    va_copy(args, fm->args);
	    oform = fm->oform;
	    va_copy(oargs, fm->oargs);
	    argn = fm->argn;
	    fp = fm->fp;
	}
	ft = fm->ft;
	free(fm);
	if (form && form[0])
	    goto loop_fmt;
    }

  done:
    if (fp)
	free(fp);
    while ((fm = fmstk)) {
	if (fm->eventf)
	    (*fm->eventf) (f, SF_FINAL, NIL(Void_t *), fm->ft);
	fmstk = fm->next;
	free(fm);
    }

    SFEND(f);
    SFOPEN(f, 0);

    if (n_assign == 0 && inp < 0)
	n_assign = -1;

    SFMTXRETURN(f, n_assign);
}
