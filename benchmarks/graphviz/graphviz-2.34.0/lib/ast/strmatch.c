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


/*
 * D. G. Korn
 * G. S. Fowler
 * AT&T Research
 *
 * match shell file patterns -- derived from Bourne and Korn shell gmatch()
 *
 *	sh pattern	egrep RE	description
 *	----------	--------	-----------
 *	*		.*		0 or more chars
 *	?		.		any single char
 *	[.]		[.]		char class
 *	[!.]		[^.]		negated char class
 *	[[:.:]]		[[:.:]]		ctype class
 *	[[=.=]]		[[=.=]]		equivalence class
 *	[[...]]		[[...]]		collation element
 *	*(.)		(.)*		0 or more of
 *	+(.)		(.)+		1 or more of
 *	?(.)		(.)?		0 or 1 of
 *	(.)		(.)		1 of
 *	@(.)		(.)		1 of
 *	a|b		a|b		a or b
 *	\#				() subgroup back reference [1-9]
 *	a&b				a and b
 *	!(.)				none of
 *
 * \ used to escape metacharacters
 *
 *	*, ?, (, |, &, ), [, \ must be \'d outside of [...]
 *	only ] must be \'d inside [...]
 *
 * BUG: unbalanced ) terminates top level pattern
 *
 * BOTCH: collating element sort order and character class ranges apparently
 *	  do not have strcoll() in common so we resort to fnmatch(), calling
 *	  it up to COLL_MAX times to determine the matched collating
 *	  element size
 */

#include <ast.h>
#include <ctype.h>
#include <hashkey.h>
#include <string.h>

#if _hdr_wchar && _lib_wctype && _lib_iswctype

#include <stdio.h>		/* because <wchar.h> includes it and we generate it */
#include <wchar.h>
#if _hdr_wctype
#include <wctype.h>
#endif

#undef	isalnum
#define isalnum(x)	iswalnum(x)
#undef	isalpha
#define isalpha(x)	iswalpha(x)
#undef	iscntrl
#define iscntrl(x)	iswcntrl(x)
#undef	isblank
#define isblank(x)	iswblank(x)
#undef	isdigit
#define isdigit(x)	iswdigit(x)
#undef	isgraph
#define isgraph(x)	iswgraph(x)
#undef	islower
#define islower(x)	iswlower(x)
#undef	isprint
#define isprint(x)	iswprint(x)
#undef	ispunct
#define ispunct(x)	iswpunct(x)
#undef	isspace
#define isspace(x)	iswspace(x)
#undef	isupper
#define isupper(x)	iswupper(x)
#undef	isxdigit
#define isxdigit(x)	iswxdigit(x)

#if !defined(iswblank) && !_lib_iswblank

static int iswblank(wint_t wc)
{
    static int initialized;
    static wctype_t wt;

    if (!initialized) {
	initialized = 1;
	wt = wctype("blank");
    }
    return iswctype(wc, wt);
}

#endif

#else

#undef	_lib_wctype

#ifndef	isblank
#define	isblank(x)	((x)==' '||(x)=='\t')
#endif

#ifndef isgraph
#define	isgraph(x)	(isprint(x)&&!isblank(x))
#endif

#endif

#if _DEBUG_MATCH
#include <error.h>
#endif

#define MAXGROUP	10

typedef struct {
    char *beg[MAXGROUP];
    char *end[MAXGROUP];
    char *next_s;
    short groups;
} Group_t;

typedef struct {
    Group_t current;
    Group_t best;
    char *last_s;
    char *next_p;
} Match_t;

#if _lib_mbtowc && MB_LEN_MAX > 1
#define mbgetchar(p)	((ast.locale.set&AST_LC_multibyte)?((ast.tmp_int=mbtowc(&ast.tmp_wchar,p,MB_CUR_MAX))>=0?((p+=ast.tmp_int),ast.tmp_wchar):0):(*p++))
#else
#define mbgetchar(p)	(*p++)
#endif

