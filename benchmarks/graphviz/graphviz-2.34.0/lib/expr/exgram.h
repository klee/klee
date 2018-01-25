/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=4: */

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


/*
 * grammar support routines
 * stuffed in a header so exparse.y can work
 * with both yacc and bison
 * sometimes free stuff can cost a lot
 */

#if !defined(_EXGRAM_H) && ( defined(MINTOKEN) || defined(YYTOKENTYPE) )
#define _EXGRAM_H

#if !defined(_EXPARSE_H)
#define _EXPARSE_H
#endif

#include "exlib.h"
#include <string.h>

#define exlex()		extoken_fn(expr.program)

#define ALLOCATE(p,x)	(x*)exalloc(p,sizeof(x))
#define QUALIFY(r,s)	((r)&&(expr.program->disc->flags&EX_QUALIFY)?qualify(r,s):(s))

static int		a2t[] = { 0, FLOATING, INTEGER, STRING };
static Switch_t		swstate;

Exstate_t		expr;

static int T(int t) 
{
	if (expr.program->disc->types)
	    return expr.program->disc->types[t & TMASK];
	else
	    return a2t[t & TMASK];
}

/*
 * allocate and initialize a new expression node in the current program
 */

Exnode_t*
exnewnode(Expr_t* p, int op, int binary, int type, Exnode_t* left, Exnode_t* right)
{
	register Exnode_t*	x;

	x = ALLOCATE(p, Exnode_t);
	x->op = op;
	x->type = type;
	x->binary = binary;
	x->local.number = 0;
	x->local.pointer = 0;
	x->data.operand.left = left;
	x->data.operand.right = right;
	return x;
}

/*
 * free node x and its children
 */

void
exfreenode(Expr_t* p, register Exnode_t* x)
{
	register Print_t*	pr;
	register Exref_t*	r;
	Print_t*		pn;
	Exref_t*		rn;
	int			i;

	switch (x->op)
	{
	case CALL:
		if (x->data.call.args)
			exfreenode(p, x->data.call.args);
		break;
	case CONSTANT:
		break;
	case DEFAULT:
		if (x->data.select.next)
			exfreenode(p, x->data.select.next);
		break;
	case DYNAMIC:
		if (x->data.variable.index)
			exfreenode(p, x->data.variable.index);
		if (x->data.variable.symbol->local.pointer)
		{
			dtclose((Dt_t*)x->data.variable.symbol->local.pointer);
			x->data.variable.symbol->local.pointer = 0;
		}
		break;
	case '#':
		if (x->data.variable.symbol->local.pointer) {
			dtclose((Dt_t *) x->data.variable.symbol->local.pointer);
			x->data.variable.symbol->local.pointer = 0;
		}
		break;
//	case IN_OP:
	case UNSET:
		if (x->data.variable.index)
			exfreenode(p, x->data.variable.index);
		if (x->data.variable.symbol->local.pointer) {
			dtclose((Dt_t *) x->data.variable.symbol->local.pointer);
			x->data.variable.symbol->local.pointer = 0;
		}
		break;
	case ITERATE:
	case ITERATER:
		if (x->data.generate.statement)
			exfreenode(p, x->data.generate.statement);
		break;
	case ID:
		rn = x->data.variable.reference;
		while ((r = rn))
		{
			rn = r->next;
			vmfree(p->vm, r);
		}
		if (x->data.variable.index)
			exfreenode(p, x->data.variable.index);
		break;
	case GSUB:
	case SUB:
	case SUBSTR:
		exfreenode(p, x->data.string.base);
		exfreenode(p, x->data.string.pat);
		if (x->data.string.repl)
			exfreenode(p, x->data.string.repl);
		break;
	case TOKENS:
	case SPLIT:
		if (x->data.split.seps)
			exfreenode(p, x->data.split.seps);
		exfreenode(p, x->data.split.string);
		if (x->data.split.array->local.pointer) {
			dtclose((Dt_t *) x->data.split.array->local.pointer);
			x->data.split.array->local.pointer = 0;
		}
		break;
	case PRINT:
		exfreenode(p, x->data.operand.left);
		break;
	case PRINTF:
	case SPRINTF:
		if (x->data.print.descriptor)
			exfreenode(p, x->data.print.descriptor);
		pn = x->data.print.args;
		while ((pr = pn))
		{
			for (i = 0; i < elementsof(pr->param) && pr->param[i]; i++)
				exfreenode(p, pr->param[i]);
			if (pr->arg)
				exfreenode(p, pr->arg);
			pn = pr->next;
			vmfree(p->vm, pr);
		}
		break;
	default:
		if (x->data.operand.left)
			exfreenode(p, x->data.operand.left);
		if (x->data.operand.right)
			exfreenode(p, x->data.operand.right);
		break;
	}
	vmfree(p->vm, x);
}

