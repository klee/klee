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

%{

/*
 * Glenn Fowler
 * AT&T Research
 *
 * expression library grammar and compiler
 */

#ifdef WIN32
#include <config.h>

#ifdef GVDLL
#define _BLD_sfio 1
#endif
#endif

#include <stdio.h>
#include <ast.h>

#undef	RS	/* hp.pa <signal.h> grabs this!! */

%}

%union
{
	struct Exnode_s*expr;
	double		floating;
	struct Exref_s*	reference;
	struct Exid_s*	id;
	Sflong_t	integer;
	int		op;
	char*		string;
	void*		user;
	struct Exbuf_s*	buffer;
}

%start	program

%token	MINTOKEN

%token	INTEGER
%token	UNSIGNED
%token	CHARACTER
%token	FLOATING
%token	STRING
%token	VOIDTYPE
%token	STATIC

%token	ADDRESS
%token	ARRAY
%token	BREAK
%token	CALL
%token	CASE
%token	CONSTANT
%token	CONTINUE
%token	DECLARE
%token	DEFAULT
%token	DYNAMIC
%token	ELSE
%token	EXIT
%token	FOR
%token	FUNCTION
%token	GSUB
%token	ITERATE
%token	ITERATER
%token	ID
%token	IF
%token	LABEL
%token	MEMBER
%token	NAME
%token	POS
%token	PRAGMA
%token	PRE
%token	PRINT
%token	PRINTF
%token	PROCEDURE
%token	QUERY
%token	RAND
%token	RETURN
%token	SCANF
%token	SPLIT
%token	SPRINTF
%token	SRAND
%token	SSCANF
%token	SUB
%token	SUBSTR
%token	SWITCH
%token	TOKENS
%token	UNSET
%token	WHILE

%token	F2I
%token	F2S
%token	I2F
%token	I2S
%token	S2B
%token	S2F
%token	S2I

%token	F2X
%token	I2X
%token	S2X
%token	X2F
%token	X2I
%token	X2S
%token	X2X
%token	XPRINT

%left	<op>	','
%right	<op>	'='
%right	<op>	'?'	':'
%left	<op>	OR
%left	<op>	AND
%left	<op>	'|'
%left	<op>	'^'
%left	<op>	'&'
%binary	<op>	EQ	NE
%binary	<op>	'<'	'>'	LE	GE
%left	<op>	LS	RS
%left	<op>	'+'	'-'	IN_OP
%left	<op>	'*'	'/'	'%'
%right	<op>	'!'	'~'	'#'	UNARY
%right	<op>	INC	DEC
%right	<op>	CAST
%left	<op>	'('

%type <expr>		statement	statement_list	arg_list
%type <expr>		else_opt	expr_opt	expr
%type <expr>		args		variable	assign
%type <expr>		dcl_list	dcl_item	index
%type <expr>		initialize	switch_item	constant
%type <expr>		formals		formal_list	formal_item
%type <reference>	members
%type <id>		ID		LABEL		NAME
%type <id>		CONSTANT	ARRAY		FUNCTION	DECLARE
%type <id>		EXIT		PRINT		PRINTF		QUERY
%type <id>		RAND		SRAND
%type <id>		SPRINTF		PROCEDURE	name		dcl_name
%type <id>		GSUB		SUB		SUBSTR
%type <id>		SPLIT		TOKENS          splitop
%type <id>		IF		WHILE		FOR		ITERATER
%type <id>		BREAK		CONTINUE	print		member
%type <id>		RETURN		DYNAMIC		SWITCH		UNSET
%type <id>		SCANF		SSCANF		scan
%type <floating>	FLOATING
%type <integer>		INTEGER		UNSIGNED	array
%type <integer>		static
%type <string>		STRING

%token	MAXTOKEN

%{

#include "exgram.h"

%}

%%

program		:	statement_list action_list
		{
			if ($1 && !(expr.program->disc->flags & EX_STRICT))
			{
				if (expr.program->main.value && !(expr.program->disc->flags & EX_RETAIN))
					exfreenode(expr.program, expr.program->main.value);
				if ($1->op == S2B)
				{
					Exnode_t*	x;

					x = $1;
					$1 = x->data.operand.left;
					x->data.operand.left = 0;
					exfreenode(expr.program, x);
				}
				expr.program->main.lex = PROCEDURE;
				expr.program->main.value = exnewnode(expr.program, PROCEDURE, 1, $1->type, NiL, $1);
			}
		}
		;

action_list	:	/* empty */
		|	action_list action
		;

