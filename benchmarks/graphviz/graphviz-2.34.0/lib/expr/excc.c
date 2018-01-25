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

/*
 * Glenn Fowler
 * AT&T Research
 *
 * expression library C program generator
 */

#define _EX_CC_PRIVATE_ \
	char*		id;		/* prefix + _			*/ \
	int		lastop;		/* last op			*/ \
	int		tmp;		/* temp var index		*/ \
	Exccdisc_t*	ccdisc;		/* excc() discipline		*/

#include "exlib.h"
#include <string.h>

#define EX_CC_DUMP	0x8000

static const char	quote[] = "\"";

static void		gen(Excc_t*, Exnode_t*);

/*
 * return C name for op
 */

char*
exopname(int op)
{
	static char	buf[16];

	switch (op)
	{
	case '!':
		return "!";
	case '%':
		return "%";
	case '&':
		return "&";
	case '(':
		return "(";
	case '*':
		return "*";
	case '+':
		return "+";
	case ',':
		return ",";
	case '-':
		return "-";
	case '/':
		return "/";
	case ':':
		return ":";
	case '<':
		return "<";
	case '=':
		return "=";
	case '>':
		return ">";
	case '?':
		return "?";
	case '^':
		return "^";
	case '|':
		return "|";
	case '~':
		return "~";
	case AND:
		return "&&";
	case EQ:
		return "==";
	case GE:
		return ">=";
	case LE:
		return "<=";
	case LS:
		return "<<";
	case NE:
		return "!=";
	case OR:
		return "||";
	case RS:
		return ">>";
	}
	sfsprintf(buf, sizeof(buf) - 1, "(OP=%03o)", op);
	return buf;
}

/*
 * generate printf()
 */

static void
print(Excc_t* cc, Exnode_t* expr)
{
	register Print_t*	x;
	register int		i;

	if ((x = expr->data.print.args))
	{
		sfprintf(cc->ccdisc->text, "sfprintf(%s, \"%s", expr->data.print.descriptor->op == CONSTANT && expr->data.print.descriptor->data.constant.value.integer == 2 ? "sfstderr" : "sfstdout", fmtesq(x->format, quote));
		while ((x = x->next))
			sfprintf(cc->ccdisc->text, "%s", fmtesq(x->format, quote));
		sfprintf(cc->ccdisc->text, "\"");
		for (x = expr->data.print.args; x; x = x->next)
		{
			if (x->arg)
			{
				for (i = 0; i < elementsof(x->param) && x->param[i]; i++)
				{
					sfprintf(cc->ccdisc->text, ", (");
					gen(cc, x->param[i]);
					sfprintf(cc->ccdisc->text, ")");
				}
				sfprintf(cc->ccdisc->text, ", (");
				gen(cc, x->arg);
				sfprintf(cc->ccdisc->text, ")");
			}
		}
		sfprintf(cc->ccdisc->text, ");\n");
	}
}

/*
 * generate scanf()
 */

static void
scan(Excc_t* cc, Exnode_t* expr)
{
	register Print_t*	x;
	register int		i;

	if ((x = expr->data.print.args))
	{
		sfprintf(cc->ccdisc->text, "sfscanf(sfstdin, \"%s", fmtesq(x->format, quote));
		while ((x = x->next))
			sfprintf(cc->ccdisc->text, "%s", fmtesq(x->format, quote));
		sfprintf(cc->ccdisc->text, "\"");
		for (x = expr->data.print.args; x; x = x->next)
		{
			if (x->arg)
			{
				for (i = 0; i < elementsof(x->param) && x->param[i]; i++)
				{
					sfprintf(cc->ccdisc->text, ", &(");
					gen(cc, x->param[i]);
					sfprintf(cc->ccdisc->text, ")");
				}
				sfprintf(cc->ccdisc->text, ", &(");
				gen(cc, x->arg);
				sfprintf(cc->ccdisc->text, ")");
			}
		}
		sfprintf(cc->ccdisc->text, ");\n");
	}
}

/*
 * internal excc
 */