/* extract:
 * Given an argument list, extract first argument,
 * check its type, reset argument list, and 
 * return first argument.
 * Return 0 on failure.
 */
static Exnode_t *extract(Expr_t * p, Exnode_t ** argp, int type) {
	Exnode_t *args = *argp;
	Exnode_t *left;

	if (!args || (type != args->data.operand.left->type))
	    return 0;
	*argp = args->data.operand.right;
	left = args->data.operand.left;
	args->data.operand.left = args->data.operand.right = 0;
	exfreenode(p, args);
	return left;
}

/* exnewsplit:
 * Generate split/tokens node.
 * Fifth argument is optional.
 */
static Exnode_t *exnewsplit(Expr_t * p, int op, Exid_t* dyn, Exnode_t * s, Exnode_t* seps) {
	Exnode_t *ss = 0;

	if (dyn->local.pointer == 0)
              	exerror("cannot use non-array %s in %s", dyn->name, exopname(op));
	if ((dyn->index_type > 0) && (dyn->index_type != INTEGER))
            exerror("in %s, array %s must have integer index type, not %s", 
		exopname(op), dyn->name, extypename(p, s->type));
	if (dyn->type != STRING)
            exerror("in %s, array %s entries must have string type, not %s", 
		exopname(op), dyn->name, extypename(p, s->type));
	if (s->type != STRING)
            exerror("first argument to %s must have string type, not %s", 
		exopname(op), extypename(p, s->type));
	if (seps && (seps->type != STRING))
            exerror("third argument to %s must have string type, not %s", 
		exopname(op), extypename(p, seps->type));
	ss = exnewnode(p, op, 0, INTEGER, NiL, NiL);
	ss->data.split.array = dyn;
	ss->data.split.string = s;
	ss->data.split.seps = seps;
	return ss;
}

/* exnewsub:
 * Generate sub node.
 * Third argument is optional.
 */
static Exnode_t *exnewsub(Expr_t * p, Exnode_t * args, int op) {
	Exnode_t *base;
	Exnode_t *pat;
	Exnode_t *repl;
	Exnode_t *ss = 0;

	base = extract(p, &args, STRING);
	if (!base)
	    exerror("invalid first argument to sub operator");
	pat = extract(p, &args, STRING);
	if (!pat)
	    exerror("invalid second argument to sub operator");
	if (args) {
	    repl = extract(p, &args, STRING);
	    if (!repl)
		exerror("invalid third argument to sub operator");
	} else
	    repl = 0;
	if (args)
	    exerror("too many arguments to sub operator");
	ss = exnewnode(p, op, 0, STRING, NiL, NiL);
	ss->data.string.base = base;
	ss->data.string.pat = pat;
	ss->data.string.repl = repl;
	return ss;
}

/* exnewsubstr:
 * Generate substr node.
 */