action		:	LABEL ':' {
				register Dtdisc_t*	disc;

				if (expr.procedure)
					exerror("no nested function definitions");
				$1->lex = PROCEDURE;
				expr.procedure = $1->value = exnewnode(expr.program, PROCEDURE, 1, $1->type, NiL, NiL);
				expr.procedure->type = INTEGER;
				if (!(disc = newof(0, Dtdisc_t, 1, 0)))
					exnospace();
				disc->key = offsetof(Exid_t, name);
				if (expr.assigned && !streq($1->name, "begin"))
				{
					if (!(expr.procedure->data.procedure.frame = dtopen(disc, Dtset)) || !dtview(expr.procedure->data.procedure.frame, expr.program->symbols))
						exnospace();
					expr.program->symbols = expr.program->frame = expr.procedure->data.procedure.frame;
				}
			} statement_list
		{
			expr.procedure = 0;
			if (expr.program->frame)
			{
				expr.program->symbols = expr.program->frame->view;
				dtview(expr.program->frame, NiL);
				expr.program->frame = 0;
			}
			if ($4 && $4->op == S2B)
			{
				Exnode_t*	x;

				x = $4;
				$4 = x->data.operand.left;
				x->data.operand.left = 0;
				exfreenode(expr.program, x);
			}
			$1->value->data.operand.right = excast(expr.program, $4, $1->type, NiL, 0);
		}
		;

statement_list	:	/* empty */
		{
			$$ = 0;
		}
		|	statement_list statement
		{
			if (!$1)
				$$ = $2;
			else if (!$2)
				$$ = $1;
			else if ($1->op == CONSTANT)
			{
				exfreenode(expr.program, $1);
				$$ = $2;
			}
#ifdef UNUSED
			else if ($1->op == ';')
			{
				$$ = $1;
				$1->data.operand.last = $1->data.operand.last->data.operand.right = exnewnode(expr.program, ';', 1, $2->type, $2, NiL);
			}
			else
			{
				$$ = exnewnode(expr.program, ';', 1, $1->type, $1, NiL);
				$$->data.operand.last = $$->data.operand.right = exnewnode(expr.program, ';', 1, $2->type, $2, NiL);
			}
#endif
			else $$ = exnewnode(expr.program, ';', 1, $2->type, $1, $2);
		}
		;

