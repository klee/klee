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

/*
 * Glenn Fowler
 * AT&T Research
 *
 * expression library default lexical analyzer
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "exlib.h"
#include <string.h>

#if !defined(TRACE_lex) && _BLD_DEBUG
#define TRACE_lex	-10
#endif

#if TRACE_lex

/*
 * trace c for op
 */

static void
trace(Expr_t* ex, int lev, char* op, int c)
{
	char*	s = 0;
	char*	t;
	char	buf[16];
	void*	x = 0;

	t = "";
	switch (c)
	{
	case 0:
		s = " EOF";
		break;
	case '=':
		s = t = buf;
		*t++ = ' ';
		if (!lev && exlval.op != c)
			*t++ = exlval.op;
		*t++ = c;
		*t = 0;
		break;
	case AND:
		s = " AND ";
		t = "&&";
		break;
	case DEC:
		s = " DEC ";
		t = "--";
		break;
	case DECLARE:
		s = " DECLARE ";
		t = exlval.id->name;
		break;
	case DYNAMIC:
		s = " DYNAMIC ";
		t = exlval.id->name;
		x = (void *) (exlval.id);
		break;
	case EQ:
		s = " EQ ";
		t = "==";
		break;
	case FLOATING:
		s = " FLOATING ";
		sfsprintf(t = buf, sizeof(buf), "%f", exlval.floating);
		break;
	case GE:
		s = " GE ";
		t = ">=";
		break;
	case ID:
		s = " ID ";
		t = exlval.id->name;
		break;
	case INC:
		s = "INC ";
		t = "++";
		break;
	case INTEGER:
		s = " INTEGER ";
		sfsprintf(t = buf, sizeof(buf), "%I*d", sizeof(exlval.integer), exlval.integer);
		break;
	case LABEL:
		s = " LABEL ";
		t = exlval.id->name;
		break;
	case LE:
		s = " LE ";
		t = "<=";
		break;
	case LS:
		s = " LS ";
		t = "<<";
		break;
	case NAME:
		s = " NAME ";
		t = exlval.id->name;
		x = (void *) (exlval.id);
		break;
	case NE:
		s = " NE ";
		t = "!=";
		break;
	case OR:
		s = " OR ";
		t = "||";
		break;
	case RS:
		s = " RS ";
		t = ">>";
		break;
	case STRING:
		s = " STRING ";
		t = fmtesc(exlval.string);
		break;
	case UNSIGNED:
		s = " UNSIGNED ";
		sfsprintf(t = buf, sizeof(buf), "%I*u", sizeof(exlval.integer), exlval.integer);
		break;
	case BREAK:
		s = " break";
		break;
	case CASE:
		s = " case";
		break;
	case CONTINUE:
		s = " continue";
		break;
	case DEFAULT:
		s = " default";
		break;
	case ELSE:
		s = " else";
		break;
	case EXIT:
		s = " exit";
		break;
	case FOR:
		s = " for";
		break;
	case ITERATER:
		s = " forf";
		break;
	case GSUB:
		s = " gsub";
		break;
	case IF:
		s = " if";
		break;
	case IN_OP:
		s = " in";
		break;
	case PRAGMA:
		s = " pragma";
		break;
	case PRINT:
		s = " print";
		break;
	case PRINTF:
		s = " printf";
		break;
	case QUERY:
		s = " query";
		break;
	case RAND:
		s = " rand";
		break;
	case RETURN:
		s = " return";
		break;
	case SPLIT:
		s = " split";
		break;
	case SPRINTF:
		s = " sprintf";
		break;
	case SRAND:
		s = " srand";
		break;
	case SUB:
		s = " sub";
		break;
	case SUBSTR:
		s = " substr";
		break;
	case SWITCH:
		s = " switch";
		break;
	case TOKENS:
		s = " tokens";
		break;
	case UNSET:
		s = " unset";
		break;
	case WHILE:
		s = " while";
		break;
	default:
		if (c < 0177)
		{
			s = buf;
			*s++ = c;
			*s = 0;
			t = fmtesc(buf);
			s = " ";
		}
		break;
	}
	if (x)
		error(TRACE_lex + lev, "%s: [%d] %04d%s%s (%x)", op, ex->input->nesting, c, s, t, x);

	else
		error(TRACE_lex + lev, "%s: [%d] %04d%s%s", op, ex->input->nesting, c, s, t);
}

/*
 * trace wrapper for extoken()
 */

extern int	_extoken_fn_(Expr_t*);

int
extoken_fn(Expr_t* ex)
{
	int	c;

#define extoken_fn	_extoken_fn_

	c = extoken_fn(ex);
	trace(ex, 0, "exlex", c);
	return c;
}

