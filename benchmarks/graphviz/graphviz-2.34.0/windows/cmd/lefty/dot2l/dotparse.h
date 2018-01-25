/* A Bison parser, made by GNU Bison 2.0.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_graph = 258,
     T_digraph = 259,
     T_strict = 260,
     T_node = 261,
     T_edge = 262,
     T_edgeop = 263,
     T_id = 264,
     T_subgraph = 265
   };
#endif
#define T_graph 258
#define T_digraph 259
#define T_strict 260
#define T_node 261
#define T_edge 262
#define T_edgeop 263
#define T_id 264
#define T_subgraph 265




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 37 "../../../cmd/lefty/dot2l/dotparse.y"
typedef union YYSTYPE {
    long i;
    char *s;
    void *o;
} YYSTYPE;
/* Line 1318 of yacc.c.  */
#line 63 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