statement	:	'{' statement_list '}'
		{
			$$ = $2;
		}
		|	expr_opt ';'
		{
			$$ = ($1 && $1->type == STRING) ? exnewnode(expr.program, S2B, 1, INTEGER, $1, NiL) : $1;
		}
		|	static {expr.instatic=$1;} DECLARE {expr.declare=$3->type;} dcl_list ';'
		{
			$$ = $5;
			expr.declare = 0;
		}
		|	IF '(' expr ')' statement else_opt
		{
			if (exisAssign ($3))
				exwarn ("assignment used as boolean in if statement");
			if ($3->type == STRING)
				$3 = exnewnode(expr.program, S2B, 1, INTEGER, $3, NiL);
			else if (!INTEGRAL($3->type))
				$3 = excast(expr.program, $3, INTEGER, NiL, 0);
			$$ = exnewnode(expr.program, $1->index, 1, INTEGER, $3, exnewnode(expr.program, ':', 1, $5 ? $5->type : 0, $5, $6));
		}
		|	FOR '(' variable ')' statement
		{
			$$ = exnewnode(expr.program, ITERATE, 0, INTEGER, NiL, NiL);
			$$->data.generate.array = $3;
			if (!$3->data.variable.index || $3->data.variable.index->op != DYNAMIC)
				exerror("simple index variable expected");
			$$->data.generate.index = $3->data.variable.index->data.variable.symbol;
			if ($3->op == ID && $$->data.generate.index->type != INTEGER)
				exerror("integer index variable expected");
			exfreenode(expr.program, $3->data.variable.index);
			$3->data.variable.index = 0;
			$$->data.generate.statement = $5;
		}
		|	FOR '(' expr_opt ';' expr_opt ';' expr_opt ')' statement
		{
			if (!$5)
			{
				$5 = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
				$5->data.constant.value.integer = 1;
			}
			else if ($5->type == STRING)
				$5 = exnewnode(expr.program, S2B, 1, INTEGER, $5, NiL);
			else if (!INTEGRAL($5->type))
				$5 = excast(expr.program, $5, INTEGER, NiL, 0);
			$$ = exnewnode(expr.program, $1->index, 1, INTEGER, $5, exnewnode(expr.program, ';', 1, 0, $7, $9));
			if ($3)
				$$ = exnewnode(expr.program, ';', 1, INTEGER, $3, $$);
		}
		|	ITERATER '(' variable ')' statement
		{
			$$ = exnewnode(expr.program, ITERATER, 0, INTEGER, NiL, NiL);
			$$->data.generate.array = $3;
			if (!$3->data.variable.index || $3->data.variable.index->op != DYNAMIC)
				exerror("simple index variable expected");
			$$->data.generate.index = $3->data.variable.index->data.variable.symbol;
			if ($3->op == ID && $$->data.generate.index->type != INTEGER)
				exerror("integer index variable expected");
			exfreenode(expr.program, $3->data.variable.index);
			$3->data.variable.index = 0;
			$$->data.generate.statement = $5;
		}
		|	UNSET '(' DYNAMIC ')'
		{
			if ($3->local.pointer == 0)
              			exerror("cannot apply unset to non-array %s", $3->name);
			$$ = exnewnode(expr.program, UNSET, 0, INTEGER, NiL, NiL);
			$$->data.variable.symbol = $3;
			$$->data.variable.index = NiL;
		}
		|	UNSET '(' DYNAMIC ',' expr  ')'
		{
			if ($3->local.pointer == 0)
              			exerror("cannot apply unset to non-array %s", $3->name);
			if (($3->index_type > 0) && ($5->type != $3->index_type))
            		    exerror("%s indices must have type %s, not %s", 
				$3->name, extypename(expr.program, $3->index_type),extypename(expr.program, $5->type));
			$$ = exnewnode(expr.program, UNSET, 0, INTEGER, NiL, NiL);
			$$->data.variable.symbol = $3;
			$$->data.variable.index = $5;
		}
		|	WHILE '(' expr ')' statement
		{
			if (exisAssign ($3))
				exwarn ("assignment used as boolean in while statement");
			if ($3->type == STRING)
				$3 = exnewnode(expr.program, S2B, 1, INTEGER, $3, NiL);
			else if (!INTEGRAL($3->type))
				$3 = excast(expr.program, $3, INTEGER, NiL, 0);
			$$ = exnewnode(expr.program, $1->index, 1, INTEGER, $3, exnewnode(expr.program, ';', 1, 0, NiL, $5));
		}
		|	SWITCH '(' expr {expr.declare=$3->type;} ')' '{' switch_list '}'
		{
			register Switch_t*	sw = expr.swstate;

			$$ = exnewnode(expr.program, $1->index, 1, INTEGER, $3, exnewnode(expr.program, DEFAULT, 1, 0, sw->defcase, sw->firstcase));
			expr.swstate = expr.swstate->prev;
			if (sw->base)
				free(sw->base);
			if (sw != &swstate)
				free(sw);
			expr.declare = 0;
		}
		|	BREAK expr_opt ';'
		{
		loopop:
			if (!$2)
			{
				$2 = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
				$2->data.constant.value.integer = 1;
			}
			else if (!INTEGRAL($2->type))
				$2 = excast(expr.program, $2, INTEGER, NiL, 0);
			$$ = exnewnode(expr.program, $1->index, 1, INTEGER, $2, NiL);
		}
		|	CONTINUE expr_opt ';'
		{
			goto loopop;
		}
		|	RETURN expr_opt ';'
		{
			if ($2)
			{
				if (expr.procedure && !expr.procedure->type)
					exerror("return in void function");
				$2 = excast(expr.program, $2, expr.procedure ? expr.procedure->type : INTEGER, NiL, 0);
			}
			$$ = exnewnode(expr.program, RETURN, 1, $2 ? $2->type : 0, $2, NiL);
		}
		;

switch_list	:	/* empty */
		{
			register Switch_t*		sw;
			int				n;

			if (expr.swstate)
			{
				if (!(sw = newof(0, Switch_t, 1, 0)))
				{
					exnospace();
					sw = &swstate;
				}
				sw->prev = expr.swstate;
			}
			else
				sw = &swstate;
			expr.swstate = sw;
			sw->type = expr.declare;
			sw->firstcase = 0;
			sw->lastcase = 0;
			sw->defcase = 0;
			sw->def = 0;
			n = 8;
			if (!(sw->base = newof(0, Extype_t*, n, 0)))
			{
				exnospace();
				n = 0;
			}
			sw->cur = sw->base;
			sw->last = sw->base + n;
		}
		|	switch_list switch_item
		;

switch_item	:	case_list statement_list
		{
			register Switch_t*	sw = expr.swstate;
			int			n;

			$$ = exnewnode(expr.program, CASE, 1, 0, $2, NiL);
			if (sw->cur > sw->base)
			{
				if (sw->lastcase)
					sw->lastcase->data.select.next = $$;
				else
					sw->firstcase = $$;
				sw->lastcase = $$;
				n = sw->cur - sw->base;
				sw->cur = sw->base;
				$$->data.select.constant = (Extype_t**)exalloc(expr.program, (n + 1) * sizeof(Extype_t*));
				memcpy($$->data.select.constant, sw->base, n * sizeof(Extype_t*));
				$$->data.select.constant[n] = 0;
			}
			else
				$$->data.select.constant = 0;
			if (sw->def)
			{
				sw->def = 0;
				if (sw->defcase)
					exerror("duplicate default in switch");
				else
					sw->defcase = $2;
			}
		}
		;

case_list	:	case_item
		|	case_list case_item
		;