static void
gen(Excc_t* cc, register Exnode_t* expr)
{
	register Exnode_t*	x;
	register Exnode_t*	y;
	register int		n;
	register int		m;
	register int		t;
	char*			s;
	Extype_t*		v;
	Extype_t**		p;

	if (!expr)
		return;
	if (expr->op == CALL) {
		sfprintf(cc->ccdisc->text, "%s(", expr->data.call.procedure->name);
		if (expr->data.call.args)
			gen(cc, expr->data.call.args);
		sfprintf(cc->ccdisc->text, ")");
		return;
	}
	x = expr->data.operand.left;
	switch (expr->op)
	{
	case BREAK:
		sfprintf(cc->ccdisc->text, "break;\n");
		return;
	case CONTINUE:
		sfprintf(cc->ccdisc->text, "continue;\n");
		return;
	case CONSTANT:
		switch (expr->type)
		{
		case FLOATING:
			sfprintf(cc->ccdisc->text, "%g", expr->data.constant.value.floating);
			break;
		case STRING:
			sfprintf(cc->ccdisc->text, "\"%s\"", fmtesq(expr->data.constant.value.string, quote));
			break;
		case UNSIGNED:
			sfprintf(cc->ccdisc->text, "%I*u", sizeof(expr->data.constant.value.integer), expr->data.constant.value.integer);
			break;
		default:
			sfprintf(cc->ccdisc->text, "%I*d", sizeof(expr->data.constant.value.integer), expr->data.constant.value.integer);
			break;
		}
		return;
	case DEC:
		sfprintf(cc->ccdisc->text, "%s--", x->data.variable.symbol->name);
		return;
	case DYNAMIC:
		sfprintf(cc->ccdisc->text, "%s", expr->data.variable.symbol->name);
		return;
	case EXIT:
		sfprintf(cc->ccdisc->text, "exit(");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ");\n");
		return;
	case FUNCTION:
		gen(cc, x);
		sfprintf(cc->ccdisc->text, "(");
		if ((y = expr->data.operand.right)) {
			gen(cc, y);
		}
		sfprintf(cc->ccdisc->text, ")");
		return;
	case RAND:
		sfprintf(cc->ccdisc->text, "rand();\n");
		return;
	case SRAND:
		if (expr->binary) {
			sfprintf(cc->ccdisc->text, "srand(");
			gen(cc, x);
			sfprintf(cc->ccdisc->text, ");\n");
		} else
			sfprintf(cc->ccdisc->text, "srand();\n");
		return;
   	case GSUB:
   	case SUB:
   	case SUBSTR:
		s = (expr->op == GSUB ? "gsub(" : expr->op == SUB ? "sub(" : "substr(");
		sfprintf(cc->ccdisc->text, s);
		gen(cc, expr->data.string.base);
		sfprintf(cc->ccdisc->text, ", ");
		gen(cc, expr->data.string.pat);
		if (expr->data.string.repl) {
			sfprintf(cc->ccdisc->text, ", ");
			gen(cc, expr->data.string.repl);
		}
		sfprintf(cc->ccdisc->text, ")");
		return;
   	case IN_OP:
		gen(cc, expr->data.variable.index);
		sfprintf(cc->ccdisc->text, " in %s", expr->data.variable.symbol->name);
		return;
	case IF:
		sfprintf(cc->ccdisc->text, "if (");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ") {\n");
		gen(cc, expr->data.operand.right->data.operand.left);
		if (expr->data.operand.right->data.operand.right)
		{
			sfprintf(cc->ccdisc->text, "} else {\n");
			gen(cc, expr->data.operand.right->data.operand.right);
		}
		sfprintf(cc->ccdisc->text, "}\n");
		return;
	case FOR:
		sfprintf(cc->ccdisc->text, "for (;");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ");");
		if (expr->data.operand.left)
		{
			sfprintf(cc->ccdisc->text, "(");
			gen(cc, expr->data.operand.left);
			sfprintf(cc->ccdisc->text, ")");
		}
		sfprintf(cc->ccdisc->text, ") {");
		if (expr->data.operand.right)
			gen(cc, expr->data.operand.right);
		sfprintf(cc->ccdisc->text, "}");
		return;
	case ID:
		if (cc->ccdisc->ccf)
			(*cc->ccdisc->ccf)(cc, expr, expr->data.variable.symbol, expr->data.variable.reference, expr->data.variable.index, cc->ccdisc);
		else
			sfprintf(cc->ccdisc->text, "%s", expr->data.variable.symbol->name);
		return;
	case INC:
		sfprintf(cc->ccdisc->text, "%s++", x->data.variable.symbol->name);
		return;
	case ITERATE:
	case ITERATER:
		if (expr->op == DYNAMIC)
		{
			sfprintf(cc->ccdisc->text, "{ Exassoc_t* %stmp_%d;", cc->id, ++cc->tmp);
			sfprintf(cc->ccdisc->text, "for (%stmp_%d = (Exassoc_t*)dtfirst(%s); %stmp_%d && (%s = %stmp_%d->name); %stmp_%d = (Exassoc_t*)dtnext(%s, %stmp_%d)) {", cc->id, cc->tmp, expr->data.generate.array->data.variable.symbol->name, cc->id, cc->tmp, expr->data.generate.index->name, cc->id, cc->tmp, cc->id, cc->tmp, expr->data.generate.array->data.variable.symbol->name, cc->id, cc->tmp);
			gen(cc, expr->data.generate.statement);
			sfprintf(cc->ccdisc->text, "} }");
		}
		return;
	case PRINT:
		sfprintf(cc->ccdisc->text, "print");
		if (x)
			gen(cc, x);
		else
			sfprintf(cc->ccdisc->text, "()");
		return;
	case PRINTF:
		print(cc, expr);
		return;
	case RETURN:
		sfprintf(cc->ccdisc->text, "return(");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ");\n");
		return;
	case SCANF:
		scan(cc, expr);
		return;
	case SPLIT:
	case TOKENS:
		if (expr->op == SPLIT)
			sfprintf(cc->ccdisc->text, "split (");
		else
			sfprintf(cc->ccdisc->text, "tokens (");
		gen(cc, expr->data.split.string);
		sfprintf(cc->ccdisc->text, ", %s", expr->data.split.array->name);
		if (expr->data.split.seps) {
			sfprintf(cc->ccdisc->text, ",");
			gen(cc, expr->data.split.seps);
		}
		sfprintf(cc->ccdisc->text, ")");
		return;
	case SWITCH:
		t = x->type;
		sfprintf(cc->ccdisc->text, "{ %s %stmp_%d = ", extype(t), cc->id, ++cc->tmp);
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ";");
		x = expr->data.operand.right;
		y = x->data.select.statement;
		n = 0;
		while ((x = x->data.select.next))
		{
			if (n)
				sfprintf(cc->ccdisc->text, "else ");
			if (!(p = x->data.select.constant))
				y = x->data.select.statement;
			else
			{
				m = 0;
				while ((v = *p++))
				{
					if (m)
						sfprintf(cc->ccdisc->text, "||");
					else
					{
						m = 1;
						sfprintf(cc->ccdisc->text, "if (");
					}
					if (t == STRING)
						sfprintf(cc->ccdisc->text, "strmatch(%stmp_%d, \"%s\")", cc->id, cc->tmp, fmtesq(v->string, quote));
					else
					{
						sfprintf(cc->ccdisc->text, "%stmp_%d == ", cc->id, cc->tmp);
						switch (t)
						{
						case INTEGER:
						case UNSIGNED:
							sfprintf(cc->ccdisc->text, "%I*u", sizeof(v->integer), v->integer);
							break;
						default:
							sfprintf(cc->ccdisc->text, "%g", v->floating);
							break;
						}
					}
				}
				sfprintf(cc->ccdisc->text, ") {");
				gen(cc, x->data.select.statement);
				sfprintf(cc->ccdisc->text, "}");
			}
		}
		if (y)
		{
			if (n)
				sfprintf(cc->ccdisc->text, "else ");
			sfprintf(cc->ccdisc->text, "{");
			gen(cc, y);
			sfprintf(cc->ccdisc->text, "}");
		}
		sfprintf(cc->ccdisc->text, "}");
		return;
	case UNSET:
		sfprintf(cc->ccdisc->text, "unset(%s", expr->data.variable.symbol->name);
		if (expr->data.variable.index) {
			sfprintf(cc->ccdisc->text, ",");
			gen(cc, expr->data.variable.index);
		}
		sfprintf(cc->ccdisc->text, ")");
		return;
	case WHILE:
		sfprintf(cc->ccdisc->text, "while (");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ") {");
		if (expr->data.operand.right)
			gen(cc, expr->data.operand.right);
		sfprintf(cc->ccdisc->text, "}");
		return;
    case '#':
		sfprintf(cc->ccdisc->text, "# %s", expr->data.variable.symbol->name);
		return;
	case '=':
		sfprintf(cc->ccdisc->text, "(%s%s=", x->data.variable.symbol->name, expr->subop == '=' ? "" : exopname(expr->subop));
		gen(cc, expr->data.operand.right);
		sfprintf(cc->ccdisc->text, ")");
		return;
	case ';':
		for (;;)
		{
			if (!(x = expr->data.operand.right))
				switch (cc->lastop = expr->data.operand.left->op)
				{
				case FOR:
				case IF:
				case PRINTF:
				case PRINT:
				case RETURN:
				case WHILE:
					break;
				default:
					sfprintf(cc->ccdisc->text, "_%svalue=", cc->id);
					break;
				}
			gen(cc, expr->data.operand.left);
			sfprintf(cc->ccdisc->text, ";\n");
			if (!(expr = x))
				break;
			switch (cc->lastop = expr->op)
			{
			case ';':
				continue;
			case FOR:
			case IF:
			case PRINTF:
			case PRINT:
			case RETURN:
			case WHILE:
				break;
			default:
				sfprintf(cc->ccdisc->text, "_%svalue=", cc->id);
				break;
			}
			gen(cc, expr);
			sfprintf(cc->ccdisc->text, ";\n");
			break;
		}
		return;
	case ',':
		sfprintf(cc->ccdisc->text, "(");
		gen(cc, x);
		while ((expr = expr->data.operand.right) && expr->op == ',')
		{
			sfprintf(cc->ccdisc->text, "), (");
			gen(cc, expr->data.operand.left);
		}
		if (expr)
		{
			sfprintf(cc->ccdisc->text, "), (");
			gen(cc, expr);
		}
		sfprintf(cc->ccdisc->text, ")");
		return;
	case '?':
		sfprintf(cc->ccdisc->text, "(");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ") ? (");
		gen(cc, expr->data.operand.right->data.operand.left);
		sfprintf(cc->ccdisc->text, ") : (");
		gen(cc, expr->data.operand.right->data.operand.right);
		sfprintf(cc->ccdisc->text, ")");
		return;
	case AND:
		sfprintf(cc->ccdisc->text, "(");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ") && (");
		gen(cc, expr->data.operand.right);
		sfprintf(cc->ccdisc->text, ")");
		return;
	case OR:
		sfprintf(cc->ccdisc->text, "(");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ") || (");
		gen(cc, expr->data.operand.right);
		sfprintf(cc->ccdisc->text, ")");
		return;
	case F2I:
		sfprintf(cc->ccdisc->text, "(%s)(", extype(INTEGER));
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ")");
		return;
	case I2F:
		sfprintf(cc->ccdisc->text, "(%s)(", extype(FLOATING));
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ")");
		return;
	case S2I:
		/* sfprintf(cc->ccdisc->text, "strto%s(", sizeof(intmax_t) > sizeof(long) ? "ll" : "l"); */
		sfprintf(cc->ccdisc->text, "strtoll(");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ",(char**)0,0)");
		return;
    case X2I:
		sfprintf(cc->ccdisc->text, "X2I(");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ")");
		return;
	case X2X:
		sfprintf(cc->ccdisc->text, "X2X(");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ")");
		return;
	}
	y = expr->data.operand.right;
	if (x->type == STRING)
	{
		switch (expr->op)
		{
		case S2B:
			sfprintf(cc->ccdisc->text, "*(");
			gen(cc, x);
			sfprintf(cc->ccdisc->text, ")!=0");
			return;
		case S2F:
			sfprintf(cc->ccdisc->text, "strtod(");
			gen(cc, x);
			sfprintf(cc->ccdisc->text, ",0)");
			return;
		case S2I:
			sfprintf(cc->ccdisc->text, "strtol(");
			gen(cc, x);
			sfprintf(cc->ccdisc->text, ",0,0)");
			return;
		case S2X:
			sfprintf(cc->ccdisc->text, "** cannot convert string value to external **");
			return;
		case NE:
			sfprintf(cc->ccdisc->text, "!");
			/*FALLTHROUGH*/
		case EQ:
			sfprintf(cc->ccdisc->text, "strmatch(");
			gen(cc, x);
			sfprintf(cc->ccdisc->text, ",");
			gen(cc, y);
			sfprintf(cc->ccdisc->text, ")");
			return;
		case '+':
		case '|':
		case '&':
		case '^':
		case '%':
		case '*':
			sfprintf(cc->ccdisc->text, "** string bits not supported **");
			return;
		}
		switch (expr->op)
		{
		case '<':
			s = "<0";
			break;
		case LE:
			s = "<=0";
			break;
		case GE:
			s = ">=0";
			break;
		case '>':
			s = ">0";
			break;
		default:
			s = "** unknown string op **";
			break;
		}
		sfprintf(cc->ccdisc->text, "strcoll(");
		gen(cc, x);
		sfprintf(cc->ccdisc->text, ",");
		gen(cc, y);
		sfprintf(cc->ccdisc->text, ")%s", s);
		return;
	}
	else
	{
		if (!y)
			sfprintf(cc->ccdisc->text, "%s", exopname(expr->op));
		sfprintf(cc->ccdisc->text, "(");
		gen(cc, x);
		if (y)
		{
			sfprintf(cc->ccdisc->text, ")%s(", exopname(expr->op));
			gen(cc, y);
		}
		sfprintf(cc->ccdisc->text, ")");
	}
	return;
}