#ifndef isxdigit
#define isxdigit(c)	(((c)>='0'&&(c)<='9')||((c)>='a'&&(c)<='f')||((c)>='A'&&(c)<='F'))
#endif

#define getsource(s,e)	(((s)>=(e))?0:mbgetchar(s))

#define COLL_MAX	3

#if !_lib_strcoll
#undef	_lib_fnmatch
#endif

#if _lib_fnmatch
extern int fnmatch(const char *, const char *, int);
#endif

/*
 * gobble chars up to <sub> or ) keeping track of (...) and [...]
 * sub must be one of { '|', '&', 0 }
 * 0 returned if s runs out
 */

static char *gobble(Match_t * mp, register char *s, register int sub,
		    int *g, int clear)
{
    register int p = 0;
    register char *b = 0;
    int c = 0;
    int n;

    for (;;)
	switch (mbgetchar(s)) {
	case '\\':
	    if (mbgetchar(s))
		break;
	 /*FALLTHROUGH*/ case 0:
	    return 0;
	case '[':
	    if (!b) {
		if (*s == '!')
		    (void)mbgetchar(s);
		b = s;
	    } else if (*s == '.' || *s == '=' || *s == ':')
		c = *s;
	    break;
	case ']':
	    if (b) {
		if (*(s - 2) == c)
		    c = 0;
		else if (b != (s - 1))
		    b = 0;
	    }
	    break;
	case '(':
	    if (!b) {
		p++;
		n = (*g)++;
		if (clear) {
		    if (!sub)
			n++;
		    if (n < MAXGROUP)
			mp->current.beg[n] = mp->current.end[n] = 0;
		}
	    }
	    break;
	case ')':
	    if (!b && p-- <= 0)
		return sub ? 0 : s;
	    break;
	case '|':
	    if (!b && !p && sub == '|')
		return s;
	    break;
	}
}

static int grpmatch(Match_t *, int, char *, register char *, char *, int);

#if _DEBUG_MATCH
#define RETURN(v)	{error_info.indent--;return (v);}
#else
#define RETURN(v)	{return (v);}
#endif

/*
 * match a single pattern
 * e is the end (0) of the substring in s
 * r marks the start of a repeated subgroup pattern
 */