#else

#define trace(p,a,b,c)

#endif

/*
 * get the next expression char
 */

static int
lex(register Expr_t* ex)
{
	register int	c;

	for (;;)
	{
		if ((c = ex->input->peek))
			ex->input->peek = 0;
		else if (ex->input->pp)
		{
			if (!(c = *ex->input->pp++))
			{
				ex->input->pp = 0;
				continue;
			}
		}
		else if (ex->input->sp)
		{
			if (!(c = *ex->input->sp++))
			{
				if (!expop(ex))
					continue;
				else trace(ex, -1, "expop sp FAIL", 0);
				ex->input->sp--;
			}
		}
		else if (ex->input->fp)
		{
			if ((c = sfgetc(ex->input->fp)) == EOF)
			{
				if (!expop(ex))
					continue;
				else trace(ex, -1, "expop fp FAIL", 0);
				c = 0;
			}
			else if ((ex->disc->flags & EX_INTERACTIVE) && c == '\n' && ex->input->next && !ex->input->next->next && ex->input->nesting <= 0)
			{
				error_info.line++;
				expop(ex);
				trace(ex, -1, "expop sp FORCE", 0);
				c = 0;
			}
		}
		else c = 0;
		if (c == '\n')
			setcontext(ex);
		else if (c)
			putcontext(ex, c);
		trace(ex, -3, "ex--lex", c);
		return c;
	}
}

/*
 * get the next expression token
 */