case_item	:	CASE constant ':'
		{
			int	n;

			if (expr.swstate->cur >= expr.swstate->last)
			{
				n = expr.swstate->cur - expr.swstate->base;
				if (!(expr.swstate->base = newof(expr.swstate->base, Extype_t*, 2 * n, 0)))
				{
					exerror("too many case labels for switch");
					n = 0;
				}
				expr.swstate->cur = expr.swstate->base + n;
				expr.swstate->last = expr.swstate->base + 2 * n;
			}
			if (expr.swstate->cur)
			{
				$2 = excast(expr.program, $2, expr.swstate->type, NiL, 0);
				*expr.swstate->cur++ = &($2->data.constant.value);
			}
		}
		|	DEFAULT ':'
		{
			expr.swstate->def = 1;
		}
		;

static	:	/* empty */
		{
			$$ = 0;
		}
		|	STATIC
		{
			$$ = 1;
		}
		;

dcl_list	:	dcl_item
		|	dcl_list ',' dcl_item
		{
			if ($3)
				$$ = $1 ? exnewnode(expr.program, ',', 1, $3->type, $1, $3) : $3;
		}
		;

dcl_item	:	dcl_name {checkName ($1); expr.id=$1;} array initialize
		{
			$$ = 0;
			if (!$1->type || expr.declare)
				$1->type = expr.declare;
			if ($4 && $4->op == PROCEDURE)
			{
				$1->lex = PROCEDURE;
				$1->type = $4->type;
				$1->value = $4;
			}
			else
			{
				$1->lex = DYNAMIC;
				$1->value = exnewnode(expr.program, 0, 0, 0, NiL, NiL);
				if ($3 && !$1->local.pointer)
				{
					Dtdisc_t*	disc;

					if (!(disc = newof(0, Dtdisc_t, 1, 0)))
						exnospace();
					if ($3 == INTEGER) {
						disc->key = offsetof(Exassoc_t, key);
						disc->size = sizeof(Extype_t);
						disc->comparf = (Dtcompar_f)cmpKey;
					}
					else
						disc->key = offsetof(Exassoc_t, name);
					if (!($1->local.pointer = (char*)dtopen(disc, Dtoset)))
						exerror("%s: cannot initialize associative array", $1->name);
					$1->index_type = $3; /* -1 indicates no typechecking */
				}
				if ($4)
				{
					if ($4->type != $1->type)
					{
						$4->type = $1->type;
						$4->data.operand.right = excast(expr.program, $4->data.operand.right, $1->type, NiL, 0);
					}
					$4->data.operand.left = exnewnode(expr.program, DYNAMIC, 0, $1->type, NiL, NiL);
					$4->data.operand.left->data.variable.symbol = $1;
					$$ = $4;
#if UNUSED
					if (!expr.program->frame && !expr.program->errors)
					{
						expr.assigned++;
						exeval(expr.program, $$, NiL);
					}
#endif
				}
				else if (!$3)
					$1->value->data.value = exzero($1->type);
			}
		}
		;

dcl_name	:	NAME
		|	DYNAMIC
		|	ID
		|	FUNCTION
		;

name		:	NAME
		|	DYNAMIC
		;

else_opt	:	/* empty */
		{
			$$ = 0;
		}
		|	ELSE statement
		{
			$$ = $2;
		}
		;

expr_opt	:	/* empty */
		{
			$$ = 0;
		}
		|	expr
		;