static Exnode_t *exnewsubstr(Expr_t * p, Exnode_t * args) {
	Exnode_t *base;
	Exnode_t *pat;
	Exnode_t *repl;
	Exnode_t *ss = 0;

	base = extract(p, &args, STRING);
	if (!base)
	    exerror("invalid first argument to substr operator");
	pat = extract(p, &args, INTEGER);
	if (!pat)
	    exerror("invalid second argument to substr operator");
	if (args) {
	    repl = extract(p, &args, INTEGER);
	    if (!repl)
		exerror("invalid third argument to substr operator");
	} else
	    repl = 0;
	if (args)
	    exerror("too many arguments to substr operator");
	ss = exnewnode(p, SUBSTR, 0, STRING, NiL, NiL);
	ss->data.string.base = base;
	ss->data.string.pat = pat;
	ss->data.string.repl = repl;
	return ss;
}

/* exstringOf:
 * Cast x to type STRING
 * Assume x->type != STRING
 */
static Exnode_t *exstringOf(Expr_t * p, register Exnode_t * x) {
	int type = x->type;
	int cvt = 0;

	if (!type) {
	    x->type = STRING;
	    return x;
	}
	if (!BUILTIN(type) && !p->disc->stringof)
	    exerror("cannot convert %s to STRING", extypename(p, type));
	if (x->op != CONSTANT) {
	    if (!BUILTIN(type)) {
		if ((*p->disc->stringof) (p, x, 1, p->disc) < 0) {
		    exerror("cannot convert %s to STRING",
			    extypename(p, type));
		}
		cvt = XPRINT;
	    } else
		switch (type) {
		case FLOATING:
		    cvt = F2S;
		    break;
		case INTEGER:
		    cvt = I2S;
		    break;
		}
	    x = exnewnode(p, cvt, 0, STRING, x, 0);
	} else if (!BUILTIN(type)) {
	    if ((*p->disc->stringof) (p, x, 0, p->disc) < 0)
		exerror("cannot convert constant %s to STRING",
			extypename(p, x->type));
	} else
	    switch (type) {
	    case FLOATING:
		sfprintf(p->tmp, "%g", x->data.constant.value.floating);
		x->data.constant.value.string =
		    vmstrdup(p->vm, sfstruse(p->tmp));
		break;
	    case INTEGER:
		sfprintf(p->tmp, "%I*d",
			 sizeof(x->data.constant.value.integer),
			 x->data.constant.value.integer);
		x->data.constant.value.string =
		    vmstrdup(p->vm, sfstruse(p->tmp));
		break;
	    default:
		exerror("internal error: %d: unknown type", type);
		break;
	    }
	x->type = STRING;
	return x;
}

/* exprint:
 * Generate argument list of strings.
 */
static Exnode_t *exprint(Expr_t * p, Exid_t * ex, Exnode_t * args) {
	Exnode_t *arg = args;
	Exnode_t *pr;

	while (arg) {
	    if (arg->data.operand.left->type != STRING)
		arg->data.operand.left =
		    exstringOf(p, arg->data.operand.left);
	    arg = arg->data.operand.right;
	}
	pr = exnewnode(p, ex->index, 1, ex->type, args, NiL);
	return pr;
}

/* makeVar:
 *
 * Create variable from s[idx].refs 
 * If s is DYNAMIC, refs is non-empty and dyna represents s[idx].
 * The rightmost element in s[idx].refs becomes the dominant symbol,
 * and the prefix gets stored in refs. (This format is used to simplify
 * the yacc parser.)
 */
static Exnode_t *makeVar(Expr_t * prog, Exid_t * s, Exnode_t * idx,
			     Exnode_t * dyna, Exref_t * refs) {
	Exnode_t *nn;
	int kind;
	Exid_t *sym;

	/* parse components */
	if (refs) {
	    if (refs->next) {
		sym = refs->next->symbol;
		refs->next->symbol = refs->symbol;
	    } else
		sym = refs->symbol;
	    refs->symbol = s;
	    refs->index = idx;
	} else
	    sym = s;

	if (sym->type)
	    kind = sym->type;
	else
	    kind = STRING;

	nn = exnewnode(prog, ID, 0, kind, NiL, NiL);
	nn->data.variable.symbol = sym;
	nn->data.variable.reference = refs;
	nn->data.variable.index = 0;
	nn->data.variable.dyna = dyna;
	if (!prog->disc->getf)
	    exerror("%s: identifier references not supported", sym->name);
	else if (expr.program->disc->reff)
	    (*expr.program->disc->reff) (prog, nn,
					 nn->data.variable.symbol, refs,
					 NiL, EX_SCALAR, prog->disc);

	return nn;
}

