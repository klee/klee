#ifndef _EXPARSE_H
#define _EXPARSE_H
/* A Bison parser, made by GNU Bison 2.6.1.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef EX_Y_TAB_H
# define EX_Y_TAB_H
/* Enabling traces.  */
#ifndef EXDEBUG
# define EXDEBUG 1
#endif
#if EXDEBUG
extern int exdebug;
#endif

/* Tokens.  */
#ifndef EXTOKENTYPE
# define EXTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum extokentype {
     MINTOKEN = 258,
     INTEGER = 259,
     UNSIGNED = 260,
     CHARACTER = 261,
     FLOATING = 262,
     STRING = 263,
     VOIDTYPE = 264,
     STATIC = 265,
     ADDRESS = 266,
     ARRAY = 267,
     BREAK = 268,
     CALL = 269,
     CASE = 270,
     CONSTANT = 271,
     CONTINUE = 272,
     DECLARE = 273,
     DEFAULT = 274,
     DYNAMIC = 275,
     ELSE = 276,
     EXIT = 277,
     FOR = 278,
     FUNCTION = 279,
     GSUB = 280,
     ITERATE = 281,
     ITERATER = 282,
     ID = 283,
     IF = 284,
     LABEL = 285,
     MEMBER = 286,
     NAME = 287,
     POS = 288,
     PRAGMA = 289,
     PRE = 290,
     PRINT = 291,
     PRINTF = 292,
     PROCEDURE = 293,
     QUERY = 294,
     RAND = 295,
     RETURN = 296,
     SCANF = 297,
     SPLIT = 298,
     SPRINTF = 299,
     SRAND = 300,
     SSCANF = 301,
     SUB = 302,
     SUBSTR = 303,
     SWITCH = 304,
     TOKENS = 305,
     UNSET = 306,
     WHILE = 307,
     F2I = 308,
     F2S = 309,
     I2F = 310,
     I2S = 311,
     S2B = 312,
     S2F = 313,
     S2I = 314,
     F2X = 315,
     I2X = 316,
     S2X = 317,
     X2F = 318,
     X2I = 319,
     X2S = 320,
     X2X = 321,
     XPRINT = 322,
     OR = 323,
     AND = 324,
     NE = 325,
     EQ = 326,
     GE = 327,
     LE = 328,
     RS = 329,
     LS = 330,
     IN_OP = 331,
     UNARY = 332,
     DEC = 333,
     INC = 334,
     CAST = 335,
     MAXTOKEN = 336
   };
#endif
/* Tokens.  */
#define MINTOKEN 258
#define INTEGER 259
#define UNSIGNED 260
#define CHARACTER 261
#define FLOATING 262
#define STRING 263
#define VOIDTYPE 264
#define STATIC 265
#define ADDRESS 266
#define ARRAY 267
#define BREAK 268
#define CALL 269
#define CASE 270
#define CONSTANT 271
#define CONTINUE 272
#define DECLARE 273
#define DEFAULT 274
#define DYNAMIC 275
#define ELSE 276
#define EXIT 277
#define FOR 278
#define FUNCTION 279
#define GSUB 280
#define ITERATE 281
#define ITERATER 282
#define ID 283
#define IF 284
#define LABEL 285
#define MEMBER 286
#define NAME 287
#define POS 288
#define PRAGMA 289
#define PRE 290
#define PRINT 291
#define PRINTF 292
#define PROCEDURE 293
#define QUERY 294
#define RAND 295
#define RETURN 296
#define SCANF 297
#define SPLIT 298
#define SPRINTF 299
#define SRAND 300
#define SSCANF 301
#define SUB 302
#define SUBSTR 303
#define SWITCH 304
#define TOKENS 305
#define UNSET 306
#define WHILE 307
#define F2I 308
#define F2S 309
#define I2F 310
#define I2S 311
#define S2B 312
#define S2F 313
#define S2I 314
#define F2X 315
#define I2X 316
#define S2X 317
#define X2F 318
#define X2I 319
#define X2S 320
#define X2X 321
#define XPRINT 322
#define OR 323
#define AND 324
#define NE 325
#define EQ 326
#define GE 327
#define LE 328
#define RS 329
#define LS 330
#define IN_OP 331
#define UNARY 332
#define DEC 333
#define INC 334
#define CAST 335
#define MAXTOKEN 336



#if ! defined EXSTYPE && ! defined EXSTYPE_IS_DECLARED
typedef union EXSTYPE
{
/* Line 2049 of yacc.c  */
#line 39 "../../lib/expr/exparse.y"

	struct Exnode_s*expr;
	double		floating;
	struct Exref_s*	reference;
	struct Exid_s*	id;
	Sflong_t	integer;
	int		op;
	char*		string;
	void*		user;
	struct Exbuf_s*	buffer;


/* Line 2049 of yacc.c  */
#line 232 "y.tab.h"
} EXSTYPE;
# define EXSTYPE_IS_TRIVIAL 1
# define exstype EXSTYPE /* obsolescent; will be withdrawn */
# define EXSTYPE_IS_DECLARED 1
#endif

extern EXSTYPE exlval;

#ifdef EXPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int exparse (void *EXPARSE_PARAM);
#else
int exparse ();
#endif
#else /* ! EXPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int exparse (void);
#else
int exparse ();
#endif
#endif /* ! EXPARSE_PARAM */

#endif /* !EX_Y_TAB_H  */
#endif /* _EXPARSE_H */