expr		:	'(' expr ')'
		{
			$$ = $2;
		}
		|	'(' DECLARE ')' expr	%prec CAST
		{
			$$ = ($4->type == $2->type) ? $4 : excast(expr.program, $4, $2->type, NiL, 0);
		}
		|	expr '<' expr
		{
			int	rel;

		relational:
			rel = INTEGER;
			goto coerce;
		binary:
			rel = 0;
		coerce:
			if (!$1->type)
			{
				if (!$3->type)
					$1->type = $3->type = rel ? STRING : INTEGER;
				else
					$1->type = $3->type;
			}
			else if (!$3->type)
				$3->type = $1->type;
			if ($1->type != $3->type)
			{
				if ($1->type == STRING)
					$1 = excast(expr.program, $1, $3->type, $3, 0);
				else if ($3->type == STRING)
					$3 = excast(expr.program, $3, $1->type, $1, 0);
				else if ($1->type == FLOATING)
					$3 = excast(expr.program, $3, FLOATING, $1, 0);
				else if ($3->type == FLOATING)
					$1 = excast(expr.program, $1, FLOATING, $3, 0);
			}
			if (!rel)
				rel = ($1->type == STRING) ? STRING : (($1->type == UNSIGNED) ? UNSIGNED : $3->type);
			$$ = exnewnode(expr.program, $2, 1, rel, $1, $3);
			if (!expr.program->errors && $1->op == CONSTANT && $3->op == CONSTANT)
			{
				$$->data.constant.value = exeval(expr.program, $$, NiL);
				$$->binary = 0;
				$$->op = CONSTANT;
				exfreenode(expr.program, $1);
				exfreenode(expr.program, $3);
			}
			else if (!BUILTIN($1->type) || !BUILTIN($3->type)) {
				checkBinary(expr.program, $1, $$, $3);
			}
		}
		|	expr '-' expr
		{
			goto binary;
		}
		|	expr '*' expr
		{
			goto binary;
		}
		|	expr '/' expr
		{
			goto binary;
		}
		|	expr '%' expr
		{
			goto binary;
		}
		|	expr LS expr
		{
			goto binary;
		}
		|	expr RS expr
		{
			goto binary;
		}
		|	expr '>' expr
		{
			goto relational;
		}
		|	expr LE expr
		{
			goto relational;
		}
		|	expr GE expr
		{
			goto relational;
		}
		|	expr EQ expr
		{
			goto relational;
		}
		|	expr NE expr
		{
			goto relational;
		}
		|	expr '&' expr
		{
			goto binary;
		}
		|	expr '|' expr
		{
			goto binary;
		}
		|	expr '^' expr
		{
			goto binary;
		}
		|	expr '+' expr
		{
			goto binary;
		}
		|	expr AND expr
		{
		logical:
			if ($1->type == STRING)
				$1 = exnewnode(expr.program, S2B, 1, INTEGER, $1, NiL);
			else if (!BUILTIN($1->type))
				$1 = excast(expr.program, $1, INTEGER, NiL, 0);
			if ($3->type == STRING)
				$3 = exnewnode(expr.program, S2B, 1, INTEGER, $3, NiL);
			else if (!BUILTIN($3->type))
				$3 = excast(expr.program, $3, INTEGER, NiL, 0);
			goto binary;
		}
		|	expr OR expr
		{
			goto logical;
		}
		|	expr ',' expr
		{
			if ($1->op == CONSTANT)
			{
				exfreenode(expr.program, $1);
				$$ = $3;
			}
			else
				$$ = exnewnode(expr.program, ',', 1, $3->type, $1, $3);
		}
		|	expr '?' {expr.nolabel=1;} expr ':' {expr.nolabel=0;} expr
		{
			if (!$4->type)
			{
				if (!$7->type)
					$4->type = $7->type = INTEGER;
				else
					$4->type = $7->type;
			}
			else if (!$7->type)
				$7->type = $4->type;
			if ($1->type == STRING)
				$1 = exnewnode(expr.program, S2B, 1, INTEGER, $1, NiL);
			else if (!INTEGRAL($1->type))
				$1 = excast(expr.program, $1, INTEGER, NiL, 0);
			if ($4->type != $7->type)
			{
				if ($4->type == STRING || $7->type == STRING)
					exerror("if statement string type mismatch");
				else if ($4->type == FLOATING)
					$7 = excast(expr.program, $7, FLOATING, NiL, 0);
				else if ($7->type == FLOATING)
					$4 = excast(expr.program, $4, FLOATING, NiL, 0);
			}
			if ($1->op == CONSTANT)
			{
				if ($1->data.constant.value.integer)
				{
					$$ = $4;
					exfreenode(expr.program, $7);
				}
				else
				{
					$$ = $7;
					exfreenode(expr.program, $4);
				}
				exfreenode(expr.program, $1);
			}
			else
				$$ = exnewnode(expr.program, '?', 1, $4->type, $1, exnewnode(expr.program, ':', 1, $4->type, $4, $7));
		}
		|	'!' expr
		{
		iunary:
			if ($2->type == STRING)
				$2 = exnewnode(expr.program, S2B, 1, INTEGER, $2, NiL);
			else if (!INTEGRAL($2->type))
				$2 = excast(expr.program, $2, INTEGER, NiL, 0);
		unary:
			$$ = exnewnode(expr.program, $1, 1, $2->type == UNSIGNED ? INTEGER : $2->type, $2, NiL);
			if ($2->op == CONSTANT)
			{
				$$->data.constant.value = exeval(expr.program, $$, NiL);
				$$->binary = 0;
				$$->op = CONSTANT;
				exfreenode(expr.program, $2);
			}
			else if (!BUILTIN($2->type)) {
				checkBinary(expr.program, $2, $$, 0);
			}
		}
		|	'#' DYNAMIC
		{
			if ($2->local.pointer == 0)
              			exerror("cannot apply '#' operator to non-array %s", $2->name);
			$$ = exnewnode(expr.program, '#', 0, INTEGER, NiL, NiL);
			$$->data.variable.symbol = $2;
		}
		|	'~' expr
		{
			goto iunary;
		}
		|	'-' expr	%prec UNARY
		{
			goto unary;
		}
		|	'+' expr	%prec UNARY
		{
			$$ = $2;
		}
		|	'&' variable	%prec UNARY
		{
			$$ = exnewnode(expr.program, ADDRESS, 0, T($2->type), $2, NiL);
		}
		|	ARRAY '[' args ']'
		{
			$$ = exnewnode(expr.program, ARRAY, 1, T($1->type), call(0, $1, $3), $3);
		}
		|	FUNCTION '(' args ')'
		{
			$$ = exnewnode(expr.program, FUNCTION, 1, T($1->type), call(0, $1, $3), $3);
#ifdef UNUSED
			if (!expr.program->disc->getf)
				exerror("%s: function references not supported", $$->data.operand.left->data.variable.symbol->name);
			else if (expr.program->disc->reff)
				(*expr.program->disc->reff)(expr.program, $$->data.operand.left, $$->data.operand.left->data.variable.symbol, 0, NiL, EX_CALL, expr.program->disc);
#endif
		}
		|	GSUB '(' args ')'
		{
			$$ = exnewsub (expr.program, $3, GSUB);
		}
		|	SUB '(' args ')'
		{
			$$ = exnewsub (expr.program, $3, SUB);
		}
		|	SUBSTR '(' args ')'
		{
			$$ = exnewsubstr (expr.program, $3);
		}
		|	splitop '(' expr ',' DYNAMIC ')'
		{
			$$ = exnewsplit (expr.program, $1->index, $5, $3, NiL);
		}
		|	splitop '(' expr ',' DYNAMIC ',' expr ')'
		{
			$$ = exnewsplit (expr.program, $1->index, $5, $3, $7);
		}
		|	EXIT '(' expr ')'
		{
			if (!INTEGRAL($3->type))
				$3 = excast(expr.program, $3, INTEGER, NiL, 0);
			$$ = exnewnode(expr.program, EXIT, 1, INTEGER, $3, NiL);
		}
		|	RAND '(' ')'
		{
			$$ = exnewnode(expr.program, RAND, 0, FLOATING, NiL, NiL);
		}
		|	SRAND '(' ')'
		{
			$$ = exnewnode(expr.program, SRAND, 0, INTEGER, NiL, NiL);
		}
		|	SRAND '(' expr ')'
		{
			if (!INTEGRAL($3->type))
				$3 = excast(expr.program, $3, INTEGER, NiL, 0);
			$$ = exnewnode(expr.program, SRAND, 1, INTEGER, $3, NiL);
		}
		|	PROCEDURE '(' args ')'
		{
			$$ = exnewnode(expr.program, CALL, 1, $1->type, NiL, $3);
			$$->data.call.procedure = $1;
		}
		|	PRINT '(' args ')'
		{
			$$ = exprint(expr.program, $1, $3);
		}
		|	print '(' args ')'
		{
			$$ = exnewnode(expr.program, $1->index, 0, $1->type, NiL, NiL);
			if ($3 && $3->data.operand.left->type == INTEGER)
			{
				$$->data.print.descriptor = $3->data.operand.left;
				$3 = $3->data.operand.right;
			}
			else 
				switch ($1->index)
				{
				case QUERY:
					$$->data.print.descriptor = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
					$$->data.print.descriptor->data.constant.value.integer = 2;
					break;
				case PRINTF:
					$$->data.print.descriptor = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
					$$->data.print.descriptor->data.constant.value.integer = 1;
					break;
				case SPRINTF:
					$$->data.print.descriptor = 0;
					break;
				}
			$$->data.print.args = preprint($3);
		}
		|	scan '(' args ')'
		{
			register Exnode_t*	x;

			$$ = exnewnode(expr.program, $1->index, 0, $1->type, NiL, NiL);
			if ($3 && $3->data.operand.left->type == INTEGER)
			{
				$$->data.scan.descriptor = $3->data.operand.left;
				$3 = $3->data.operand.right;
			}
			else 
				switch ($1->index)
				{
				case SCANF:
					$$->data.scan.descriptor = 0;
					break;
				case SSCANF:
					if ($3 && $3->data.operand.left->type == STRING)
					{
						$$->data.scan.descriptor = $3->data.operand.left;
						$3 = $3->data.operand.right;
					}
					else
						exerror("%s: string argument expected", $1->name);
					break;
				}
			if (!$3 || !$3->data.operand.left || $3->data.operand.left->type != STRING)
				exerror("%s: format argument expected", $1->name);
			$$->data.scan.format = $3->data.operand.left;
			for (x = $$->data.scan.args = $3->data.operand.right; x; x = x->data.operand.right)
			{
				if (x->data.operand.left->op != ADDRESS)
					exerror("%s: address argument expected", $1->name);
				x->data.operand.left = x->data.operand.left->data.operand.left;
			}
		}
		|	variable assign
		{
			if ($2)
			{
				if ($1->op == ID && !expr.program->disc->setf)
					exerror("%s: variable assignment not supported", $1->data.variable.symbol->name);
				else
				{
					if (!$1->type)
						$1->type = $2->type;
#if 0
					else if ($2->type != $1->type && $1->type >= 0200)
#else
					else if ($2->type != $1->type)
#endif
					{
						$2->type = $1->type;
						$2->data.operand.right = excast(expr.program, $2->data.operand.right, $1->type, NiL, 0);
					}
					$2->data.operand.left = $1;
					$$ = $2;
				}
			}
		}
		|	INC variable
		{
		pre:
			if ($2->type == STRING)
				exerror("++ and -- invalid for string variables");
			$$ = exnewnode(expr.program, $1, 0, $2->type, $2, NiL);
			$$->subop = PRE;
		}
		|	variable INC
		{
		pos:
			if ($1->type == STRING)
				exerror("++ and -- invalid for string variables");
			$$ = exnewnode(expr.program, $2, 0, $1->type, $1, NiL);
			$$->subop = POS;
		}
		|	expr IN_OP DYNAMIC
		{
			if ($3->local.pointer == 0)
              			exerror("cannot apply IN to non-array %s", $3->name);
			if (($3->index_type > 0) && ($1->type != $3->index_type))
            		    exerror("%s indices must have type %s, not %s", 
				$3->name, extypename(expr.program, $3->index_type),extypename(expr.program, $1->type));
			$$ = exnewnode(expr.program, IN_OP, 0, INTEGER, NiL, NiL);
			$$->data.variable.symbol = $3;
			$$->data.variable.index = $1;
		}
		|	DEC variable
		{
			goto pre;
		}
		|	variable DEC
		{
			goto pos;
		}
		|	constant
		;