/*
 * cast x to type
 */

static char*	typename[] =
{
	"external", "integer", "unsigned", "char", "float", "string"
};

static int	typecast[6][6] =
{
	{X2X,	X2I,	X2I,	X2I,	X2F,	X2S},
	{I2X,	0,	0,	0,	I2F,	I2S},
	{I2X,	0,	0,	0,	I2F,	I2S},
	{I2X,	0,	0,	0,	I2F,	I2S},
	{F2X,	F2I,	F2I,	F2I,	0,	F2S},
	{S2X,	S2I,	S2I,	S2I,	S2F,	0},
};

#define TYPEINDEX(t)	(((t)>=INTEGER&&(t)<=STRING)?((t)-INTEGER+1):0)
#define TYPENAME(t)	typename[TYPEINDEX(t)]
#define TYPECAST(f,t)	typecast[TYPEINDEX(f)][TYPEINDEX(t)]

#define EXTERNAL(t)	((t)>=F2X)

char *extypename(Expr_t * p, int type) {
	if (BUILTIN(type))
	    return TYPENAME(type);
	return (p->disc->typename) (p, type);
}

/* exnoncast:
 * Return first non-cast node.
 */
Exnode_t *exnoncast(register Exnode_t * x) {
	while (x && (x->op >= F2I) && (x->op <= X2X))
	    x = x->data.operand.left;
	return x;
}

Exnode_t*
excast(Expr_t* p, register Exnode_t* x, register int type, register Exnode_t* xref, int arg)
{
	register int	t2t;
	char*		s;
	char*		e;

	if (x && x->type != type && type && type != VOIDTYPE)
	{
		if (!x->type)
		{
			x->type = type;
			return x;
		}
		if (!(t2t = TYPECAST(x->type, type)))
			return x;
		if (EXTERNAL(t2t) && !p->disc->convertf)
			exerror("cannot convert %s to %s", extypename(p, x->type), extypename(p, type));
		if (x->op != CONSTANT) {
			Exid_t *sym = (xref ? xref->data.variable.symbol : NiL);
			if (EXTERNAL(t2t)) {
				int a = (arg ? arg : 1);
		    	if ((*p->disc->convertf) (p, x, type, sym, a, p->disc) < 0) {
					if (xref) {
						if ((sym->lex == FUNCTION) && arg)
							exerror ("%s: cannot use value of type %s as argument %d in function %s",
				     			sym->name, extypename(p, x->type),
				     			arg, sym->name);
						else
							exerror("%s: cannot convert %s to %s",
							xref->data.variable.symbol->name,
							extypename(p, x->type),
							extypename(p, type));
					} else {
			    		exerror("cannot convert %s to %s",
							extypename(p, x->type), extypename(p, type));
					}
				}
			}
			x = exnewnode(p, t2t, 0, type, x, xref);
		}
		else switch (t2t)
		{
		case F2X:
		case I2X:
		case S2X:
		case X2F:
		case X2I:
		case X2S:
		case X2X:
			if (xref && xref->op == ID)
			{
				if ((*p->disc->convertf)(p, x, type, xref->data.variable.symbol, arg, p->disc) < 0)
					exerror("%s: cannot cast constant %s to %s", xref->data.variable.symbol->name, extypename(p, x->type), extypename(p, type));
			}
			else if ((*p->disc->convertf)(p, x, type, NiL, arg, p->disc) < 0)
				exerror("cannot cast constant %s to %s", extypename(p, x->type), extypename(p, type));
			break;
		case F2I:
			x->data.constant.value.integer = x->data.constant.value.floating;
			break;
		case F2S:
			sfprintf(p->tmp, "%g", x->data.constant.value.floating);
			x->data.constant.value.string = exstash(p->tmp, p->vm);
			break;
		case I2F:
			x->data.constant.value.floating = x->data.constant.value.integer;
			break;
		case I2S:
			sfprintf(p->tmp, "%I*d", sizeof(x->data.constant.value.integer), x->data.constant.value.integer);
			x->data.constant.value.string = exstash(p->tmp, p->vm);
			break;
		case S2F:
			s =  x->data.constant.value.string;
			x->data.constant.value.integer = strtod(s, &e);
			if (*e)
				x->data.constant.value.floating = (*s != 0);
			break;
		case S2I:
			s = x->data.constant.value.string;
			x->data.constant.value.integer = strtoll(s, &e, 0);
			if (*e)
				x->data.constant.value.integer = (*s != 0);
			break;
		default:
			exerror("internal error: %d: unknown cast op", t2t);
			break;
		}
		x->type = type;
	}
	return x;
}