static int
onematch(Match_t * mp, int g, char *s, char *p, char *e, char *r,
	 int flags)
{
    register int pc;
    register int sc;
    register int n;
    register int icase;
    char *olds;
    char *oldp;

#if _DEBUG_MATCH
    error_info.indent++;
    error(-1, "onematch g=%d s=%-.*s p=%s r=%p flags=%o", g, e - s, s, p,
	  r, flags);
#endif
    icase = flags & STR_ICASE;
    do {
	olds = s;
	sc = getsource(s, e);
	if (icase && isupper(sc))
	    sc = tolower(sc);
	oldp = p;
	switch (pc = mbgetchar(p)) {
	case '(':
	case '*':
	case '?':
	case '+':
	case '@':
	case '!':
	    if (pc == '(' || *p == '(') {
		char *subp;
		int oldg;

		s = olds;
		subp = p + (pc != '(');
		oldg = g;
		n = ++g;
		if (g < MAXGROUP && (!r || g > mp->current.groups))
		    mp->current.beg[g] = mp->current.end[g] = 0;
		if (!(p = gobble(mp, subp, 0, &g, !r)))
		    RETURN(0);
		if (pc == '*' || pc == '?' || (pc == '+' && oldp == r)) {
		    if (onematch(mp, g, s, p, e, NiL, flags))
			RETURN(1);
		    if (!sc || !getsource(s, e)) {
			mp->current.groups = oldg;
			RETURN(0);
		    }
		}
		if (pc == '*' || pc == '+') {
		    p = oldp;
		    sc = n - 1;
		} else
		    sc = g;
		pc = (pc != '!');
		do {
		    if (grpmatch(mp, n, olds, subp, s, flags) == pc) {
			if (n < MAXGROUP) {
			    if (!mp->current.beg[n]
				|| mp->current.beg[n] > olds)
				mp->current.beg[n] = olds;
			    if (s > mp->current.end[n])
				mp->current.end[n] = s;
#if _DEBUG_MATCH
			    error(-4,
				  "subgroup#%d n=%d beg=%p end=%p len=%d",
				  __LINE__, n, mp->current.beg[n],
				  mp->current.end[n],
				  mp->current.end[n] - mp->current.beg[n]);
#endif
			}
			if (onematch(mp, sc, s, p, e, oldp, flags)) {
			    if (p == oldp && n < MAXGROUP) {
				if (!mp->current.beg[n]
				    || mp->current.beg[n] > olds)
				    mp->current.beg[n] = olds;
				if (s > mp->current.end[n])
				    mp->current.end[n] = s;
#if _DEBUG_MATCH
				error(-4,
				      "subgroup#%d n=%d beg=%p end=%p len=%d",
				      __LINE__, n, mp->current.beg[n],
				      mp->current.end[n],
				      mp->current.end[n] -
				      mp->current.beg[n]);
#endif
			    }
			    RETURN(1);
			}
		    }
		} while (s < e && mbgetchar(s));
		mp->current.groups = oldg;
		RETURN(0);
	    } else if (pc == '*') {
		/*
		 * several stars are the same as one
		 */

		while (*p == '*' && *(p + 1) != '(')
		    p++;
		oldp = p;
		switch (pc = mbgetchar(p)) {
		case '@':
		case '!':
		case '+':
		    n = *p == '(';
		    break;
		case '(':
		case '[':
		case '?':
		case '*':
		    n = 1;
		    break;
		case 0:
		case '|':
		case '&':
		case ')':
		    mp->current.next_s = (flags & STR_MAXIMAL) ? e : olds;
		    mp->next_p = oldp;
		    mp->current.groups = g;
		    if (!pc
			&& (!mp->best.next_s
			    || ((flags & STR_MAXIMAL)
				&& mp->current.next_s > mp->best.next_s)
			    || (!(flags & STR_MAXIMAL)
				&& mp->current.next_s <
				mp->best.next_s))) {
			mp->best = mp->current;
#if _DEBUG_MATCH
			error(-3, "best#%d groups=%d next=\"%s\"",
			      __LINE__, mp->best.groups, mp->best.next_s);
#endif
		    }
		    RETURN(1);
		case '\\':
		    if (!(pc = mbgetchar(p)))
			RETURN(0);
		    if (pc >= '0' && pc <= '9') {
			n = pc - '0';
#if _DEBUG_MATCH
			error(-2,
			      "backref#%d n=%d g=%d beg=%p end=%p len=%d",
			      __LINE__, n, g, mp->current.beg[n],
			      mp->current.end[n],
			      mp->current.end[n] - mp->current.beg[n]);
#endif
			if (n <= g && mp->current.beg[n])
			    pc = *mp->current.beg[n];
		    }
		/*FALLTHROUGH*/ default:
		    if (icase && isupper(pc))
			pc = tolower(pc);
		    n = 0;
		    break;
		}
		p = oldp;
		for (;;) {
		    if ((n || pc == sc)
			&& onematch(mp, g, olds, p, e, NiL, flags))
			RETURN(1);
		    if (!sc)
			RETURN(0);
		    olds = s;
		    sc = getsource(s, e);
		    if ((flags & STR_ICASE) && isupper(sc))
			sc = tolower(sc);
		}
	    } else if (pc != '?' && pc != sc)
		RETURN(0);
	    break;
	case 0:
	    if (!(flags & STR_MAXIMAL))
		sc = 0;
	 /*FALLTHROUGH*/ case '|':
	case '&':
	case ')':
	    if (!sc) {
		mp->current.next_s = olds;
		mp->next_p = oldp;
		mp->current.groups = g;
	    }
	    if (!pc
		&& (!mp->best.next_s
		    || ((flags & STR_MAXIMAL) && olds > mp->best.next_s)
		    || (!(flags & STR_MAXIMAL)
			&& olds < mp->best.next_s))) {
		mp->best = mp->current;
		mp->best.next_s = olds;
		mp->best.groups = g;
#if _DEBUG_MATCH
		error(-3, "best#%d groups=%d next=\"%s\"", __LINE__,
		      mp->best.groups, mp->best.next_s);
#endif
	    }
	    RETURN(!sc);
	case '[':
	    {
		/*UNDENT... */

		int invert;
		int x;
		int ok = 0;
		char *range;

		if (!sc)
		    RETURN(0);
#if _lib_fnmatch
		if (ast.locale.set & (1 << AST_LC_COLLATE))
		    range = p - 1;
		else
#endif
		    range = 0;
		n = 0;
		if ((invert = (*p == '!')))
		    p++;
		for (;;) {
		    oldp = p;
		    if (!(pc = mbgetchar(p))) {
			RETURN(0);
		    } else if (pc == '['
			       && (*p == ':' || *p == '=' || *p == '.')) {
			x = 0;
			n = mbgetchar(p);
			oldp = p;
			for (;;) {
			    if (!(pc = mbgetchar(p)))
				RETURN(0);
			    if (pc == n && *p == ']')
				break;
			    x++;
			}
			(void)mbgetchar(p);
			if (ok)
			    /*NOP*/;
			else if (n == ':') {
			    switch (HASHNKEY5
				    (x, oldp[0], oldp[1], oldp[2], oldp[3],
				     oldp[4])) {
			    case HASHNKEY5(5, 'a', 'l', 'n', 'u', 'm'):
				if (isalnum(sc))
				    ok = 1;
				break;
			    case HASHNKEY5(5, 'a', 'l', 'p', 'h', 'a'):
				if (isalpha(sc))
				    ok = 1;
				break;
			    case HASHNKEY5(5, 'b', 'l', 'a', 'n', 'k'):
				if (isblank(sc))
				    ok = 1;
				break;
			    case HASHNKEY5(5, 'c', 'n', 't', 'r', 'l'):
				if (iscntrl(sc))
				    ok = 1;
				break;
			    case HASHNKEY5(5, 'd', 'i', 'g', 'i', 't'):
				if (isdigit(sc))
				    ok = 1;
				break;
			    case HASHNKEY5(5, 'g', 'r', 'a', 'p', 'h'):
				if (isgraph(sc))
				    ok = 1;
				break;
			    case HASHNKEY5(5, 'l', 'o', 'w', 'e', 'r'):
				if (islower(sc))
				    ok = 1;
				break;
			    case HASHNKEY5(5, 'p', 'r', 'i', 'n', 't'):
				if (isprint(sc))
				    ok = 1;
				break;
			    case HASHNKEY5(5, 'p', 'u', 'n', 'c', 't'):
				if (ispunct(sc))
				    ok = 1;
				break;
			    case HASHNKEY5(5, 's', 'p', 'a', 'c', 'e'):
				if (isspace(sc))
				    ok = 1;
				break;
			    case HASHNKEY5(5, 'u', 'p', 'p', 'e', 'r'):
				if (icase ? islower(sc) : isupper(sc))
				    ok = 1;
				break;
			    case HASHNKEY5(6, 'x', 'd', 'i', 'g', 'i'):
				if (oldp[5] == 't' && isxdigit(sc))
				    ok = 1;
				break;
#if _lib_wctype
			    default:
				{
				    char cc[32];

				    if (x >= sizeof(cc))
					x = sizeof(cc) - 1;
				    strncpy(cc, oldp, x);
				    cc[x] = 0;
				    if (iswctype(sc, wctype(cc)))
					ok = 1;
				}
				break;
#endif
			    }
			}
#if _lib_fnmatch
			else if (ast.locale.set & (1 << AST_LC_COLLATE))
			    ok = -1;
#endif
			else if (range)
			    goto getrange;
			else if (*p == '-' && *(p + 1) != ']') {
			    (void)mbgetchar(p);
			    range = oldp;
			} else
			    if ((isalpha(*oldp) && isalpha(*olds)
				 && tolower(*oldp) == tolower(*olds))
				|| sc == mbgetchar(oldp))
			    ok = 1;
			n = 1;
		    } else if (pc == ']' && n) {
#if _lib_fnmatch
			if (ok < 0) {
			    char pat[2 * UCHAR_MAX];
			    char str[COLL_MAX + 1];

			    if (p - range > sizeof(pat) - 2)
				RETURN(0);
			    memcpy(pat, range, p - range);
			    pat[p - range] = '*';
			    pat[p - range + 1] = 0;
			    if (fnmatch(pat, olds, 0))
				RETURN(0);
			    pat[p - range] = 0;
			    ok = 0;
			    for (x = 0; x < sizeof(str) - 1 && olds[x];
				 x++) {
				str[x] = olds[x];
				str[x + 1] = 0;
				if (!fnmatch(pat, str, 0))
				    ok = 1;
				else if (ok)
				    break;
			    }
			    s = olds + x;
			    break;
			}
#endif
			if (ok != invert)
			    break;
			RETURN(0);
		    } else if (pc == '\\'
			       && (oldp = p, !(pc = mbgetchar(p)))) {
			RETURN(0);
		    } else if (ok)
			/*NOP*/;
#if _lib_fnmatch
		    else if (range
			     && !(ast.locale.set & (1 << AST_LC_COLLATE)))
#else
		    else if (range)
#endif
		    {
		      getrange:
#if _lib_mbtowc
			if (ast.locale.set & AST_LC_multibyte) {
			    wchar_t sw;
			    wchar_t bw;
			    wchar_t ew;
			    int sz;
			    int bz;
			    int ez;

			    sz = mbtowc(&sw, olds, MB_CUR_MAX);
			    bz = mbtowc(&bw, range, MB_CUR_MAX);
			    ez = mbtowc(&ew, oldp, MB_CUR_MAX);
			    if (sw == bw || sw == ew)
				ok = 1;
			    else if (sz > 1 || bz > 1 || ez > 1) {
				if (sz == bz && sz == ez && sw > bw
				    && sw < ew)
				    ok = 1;
				else
				    RETURN(0);
			    }
			}
			if (!ok)
#endif
			    if (icase && isupper(pc))
				pc = tolower(pc);
			x = mbgetchar(range);
			if (icase && isupper(x))
			    x = tolower(x);
			if (sc == x || sc == pc || (sc > x && sc < pc))
			    ok = 1;
			if (*p == '-' && *(p + 1) != ']') {
			    (void)mbgetchar(p);
			    range = oldp;
			} else
			    range = 0;
			n = 1;
		    } else if (*p == '-' && *(p + 1) != ']') {
			(void)mbgetchar(p);
#if _lib_fnmatch
			if (ast.locale.set & (1 << AST_LC_COLLATE))
			    ok = -1;
			else
#endif
			    range = oldp;
			n = 1;
		    } else {
			if (icase && isupper(pc))
			    pc = tolower(pc);
			if (sc == pc)
			    ok = 1;
			n = pc;
		    }
		}

		/*...INDENT */
	    }
	    break;
	case '\\':
	    if (!(pc = mbgetchar(p)))
		RETURN(0);
	    if (pc >= '0' && pc <= '9') {
		n = pc - '0';
#if _DEBUG_MATCH
		error(-2, "backref#%d n=%d g=%d beg=%p end=%p len=%d",
		      __LINE__, n, g, mp->current.beg[n],
		      mp->current.end[n],
		      mp->current.end[n] - mp->current.beg[n]);
#endif
		if (n <= g && (oldp = mp->current.beg[n])) {
		    while (oldp < mp->current.end[n])
			if (!*olds || *olds++ != *oldp++)
			    RETURN(0);
		    s = olds;
		    break;
		}
	    }
	/*FALLTHROUGH*/ default:
	    if (icase && isupper(pc))
		pc = tolower(pc);
	    if (pc != sc)
		RETURN(0);
	    break;
	}
    } while (sc);
    RETURN(0);
}