splitop		:	SPLIT
		|	TOKENS
		;
constant	:	CONSTANT
		{
			$$ = exnewnode(expr.program, CONSTANT, 0, $1->type, NiL, NiL);
			if (!expr.program->disc->reff)
				exerror("%s: identifier references not supported", $1->name);
			else
				$$->data.constant.value = (*expr.program->disc->reff)(expr.program, $$, $1, NiL, NiL, EX_SCALAR, expr.program->disc);
		}
		|	FLOATING
		{
			$$ = exnewnode(expr.program, CONSTANT, 0, FLOATING, NiL, NiL);
			$$->data.constant.value.floating = $1;
		}
		|	INTEGER
		{
			$$ = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
			$$->data.constant.value.integer = $1;
		}
		|	STRING
		{
			$$ = exnewnode(expr.program, CONSTANT, 0, STRING, NiL, NiL);
			$$->data.constant.value.string = $1;
		}
		|	UNSIGNED
		{
			$$ = exnewnode(expr.program, CONSTANT, 0, UNSIGNED, NiL, NiL);
			$$->data.constant.value.integer = $1;
		}
		;

print		:	PRINTF
		|	QUERY
		|	SPRINTF
		;

scan		:	SCANF
		|	SSCANF
		;

variable	:	ID members
		{
			$$ = makeVar(expr.program, $1, 0, 0, $2);
		}
		|	DYNAMIC index members
		{
			Exnode_t*   n;

			n = exnewnode(expr.program, DYNAMIC, 0, $1->type, NiL, NiL);
			n->data.variable.symbol = $1;
			n->data.variable.reference = 0;
			if (((n->data.variable.index = $2) == 0) != ($1->local.pointer == 0))
				exerror("%s: is%s an array", $1->name, $1->local.pointer ? "" : " not");
			if ($1->local.pointer && ($1->index_type > 0)) {
				if ($2->type != $1->index_type)
					exerror("%s: indices must have type %s, not %s", 
						$1->name, extypename(expr.program, $1->index_type),extypename(expr.program, $2->type));
			}
			if ($3) {
				n->data.variable.dyna =exnewnode(expr.program, 0, 0, 0, NiL, NiL);
				$$ = makeVar(expr.program, $1, $2, n, $3);
			}
			else $$ = n;
		}
		|	NAME
		{
			$$ = exnewnode(expr.program, ID, 0, STRING, NiL, NiL);
			$$->data.variable.symbol = $1;
			$$->data.variable.reference = 0;
			$$->data.variable.index = 0;
			$$->data.variable.dyna = 0;
			if (!(expr.program->disc->flags & EX_UNDECLARED))
				exerror("unknown identifier");
		}
		;