#if 0

/*
 * convert value v from type from to type to
 * string data placed in buf
 */

Extype_t
exconvert(Expr_t* p, Extype_t v, int from, int to, char* buf, size_t size)
{
	register int	t2t;
	int		n;
	Exnode_t	tmp;

	if (from && (t2t = TYPECAST(from, to)))
	{
		if (EXTERNAL(t2t) && !p->disc->convertf)
			exerror("cannot cast %s to %s", TYPENAME(from), TYPENAME(to));
		switch (t2t)
		{
		case F2X:
		case I2X:
		case S2X:
		case X2F:
		case X2I:
		case X2S:
			tmp.type = from;
			tmp.name = "*INTERNAL*";
			tmp.data.constant.value = v;
			if ((*p->disc->convertf)(p, &tmp, to, NiL, -1, p->disc) < 0)
				exerror("cannot convert %s to %s", TYPENAME(from), TYPENAME(to));
			if (t2t == X2S)
			{
				n = strlen(tmp.data.constant.value.string);
				if (n >= size)
					n = size - 1;
				memcpy(buf, tmp.data.constant.value.string, n);
				buf[n] = 0;
				vmfree(p->vm, tmp.data.constant.value.string);
				tmp.data.constant.value.string = buf;
			}
			return tmp.data.constant.value;
		case F2I:
			v.integer = (type == UNSIGNED) ? (Sfulong_t)v.floating : v.floating;
			break;
		case F2S:
			sfsprintf(buf, size, "%g", v.floating);
			v.string = buf;
			break;
		case I2F:
			v.floating = (from == UNSIGNED) ? (Sfulong_t)v.integer : v.integer;
			break;
		case I2S:
			sfsprintf(buf, size, "%I*d", sizeof(v.integer), v.integer);
			v.string = buf;
			break;
		case S2F:
			v.floating = *v.string != 0;
			break;
		case S2I:
			v.integer = *v.string != 0;
			break;
		default:
			exerror("internal error: %d: unknown conversion op", t2t);
			break;
		}
	}
	return v;
}

#endif

/*
 * force ref . sym qualification
 */

static Exid_t*
qualify(register Exref_t* ref, register Exid_t* sym)
{
	register Exid_t*	x;
	char*			s;

	while (ref->next)
		ref = ref->next;
	sfprintf(expr.program->tmp, "%s.%s", ref->symbol->name, sym->name);
	s = exstash(expr.program->tmp, NiL);
	if (!(x = (Exid_t*)dtmatch(expr.program->symbols, s)))
	{
		if ((x = newof(0, Exid_t, 1, strlen(s) - EX_NAMELEN + 1)))
		{
			memcpy(x, sym, sizeof(Exid_t) - EX_NAMELEN);
			strcpy(x->name, s);
			dtinsert(expr.program->symbols, x);
		}
		else
		{
			exnospace();
			x = sym;
		}
	}
	return x;
}

/*
 * check function call arg types and count
 * return function identifier node
 */