int
extoken_fn(register Expr_t* ex)
{
	register int	c;
	register char*	s;
	register int	q;
	int		b;
	char*		e;
	Dt_t*		v;

	if (ex->eof || ex->errors)
		return 0;
 again:
	for (;;)
		switch (c = lex(ex))
		{
		case 0:
			goto eof;
		case '/':
			switch (q = lex(ex))
			{
			case '*':
				for (;;) switch (lex(ex))
				{
				case '\n':
					if (error_info.line)
						error_info.line++;
					else error_info.line = 2;
					continue;
				case '*':
					switch (lex(ex))
					{
					case 0:
						goto eof;
					case '\n':
						if (error_info.line)
							error_info.line++;
						else error_info.line = 2;
						break;
					case '*':
						exunlex(ex, '*');
						break;
					case '/':
						goto again;
					}
					break;
				}
				break;
			case '/':
				while ((c = lex(ex)) != '\n')
					if (!c)
						goto eof;
				break;
			default:
				goto opeq;
			}
			/*FALLTHROUGH*/
		case '\n':
			if (error_info.line)
				error_info.line++;
			else error_info.line = 2;
			/*FALLTHROUGH*/
		case ' ':
		case '\t':
			break;
		case '(':
		case '{':
		case '[':
			ex->input->nesting++;
			return exlval.op = c;
		case ')':
		case '}':
		case ']':
			ex->input->nesting--;
			return exlval.op = c;
		case '+':
		case '-':
			if ((q = lex(ex)) == c)
				return exlval.op = c == '+' ? INC : DEC;
			goto opeq;
		case '*':
		case '%':
		case '^':
			q = lex(ex);
		opeq:
			exlval.op = c;
			if (q == '=')
				c = '=';
			else if (q == '%' && c == '%')
			{
				if (ex->input->fp)
					ex->more = (const char*)ex->input->fp;
				else ex->more = ex->input->sp;
				goto eof;
			}
			else exunlex(ex, q);
			return c;
		case '&':
		case '|':
			if ((q = lex(ex)) == '=')
			{
				exlval.op = c;
				return '=';
			}
			if (q == c)
				c = c == '&' ? AND : OR;
			else exunlex(ex, q);
			return exlval.op = c;
		case '<':
		case '>':
			if ((q = lex(ex)) == c)
			{
				exlval.op = c = c == '<' ? LS : RS;
				if ((q = lex(ex)) == '=')
					c = '=';
				else exunlex(ex, q);
				return c;
			}
			goto relational;
		case '=':
		case '!':
			q = lex(ex);
		relational:
			if (q == '=') switch (c)
			{
			case '<':
				c = LE;
				break;
			case '>':
				c = GE;
				break;
			case '=':
				c = EQ;
				break;
			case '!':
				c = NE;
				break;
			}
			else exunlex(ex, q);
			return exlval.op = c;
		case '#':
			if (!ex->linewrap && !(ex->disc->flags & EX_PURE))
			{
				s = ex->linep - 1;
				while (s > ex->line && isspace(*(s - 1)))
					s--;
				if (s == ex->line)
				{
					switch (extoken_fn(ex))
					{
					case DYNAMIC:
					case ID:
					case NAME:
						s = exlval.id->name;
						break;
					default:
						s = "";
						break;
					}
					if (streq(s, "include"))
					{
						if (extoken_fn(ex) != STRING)
							exerror("#%s: string argument expected", s);
						else if (!expush(ex, exlval.string, 1, NiL, NiL))
						{
							setcontext(ex);
							goto again;
						}
					}
					else exerror("unknown directive");
				}
			}
			return exlval.op = c;
		case '\'':
		case '"':
			q = c;
			sfstrseek(ex->tmp, 0, SEEK_SET);
			ex->input->nesting++;
			while ((c = lex(ex)) != q)
			{
				if (c == '\\')
				{
					sfputc(ex->tmp, c);
					c = lex(ex);
				}
				if (!c)
				{
					exerror("unterminated %c string", q);
					goto eof;
				}
				if (c == '\n')
				{
					if (error_info.line)
						error_info.line++;
					else error_info.line = 2;
				}
				sfputc(ex->tmp, c);
			}
			ex->input->nesting--;
			s = exstash(ex->tmp, NiL);
			if (q == '"' || (ex->disc->flags & EX_CHARSTRING))
			{
				if (!(exlval.string = vmstrdup(ex->vm, s)))
					goto eof;
				stresc(exlval.string);
				return STRING;
			}
			exlval.integer = chrtoi(s);
			return INTEGER;
		case '.':
			if (isdigit(c = lex(ex)))
			{
				sfstrseek(ex->tmp, 0, SEEK_SET);
				sfputc(ex->tmp, '0');
				sfputc(ex->tmp, '.');
				goto floating;
			}
			exunlex(ex, c);
			return exlval.op = '.';
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			sfstrseek(ex->tmp, 0, SEEK_SET);
			sfputc(ex->tmp, c);
			q = INTEGER;
			b = 0;
			if ((c = lex(ex)) == 'x' || c == 'X')
			{
				b = 16;
				sfputc(ex->tmp, c);
				for (;;)
				{
					switch (c = lex(ex))
					{
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
					case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
					case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
						sfputc(ex->tmp, c);
						continue;
					}
					break;
				}
			}
			else
			{
				while (isdigit(c))
				{
					sfputc(ex->tmp, c);
					c = lex(ex);
				}
				if (c == '#')
				{
					sfputc(ex->tmp, c);
					/* s = exstash(ex->tmp, NiL); */
					/* b = strtol(s, NiL, 10); */
					do
					{
						sfputc(ex->tmp, c);
					} while (isalnum(c = lex(ex)));
				}
				else
				{
					if (c == '.')
					{
					floating:
						q = FLOATING;
						sfputc(ex->tmp, c);
						while (isdigit(c = lex(ex)))
							sfputc(ex->tmp, c);
					}
					if (c == 'e' || c == 'E')
					{
						q = FLOATING;
						sfputc(ex->tmp, c);
						if ((c = lex(ex)) == '-' || c == '+')
						{
							sfputc(ex->tmp, c);
							c = lex(ex);
						}
						while (isdigit(c))
						{
							sfputc(ex->tmp, c);
							c = lex(ex);
						}
					}
				}
			}
			s = exstash(ex->tmp, NiL);
			if (q == FLOATING)
				exlval.floating = strtod(s, &e);
			else
			{
				if (c == 'u' || c == 'U')
				{
					q = UNSIGNED;
					c = lex(ex);
					exlval.integer = strtoull(s, &e, b);
				}
				else
					exlval.integer = strtoll(s, &e, b);
				if (*e)
				{
					*--e = 1;
					exlval.integer *= strton(e, &e, NiL, 0);
				}
			}
			exunlex(ex, c);
			if (*e || isalpha(c) || c == '_' || c == '$')
			{
				exerror("%s: invalid numeric constant", s);
				goto eof;
			}
			return q;
		default:
			if (isalpha(c) || c == '_' || c == '$')
			{
				sfstrseek(ex->tmp, 0, SEEK_SET);
				sfputc(ex->tmp, c);
				while (isalnum(c = lex(ex)) || c == '_' || c == '$')
					sfputc(ex->tmp, c);
				exunlex(ex, c);
				s = exstash(ex->tmp, NiL);
				/* v = expr.declare ? dtview(ex->symbols, NiL) : (Dt_t*)0; FIX */
				v = (Dt_t*)0;
				exlval.id = (Exid_t*)dtmatch(ex->symbols, s);
				if (v)
					dtview(ex->symbols, v);
				if (!exlval.id)
				{
					if (!(exlval.id = newof(0, Exid_t, 1, strlen(s) - EX_NAMELEN + 1)))
					{
						exnospace();
						goto eof;
					}
					strcpy(exlval.id->name, s);
					exlval.id->lex = NAME;
					expr.statics += exlval.id->isstatic = expr.instatic;

					/*
					 * LABELs are in the parent scope!
					 */

					if (c == ':' && !expr.nolabel && ex->frame && ex->frame->view)
						dtinsert(ex->frame->view, exlval.id);
					else
						dtinsert(ex->symbols, exlval.id);
				}

				/*
				 * lexical analyzer state controlled by the grammar
				 */

				switch (exlval.id->lex)
				{
				case DECLARE:
					if (exlval.id->index == CHARACTER)
					{
						/*
						 * `char*' === `string'
						 * the * must immediately follow char
						 */

						if (c == '*')
						{
							lex(ex);
							exlval.id = id_string;
						}
					}
					break;
				case NAME:
					/*
					 * action labels are disambiguated from ?:
					 * through the expr.nolabel grammar hook
					 * the : must immediately follow labels
					 */

					if (c == ':' && !expr.nolabel)
						return LABEL;
					break;
				case PRAGMA:
					/*
					 * user specific statement stripped and
					 * passed as string
					 */

					{
						int	b;
						int	n;
						int	pc;
						int	po;
						int	t;

						/*UNDENT...*/
		sfstrseek(ex->tmp, 0, SEEK_SET);
		b = 1;
		n = 0;
		po = 0;
		t = 0;
		for (c = t = lex(ex);; c = lex(ex))
		{
			switch (c)
			{
			case 0:
				goto eof;
			case '/':
				switch (q = lex(ex))
				{
				case '*':
					for (;;)
					{
						switch (lex(ex))
						{
						case '\n':
							if (error_info.line)
								error_info.line++;
							else error_info.line = 2;
							continue;
						case '*':
							switch (lex(ex))
							{
							case 0:
								goto eof;
							case '\n':
								if (error_info.line)
									error_info.line++;
								else error_info.line = 2;
								continue;
							case '*':
								exunlex(ex, '*');
								continue;
							case '/':
								break;
							default:
								continue;
							}
							break;
						}
						if (!b++)
							goto eof;
						sfputc(ex->tmp, ' ');
						break;
					}
					break;
				case '/':
					while ((c = lex(ex)) != '\n')
						if (!c)
							goto eof;
					if (error_info.line)
						error_info.line++;
					else error_info.line = 2;
					b = 1;
					sfputc(ex->tmp, '\n');
					break;
				default:
					b = 0;
					sfputc(ex->tmp, c);
					sfputc(ex->tmp, q);
					break;
				}
				continue;
			case '\n':
				if (error_info.line)
					error_info.line++;
				else error_info.line = 2;
				b = 1;
				sfputc(ex->tmp, '\n');
				continue;
			case ' ':
			case '\t':
				if (!b++)
					goto eof;
				sfputc(ex->tmp, ' ');
				continue;
			case '(':
			case '{':
			case '[':
				b = 0;
				if (!po)
				{
					switch (po = c)
					{
					case '(':
						pc = ')';
						break;
					case '{':
						pc = '}';
						break;
					case '[':
						pc = ']';
						break;
					}
					n++;
				}
				else if (c == po)
					n++;
				sfputc(ex->tmp, c);
				continue;
			case ')':
			case '}':
			case ']':
				b = 0;
				if (!po)
				{
					exunlex(ex, c);
					break;
				}
				sfputc(ex->tmp, c);
				if (c == pc && --n <= 0)
				{
					if (t == po)
						break;
					po = 0;
				}
				continue;
			case ';':
				b = 0;
				if (!n)
					break;
				sfputc(ex->tmp, c);
				continue;
			case '\'':
			case '"':
				b = 0;
				sfputc(ex->tmp, c);
				ex->input->nesting++;
				q = c;
				while ((c = lex(ex)) != q)
				{
					if (c == '\\')
					{
						sfputc(ex->tmp, c);
						c = lex(ex);
					}
					if (!c)
					{
						exerror("unterminated %c string", q);
						goto eof;
					}
					if (c == '\n')
					{
						if (error_info.line)
							error_info.line++;
						else error_info.line = 2;
					}
					sfputc(ex->tmp, c);
				}
				ex->input->nesting--;
				continue;
			default:
				b = 0;
				sfputc(ex->tmp, c);
				continue;
			}
			break;
		}
		(*ex->disc->reff)(ex, NiL, exlval.id, NiL, exstash(ex->tmp, NiL), 0, ex->disc);

						/*..INDENT*/
					}
					goto again;
				}
				return exlval.id->lex;
			}
			return exlval.op = c;
		}
 eof:
	ex->eof = 1;
	return exlval.op = ';';
}