array		:	/* empty */
		{
			$$ = 0;
		}
		|	'[' ']'
		{
			$$ = -1;
		}
		|	'[' DECLARE ']'
		{
			/* If DECLARE is VOID, its type is 0, so this acts like
			 * the empty case.
			 */
			if (INTEGRAL($2->type))
				$$ = INTEGER;
			else
				$$ = $2->type;
				
		}
		;

index		:	/* empty */
		{
			$$ = 0;
		}
		|	'[' expr ']'
		{
			$$ = $2;
		}
		;

args		:	/* empty */
		{
			$$ = 0;
		}
		|	arg_list
		{
			$$ = $1->data.operand.left;
			$1->data.operand.left = $1->data.operand.right = 0;
			exfreenode(expr.program, $1);
		}
		;

arg_list	:	expr		%prec ','
		{
			$$ = exnewnode(expr.program, ',', 1, 0, exnewnode(expr.program, ',', 1, $1->type, $1, NiL), NiL);
			$$->data.operand.right = $$->data.operand.left;
		}
		|	arg_list ',' expr
		{
			$1->data.operand.right = $1->data.operand.right->data.operand.right = exnewnode(expr.program, ',', 1, $1->type, $3, NiL);
		}
		;

formals		:	/* empty */
		{
			$$ = 0;
		}
		|	DECLARE
		{
			$$ = 0;
			if ($1->type)
				exerror("(void) expected");
		}
		|	formal_list
		;