static Exnode_t*
call(Exref_t* ref, register Exid_t* fun, register Exnode_t* args)
{
	register int	t;
	register int	type;
	Exnode_t*	x;
	int		num;

	x = exnewnode(expr.program, ID, 0, 0, NiL, NiL);
	t = fun->type;
	x->data.variable.symbol = fun = QUALIFY(ref, fun);
	x->data.variable.reference = ref;
	num = 0;
	N(t);
	while ((type = T(t)))
	{
		if (!args)
		{
			exerror("%s: not enough args", fun->name);
			return args;
		}
		num++;
		if (type != args->data.operand.left->type)
			args->data.operand.left = excast(expr.program, args->data.operand.left, type, NiL, num);
		args = args->data.operand.right;
		N(t);
	}
	if (args)
		exerror("%s: too many args", fun->name);
	return x;
}

/*
 * precompile a printf/scanf call
 */

static Print_t*
preprint(register Exnode_t* args)
{
	register Print_t*	x;
	register char*		s;
	register int		c;
	int			t;
	int			i;
	int			n;
	char*			e;
	char*			f;
	Print_t*		p = 0;
	Print_t*		q;

	if (!args || args->data.operand.left->type != STRING)
		exerror("format string argument expected");
	if (args->data.operand.left->op != CONSTANT)
	{
		x = ALLOCATE(expr.program, Print_t);
		memzero(x, sizeof(*x));
		x->arg = args;
		return x;
	}
	f = args->data.operand.left->data.constant.value.string;
	args = args->data.operand.right;
	for (s = f; *s; s++)
	{
		sfputc(expr.program->tmp, *s);
		if (*s == '%')
		{
			if (!*++s)
				exerror("%s: trailing %% in format", f);
			if (*s != '%')
				break;
			if (args)
				sfputc(expr.program->tmp, '%');
		}
	}
	x = 0;
	for (;;)
	{
		q = ALLOCATE(expr.program, Print_t);
		if (x)
			x->next = q;
		else
			p = q;
		x = q;
		memzero(x, sizeof(*x));
		if (*s)
		{
			i = 0;
			t = INTEGER;
			for (;;)
			{
				switch (c = *s++)
				{
				case 0:
					exerror("unterminated %%... in format");
					goto done;
				case '*':
					if (i >= elementsof(x->param))
					{
						*s = 0;
						exerror("format %s has too many * arguments", f);
						goto done;
					}
					if (!args)
					{
						*s = 0;
						exerror("format %s * argument expected", f);
						goto done;
					}
					x->param[i++] = args->data.operand.left;
					args = args->data.operand.right;
					break;
				case '(':
					n = 1;
					for (;;)
					{
						sfputc(expr.program->tmp, c);
						switch (c = *s++)
						{
						case 0:
							s--;
							break;
						case '(':
							n++;
							continue;
						case ')':
							if (--n <= 0)
								break;
							continue;
						default:
							continue;
						}
						break;
					}
					break;
				case 'c':
				case 'd':
					goto specified;
				case 'e':
				case 'f':
				case 'g':
					t = FLOATING;
					goto specified;
				case 'h':
					exerror("short formats not supported");
					goto done;
				case 'l':
					t = INTEGER;
					break;
				case 'o':
				case 'u':
				case 'x':
				case 'T':
					t = UNSIGNED;
					goto specified;
				case 's':
				case 'S':
					t = STRING;
					goto specified;
				default:
					if (isalpha(c))
						goto specified;
					break;
				}
				sfputc(expr.program->tmp, c);
			}
		specified:
			sfputc(expr.program->tmp, c);
			for (e = s; *s; s++)
			{
				if (*s == '%')
				{
					if (!*++s)
					{
						*e = 0;
						exerror("%s: trailing %% in format", f);
						goto done;
					}
					if (*s != '%')
					{
						s--;
						break;
					}
				}
				sfputc(expr.program->tmp, *s);
			}
			if (!args)
			{
				*e = 0;
				exerror("%s format argument expected", f);
				goto done;
			}
			x->arg = args->data.operand.left;
			switch (t)
			{
			case FLOATING:
				if (x->arg->type != FLOATING)
					x->arg = exnewnode(expr.program, x->arg->type == STRING ? S2F : INTEGRAL(x->arg->type) ? I2F : X2F, 0, FLOATING, x->arg, x->arg->op == ID ? x->arg : (Exnode_t*)0);
				break;
			case INTEGER:
			case UNSIGNED:
				if (!INTEGRAL(x->arg->type))
					x->arg = exnewnode(expr.program, x->arg->type == STRING ? S2I : x->arg->type == FLOATING ? F2I : X2I, 0, INTEGER, x->arg, x->arg->op == ID ? x->arg : (Exnode_t*)0);
				x->arg->type = t;
				break;
			case STRING:
				if (x->arg->type != STRING)
				{
					if (x->arg->op == CONSTANT && x->arg->data.constant.reference && expr.program->disc->convertf)
					{
						if ((*expr.program->disc->convertf)(expr.program, x->arg, STRING, x->arg->data.constant.reference, 0, expr.program->disc) < 0)
							exerror("cannot convert string format argument");
						else x->arg->data.constant.value.string = vmstrdup(expr.program->vm, x->arg->data.constant.value.string);
					}
					else if (!expr.program->disc->convertf || (x->arg->op != ID && x->arg->op != DYNAMIC && x->arg->op != F2X && x->arg->op != I2X && x->arg->op != S2X))
						exerror("string format argument expected");
					else
						x->arg = exnewnode(expr.program, x->arg->type == FLOATING ? F2S : INTEGRAL(x->arg->type) ? I2S : X2S, 0, STRING, x->arg, x->arg->op == ID ? x->arg : (Exnode_t*)0);
				}
				break;
			}
			args = args->data.operand.right;
		}
		x->format = exstash(expr.program->tmp, expr.program->vm);
		if (!*s)
			break;
		f = s;
	}
	if (args)
		exerror("too many format arguments");
 done:
	sfstrseek(expr.program->tmp, 0, SEEK_SET);
	return p;
}