/*
 * generate global declarations
 */

static int
global(Dt_t* table, void* object, void* handle)
{
	register Excc_t*	cc = (Excc_t*)handle;
	register Exid_t*	sym = (Exid_t*)object;

	if (sym->lex == DYNAMIC)
		sfprintf(cc->ccdisc->text, "static %s	%s;\n", extype(sym->type), sym->name);
	return 0;
}

/*
 * open C program generator context
 */

Excc_t*
exccopen(Expr_t* expr, Exccdisc_t* disc)
{
	register Excc_t*	cc;
	char*			id;

	if (!(id = disc->id))
		id = "";
	if (!(cc = newof(0, Excc_t, 1, strlen(id) + 2)))
		return 0;
	cc->expr = expr;
	cc->disc = expr->disc;
	cc->id = (char*)(cc + 1);
	cc->ccdisc = disc;
	if (!(disc->flags & EX_CC_DUMP))
	{
		sfprintf(disc->text, "/* : : generated by %s : : */\n", exversion);
		sfprintf(disc->text, "\n#include <ast.h>\n");
		if (*id)
			sfsprintf(cc->id, strlen(id) + 2, "%s_", id);
		sfprintf(disc->text, "\n");
		dtwalk(expr->symbols, global, cc);
	}
	return cc;
}