formal_list	:	formal_item
		{
			$$ = exnewnode(expr.program, ',', 1, $1->type, $1, NiL);
		}
		|	formal_list ',' formal_item
		{
			register Exnode_t*	x;
			register Exnode_t*	y;

			$$ = $1;
			for (x = $1; (y = x->data.operand.right); x = y);
			x->data.operand.right = exnewnode(expr.program, ',', 1, $3->type, $3, NiL);
		}
		;

formal_item	:	DECLARE {expr.declare=$1->type;} name
		{
			$$ = exnewnode(expr.program, ID, 0, $1->type, NiL, NiL);
			$$->data.variable.symbol = $3;
			$3->lex = DYNAMIC;
			$3->type = $1->type;
			$3->value = exnewnode(expr.program, 0, 0, 0, NiL, NiL);
			expr.procedure->data.procedure.arity++;
			expr.declare = 0;
		}
		;

members	:	/* empty */
		{
			$$ = expr.refs = expr.lastref = 0;
		}
		|	member
		{
			Exref_t*	r;

			r = ALLOCATE(expr.program, Exref_t);
			r->symbol = $1;
			expr.refs = r;
			expr.lastref = r;
			r->next = 0;
			r->index = 0;
			$$ = expr.refs;
		}
		|	'.' ID member
		{
			Exref_t*	r;
			Exref_t*	l;

			r = ALLOCATE(expr.program, Exref_t);
			r->symbol = $3;
			r->index = 0;
			r->next = 0;
			l = ALLOCATE(expr.program, Exref_t);
			l->symbol = $2;
			l->index = 0;
			l->next = r;
			expr.refs = l;
			expr.lastref = r;
			$$ = expr.refs;
		}
		;

member	:	'.' ID
		{
			$$ = $2;
		}
		|	'.' NAME
		{
			$$ = $2;
		}
		;
assign		:	/* empty */
		{
			$$ = 0;
		}
		|	'=' expr
		{
			$$ = exnewnode(expr.program, '=', 1, $2->type, NiL, $2);
			$$->subop = $1;
		}
		;

initialize	:	assign
		|	'(' {
				register Dtdisc_t*	disc;

				if (expr.procedure)
					exerror("%s: nested function definitions not supported", expr.id->name);
				expr.procedure = exnewnode(expr.program, PROCEDURE, 1, expr.declare, NiL, NiL);
				if (!(disc = newof(0, Dtdisc_t, 1, 0)))
					exnospace();
				disc->key = offsetof(Exid_t, name);
				if (!streq(expr.id->name, "begin"))
				{
					if (!(expr.procedure->data.procedure.frame = dtopen(disc, Dtset)) || !dtview(expr.procedure->data.procedure.frame, expr.program->symbols))
						exnospace();
					expr.program->symbols = expr.program->frame = expr.procedure->data.procedure.frame;
					expr.program->formals = 1;
				}
				expr.declare = 0;
			} formals {
				expr.id->lex = PROCEDURE;
				expr.id->type = expr.procedure->type;
				expr.program->formals = 0;
				expr.declare = 0;
			} ')' '{' statement_list '}'
		{
			$$ = expr.procedure;
			expr.procedure = 0;
			if (expr.program->frame)
			{
				expr.program->symbols = expr.program->frame->view;
				dtview(expr.program->frame, NiL);
				expr.program->frame = 0;
			}
			$$->data.operand.left = $3;
			$$->data.operand.right = excast(expr.program, $7, $$->type, NiL, 0);

			/*
			 * NOTE: procedure definition was slipped into the
			 *	 declaration initializer statement production,
			 *	 therefore requiring the statement terminator
			 */

			exunlex(expr.program, ';');
		}
		;

%%

#include "exgram.h"
