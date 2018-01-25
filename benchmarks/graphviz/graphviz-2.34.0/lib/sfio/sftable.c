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

/*	Dealing with $ argument addressing stuffs.
**
**	Written by Kiem-Phong Vo.
*/

#if __STD_C
static char *sffmtint(const char *str, int *v)
#else
static char *sffmtint(str, v)
char *str;
int *v;
#endif
{
    for (*v = 0; isdigit(*str); ++str)
	*v = *v * 10 + (*str - '0');
    *v -= 1;
    return (char *) str;
}

#if __STD_C
static Fmtpos_t *sffmtpos(Sfio_t * f, const char *form, va_list args,
			  int type)
#else
static Fmtpos_t *sffmtpos(f, form, args, type)
Sfio_t *f;
char *form;
va_list args;
int type;
#endif
{
    int base, fmt, flags, dot, width, precis;
    ssize_t n_str, size = 0;
    char *t_str, *sp;
    int v, n, skip, dollar, decimal, thousand;
    Sffmt_t *ft, savft;
    Fmtpos_t *fp;		/* position array of arguments  */
    int argp, argn, maxp, need[FP_INDEX];

    if (type < 0)
	fp = NIL(Fmtpos_t *);
    else if (!(fp = sffmtpos(f, form, args, -1)))
	return NIL(Fmtpos_t *);

    dollar = decimal = thousand = 0;
    argn = maxp = -1;
    while ((n = *form)) {
	if (n != '%') {		/* collect the non-pattern chars */
	    sp = (char *) form++;
	    while (*form && *form != '%')
		form += 1;
	    continue;
	} else
	    form += 1;
	if (*form == 0)
	    break;
	else if (*form == '%') {
	    form += 1;
	    continue;
	}

	if (*form == '*' && type > 0) {	/* skip in scanning */
	    skip = 1;
	    form += 1;
	    argp = -1;
	} else {		/* get the position of this argument */
	    skip = 0;
	    sp = sffmtint(form, &argp);
	    if (*sp == '$') {
		dollar = 1;
		form = sp + 1;
	    } else
		argp = -1;
	}

	flags = dot = 0;
	t_str = NIL(char *);
	n_str = 0;
	size = width = precis = base = -1;
	for (n = 0; n < FP_INDEX; ++n)
	    need[n] = -1;

      loop_flags:		/* LOOP FOR \0, %, FLAGS, WIDTH, PRECISION, BASE, TYPE */
	switch ((fmt = *form++)) {
	case LEFTP:		/* get the type enclosed in balanced parens */
	    t_str = (char *) form;
	    for (v = 1;;) {
		switch (*form++) {
		case 0:	/* not balancable, retract */
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
		    n_str = form - t_str;
		    if (*t_str == '*') {
			t_str = sffmtint(t_str + 1, &n);
			if (*t_str == '$')
			    dollar = 1;
			else
			    n = -1;
			if ((n = FP_SET(n, argn)) > maxp)
			    maxp = n;
			if (fp && fp[n].ft.fmt == 0) {
			    fp[n].ft.fmt = LEFTP;
			    fp[n].ft.form = (char *) form;
			}
			need[FP_STR] = n;
		    }
		    goto loop_flags;
		}
	    }

	case '-':
	    flags |= SFFMT_LEFT;
	    flags &= ~SFFMT_ZERO;
	    goto loop_flags;
	case '0':
	    if (!(flags & SFFMT_LEFT))
		flags |= SFFMT_ZERO;
	    goto loop_flags;
	case ' ':
	    if (!(flags & SFFMT_SIGN))
		flags |= SFFMT_BLANK;
	    goto loop_flags;
	case '+':
	    flags |= SFFMT_SIGN;
	    flags &= ~SFFMT_BLANK;
	    goto loop_flags;
	case '#':
	    flags |= SFFMT_ALTER;
	    goto loop_flags;
	case QUOTE:
	    SFSETLOCALE(&decimal, &thousand);
	    if (thousand)
		flags |= SFFMT_THOUSAND;
	    goto loop_flags;

	case '.':
	    if ((dot += 1) == 2)
		base = 0;	/* for %s,%c */
	    if (isdigit(*form)) {
		fmt = *form++;
		goto dot_size;
	    } else if (*form != '*')
		goto loop_flags;
	    else
		form += 1;	/* drop thru below */

	case '*':
	    form = sffmtint(form, &n);
	    if (*form == '$') {
		dollar = 1;
		form += 1;
	    } else
		n = -1;
	    if ((n = FP_SET(n, argn)) > maxp)
		maxp = n;
	    if (fp && fp[n].ft.fmt == 0) {
		fp[n].ft.fmt = '.';
		fp[n].ft.size = dot;
		fp[n].ft.form = (char *) form;
	    }
	    if (dot <= 2)
		need[dot] = n;
	    goto loop_flags;

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
	    for (v = fmt - '0', fmt = *form; isdigit(fmt); fmt = *++form)
		v = v * 10 + (fmt - '0');
	    if (dot == 0)
		width = v;
	    else if (dot == 1)
		precis = v;
	    else if (dot == 2)
		base = v;
	    goto loop_flags;

	case 'I':		/* object length */
	    size = 0;
	    flags = (flags & ~SFFMT_TYPES) | SFFMT_IFLAG;
	    if (isdigit(*form)) {
		for (n = *form; isdigit(n); n = *++form)
		    size = size * 10 + (n - '0');
	    } else if (*form == '*') {
		form = sffmtint(form + 1, &n);
		if (*form == '$') {
		    dollar = 1;
		    form += 1;
		} else
		    n = -1;
		if ((n = FP_SET(n, argn)) > maxp)
		    maxp = n;
		if (fp && fp[n].ft.fmt == 0) {
		    fp[n].ft.fmt = 'I';
		    fp[n].ft.size = sizeof(int);
		    fp[n].ft.form = (char *) form;
		}
		need[FP_SIZE] = n;
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
	}

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

	if (skip)
	    continue;

	if ((argp = FP_SET(argp, argn)) > maxp)
	    maxp = argp;

	if (dollar && fmt == '!')
	    return NIL(Fmtpos_t *);

	if (fp && fp[argp].ft.fmt == 0) {
	    fp[argp].ft.form = (char *) form;
	    fp[argp].ft.fmt = fp[argp].fmt = fmt;
	    fp[argp].ft.size = size;
	    fp[argp].ft.flags = flags;
	    fp[argp].ft.width = width;
	    fp[argp].ft.precis = precis;
	    fp[argp].ft.base = base;
	    fp[argp].ft.t_str = t_str;
	    fp[argp].ft.n_str = n_str;
	    for (n = 0; n < FP_INDEX; ++n)
		fp[argp].need[n] = need[n];
	}
    }

    if (!fp) {			/* constructing position array only */
	if (!dollar
	    || !(fp = (Fmtpos_t *) malloc((maxp + 1) * sizeof(Fmtpos_t))))
	    return NIL(Fmtpos_t *);
	for (n = 0; n <= maxp; ++n)
	    fp[n].ft.fmt = 0;
	return fp;
    }

    /* get value for positions */
    for (n = 0, ft = NIL(Sffmt_t *); n <= maxp; ++n) {
	if (fp[n].ft.fmt == 0) {	/* gap: pretend it's a 'd' pattern */
	    fp[n].ft.fmt = 'd';
	    fp[n].ft.width = 0;
	    fp[n].ft.precis = 0;
	    fp[n].ft.base = 0;
	    fp[n].ft.size = 0;
	    fp[n].ft.t_str = 0;
	    fp[n].ft.n_str = 0;
	    fp[n].ft.flags = 0;
	    for (v = 0; v < FP_INDEX; ++v)
		fp[n].need[v] = -1;
	}

	if (ft && ft->extf) {
	    fp[n].ft.version = ft->version;
	    fp[n].ft.extf = ft->extf;
	    fp[n].ft.eventf = ft->eventf;
	    if ((v = fp[n].need[FP_WIDTH]) >= 0 && v < n)
		fp[n].ft.width = fp[v].argv.i;
	    if ((v = fp[n].need[FP_PRECIS]) >= 0 && v < n)
		fp[n].ft.precis = fp[v].argv.i;
	    if ((v = fp[n].need[FP_BASE]) >= 0 && v < n)
		fp[n].ft.base = fp[v].argv.i;
	    if ((v = fp[n].need[FP_STR]) >= 0 && v < n)
		fp[n].ft.t_str = fp[v].argv.s;
	    if ((v = fp[n].need[FP_SIZE]) >= 0 && v < n)
		fp[n].ft.size = fp[v].argv.i;

	    memcpy(ft, &fp[n].ft, sizeof(Sffmt_t));
	    va_copy(ft->args, args);
	    ft->flags |= SFFMT_ARGPOS;
	    v = (*ft->extf) (f, (Void_t *) (&fp[n].argv), ft);
	    va_copy(args, ft->args);
	    memcpy(&fp[n].ft, ft, sizeof(Sffmt_t));
	    if (v < 0) {
		memcpy(ft, &savft, sizeof(Sffmt_t));
		ft = NIL(Sffmt_t *);
	    }

	    if (!(fp[n].ft.flags & SFFMT_VALUE))
		goto arg_list;
	} else {
	  arg_list:
	    if (fp[n].ft.fmt == LEFTP) {
		fp[n].argv.s = va_arg(args, char *);
		fp[n].ft.size = strlen(fp[n].argv.s);
	    } else if (fp[n].ft.fmt == '.' || fp[n].ft.fmt == 'I')
		fp[n].argv.i = va_arg(args, int);
	    else if (fp[n].ft.fmt == '!') {
		if (ft)
		    memcpy(ft, &savft, sizeof(Sffmt_t));
		fp[n].argv.ft = ft = va_arg(args, Sffmt_t *);
		if (ft->form)
		    ft = NIL(Sffmt_t *);
		if (ft)
		    memcpy(&savft, ft, sizeof(Sffmt_t));
	    } else if (type > 0)	/* from sfvscanf */
		fp[n].argv.vp = va_arg(args, Void_t *);
	    else
		switch (_Sftype[fp[n].ft.fmt]) {
		case SFFMT_INT:
		case SFFMT_UINT:
#if !_ast_intmax_long
		    if (FMTCMP(size, Sflong_t, Sflong_t))
			fp[n].argv.ll = va_arg(args, Sflong_t);
		    else
#endif
		    if (FMTCMP(size, long, Sflong_t))
			 fp[n].argv.l = va_arg(args, long);
		    else
			fp[n].argv.i = va_arg(args, int);
		    break;
		case SFFMT_FLOAT:
#if !_ast_fltmax_double
		    if (FMTCMP(size, Sfdouble_t, Sfdouble_t))
			fp[n].argv.ld = va_arg(args, Sfdouble_t);
		    else
#endif
			fp[n].argv.d = va_arg(args, double);
		    break;
		case SFFMT_POINTER:
		    fp[n].argv.vp = va_arg(args, Void_t *);
		    break;
		case SFFMT_BYTE:
		    if (fp[n].ft.base >= 0)
			fp[n].argv.s = va_arg(args, char *);
		    else
			fp[n].argv.c = (char) va_arg(args, int);
		    break;
		default:	/* unknown pattern */
		    break;
		}
	}
    }

    if (ft)
	memcpy(ft, &savft, sizeof(Sffmt_t));
    return fp;
}


/* function to initialize conversion tables */
static int sfcvinit(void)
{
    reg int d, l;

    for (d = 0; d <= SF_MAXCHAR; ++d) {
	_Sfcv36[d] = SF_RADIX;
	_Sfcv64[d] = SF_RADIX;
    }

    /* [0-9] */
    for (d = 0; d < 10; ++d) {
	_Sfcv36[(uchar) _Sfdigits[d]] = d;
	_Sfcv64[(uchar) _Sfdigits[d]] = d;
    }

    /* [a-z] */
    for (; d < 36; ++d) {
	_Sfcv36[(uchar) _Sfdigits[d]] = d;
	_Sfcv64[(uchar) _Sfdigits[d]] = d;
    }

    /* [A-Z] */
    for (l = 10; d < 62; ++l, ++d) {
	_Sfcv36[(uchar) _Sfdigits[d]] = l;
	_Sfcv64[(uchar) _Sfdigits[d]] = d;
    }

    /* remaining digits */
    for (; d < SF_RADIX; ++d) {
	_Sfcv36[(uchar) _Sfdigits[d]] = d;
	_Sfcv64[(uchar) _Sfdigits[d]] = d;
    }

    _Sftype['d'] = _Sftype['i'] = SFFMT_INT;
    _Sftype['u'] = _Sftype['o'] = _Sftype['x'] = _Sftype['X'] = SFFMT_UINT;
    _Sftype['e'] = _Sftype['E'] =
	_Sftype['g'] = _Sftype['G'] = _Sftype['f'] = SFFMT_FLOAT;
    _Sftype['s'] = _Sftype['n'] = _Sftype['p'] = _Sftype['!'] =
	SFFMT_POINTER;
    _Sftype['c'] = SFFMT_BYTE;
    _Sftype['['] = SFFMT_CLASS;

    return 1;
}

/* table for floating point and integer conversions */
Sftab_t _Sftable = {
    {1e1, 1e2, 1e4, 1e8, 1e16, 1e32}
    ,				/* _Sfpos10     */

    {1e-1, 1e-2, 1e-4, 1e-8, 1e-16, 1e-32}
    ,				/* _Sfneg10     */

    {'0', '0', '0', '1', '0', '2', '0', '3', '0', '4',	/* _Sfdec       */
     '0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
     '1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
     '1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
     '2', '0', '2', '1', '2', '2', '2', '3', '2', '4',
     '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
     '3', '0', '3', '1', '3', '2', '3', '3', '3', '4',
     '3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
     '4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
     '4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
     '5', '0', '5', '1', '5', '2', '5', '3', '5', '4',
     '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
     '6', '0', '6', '1', '6', '2', '6', '3', '6', '4',
     '6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
     '7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
     '7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
     '8', '0', '8', '1', '8', '2', '8', '3', '8', '4',
     '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
     '9', '0', '9', '1', '9', '2', '9', '3', '9', '4',
     '9', '5', '9', '6', '9', '7', '9', '8', '9', '9',
     }
    ,

    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@_",

    sfcvinit, 0,
    sffmtpos,
    sffmtint
};