/*
 * close C program generator context
 */

int
exccclose(Excc_t* cc)
{
	int	r = 0;

	if (!cc)
		r = -1;
	else
	{
		if (!(cc->ccdisc->flags & EX_CC_DUMP))
		{
			if (cc->ccdisc->text)
				sfclose(cc->ccdisc->text);
			else
				r = -1;
		}
		free(cc);
	}
	return r;
}

/*
 * generate the program for name or sym coerced to type
 */

int
excc(Excc_t* cc, const char* name, Exid_t* sym, int type)
{
	register char*	t;

	if (!cc)
		return -1;
	if (!sym)
		sym = name ? (Exid_t*)dtmatch(cc->expr->symbols, name) : &cc->expr->main;
	if (sym && sym->lex == PROCEDURE && sym->value)
	{
		t = extype(type);
		sfprintf(cc->ccdisc->text, "\n%s %s%s(data) char** data; {\n%s _%svalue = 0;\n", t, cc->id, sym->name, t, cc->id);
		gen(cc, sym->value->data.procedure.body);
		sfprintf(cc->ccdisc->text, ";\n");
		if (cc->lastop != RETURN)
			sfprintf(cc->ccdisc->text, "return _%svalue;\n", cc->id);
		sfprintf(cc->ccdisc->text, "}\n");
		return 0;
	}
	return -1;
}

/*
 * dump an expression tree on sp
 */

int
exdump(Expr_t* expr, Exnode_t* node, Sfio_t* sp)
{
	Excc_t*		cc;
	Exccdisc_t	ccdisc;
	Exid_t*		sym;

	memset(&ccdisc, 0, sizeof(ccdisc));
	ccdisc.flags = EX_CC_DUMP;
	ccdisc.text = sp;
	if (!(cc = exccopen(expr, &ccdisc)))
		return -1;
	if (node)
		gen(cc, node);
	else
		for (sym = (Exid_t*)dtfirst(expr->symbols); sym; sym = (Exid_t*)dtnext(expr->symbols, sym))
			if (sym->lex == PROCEDURE && sym->value)
			{
				sfprintf(sp, "%s:\n", sym->name);
				gen(cc, sym->value->data.procedure.body);
			}
	sfprintf(sp, "\n");
	return exccclose(cc);
}
