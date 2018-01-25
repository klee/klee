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
 * expression library readonly tables
 */

static const char id[] = "\n@(#)$Id: libexpr (AT&T Research) 2011-06-30 $\0\n";

#include <exlib.h>

const char*	exversion = id + 10;

Exid_t		exbuiltin[] =
{

	/* id_string references the first entry */

	EX_ID("string",	DECLARE,	STRING,		STRING,	0),

	/* order not important after this point (but sorted anyway) */

	EX_ID("break",	BREAK,		BREAK,		0,	0),
	EX_ID("case",	CASE,		CASE,		0,	0),
	EX_ID("char",	DECLARE,	CHARACTER,		CHARACTER,	0),
	EX_ID("continue",CONTINUE,	CONTINUE,	0,	0),
	EX_ID("default",	DEFAULT,	DEFAULT,	0,	0),
	EX_ID("double",	DECLARE,	FLOATING,	FLOATING,0),
	EX_ID("else",	ELSE,		ELSE,		0,	0),
	EX_ID("exit",	EXIT,		EXIT,		INTEGER,0),
	EX_ID("for",	FOR,		FOR,		0,	0),
	EX_ID("forr",	ITERATER,	ITERATER,	0,	0),
	EX_ID("float",	DECLARE,	FLOATING,	FLOATING,0),
	EX_ID("gsub",	GSUB,		GSUB,		STRING,	0),
	EX_ID("if",	IF,		IF,		0,	0),
	EX_ID("in",	IN_OP,		IN_OP,		0,	0),
	EX_ID("int",	DECLARE,	INTEGER,	INTEGER,0),
	EX_ID("long",	DECLARE,	INTEGER,	INTEGER,0),
	EX_ID("print",	PRINT,		PRINT,		INTEGER,0),
	EX_ID("printf",	PRINTF,		PRINTF,		INTEGER,0),
	EX_ID("query",	QUERY,		QUERY,		INTEGER,0),
	EX_ID("rand",	RAND,		RAND,		FLOATING,0),
	EX_ID("return",	RETURN,		RETURN,		0,	0),
	EX_ID("scanf",	SCANF,		SCANF,		INTEGER,0),
	EX_ID("sscanf",	SSCANF,		SSCANF,		INTEGER,0),
	EX_ID("split",	SPLIT,		SPLIT,		INTEGER,0),
	EX_ID("sprintf",	SPRINTF,	SPRINTF,	STRING,	0),
	EX_ID("srand",	SRAND,		SRAND,		INTEGER,0),
	EX_ID("static",	STATIC,		STATIC,		0,	0),
	EX_ID("sub",	SUB,		SUB,		STRING,	0),
	EX_ID("substr",	SUBSTR,		SUBSTR,		STRING,	0),
	EX_ID("switch",	SWITCH,		SWITCH,		0,	0),
	EX_ID("tokens",	TOKENS,		TOKENS,		INTEGER,0),
	EX_ID("unset",	UNSET,		UNSET,		0,	0),
	EX_ID("unsigned",DECLARE,	UNSIGNED,	UNSIGNED,0),
	EX_ID("void",	DECLARE,	VOIDTYPE,	0,	0),
	EX_ID("while",	WHILE,		WHILE,		0,	0),
	EX_ID("while",	WHILE,		WHILE,		0,	0),
	EX_ID({0},		0,		0,		0,	0)

};