/*
 * push a new input stream and program
 */

int
expush(Expr_t* p, const char* name, int line, const char* sp, Sfio_t* fp)
{
	register Exinput_t*	in;
	register char*		s;
	char			buf[PATH_MAX];

	if (!(in = newof(0, Exinput_t, 1, 0)))
	{
		exnospace();
		return -1;
	}
	if (!p->input)
		p->input = &expr.null;
	if (!(in->bp = in->sp = (char*)sp))
	{
		if ((in->fp = fp))
			in->close = 0;
		else if (name)
		{
			if (!(s = pathfind(name, p->disc->lib, p->disc->type, buf, sizeof(buf))) || !(in->fp = sfopen(NiL, s, "r")))
			{
				exerror("%s: file not found", name);
				in->bp = in->sp = "";
			}
			else
			{
				name = (const char*)vmstrdup(p->vm, s);
				in->close = 1;
			}
		}
	}
	else in->fp = 0;
	if (!(in->next = p->input)->next)
	{
		p->errors = 0;
		if (!(p->disc->flags & EX_INTERACTIVE))
		{
			if (line >= 0)
				error_info.line = line;
		}
		else if (!error_info.line)
			error_info.line = 1;
	}
	else if (line >= 0)
		error_info.line = line;
	setcontext(p);
	p->eof = 0;
	p->input = in;
	in->file = error_info.file;
	if (line >= 0)
		error_info.file = (char*)name;
	in->line = error_info.line;
	in->nesting = 0;
	in->unit = !name && !line;
	p->program = expr.program;
	expr.program = p;
	return 0;
}

/*
 * pop the current input stream
 */

int
expop(register Expr_t* p)
{
	register int		c;
	register Exinput_t*	in;

	if (!(in = p->input) || !in->next || in->unit)
		return -1;
	if (in->nesting)
		exerror("unbalanced quote or nesting construct");
	error_info.file = in->file;
	if (in->next->next)
		error_info.line = in->line;
	else
	{
		if (p->errors && in->fp && p->linep != p->line)
			while ((c = sfgetc(in->fp)) != EOF)
				if (c == '\n')
				{
					error_info.line++;
					break;
				}
		if (!(p->disc->flags & EX_INTERACTIVE))
			error_info.line = in->line;
	}
	if (in->fp && in->close)
		sfclose(in->fp);
	if (in->pushback)
		free(in->pushback);
	p->input = in->next;
	free(in);
	setcontext(p);
	if (p->program)
		expr.program = p->program;
	return 0;
}