/*
 * match any pattern in a group
 * | and & subgroups are parsed here
 */

static int
grpmatch(Match_t * mp, int g, char *s, register char *p, char *e,
	 int flags)
{
    register char *a;

#if _DEBUG_MATCH
    error_info.indent++;
    error(-1, "grpmatch g=%d s=%-.*s p=%s flags=%o", g, e - s, s, p,
	  flags);
#endif
    do {
	for (a = p; onematch(mp, g, s, a, e, NiL, flags); a++)
	    if (*(a = mp->next_p) != '&')
		RETURN(1);
    } while ((p = gobble(mp, p, '|', &g, 1)));
    RETURN(0);
}

/*
 * subgroup match
 * 0 returned if no match
 * otherwise number of subgroups matched returned
 * match group begin offsets are even elements of sub
 * match group end offsets are odd elements of sub
 * the matched string is from s+sub[0] up to but not
 * including s+sub[1]
 */

int strgrpmatch(const char *b, const char *p, int *sub, int n, int flags)
{
    register int i;
    register char *s;
    char *e;
    Match_t match;

    s = (char *) b;
    match.last_s = e = s + strlen(s);
    for (;;) {
	match.best.next_s = 0;
	match.current.groups = 0;
	match.current.beg[0] = 0;
	if ((i = grpmatch(&match, 0, s, (char *) p, e, flags))
	    || match.best.next_s) {
	    if (!(flags & STR_RIGHT) || (match.current.next_s == e)) {
		if (!i)
		    match.current = match.best;
		match.current.groups++;
		match.current.end[0] = match.current.next_s;
#if _DEBUG_MATCH
		error(-1,
		      "match i=%d s=\"%s\" p=\"%s\" flags=%o groups=%d next=\"%s\"",
		      i, s, p, flags, match.current.groups,
		      match.current.next_s);
#endif
		break;
	    }
	}
	if ((flags & STR_LEFT) || s >= e)
	    return 0;
	s++;
    }
    if ((flags & STR_RIGHT) && match.current.next_s != e)
	return 0;
    if (!sub)
	return 1;
    match.current.beg[0] = s;
    s = (char *) b;
    if (n > match.current.groups)
	n = match.current.groups;
    for (i = 0; i < n; i++) {
	sub[i * 2] = match.current.end[i] ? match.current.beg[i] - s : 0;
	sub[i * 2 + 1] =
	    match.current.end[i] ? match.current.end[i] - s : 0;
    }
    return n;
}

/*
 * compare the string s with the shell pattern p
 * returns 1 for match 0 otherwise
 */

int strmatch(const char *s, const char *p)
{
    return strgrpmatch(s, p, NiL, 0, STR_MAXIMAL | STR_LEFT | STR_RIGHT);
}

/*
 * leading substring match
 * first char after end of substring returned
 * 0 returned if no match
 *
 * OBSOLETE: use strgrpmatch()
 */

char *strsubmatch(const char *s, const char *p, int flags)
{
    int match[2];

    return strgrpmatch(s, p, match, 1,
		       (flags ? STR_MAXIMAL : 0) | STR_LEFT) ? (char *) s +
	match[1] : (char *) 0;
}