/*
 * clear global state of stale pointers
 */

void exinit(void) {
	memset (&expr, 0, sizeof(Exstate_t));
}

/*
 * compile the expression in [sf]p
 */

int
excomp(register Expr_t* p, const char* name, int line, const char* sp, Sfio_t* fp)
{
	Exid_t*	v;
	int	eof;

	p->more = 0;
	eof = p->eof;
	if (!sp && !fp)
	{
		if (!p->input)
			return -1;
	}
	else if (expush(p, name, line, sp, fp))
		return -1;
	else
		p->input->unit = line >= 0;
	exparse();
	p->input->unit = 0;
	expop(p);
	p->eof = eof;
	if (expr.statics)
	{
		for (v = (Exid_t*)dtfirst(p->symbols); v; v = (Exid_t*)dtnext(p->symbols, v))
			if (v->isstatic)
			{
				dtdelete(p->symbols, v);
				if (!--expr.statics)
					break;
			}
		expr.statics = 0;
	}
	return 0;
}

/*
 * free the program p
 */

void
exclose(register Expr_t* p, int all)
{
	register int		i;
	register Exinput_t*	in;

	if (p)
	{
		if (all)
		{
			for (i = 3; i < elementsof(p->file); i++)
				if (p->file[i])
					sfclose(p->file[i]);
			if (p->vm)
				vmclose(p->vm);
			if (p->ve)
				vmclose(p->ve);
			if (p->symbols)
				dtclose(p->symbols);
			if (p->tmp)
				sfclose(p->tmp);
			while ((in = p->input))
			{
				if (in->pushback)
					free(in->pushback);
				if (in->fp && in->close)
					sfclose(in->fp);
				if ((p->input = in->next))
					free(in);
			}
			free(p);
		}
		else
		{
			vmclear(p->ve);
			p->main.value = 0;
		}
	}
}

/* checkBinary:
 * See if application wants to allow the given expression
 * combination. l and r give the operands; the operator
 * is given by ex. r may be NULL.
 */
static void
checkBinary(Expr_t * p, Exnode_t * l, Exnode_t * ex, Exnode_t * r) 
{
	if ((*p->disc->binaryf) (p, l, ex, r, 1, p->disc) < 0) {
	    if (r)
		exerror
		    ("cannot apply operator %s to expressions of types %s and %s",
		     exopname(ex->op), extypename(p, l->type),
		     extypename(p, r->type));
	    else
		exerror
		    ("cannot apply operator %s to expression of type %s",
		     exopname(ex->op), extypename(p, l->type));
	}
}

/* checkName:
 * We allow parser to accept any name in a declaration, in
 * order to check that the name is undeclared and give a better
 * error message if it isn't.
 */
static void checkName(Exid_t * id) 
{
	switch (id->lex) {
	case DYNAMIC:
	    exerror("Variable \"%s\" already declared", id->name);
	    break;
	case FUNCTION:
	    exerror("Name \"%s\" already used as a function", id->name);
	    break;
	case ID:
	    exerror("Name \"%s\" already used as a keyword", id->name);
	    break;
	case NAME:
	    break;
	default:
	    error(ERROR_PANIC,
		  "Unexpected token \"%s\" as name in dcl_item", id->name);
	    break;
	}
}

static int
cmpKey(Dt_t * d, Extype_t * key1, Extype_t * key2, Dtdisc_t * disc) 
{
	if (key1->integer < key2->integer)
	    return -1;
	else if (key1->integer > key2->integer)
	    return 1;
	else
	    return 0;
}

int
exisAssign(Exnode_t * n) 
{
	return ((n->op == '=') && (n->subop == '='));
}

#endif

#ifdef __cplusplus
}
#endif
