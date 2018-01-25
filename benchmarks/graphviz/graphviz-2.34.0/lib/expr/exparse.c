/* A Bison parser, made by GNU Bison 2.6.1.  */

/* Bison implementation for Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with ex or EX, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define EXBISON 1

/* Bison version.  */
#define EXBISON_VERSION "2.6.1"

/* Skeleton name.  */
#define EXSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define EXPURE 0

/* Push parsers.  */
#define EXPUSH 0

/* Pull parsers.  */
#define EXPULL 1




/* Copy the first part of user declarations.  */
/* Line 336 of yacc.c  */
#line 14 "../../lib/expr/exparse.y"


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


/* Line 336 of yacc.c  */
#line 92 "y.tab.c"

# ifndef EX_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define EX_NULL nullptr
#  else
#   define EX_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef EXERROR_VERBOSE
# undef EXERROR_VERBOSE
# define EXERROR_VERBOSE 1
#else
# define EXERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
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
/* Line 350 of yacc.c  */
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


/* Line 350 of yacc.c  */
#line 310 "y.tab.c"
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

/* Copy the second part of user declarations.  */
/* Line 353 of yacc.c  */
#line 166 "../../lib/expr/exparse.y"


#include "exgram.h"


/* Line 353 of yacc.c  */
#line 344 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef EXTYPE_UINT8
typedef EXTYPE_UINT8 extype_uint8;
#else
typedef unsigned char extype_uint8;
#endif

#ifdef EXTYPE_INT8
typedef EXTYPE_INT8 extype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char extype_int8;
#else
typedef short int extype_int8;
#endif

#ifdef EXTYPE_UINT16
typedef EXTYPE_UINT16 extype_uint16;
#else
typedef unsigned short int extype_uint16;
#endif

#ifdef EXTYPE_INT16
typedef EXTYPE_INT16 extype_int16;
#else
typedef short int extype_int16;
#endif

#ifndef EXSIZE_T
# ifdef __SIZE_TYPE__
#  define EXSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define EXSIZE_T size_t
# elif ! defined EXSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define EXSIZE_T size_t
# else
#  define EXSIZE_T unsigned int
# endif
#endif

#define EXSIZE_MAXIMUM ((EXSIZE_T) -1)

#ifndef EX_
# if defined EXENABLE_NLS && EXENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define EX_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef EX_
#  define EX_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define EXUSE(e) ((void) (e))
#else
# define EXUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define EXID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
EXID (int exi)
#else
static int
EXID (exi)
    int exi;
#endif
{
  return exi;
}
#endif

#if ! defined exoverflow || EXERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef EXSTACK_USE_ALLOCA
#  if EXSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define EXSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define EXSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define EXSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef EXSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define EXSTACK_FREE(Ptr) do { /* empty */; } while (EXID (0))
#  ifndef EXSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define EXSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define EXSTACK_ALLOC EXMALLOC
#  define EXSTACK_FREE EXFREE
#  ifndef EXSTACK_ALLOC_MAXIMUM
#   define EXSTACK_ALLOC_MAXIMUM EXSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined EXMALLOC || defined malloc) \
	     && (defined EXFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef EXMALLOC
#   define EXMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (EXSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef EXFREE
#   define EXFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined exoverflow || EXERROR_VERBOSE */


#if (! defined exoverflow \
     && (! defined __cplusplus \
	 || (defined EXSTYPE_IS_TRIVIAL && EXSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union exalloc
{
  extype_int16 exss_alloc;
  EXSTYPE exvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define EXSTACK_GAP_MAXIMUM (sizeof (union exalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define EXSTACK_BYTES(N) \
     ((N) * (sizeof (extype_int16) + sizeof (EXSTYPE)) \
      + EXSTACK_GAP_MAXIMUM)

# define EXCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables EXSIZE and EXSTACKSIZE give the old and new number of
   elements in the stack, and EXPTR gives the new location of the
   stack.  Advance EXPTR to a properly aligned location for the next
   stack.  */
# define EXSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	EXSIZE_T exnewbytes;						\
	EXCOPY (&exptr->Stack_alloc, Stack, exsize);			\
	Stack = &exptr->Stack_alloc;					\
	exnewbytes = exstacksize * sizeof (*Stack) + EXSTACK_GAP_MAXIMUM; \
	exptr += exnewbytes / sizeof (*exptr);				\
      }									\
    while (EXID (0))

#endif

#if defined EXCOPY_NEEDED && EXCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef EXCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define EXCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define EXCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          EXSIZE_T exi;                         \
          for (exi = 0; exi < (Count); exi++)   \
            (Dst)[exi] = (Src)[exi];            \
        }                                       \
      while (EXID (0))
#  endif
# endif
#endif /* !EXCOPY_NEEDED */

/* EXFINAL -- State number of the termination state.  */
#define EXFINAL  3
/* EXLAST -- Last index in EXTABLE.  */
#define EXLAST   1112

/* EXNTOKENS -- Number of terminals.  */
#define EXNTOKENS  107
/* EXNNTS -- Number of nonterminals.  */
#define EXNNTS  44
/* EXNRULES -- Number of rules.  */
#define EXNRULES  142
/* EXNRULES -- Number of states.  */
#define EXNSTATES  286

/* EXTRANSLATE(EXLEX) -- Bison symbol number corresponding to EXLEX.  */
#define EXUNDEFTOK  2
#define EXMAXUTOK   336

#define EXTRANSLATE(EXX)						\
  ((unsigned int) (EXX) <= EXMAXUTOK ? extranslate[EXX] : EXUNDEFTOK)

/* EXTRANSLATE[EXLEX] -- Bison symbol number corresponding to EXLEX.  */
static const extype_uint8 extranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    91,     2,    93,     2,    90,    76,     2,
      98,   103,    88,    85,    68,    86,   106,    89,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    71,   102,
      79,    69,    80,    70,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   104,     2,   105,    75,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   100,    74,   101,    92,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    72,    73,    77,    78,    81,    82,    83,
      84,    87,    94,    95,    96,    97,    99
};

#if EXDEBUG
/* EXPRHS[EXN] -- Index of the first RHS symbol of rule number EXN in
   EXRHS.  */
static const extype_uint16 exprhs[] =
{
       0,     0,     3,     6,     7,    10,    11,    16,    17,    20,
      24,    27,    28,    29,    36,    43,    49,    59,    65,    70,
      77,    83,    84,    93,    97,   101,   105,   106,   109,   112,
     114,   117,   121,   124,   125,   127,   129,   133,   134,   139,
     141,   143,   145,   147,   149,   151,   152,   155,   156,   158,
     162,   167,   171,   175,   179,   183,   187,   191,   195,   199,
     203,   207,   211,   215,   219,   223,   227,   231,   235,   239,
     243,   244,   245,   253,   256,   259,   262,   265,   268,   271,
     276,   281,   286,   291,   296,   303,   312,   317,   321,   325,
     330,   335,   340,   345,   350,   353,   356,   359,   363,   366,
     369,   371,   373,   375,   377,   379,   381,   383,   385,   387,
     389,   391,   393,   395,   398,   402,   404,   405,   408,   412,
     413,   417,   418,   420,   422,   426,   427,   429,   431,   433,
     437,   438,   442,   443,   445,   449,   452,   455,   456,   459,
     461,   462,   463
};

/* EXRHS -- A `-1'-separated list of the rules' RHS.  */
static const extype_int16 exrhs[] =
{
     108,     0,    -1,   112,   109,    -1,    -1,   109,   110,    -1,
      -1,    30,    71,   111,   112,    -1,    -1,   112,   113,    -1,
     100,   112,   101,    -1,   128,   102,    -1,    -1,    -1,   121,
     114,    18,   115,   122,   102,    -1,    29,    98,   129,   103,
     113,   127,    -1,    23,    98,   136,   103,   113,    -1,    23,
      98,   128,   102,   128,   102,   128,   103,   113,    -1,    27,
      98,   136,   103,   113,    -1,    51,    98,    20,   103,    -1,
      51,    98,    20,    68,   129,   103,    -1,    52,    98,   129,
     103,   113,    -1,    -1,    49,    98,   129,   116,   103,   100,
     117,   101,    -1,    13,   128,   102,    -1,    17,   128,   102,
      -1,    41,   128,   102,    -1,    -1,   117,   118,    -1,   119,
     112,    -1,   120,    -1,   119,   120,    -1,    15,   133,    71,
      -1,    19,    71,    -1,    -1,    10,    -1,   123,    -1,   122,
      68,   123,    -1,    -1,   125,   124,   137,   148,    -1,    32,
      -1,    20,    -1,    28,    -1,    24,    -1,    32,    -1,    20,
      -1,    -1,    21,   113,    -1,    -1,   129,    -1,    98,   129,
     103,    -1,    98,    18,   103,   129,    -1,   129,    79,   129,
      -1,   129,    86,   129,    -1,   129,    88,   129,    -1,   129,
      89,   129,    -1,   129,    90,   129,    -1,   129,    84,   129,
      -1,   129,    83,   129,    -1,   129,    80,   129,    -1,   129,
      82,   129,    -1,   129,    81,   129,    -1,   129,    78,   129,
      -1,   129,    77,   129,    -1,   129,    76,   129,    -1,   129,
      74,   129,    -1,   129,    75,   129,    -1,   129,    85,   129,
      -1,   129,    73,   129,    -1,   129,    72,   129,    -1,   129,
      68,   129,    -1,    -1,    -1,   129,    70,   130,   129,    71,
     131,   129,    -1,    91,   129,    -1,    93,    20,    -1,    92,
     129,    -1,    86,   129,    -1,    85,   129,    -1,    76,   136,
      -1,    12,   104,   139,   105,    -1,    24,    98,   139,   103,
      -1,    25,    98,   139,   103,    -1,    47,    98,   139,   103,
      -1,    48,    98,   139,   103,    -1,   132,    98,   129,    68,
      20,   103,    -1,   132,    98,   129,    68,    20,    68,   129,
     103,    -1,    22,    98,   129,   103,    -1,    40,    98,   103,
      -1,    45,    98,   103,    -1,    45,    98,   129,   103,    -1,
      38,    98,   139,   103,    -1,    36,    98,   139,   103,    -1,
     134,    98,   139,   103,    -1,   135,    98,   139,   103,    -1,
     136,   147,    -1,    96,   136,    -1,   136,    96,    -1,   129,
      87,    20,    -1,    95,   136,    -1,   136,    95,    -1,   133,
      -1,    43,    -1,    50,    -1,    16,    -1,     7,    -1,     4,
      -1,     8,    -1,     5,    -1,    37,    -1,    39,    -1,    44,
      -1,    42,    -1,    46,    -1,    28,   145,    -1,    20,   138,
     145,    -1,    32,    -1,    -1,   104,   105,    -1,   104,    18,
     105,    -1,    -1,   104,   129,   105,    -1,    -1,   140,    -1,
     129,    -1,   140,    68,   129,    -1,    -1,    18,    -1,   142,
      -1,   143,    -1,   142,    68,   143,    -1,    -1,    18,   144,
     126,    -1,    -1,   146,    -1,   106,    28,   146,    -1,   106,
      28,    -1,   106,    32,    -1,    -1,    69,   129,    -1,   147,
      -1,    -1,    -1,    98,   149,   141,   150,   103,   100,   112,
     101,    -1
};

/* EXRLINE[EXN] -- source line where rule number EXN was defined.  */
static const extype_uint16 exrline[] =
{
       0,   174,   174,   195,   196,   199,   199,   239,   242,   269,
     273,   277,   277,   277,   282,   292,   305,   320,   333,   341,
     352,   362,   362,   374,   386,   390,   403,   433,   436,   468,
     469,   472,   493,   500,   503,   509,   510,   517,   517,   573,
     574,   575,   576,   579,   580,   584,   587,   594,   597,   600,
     604,   608,   653,   657,   661,   665,   669,   673,   677,   681,
     685,   689,   693,   697,   701,   705,   709,   713,   726,   730,
     740,   740,   740,   781,   801,   808,   812,   816,   820,   824,
     828,   838,   842,   846,   850,   854,   858,   864,   868,   872,
     878,   883,   887,   912,   948,   972,   980,   988,   999,  1003,
    1007,  1010,  1011,  1013,  1021,  1026,  1031,  1036,  1043,  1044,
    1045,  1048,  1049,  1052,  1056,  1076,  1089,  1092,  1096,  1110,
    1113,  1120,  1123,  1131,  1136,  1143,  1146,  1152,  1155,  1159,
    1170,  1170,  1183,  1186,  1198,  1217,  1221,  1227,  1230,  1237,
    1238,  1255,  1238
};
#endif

#if EXDEBUG || EXERROR_VERBOSE || 0
/* EXTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at EXNTOKENS, nonterminals.  */
static const char *const extname[] =
{
  "$end", "error", "$undefined", "MINTOKEN", "INTEGER", "UNSIGNED",
  "CHARACTER", "FLOATING", "STRING", "VOIDTYPE", "STATIC", "ADDRESS",
  "ARRAY", "BREAK", "CALL", "CASE", "CONSTANT", "CONTINUE", "DECLARE",
  "DEFAULT", "DYNAMIC", "ELSE", "EXIT", "FOR", "FUNCTION", "GSUB",
  "ITERATE", "ITERATER", "ID", "IF", "LABEL", "MEMBER", "NAME", "POS",
  "PRAGMA", "PRE", "PRINT", "PRINTF", "PROCEDURE", "QUERY", "RAND",
  "RETURN", "SCANF", "SPLIT", "SPRINTF", "SRAND", "SSCANF", "SUB",
  "SUBSTR", "SWITCH", "TOKENS", "UNSET", "WHILE", "F2I", "F2S", "I2F",
  "I2S", "S2B", "S2F", "S2I", "F2X", "I2X", "S2X", "X2F", "X2I", "X2S",
  "X2X", "XPRINT", "','", "'='", "'?'", "':'", "OR", "AND", "'|'", "'^'",
  "'&'", "NE", "EQ", "'<'", "'>'", "GE", "LE", "RS", "LS", "'+'", "'-'",
  "IN_OP", "'*'", "'/'", "'%'", "'!'", "'~'", "'#'", "UNARY", "DEC", "INC",
  "CAST", "'('", "MAXTOKEN", "'{'", "'}'", "';'", "')'", "'['", "']'",
  "'.'", "$accept", "program", "action_list", "action", "$@1",
  "statement_list", "statement", "$@2", "$@3", "$@4", "switch_list",
  "switch_item", "case_list", "case_item", "static", "dcl_list",
  "dcl_item", "$@5", "dcl_name", "name", "else_opt", "expr_opt", "expr",
  "$@6", "$@7", "splitop", "constant", "print", "scan", "variable",
  "array", "index", "args", "arg_list", "formals", "formal_list",
  "formal_item", "$@8", "members", "member", "assign", "initialize", "$@9",
  "$@10", EX_NULL
};
#endif

# ifdef EXPRINT
/* EXTOKNUM[EXLEX-NUM] -- Internal token number corresponding to
   token EXLEX-NUM.  */
static const extype_uint16 extoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,    44,    61,
      63,    58,   323,   324,   124,    94,    38,   325,   326,    60,
      62,   327,   328,   329,   330,    43,    45,   331,    42,    47,
      37,    33,   126,    35,   332,   333,   334,   335,    40,   336,
     123,   125,    59,    41,    91,    93,    46
};
# endif

/* EXR1[EXN] -- Symbol number of symbol that rule EXN derives.  */
static const extype_uint8 exr1[] =
{
       0,   107,   108,   109,   109,   111,   110,   112,   112,   113,
     113,   114,   115,   113,   113,   113,   113,   113,   113,   113,
     113,   116,   113,   113,   113,   113,   117,   117,   118,   119,
     119,   120,   120,   121,   121,   122,   122,   124,   123,   125,
     125,   125,   125,   126,   126,   127,   127,   128,   128,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     130,   131,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   132,   132,   133,   133,   133,   133,   133,   134,   134,
     134,   135,   135,   136,   136,   136,   137,   137,   137,   138,
     138,   139,   139,   140,   140,   141,   141,   141,   142,   142,
     144,   143,   145,   145,   145,   146,   146,   147,   147,   148,
     149,   150,   148
};

/* EXR2[EXN] -- Number of symbols composing right hand side of rule EXN.  */
static const extype_uint8 exr2[] =
{
       0,     2,     2,     0,     2,     0,     4,     0,     2,     3,
       2,     0,     0,     6,     6,     5,     9,     5,     4,     6,
       5,     0,     8,     3,     3,     3,     0,     2,     2,     1,
       2,     3,     2,     0,     1,     1,     3,     0,     4,     1,
       1,     1,     1,     1,     1,     0,     2,     0,     1,     3,
       4,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       0,     0,     7,     2,     2,     2,     2,     2,     2,     4,
       4,     4,     4,     4,     6,     8,     4,     3,     3,     4,
       4,     4,     4,     4,     2,     2,     2,     3,     2,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     1,     0,     2,     3,     0,
       3,     0,     1,     1,     3,     0,     1,     1,     1,     3,
       0,     3,     0,     1,     3,     2,     2,     0,     2,     1,
       0,     0,     8
};

/* EXDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when EXTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const extype_uint8 exdefact[] =
{
       7,     0,     3,     1,   105,   107,   104,   106,    34,     0,
      47,   103,    47,   119,     0,     0,     0,     0,     0,   132,
       0,   115,     0,   108,     0,   109,     0,    47,   111,   101,
     110,     0,   112,     0,     0,     0,   102,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     7,     2,
       8,    11,     0,    48,     0,   100,     0,     0,   137,   121,
       0,     0,     0,   132,     0,    47,   121,   121,     0,     0,
     113,   133,     0,   121,   121,     0,     0,     0,   121,   121,
       0,     0,     0,    78,    77,    76,    73,    75,    74,    98,
      95,     0,     0,    33,     0,     4,     0,    10,     0,    70,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     121,   121,     0,    99,    96,    94,   123,     0,   122,    23,
      24,     0,   114,     0,     0,   137,     0,     0,     0,   135,
     136,     0,     0,     0,    87,    25,    88,     0,     0,     0,
      21,     0,     0,     0,    49,     9,     5,    12,    69,     0,
      68,    67,    64,    65,    63,    62,    61,    51,    58,    60,
      59,    57,    56,    66,    52,    97,    53,    54,    55,     0,
       0,     0,   138,    79,     0,   120,    86,    47,    33,    80,
      81,    33,     0,   134,    33,    91,    90,    89,    82,    83,
       0,     0,    18,    33,    50,     7,     0,     0,     0,    92,
      93,   124,     0,    15,    17,   135,    45,     0,     0,    20,
       6,    40,    42,    41,    39,     0,    35,    37,    71,   119,
      47,    33,    14,    26,    19,     0,    13,   116,     0,     0,
      84,     0,    46,     0,    36,     0,   137,    72,     0,    33,
       0,     0,    22,    27,     7,    29,     0,   117,   140,   139,
      38,    85,    16,     0,    32,    28,    30,   118,   125,    31,
     130,   141,   127,   128,     0,     0,     0,    44,    43,   131,
       0,   130,   129,     7,    33,   142
};

/* EXDEFGOTO[NTERM-NUM].  */
static const extype_int16 exdefgoto[] =
{
      -1,     1,    49,    95,   205,     2,    50,    96,   206,   200,
     243,   253,   254,   255,    51,   225,   226,   237,   227,   279,
     232,    52,    53,   159,   238,    54,    55,    56,    57,    58,
     246,    63,   127,   128,   271,   272,   273,   274,    70,    71,
     125,   260,   268,   275
};

/* EXPACT[STATE-NUM] -- Index in EXTABLE of the portion describing
   STATE-NUM.  */
#define EXPACT_NINF -144
static const extype_int16 expact[] =
{
    -144,     9,   200,  -144,  -144,  -144,  -144,  -144,  -144,   -89,
     691,  -144,   691,   -80,   -71,   -64,   -63,   -43,   -35,   -27,
     -11,  -144,    11,  -144,    16,  -144,    20,   691,  -144,  -144,
    -144,    23,  -144,    31,    39,    41,  -144,    56,    57,     1,
     691,   691,   691,   691,    79,     1,     1,   596,  -144,    92,
    -144,  -144,    49,   872,    58,  -144,    60,    61,   -37,   691,
      65,    66,   691,   -27,   691,   691,   691,   691,     1,   -12,
    -144,  -144,   691,   691,   691,    59,    68,    88,   691,   691,
     691,   140,   691,  -144,  -144,  -144,  -144,  -144,  -144,  -144,
    -144,    72,   284,   299,   100,  -144,   158,  -144,   691,  -144,
     691,   691,   691,   691,   691,   691,   691,   691,   691,   691,
     691,   691,   691,   691,   691,   143,   691,   691,   691,   691,
     691,   691,   691,  -144,  -144,  -144,   916,    77,   109,  -144,
    -144,   185,  -144,   383,    83,   -56,    84,    85,    95,    93,
    -144,   482,    97,    98,  -144,  -144,  -144,   577,   106,   108,
     872,   -51,   672,   691,  -144,  -144,  -144,  -144,   916,   691,
     934,   951,   967,   982,   996,  1010,  1010,  1022,  1022,  1022,
    1022,   107,   107,    53,    53,  -144,  -144,  -144,  -144,   895,
     111,   112,   916,  -144,   691,  -144,  -144,   691,   497,  -144,
    -144,   497,    29,  -144,   497,  -144,  -144,  -144,  -144,  -144,
     116,   691,  -144,   497,  -144,  -144,    87,   849,   786,  -144,
    -144,   916,   124,  -144,  -144,  -144,   168,    90,   767,  -144,
     200,  -144,  -144,  -144,  -144,   -49,  -144,  -144,  -144,   -54,
     691,   497,  -144,  -144,  -144,    87,  -144,   126,   691,   691,
    -144,   128,  -144,    -7,  -144,   -16,   -44,   916,   815,   497,
     145,   162,  -144,  -144,    86,  -144,   129,  -144,  -144,  -144,
    -144,  -144,  -144,   183,  -144,   200,  -144,  -144,   238,  -144,
     174,  -144,   210,  -144,   -10,   176,   262,  -144,  -144,  -144,
     181,  -144,  -144,  -144,   398,  -144
};

/* EXPGOTO[NTERM-NUM].  */
static const extype_int16 expgoto[] =
{
    -144,  -144,  -144,  -144,  -144,   -48,  -143,  -144,  -144,  -144,
    -144,  -144,  -144,    28,  -144,  -144,    48,  -144,  -144,  -144,
    -144,    -9,   -36,  -144,  -144,  -144,    34,  -144,  -144,   101,
    -144,  -144,    24,  -144,  -144,  -144,    12,  -144,   224,   150,
      51,  -144,  -144,  -144
};

/* EXTABLE[EXPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If EXTABLE_NINF, syntax error.  */
#define EXTABLE_NINF -127
static const extype_int16 extable[] =
{
      93,    60,   256,    61,    84,    85,    86,    87,   250,     3,
     277,    92,   251,   122,   239,    59,   139,   201,    76,   235,
     140,    13,   278,   126,    62,   122,   131,    64,   133,    19,
     126,   126,   122,    21,    65,    66,   141,   126,   126,   123,
     124,   147,   126,   126,   150,   213,   152,   188,   214,   240,
      62,   216,   202,   236,   258,    67,   134,   215,   123,   124,
     219,   140,   158,    68,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,    69,
     176,   177,   178,   179,   126,   126,   182,    72,   242,   257,
     136,   137,     4,     5,   252,     6,     7,   142,   143,    88,
       9,   250,   148,   149,    11,   251,   262,   221,    13,    73,
      14,   222,    16,    17,    74,   223,    19,   204,    75,   224,
      21,    77,    94,   207,    22,    23,    24,    25,    26,    78,
      28,    29,    30,    31,    32,    33,    34,    79,    36,    80,
      83,   116,   117,   118,   180,   181,    89,    90,   211,     4,
       5,    97,     6,     7,    81,    82,   119,   220,   120,   121,
     151,    11,   144,   175,    39,   218,   135,   129,   130,   138,
     145,   156,   158,    40,    41,   153,   157,   184,   212,    42,
      43,    44,   183,    45,    46,   187,    47,   189,   190,   231,
     233,   146,   113,   114,   115,   116,   117,   118,   191,   192,
     195,   196,   247,   248,     4,     5,   265,     6,     7,   198,
       8,   199,     9,    10,   209,   210,    11,    12,   -33,   217,
      13,   241,    14,    15,    16,    17,   230,    18,    19,    20,
     245,   249,    21,   264,   267,   284,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    98,   269,    99,   270,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,    39,  -126,   276,   280,
     281,   283,   266,   244,   263,    40,    41,   132,   282,   193,
     185,    42,    43,    44,     0,    45,    46,   259,    47,     0,
      48,     0,   -47,     4,     5,     0,     6,     7,     0,     8,
       0,     9,    10,     0,     0,    11,    12,     0,     0,    13,
       0,    14,    15,    16,    17,     0,    18,    19,    20,     0,
       0,    21,     0,     0,     0,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    98,     0,    99,     0,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,    39,     0,     0,     0,     0,
       0,     0,     0,     0,    40,    41,     0,   154,     0,     0,
      42,    43,    44,     0,    45,    46,     0,    47,     0,    48,
     155,   -47,     4,     5,     0,     6,     7,     0,     8,     0,
       9,    10,     0,     0,    11,    12,     0,     0,    13,     0,
      14,    15,    16,    17,     0,    18,    19,    20,     0,     0,
      21,     0,     0,     0,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    98,     0,    99,     0,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,    39,     0,     0,     0,     0,     0,
       0,     0,     0,    40,    41,     0,   186,     0,     0,    42,
      43,    44,     0,    45,    46,     0,    47,     0,    48,   285,
     -47,     4,     5,     0,     6,     7,     0,     8,     0,     9,
      10,     0,     0,    11,    12,     0,     0,    13,     0,    14,
      15,    16,    17,     0,    18,    19,    20,     0,     0,    21,
       0,     0,     0,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      98,     0,    99,     0,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,    39,     0,     0,     0,     0,     0,     0,
       0,     0,    40,    41,     0,   194,     0,     0,    42,    43,
      44,     0,    45,    46,     0,    47,     0,    48,     0,   -47,
       4,     5,     0,     6,     7,     0,     0,     0,     9,     0,
       0,     0,    11,     0,    91,     0,    13,     0,    14,     0,
      16,    17,     0,     0,    19,     0,     0,     0,    21,     0,
       0,     0,    22,    23,    24,    25,    26,     0,    28,    29,
      30,    31,    32,    33,    34,    98,    36,    99,     0,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,     0,     0,
       0,     0,    39,     0,     0,     0,     0,     0,     0,     0,
     197,    40,    41,     0,     0,     0,     0,    42,    43,    44,
       0,    45,    46,     0,    47,     4,     5,     0,     6,     7,
       0,     0,     0,     9,     0,     0,     0,    11,     0,     0,
       0,    13,     0,    14,     0,    16,    17,     0,     0,    19,
       0,     0,     0,    21,     0,     0,     0,    22,    23,    24,
      25,    26,     0,    28,    29,    30,    31,    32,    33,    34,
      98,    36,    99,     0,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,     0,     0,     0,     0,    39,     0,     0,
       0,     0,     0,     0,     0,   203,    40,    41,     0,     0,
       0,     0,    42,    43,    44,     0,    45,    46,     0,    47,
       4,     5,     0,     6,     7,     0,     0,     0,     9,     0,
       0,     0,    11,     0,     0,     0,   229,     0,    14,     0,
      16,    17,     0,     0,    19,     0,     0,     0,    21,     0,
       0,     0,    22,    23,    24,    25,    26,     0,    28,    29,
      30,    31,    32,    33,    34,    98,    36,    99,     0,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,     0,     0,
       0,     0,    39,     0,     0,     0,     0,     0,     0,     0,
     234,    40,    41,     0,     0,     0,     0,    42,    43,    44,
       0,    45,    46,    98,    47,    99,     0,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    98,   261,    99,
     228,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
      98,     0,    99,     0,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   208,     0,    99,     0,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,    99,     0,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,  -127,  -127,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,  -127,  -127,  -127,  -127,   111,   112,   113,   114,   115,
     116,   117,   118
};

#define expact_value_is_default(exstate) \
  ((exstate) == (-144))

#define extable_value_is_error(extable_value) \
  ((extable_value) == (-127))

static const extype_int16 excheck[] =
{
      48,    10,    18,    12,    40,    41,    42,    43,    15,     0,
      20,    47,    19,    69,    68,   104,    28,    68,    27,    68,
      32,    20,    32,    59,   104,    69,    62,    98,    64,    28,
      66,    67,    69,    32,    98,    98,    72,    73,    74,    95,
      96,    77,    78,    79,    80,   188,    82,   103,   191,   103,
     104,   194,   103,   102,    98,    98,    65,    28,    95,    96,
     203,    32,    98,    98,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   106,
     116,   117,   118,   119,   120,   121,   122,    98,   231,   105,
      66,    67,     4,     5,   101,     7,     8,    73,    74,    20,
      12,    15,    78,    79,    16,    19,   249,    20,    20,    98,
      22,    24,    24,    25,    98,    28,    28,   153,    98,    32,
      32,    98,    30,   159,    36,    37,    38,    39,    40,    98,
      42,    43,    44,    45,    46,    47,    48,    98,    50,    98,
      39,    88,    89,    90,   120,   121,    45,    46,   184,     4,
       5,   102,     7,     8,    98,    98,    98,   205,    98,    98,
      20,    16,   103,    20,    76,   201,    65,   102,   102,    68,
     102,    71,   208,    85,    86,   103,    18,    68,   187,    91,
      92,    93,   105,    95,    96,   102,    98,   103,   103,    21,
     100,   103,    85,    86,    87,    88,    89,    90,   103,   106,
     103,   103,   238,   239,     4,     5,   254,     7,     8,   103,
      10,   103,    12,    13,   103,   103,    16,    17,    18,   103,
      20,   230,    22,    23,    24,    25,   102,    27,    28,    29,
     104,   103,    32,    71,   105,   283,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    68,    71,    70,    18,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    76,   103,    68,   103,
      18,   100,   254,   235,   250,    85,    86,    63,   276,   139,
     105,    91,    92,    93,    -1,    95,    96,   246,    98,    -1,
     100,    -1,   102,     4,     5,    -1,     7,     8,    -1,    10,
      -1,    12,    13,    -1,    -1,    16,    17,    -1,    -1,    20,
      -1,    22,    23,    24,    25,    -1,    27,    28,    29,    -1,
      -1,    32,    -1,    -1,    -1,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    68,    -1,    70,    -1,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    76,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    85,    86,    -1,   103,    -1,    -1,
      91,    92,    93,    -1,    95,    96,    -1,    98,    -1,   100,
     101,   102,     4,     5,    -1,     7,     8,    -1,    10,    -1,
      12,    13,    -1,    -1,    16,    17,    -1,    -1,    20,    -1,
      22,    23,    24,    25,    -1,    27,    28,    29,    -1,    -1,
      32,    -1,    -1,    -1,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    68,    -1,    70,    -1,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    76,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    85,    86,    -1,   103,    -1,    -1,    91,
      92,    93,    -1,    95,    96,    -1,    98,    -1,   100,   101,
     102,     4,     5,    -1,     7,     8,    -1,    10,    -1,    12,
      13,    -1,    -1,    16,    17,    -1,    -1,    20,    -1,    22,
      23,    24,    25,    -1,    27,    28,    29,    -1,    -1,    32,
      -1,    -1,    -1,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      68,    -1,    70,    -1,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    76,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    85,    86,    -1,   103,    -1,    -1,    91,    92,
      93,    -1,    95,    96,    -1,    98,    -1,   100,    -1,   102,
       4,     5,    -1,     7,     8,    -1,    -1,    -1,    12,    -1,
      -1,    -1,    16,    -1,    18,    -1,    20,    -1,    22,    -1,
      24,    25,    -1,    -1,    28,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    36,    37,    38,    39,    40,    -1,    42,    43,
      44,    45,    46,    47,    48,    68,    50,    70,    -1,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    -1,    -1,
      -1,    -1,    76,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     103,    85,    86,    -1,    -1,    -1,    -1,    91,    92,    93,
      -1,    95,    96,    -1,    98,     4,     5,    -1,     7,     8,
      -1,    -1,    -1,    12,    -1,    -1,    -1,    16,    -1,    -1,
      -1,    20,    -1,    22,    -1,    24,    25,    -1,    -1,    28,
      -1,    -1,    -1,    32,    -1,    -1,    -1,    36,    37,    38,
      39,    40,    -1,    42,    43,    44,    45,    46,    47,    48,
      68,    50,    70,    -1,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    -1,    -1,    -1,    -1,    76,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   103,    85,    86,    -1,    -1,
      -1,    -1,    91,    92,    93,    -1,    95,    96,    -1,    98,
       4,     5,    -1,     7,     8,    -1,    -1,    -1,    12,    -1,
      -1,    -1,    16,    -1,    -1,    -1,    20,    -1,    22,    -1,
      24,    25,    -1,    -1,    28,    -1,    -1,    -1,    32,    -1,
      -1,    -1,    36,    37,    38,    39,    40,    -1,    42,    43,
      44,    45,    46,    47,    48,    68,    50,    70,    -1,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    -1,    -1,
      -1,    -1,    76,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     103,    85,    86,    -1,    -1,    -1,    -1,    91,    92,    93,
      -1,    95,    96,    68,    98,    70,    -1,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    68,   103,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      68,    -1,    70,    -1,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    68,    -1,    70,    -1,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    70,    -1,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90
};

/* EXSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const extype_uint8 exstos[] =
{
       0,   108,   112,     0,     4,     5,     7,     8,    10,    12,
      13,    16,    17,    20,    22,    23,    24,    25,    27,    28,
      29,    32,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    76,
      85,    86,    91,    92,    93,    95,    96,    98,   100,   109,
     113,   121,   128,   129,   132,   133,   134,   135,   136,   104,
     128,   128,   104,   138,    98,    98,    98,    98,    98,   106,
     145,   146,    98,    98,    98,    98,   128,    98,    98,    98,
      98,    98,    98,   136,   129,   129,   129,   129,    20,   136,
     136,    18,   129,   112,    30,   110,   114,   102,    68,    70,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    98,
      98,    98,    69,    95,    96,   147,   129,   139,   140,   102,
     102,   129,   145,   129,   128,   136,   139,   139,   136,    28,
      32,   129,   139,   139,   103,   102,   103,   129,   139,   139,
     129,    20,   129,   103,   103,   101,    71,    18,   129,   130,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,    20,   129,   129,   129,   129,
     139,   139,   129,   105,    68,   105,   103,   102,   103,   103,
     103,   103,   106,   146,   103,   103,   103,   103,   103,   103,
     116,    68,   103,   103,   129,   111,   115,   129,    68,   103,
     103,   129,   128,   113,   113,    28,   113,   103,   129,   113,
     112,    20,    24,    28,    32,   122,   123,   125,    71,    20,
     102,    21,   127,   100,   103,    68,   102,   124,   131,    68,
     103,   128,   113,   117,   123,   104,   137,   129,   129,   103,
      15,    19,   101,   118,   119,   120,    18,   105,    98,   147,
     148,   103,   113,   133,    71,   112,   120,   105,   149,    71,
      18,   141,   142,   143,   144,   150,    68,    20,    32,   126,
     103,    18,   143,   100,   112,   101
};

#define exerrok		(exerrstatus = 0)
#define exclearin	(exchar = EXEMPTY)
#define EXEMPTY		(-2)
#define EXEOF		0

#define EXACCEPT	goto exacceptlab
#define EXABORT		goto exabortlab
#define EXERROR		goto exerrorlab


/* Like EXERROR except do call exerror.  This remains here temporarily
   to ease the transition to the new meaning of EXERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   EXFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define EXFAIL		goto exerrlab
#if defined EXFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     EXFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define EXRECOVERING()  (!!exerrstatus)

#define EXBACKUP(Token, Value)                                  \
do                                                              \
  if (exchar == EXEMPTY)                                        \
    {                                                           \
      exchar = (Token);                                         \
      exlval = (Value);                                         \
      EXPOPSTACK (exlen);                                       \
      exstate = *exssp;                                         \
      goto exbackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      exerror (EX_("syntax error: cannot back up")); \
      EXERROR;							\
    }								\
while (EXID (0))


#define EXTERROR	1
#define EXERRCODE	256

/* EXLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef EXLLOC_DEFAULT
# define EXLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (EXID (N))                                                     \
        {                                                               \
          (Current).first_line   = EXRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = EXRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = EXRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = EXRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            EXRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            EXRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (EXID (0))
#endif

#define EXRHSLOC(Rhs, K) ((Rhs)[K])



/* This macro is provided for backward compatibility. */

#ifndef EX_LOCATION_PRINT
# define EX_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* EXLEX -- calling `exlex' with the right arguments.  */

#ifdef EXLEX_PARAM
# define EXLEX exlex (EXLEX_PARAM)
#else
# define EXLEX exlex ()
#endif

/* Enable debugging if requested.  */
#if EXDEBUG

# ifndef EXFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define EXFPRINTF sfprintf
# endif

# define EXDPRINTF(Args)			\
do {						\
  if (exdebug)					\
    EXFPRINTF Args;				\
} while (EXID (0))

# define EX_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (exdebug)								  \
    {									  \
      EXFPRINTF (sfstderr, "%s ", Title);					  \
      ex_symbol_print (sfstderr,						  \
		  Type, Value); \
      EXFPRINTF (sfstderr, "\n");						  \
    }									  \
} while (EXID (0))


/*--------------------------------.
| Print this symbol on EXOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
ex_symbol_value_print (Sfio_t *exoutput, int extype, EXSTYPE const * const exvaluep)
#else
static void
ex_symbol_value_print (exoutput, extype, exvaluep)
    Sfio_t *exoutput;
    int extype;
    EXSTYPE const * const exvaluep;
#endif
{
  Sfio_t *exo = exoutput;
  EXUSE (exo);
  if (!exvaluep)
    return;
# ifdef EXPRINT
  if (extype < EXNTOKENS)
    EXPRINT (exoutput, extoknum[extype], *exvaluep);
# else
  EXUSE (exoutput);
# endif
  switch (extype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on EXOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
ex_symbol_print (Sfio_t *exoutput, int extype, EXSTYPE const * const exvaluep)
#else
static void
ex_symbol_print (exoutput, extype, exvaluep)
    Sfio_t *exoutput;
    int extype;
    EXSTYPE const * const exvaluep;
#endif
{
  if (extype < EXNTOKENS)
    EXFPRINTF (exoutput, "token %s (", extname[extype]);
  else
    EXFPRINTF (exoutput, "nterm %s (", extname[extype]);

  ex_symbol_value_print (exoutput, extype, exvaluep);
  EXFPRINTF (exoutput, ")");
}

/*------------------------------------------------------------------.
| ex_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
ex_stack_print (extype_int16 *exbottom, extype_int16 *extop)
#else
static void
ex_stack_print (exbottom, extop)
    extype_int16 *exbottom;
    extype_int16 *extop;
#endif
{
  EXFPRINTF (sfstderr, "Stack now");
  for (; exbottom <= extop; exbottom++)
    {
      int exbot = *exbottom;
      EXFPRINTF (sfstderr, " %d", exbot);
    }
  EXFPRINTF (sfstderr, "\n");
}

# define EX_STACK_PRINT(Bottom, Top)				\
do {								\
  if (exdebug)							\
    ex_stack_print ((Bottom), (Top));				\
} while (EXID (0))


/*------------------------------------------------.
| Report that the EXRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
ex_reduce_print (EXSTYPE *exvsp, int exrule)
#else
static void
ex_reduce_print (exvsp, exrule)
    EXSTYPE *exvsp;
    int exrule;
#endif
{
  int exnrhs = exr2[exrule];
  int exi;
  unsigned long int exlno = exrline[exrule];
  EXFPRINTF (sfstderr, "Reducing stack by rule %d (line %lu):\n",
	     exrule - 1, exlno);
  /* The symbols being reduced.  */
  for (exi = 0; exi < exnrhs; exi++)
    {
      EXFPRINTF (sfstderr, "   $%d = ", exi + 1);
      ex_symbol_print (sfstderr, exrhs[exprhs[exrule] + exi],
		       &(exvsp[(exi + 1) - (exnrhs)])
		       		       );
      EXFPRINTF (sfstderr, "\n");
    }
}

# define EX_REDUCE_PRINT(Rule)		\
do {					\
  if (exdebug)				\
    ex_reduce_print (exvsp, Rule); \
} while (EXID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int exdebug;
#else /* !EXDEBUG */
# define EXDPRINTF(Args)
# define EX_SYMBOL_PRINT(Title, Type, Value, Location)
# define EX_STACK_PRINT(Bottom, Top)
# define EX_REDUCE_PRINT(Rule)
#endif /* !EXDEBUG */


/* EXINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	EXINITDEPTH
# define EXINITDEPTH 200
#endif

/* EXMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   EXSTACK_ALLOC_MAXIMUM < EXSTACK_BYTES (EXMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef EXMAXDEPTH
# define EXMAXDEPTH 10000
#endif


#if EXERROR_VERBOSE

# ifndef exstrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define exstrlen strlen
#  else
/* Return the length of EXSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static EXSIZE_T
exstrlen (const char *exstr)
#else
static EXSIZE_T
exstrlen (exstr)
    const char *exstr;
#endif
{
  EXSIZE_T exlen;
  for (exlen = 0; exstr[exlen]; exlen++)
    continue;
  return exlen;
}
#  endif
# endif

# ifndef exstpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define exstpcpy stpcpy
#  else
/* Copy EXSRC to EXDEST, returning the address of the terminating '\0' in
   EXDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
exstpcpy (char *exdest, const char *exsrc)
#else
static char *
exstpcpy (exdest, exsrc)
    char *exdest;
    const char *exsrc;
#endif
{
  char *exd = exdest;
  const char *exs = exsrc;

  while ((*exd++ = *exs++) != '\0')
    continue;

  return exd - 1;
}
#  endif
# endif

# ifndef extnamerr
/* Copy to EXRES the contents of EXSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for exerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  EXSTR is taken from extname.  If EXRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static EXSIZE_T
extnamerr (char *exres, const char *exstr)
{
  if (*exstr == '"')
    {
      EXSIZE_T exn = 0;
      char const *exp = exstr;

      for (;;)
	switch (*++exp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++exp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (exres)
	      exres[exn] = *exp;
	    exn++;
	    break;

	  case '"':
	    if (exres)
	      exres[exn] = '\0';
	    return exn;
	  }
    do_not_strip_quotes: ;
    }

  if (! exres)
    return exstrlen (exstr);

  return exstpcpy (exres, exstr) - exres;
}
# endif

/* Copy into *EXMSG, which is of size *EXMSG_ALLOC, an error message
   about the unexpected token EXTOKEN for the state stack whose top is
   EXSSP.

   Return 0 if *EXMSG was successfully written.  Return 1 if *EXMSG is
   not large enough to hold the message.  In that case, also set
   *EXMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
exsyntax_error (EXSIZE_T *exmsg_alloc, char **exmsg,
                extype_int16 *exssp, int extoken)
{
  EXSIZE_T exsize0 = extnamerr (EX_NULL, extname[extoken]);
  EXSIZE_T exsize = exsize0;
  EXSIZE_T exsize1;
  enum { EXERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *exformat = EX_NULL;
  /* Arguments of exformat. */
  char const *exarg[EXERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int excount = 0;

  /* There are many possibilities here to consider:
     - Assume EXFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  EXERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in exchar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated exchar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (extoken != EXEMPTY)
    {
      int exn = expact[*exssp];
      exarg[excount++] = extname[extoken];
      if (!expact_value_is_default (exn))
        {
          /* Start EXX at -EXN if negative to avoid negative indexes in
             EXCHECK.  In other words, skip the first -EXN actions for
             this state because they are default actions.  */
          int exxbegin = exn < 0 ? -exn : 0;
          /* Stay within bounds of both excheck and extname.  */
          int exchecklim = EXLAST - exn + 1;
          int exxend = exchecklim < EXNTOKENS ? exchecklim : EXNTOKENS;
          int exx;

          for (exx = exxbegin; exx < exxend; ++exx)
            if (excheck[exx + exn] == exx && exx != EXTERROR
                && !extable_value_is_error (extable[exx + exn]))
              {
                if (excount == EXERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    excount = 1;
                    exsize = exsize0;
                    break;
                  }
                exarg[excount++] = extname[exx];
                exsize1 = exsize + extnamerr (EX_NULL, extname[exx]);
                if (! (exsize <= exsize1
                       && exsize1 <= EXSTACK_ALLOC_MAXIMUM))
                  return 2;
                exsize = exsize1;
              }
        }
    }

  switch (excount)
    {
# define EXCASE_(N, S)                      \
      case N:                               \
        exformat = S;                       \
      break
      EXCASE_(0, EX_("syntax error"));
      EXCASE_(1, EX_("syntax error, unexpected %s"));
      EXCASE_(2, EX_("syntax error, unexpected %s, expecting %s"));
      EXCASE_(3, EX_("syntax error, unexpected %s, expecting %s or %s"));
      EXCASE_(4, EX_("syntax error, unexpected %s, expecting %s or %s or %s"));
      EXCASE_(5, EX_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef EXCASE_
    }

  exsize1 = exsize + exstrlen (exformat);
  if (! (exsize <= exsize1 && exsize1 <= EXSTACK_ALLOC_MAXIMUM))
    return 2;
  exsize = exsize1;

  if (*exmsg_alloc < exsize)
    {
      *exmsg_alloc = 2 * exsize;
      if (! (exsize <= *exmsg_alloc
             && *exmsg_alloc <= EXSTACK_ALLOC_MAXIMUM))
        *exmsg_alloc = EXSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *exp = *exmsg;
    int exi = 0;
    while ((*exp = *exformat) != '\0')
      if (*exp == '%' && exformat[1] == 's' && exi < excount)
        {
          exp += extnamerr (exp, exarg[exi++]);
          exformat += 2;
        }
      else
        {
          exp++;
          exformat++;
        }
  }
  return 0;
}
#endif /* EXERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
exdestruct (const char *exmsg, int extype, EXSTYPE *exvaluep)
#else
static void
exdestruct (exmsg, extype, exvaluep)
    const char *exmsg;
    int extype;
    EXSTYPE *exvaluep;
#endif
{
  EXUSE (exvaluep);

  if (!exmsg)
    exmsg = "Deleting";
  EX_SYMBOL_PRINT (exmsg, extype, exvaluep, exlocationp);

  switch (extype)
    {

      default:
	break;
    }
}




/* The lookahead symbol.  */
int exchar;

/* The semantic value of the lookahead symbol.  */
EXSTYPE exlval;

/* Number of syntax errors so far.  */
int exnerrs;


/*----------.
| exparse.  |
`----------*/

#ifdef EXPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
exparse (void *EXPARSE_PARAM)
#else
int
exparse (EXPARSE_PARAM)
    void *EXPARSE_PARAM;
#endif
#else /* ! EXPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
exparse (void)
#else
int
exparse ()

#endif
#endif
{
    int exstate;
    /* Number of tokens to shift before error messages enabled.  */
    int exerrstatus;

    /* The stacks and their tools:
       `exss': related to states.
       `exvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow exoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    extype_int16 exssa[EXINITDEPTH];
    extype_int16 *exss;
    extype_int16 *exssp;

    /* The semantic value stack.  */
    EXSTYPE exvsa[EXINITDEPTH];
    EXSTYPE *exvs;
    EXSTYPE *exvsp;

    EXSIZE_T exstacksize;

  int exn;
  int exresult;
  /* Lookahead token as an internal (translated) token number.  */
  int extoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  EXSTYPE exval;

#if EXERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char exmsgbuf[128];
  char *exmsg = exmsgbuf;
  EXSIZE_T exmsg_alloc = sizeof exmsgbuf;
#endif

#define EXPOPSTACK(N)   (exvsp -= (N), exssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int exlen = 0;

  extoken = 0;
  exss = exssa;
  exvs = exvsa;
  exstacksize = EXINITDEPTH;

  EXDPRINTF ((sfstderr, "Starting parse\n"));

  exstate = 0;
  exerrstatus = 0;
  exnerrs = 0;
  exchar = EXEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  exssp = exss;
  exvsp = exvs;
  goto exsetstate;

/*------------------------------------------------------------.
| exnewstate -- Push a new state, which is found in exstate.  |
`------------------------------------------------------------*/
 exnewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  exssp++;

 exsetstate:
  *exssp = exstate;

  if (exss + exstacksize - 1 <= exssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      EXSIZE_T exsize = exssp - exss + 1;

#ifdef exoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	EXSTYPE *exvs1 = exvs;
	extype_int16 *exss1 = exss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if exoverflow is a macro.  */
	exoverflow (EX_("memory exhausted"),
		    &exss1, exsize * sizeof (*exssp),
		    &exvs1, exsize * sizeof (*exvsp),
		    &exstacksize);

	exss = exss1;
	exvs = exvs1;
      }
#else /* no exoverflow */
# ifndef EXSTACK_RELOCATE
      goto exexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (EXMAXDEPTH <= exstacksize)
	goto exexhaustedlab;
      exstacksize *= 2;
      if (EXMAXDEPTH < exstacksize)
	exstacksize = EXMAXDEPTH;

      {
	extype_int16 *exss1 = exss;
	union exalloc *exptr =
	  (union exalloc *) EXSTACK_ALLOC (EXSTACK_BYTES (exstacksize));
	if (! exptr)
	  goto exexhaustedlab;
	EXSTACK_RELOCATE (exss_alloc, exss);
	EXSTACK_RELOCATE (exvs_alloc, exvs);
#  undef EXSTACK_RELOCATE
	if (exss1 != exssa)
	  EXSTACK_FREE (exss1);
      }
# endif
#endif /* no exoverflow */

      exssp = exss + exsize - 1;
      exvsp = exvs + exsize - 1;

      EXDPRINTF ((sfstderr, "Stack size increased to %lu\n",
		  (unsigned long int) exstacksize));

      if (exss + exstacksize - 1 <= exssp)
	EXABORT;
    }

  EXDPRINTF ((sfstderr, "Entering state %d\n", exstate));

  if (exstate == EXFINAL)
    EXACCEPT;

  goto exbackup;

/*-----------.
| exbackup.  |
`-----------*/
exbackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  exn = expact[exstate];
  if (expact_value_is_default (exn))
    goto exdefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* EXCHAR is either EXEMPTY or EXEOF or a valid lookahead symbol.  */
  if (exchar == EXEMPTY)
    {
      EXDPRINTF ((sfstderr, "Reading a token: "));
      exchar = EXLEX;
    }

  if (exchar <= EXEOF)
    {
      exchar = extoken = EXEOF;
      EXDPRINTF ((sfstderr, "Now at end of input.\n"));
    }
  else
    {
      extoken = EXTRANSLATE (exchar);
      EX_SYMBOL_PRINT ("Next token is", extoken, &exlval, &exlloc);
    }

  /* If the proper action on seeing token EXTOKEN is to reduce or to
     detect an error, take that action.  */
  exn += extoken;
  if (exn < 0 || EXLAST < exn || excheck[exn] != extoken)
    goto exdefault;
  exn = extable[exn];
  if (exn <= 0)
    {
      if (extable_value_is_error (exn))
        goto exerrlab;
      exn = -exn;
      goto exreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (exerrstatus)
    exerrstatus--;

  /* Shift the lookahead token.  */
  EX_SYMBOL_PRINT ("Shifting", extoken, &exlval, &exlloc);

  /* Discard the shifted token.  */
  exchar = EXEMPTY;

  exstate = exn;
  *++exvsp = exlval;

  goto exnewstate;


/*-----------------------------------------------------------.
| exdefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
exdefault:
  exn = exdefact[exstate];
  if (exn == 0)
    goto exerrlab;
  goto exreduce;


/*-----------------------------.
| exreduce -- Do a reduction.  |
`-----------------------------*/
exreduce:
  /* exn is the number of a rule to reduce with.  */
  exlen = exr2[exn];

  /* If EXLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets EXVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to EXVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that EXVAL may be used uninitialized.  */
  exval = exvsp[1-exlen];


  EX_REDUCE_PRINT (exn);
  switch (exn)
    {
        case 2:
/* Line 1787 of yacc.c  */
#line 175 "../../lib/expr/exparse.y"
    {
			if ((exvsp[(1) - (2)].expr) && !(expr.program->disc->flags & EX_STRICT))
			{
				if (expr.program->main.value && !(expr.program->disc->flags & EX_RETAIN))
					exfreenode(expr.program, expr.program->main.value);
				if ((exvsp[(1) - (2)].expr)->op == S2B)
				{
					Exnode_t*	x;

					x = (exvsp[(1) - (2)].expr);
					(exvsp[(1) - (2)].expr) = x->data.operand.left;
					x->data.operand.left = 0;
					exfreenode(expr.program, x);
				}
				expr.program->main.lex = PROCEDURE;
				expr.program->main.value = exnewnode(expr.program, PROCEDURE, 1, (exvsp[(1) - (2)].expr)->type, NiL, (exvsp[(1) - (2)].expr));
			}
		}
    break;

  case 5:
/* Line 1787 of yacc.c  */
#line 199 "../../lib/expr/exparse.y"
    {
				register Dtdisc_t*	disc;

				if (expr.procedure)
					exerror("no nested function definitions");
				(exvsp[(1) - (2)].id)->lex = PROCEDURE;
				expr.procedure = (exvsp[(1) - (2)].id)->value = exnewnode(expr.program, PROCEDURE, 1, (exvsp[(1) - (2)].id)->type, NiL, NiL);
				expr.procedure->type = INTEGER;
				if (!(disc = newof(0, Dtdisc_t, 1, 0)))
					exnospace();
				disc->key = offsetof(Exid_t, name);
				if (expr.assigned && !streq((exvsp[(1) - (2)].id)->name, "begin"))
				{
					if (!(expr.procedure->data.procedure.frame = dtopen(disc, Dtset)) || !dtview(expr.procedure->data.procedure.frame, expr.program->symbols))
						exnospace();
					expr.program->symbols = expr.program->frame = expr.procedure->data.procedure.frame;
				}
			}
    break;

  case 6:
/* Line 1787 of yacc.c  */
#line 217 "../../lib/expr/exparse.y"
    {
			expr.procedure = 0;
			if (expr.program->frame)
			{
				expr.program->symbols = expr.program->frame->view;
				dtview(expr.program->frame, NiL);
				expr.program->frame = 0;
			}
			if ((exvsp[(4) - (4)].expr) && (exvsp[(4) - (4)].expr)->op == S2B)
			{
				Exnode_t*	x;

				x = (exvsp[(4) - (4)].expr);
				(exvsp[(4) - (4)].expr) = x->data.operand.left;
				x->data.operand.left = 0;
				exfreenode(expr.program, x);
			}
			(exvsp[(1) - (4)].id)->value->data.operand.right = excast(expr.program, (exvsp[(4) - (4)].expr), (exvsp[(1) - (4)].id)->type, NiL, 0);
		}
    break;

  case 7:
/* Line 1787 of yacc.c  */
#line 239 "../../lib/expr/exparse.y"
    {
			(exval.expr) = 0;
		}
    break;

  case 8:
/* Line 1787 of yacc.c  */
#line 243 "../../lib/expr/exparse.y"
    {
			if (!(exvsp[(1) - (2)].expr))
				(exval.expr) = (exvsp[(2) - (2)].expr);
			else if (!(exvsp[(2) - (2)].expr))
				(exval.expr) = (exvsp[(1) - (2)].expr);
			else if ((exvsp[(1) - (2)].expr)->op == CONSTANT)
			{
				exfreenode(expr.program, (exvsp[(1) - (2)].expr));
				(exval.expr) = (exvsp[(2) - (2)].expr);
			}
#ifdef UNUSED
			else if ((exvsp[(1) - (2)].expr)->op == ';')
			{
				(exval.expr) = (exvsp[(1) - (2)].expr);
				(exvsp[(1) - (2)].expr)->data.operand.last = (exvsp[(1) - (2)].expr)->data.operand.last->data.operand.right = exnewnode(expr.program, ';', 1, (exvsp[(2) - (2)].expr)->type, (exvsp[(2) - (2)].expr), NiL);
			}
			else
			{
				(exval.expr) = exnewnode(expr.program, ';', 1, (exvsp[(1) - (2)].expr)->type, (exvsp[(1) - (2)].expr), NiL);
				(exval.expr)->data.operand.last = (exval.expr)->data.operand.right = exnewnode(expr.program, ';', 1, (exvsp[(2) - (2)].expr)->type, (exvsp[(2) - (2)].expr), NiL);
			}
#endif
			else (exval.expr) = exnewnode(expr.program, ';', 1, (exvsp[(2) - (2)].expr)->type, (exvsp[(1) - (2)].expr), (exvsp[(2) - (2)].expr));
		}
    break;

  case 9:
/* Line 1787 of yacc.c  */
#line 270 "../../lib/expr/exparse.y"
    {
			(exval.expr) = (exvsp[(2) - (3)].expr);
		}
    break;

  case 10:
/* Line 1787 of yacc.c  */
#line 274 "../../lib/expr/exparse.y"
    {
			(exval.expr) = ((exvsp[(1) - (2)].expr) && (exvsp[(1) - (2)].expr)->type == STRING) ? exnewnode(expr.program, S2B, 1, INTEGER, (exvsp[(1) - (2)].expr), NiL) : (exvsp[(1) - (2)].expr);
		}
    break;

  case 11:
/* Line 1787 of yacc.c  */
#line 277 "../../lib/expr/exparse.y"
    {expr.instatic=(exvsp[(1) - (1)].integer);}
    break;

  case 12:
/* Line 1787 of yacc.c  */
#line 277 "../../lib/expr/exparse.y"
    {expr.declare=(exvsp[(3) - (3)].id)->type;}
    break;

  case 13:
/* Line 1787 of yacc.c  */
#line 278 "../../lib/expr/exparse.y"
    {
			(exval.expr) = (exvsp[(5) - (6)].expr);
			expr.declare = 0;
		}
    break;

  case 14:
/* Line 1787 of yacc.c  */
#line 283 "../../lib/expr/exparse.y"
    {
			if (exisAssign ((exvsp[(3) - (6)].expr)))
				exwarn ("assignment used as boolean in if statement");
			if ((exvsp[(3) - (6)].expr)->type == STRING)
				(exvsp[(3) - (6)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (exvsp[(3) - (6)].expr), NiL);
			else if (!INTEGRAL((exvsp[(3) - (6)].expr)->type))
				(exvsp[(3) - (6)].expr) = excast(expr.program, (exvsp[(3) - (6)].expr), INTEGER, NiL, 0);
			(exval.expr) = exnewnode(expr.program, (exvsp[(1) - (6)].id)->index, 1, INTEGER, (exvsp[(3) - (6)].expr), exnewnode(expr.program, ':', 1, (exvsp[(5) - (6)].expr) ? (exvsp[(5) - (6)].expr)->type : 0, (exvsp[(5) - (6)].expr), (exvsp[(6) - (6)].expr)));
		}
    break;

  case 15:
/* Line 1787 of yacc.c  */
#line 293 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, ITERATE, 0, INTEGER, NiL, NiL);
			(exval.expr)->data.generate.array = (exvsp[(3) - (5)].expr);
			if (!(exvsp[(3) - (5)].expr)->data.variable.index || (exvsp[(3) - (5)].expr)->data.variable.index->op != DYNAMIC)
				exerror("simple index variable expected");
			(exval.expr)->data.generate.index = (exvsp[(3) - (5)].expr)->data.variable.index->data.variable.symbol;
			if ((exvsp[(3) - (5)].expr)->op == ID && (exval.expr)->data.generate.index->type != INTEGER)
				exerror("integer index variable expected");
			exfreenode(expr.program, (exvsp[(3) - (5)].expr)->data.variable.index);
			(exvsp[(3) - (5)].expr)->data.variable.index = 0;
			(exval.expr)->data.generate.statement = (exvsp[(5) - (5)].expr);
		}
    break;

  case 16:
/* Line 1787 of yacc.c  */
#line 306 "../../lib/expr/exparse.y"
    {
			if (!(exvsp[(5) - (9)].expr))
			{
				(exvsp[(5) - (9)].expr) = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
				(exvsp[(5) - (9)].expr)->data.constant.value.integer = 1;
			}
			else if ((exvsp[(5) - (9)].expr)->type == STRING)
				(exvsp[(5) - (9)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (exvsp[(5) - (9)].expr), NiL);
			else if (!INTEGRAL((exvsp[(5) - (9)].expr)->type))
				(exvsp[(5) - (9)].expr) = excast(expr.program, (exvsp[(5) - (9)].expr), INTEGER, NiL, 0);
			(exval.expr) = exnewnode(expr.program, (exvsp[(1) - (9)].id)->index, 1, INTEGER, (exvsp[(5) - (9)].expr), exnewnode(expr.program, ';', 1, 0, (exvsp[(7) - (9)].expr), (exvsp[(9) - (9)].expr)));
			if ((exvsp[(3) - (9)].expr))
				(exval.expr) = exnewnode(expr.program, ';', 1, INTEGER, (exvsp[(3) - (9)].expr), (exval.expr));
		}
    break;

  case 17:
/* Line 1787 of yacc.c  */
#line 321 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, ITERATER, 0, INTEGER, NiL, NiL);
			(exval.expr)->data.generate.array = (exvsp[(3) - (5)].expr);
			if (!(exvsp[(3) - (5)].expr)->data.variable.index || (exvsp[(3) - (5)].expr)->data.variable.index->op != DYNAMIC)
				exerror("simple index variable expected");
			(exval.expr)->data.generate.index = (exvsp[(3) - (5)].expr)->data.variable.index->data.variable.symbol;
			if ((exvsp[(3) - (5)].expr)->op == ID && (exval.expr)->data.generate.index->type != INTEGER)
				exerror("integer index variable expected");
			exfreenode(expr.program, (exvsp[(3) - (5)].expr)->data.variable.index);
			(exvsp[(3) - (5)].expr)->data.variable.index = 0;
			(exval.expr)->data.generate.statement = (exvsp[(5) - (5)].expr);
		}
    break;

  case 18:
/* Line 1787 of yacc.c  */
#line 334 "../../lib/expr/exparse.y"
    {
			if ((exvsp[(3) - (4)].id)->local.pointer == 0)
              			exerror("cannot apply unset to non-array %s", (exvsp[(3) - (4)].id)->name);
			(exval.expr) = exnewnode(expr.program, UNSET, 0, INTEGER, NiL, NiL);
			(exval.expr)->data.variable.symbol = (exvsp[(3) - (4)].id);
			(exval.expr)->data.variable.index = NiL;
		}
    break;

  case 19:
/* Line 1787 of yacc.c  */
#line 342 "../../lib/expr/exparse.y"
    {
			if ((exvsp[(3) - (6)].id)->local.pointer == 0)
              			exerror("cannot apply unset to non-array %s", (exvsp[(3) - (6)].id)->name);
			if (((exvsp[(3) - (6)].id)->index_type > 0) && ((exvsp[(5) - (6)].expr)->type != (exvsp[(3) - (6)].id)->index_type))
            		    exerror("%s indices must have type %s, not %s", 
				(exvsp[(3) - (6)].id)->name, extypename(expr.program, (exvsp[(3) - (6)].id)->index_type),extypename(expr.program, (exvsp[(5) - (6)].expr)->type));
			(exval.expr) = exnewnode(expr.program, UNSET, 0, INTEGER, NiL, NiL);
			(exval.expr)->data.variable.symbol = (exvsp[(3) - (6)].id);
			(exval.expr)->data.variable.index = (exvsp[(5) - (6)].expr);
		}
    break;

  case 20:
/* Line 1787 of yacc.c  */
#line 353 "../../lib/expr/exparse.y"
    {
			if (exisAssign ((exvsp[(3) - (5)].expr)))
				exwarn ("assignment used as boolean in while statement");
			if ((exvsp[(3) - (5)].expr)->type == STRING)
				(exvsp[(3) - (5)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (exvsp[(3) - (5)].expr), NiL);
			else if (!INTEGRAL((exvsp[(3) - (5)].expr)->type))
				(exvsp[(3) - (5)].expr) = excast(expr.program, (exvsp[(3) - (5)].expr), INTEGER, NiL, 0);
			(exval.expr) = exnewnode(expr.program, (exvsp[(1) - (5)].id)->index, 1, INTEGER, (exvsp[(3) - (5)].expr), exnewnode(expr.program, ';', 1, 0, NiL, (exvsp[(5) - (5)].expr)));
		}
    break;

  case 21:
/* Line 1787 of yacc.c  */
#line 362 "../../lib/expr/exparse.y"
    {expr.declare=(exvsp[(3) - (3)].expr)->type;}
    break;

  case 22:
/* Line 1787 of yacc.c  */
#line 363 "../../lib/expr/exparse.y"
    {
			register Switch_t*	sw = expr.swstate;

			(exval.expr) = exnewnode(expr.program, (exvsp[(1) - (8)].id)->index, 1, INTEGER, (exvsp[(3) - (8)].expr), exnewnode(expr.program, DEFAULT, 1, 0, sw->defcase, sw->firstcase));
			expr.swstate = expr.swstate->prev;
			if (sw->base)
				free(sw->base);
			if (sw != &swstate)
				free(sw);
			expr.declare = 0;
		}
    break;

  case 23:
/* Line 1787 of yacc.c  */
#line 375 "../../lib/expr/exparse.y"
    {
		loopop:
			if (!(exvsp[(2) - (3)].expr))
			{
				(exvsp[(2) - (3)].expr) = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
				(exvsp[(2) - (3)].expr)->data.constant.value.integer = 1;
			}
			else if (!INTEGRAL((exvsp[(2) - (3)].expr)->type))
				(exvsp[(2) - (3)].expr) = excast(expr.program, (exvsp[(2) - (3)].expr), INTEGER, NiL, 0);
			(exval.expr) = exnewnode(expr.program, (exvsp[(1) - (3)].id)->index, 1, INTEGER, (exvsp[(2) - (3)].expr), NiL);
		}
    break;

  case 24:
/* Line 1787 of yacc.c  */
#line 387 "../../lib/expr/exparse.y"
    {
			goto loopop;
		}
    break;

  case 25:
/* Line 1787 of yacc.c  */
#line 391 "../../lib/expr/exparse.y"
    {
			if ((exvsp[(2) - (3)].expr))
			{
				if (expr.procedure && !expr.procedure->type)
					exerror("return in void function");
				(exvsp[(2) - (3)].expr) = excast(expr.program, (exvsp[(2) - (3)].expr), expr.procedure ? expr.procedure->type : INTEGER, NiL, 0);
			}
			(exval.expr) = exnewnode(expr.program, RETURN, 1, (exvsp[(2) - (3)].expr) ? (exvsp[(2) - (3)].expr)->type : 0, (exvsp[(2) - (3)].expr), NiL);
		}
    break;

  case 26:
/* Line 1787 of yacc.c  */
#line 403 "../../lib/expr/exparse.y"
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
    break;

  case 28:
/* Line 1787 of yacc.c  */
#line 437 "../../lib/expr/exparse.y"
    {
			register Switch_t*	sw = expr.swstate;
			int			n;

			(exval.expr) = exnewnode(expr.program, CASE, 1, 0, (exvsp[(2) - (2)].expr), NiL);
			if (sw->cur > sw->base)
			{
				if (sw->lastcase)
					sw->lastcase->data.select.next = (exval.expr);
				else
					sw->firstcase = (exval.expr);
				sw->lastcase = (exval.expr);
				n = sw->cur - sw->base;
				sw->cur = sw->base;
				(exval.expr)->data.select.constant = (Extype_t**)exalloc(expr.program, (n + 1) * sizeof(Extype_t*));
				memcpy((exval.expr)->data.select.constant, sw->base, n * sizeof(Extype_t*));
				(exval.expr)->data.select.constant[n] = 0;
			}
			else
				(exval.expr)->data.select.constant = 0;
			if (sw->def)
			{
				sw->def = 0;
				if (sw->defcase)
					exerror("duplicate default in switch");
				else
					sw->defcase = (exvsp[(2) - (2)].expr);
			}
		}
    break;

  case 31:
/* Line 1787 of yacc.c  */
#line 473 "../../lib/expr/exparse.y"
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
				(exvsp[(2) - (3)].expr) = excast(expr.program, (exvsp[(2) - (3)].expr), expr.swstate->type, NiL, 0);
				*expr.swstate->cur++ = &((exvsp[(2) - (3)].expr)->data.constant.value);
			}
		}
    break;

  case 32:
/* Line 1787 of yacc.c  */
#line 494 "../../lib/expr/exparse.y"
    {
			expr.swstate->def = 1;
		}
    break;

  case 33:
/* Line 1787 of yacc.c  */
#line 500 "../../lib/expr/exparse.y"
    {
			(exval.integer) = 0;
		}
    break;

  case 34:
/* Line 1787 of yacc.c  */
#line 504 "../../lib/expr/exparse.y"
    {
			(exval.integer) = 1;
		}
    break;

  case 36:
/* Line 1787 of yacc.c  */
#line 511 "../../lib/expr/exparse.y"
    {
			if ((exvsp[(3) - (3)].expr))
				(exval.expr) = (exvsp[(1) - (3)].expr) ? exnewnode(expr.program, ',', 1, (exvsp[(3) - (3)].expr)->type, (exvsp[(1) - (3)].expr), (exvsp[(3) - (3)].expr)) : (exvsp[(3) - (3)].expr);
		}
    break;

  case 37:
/* Line 1787 of yacc.c  */
#line 517 "../../lib/expr/exparse.y"
    {checkName ((exvsp[(1) - (1)].id)); expr.id=(exvsp[(1) - (1)].id);}
    break;

  case 38:
/* Line 1787 of yacc.c  */
#line 518 "../../lib/expr/exparse.y"
    {
			(exval.expr) = 0;
			if (!(exvsp[(1) - (4)].id)->type || expr.declare)
				(exvsp[(1) - (4)].id)->type = expr.declare;
			if ((exvsp[(4) - (4)].expr) && (exvsp[(4) - (4)].expr)->op == PROCEDURE)
			{
				(exvsp[(1) - (4)].id)->lex = PROCEDURE;
				(exvsp[(1) - (4)].id)->type = (exvsp[(4) - (4)].expr)->type;
				(exvsp[(1) - (4)].id)->value = (exvsp[(4) - (4)].expr);
			}
			else
			{
				(exvsp[(1) - (4)].id)->lex = DYNAMIC;
				(exvsp[(1) - (4)].id)->value = exnewnode(expr.program, 0, 0, 0, NiL, NiL);
				if ((exvsp[(3) - (4)].integer) && !(exvsp[(1) - (4)].id)->local.pointer)
				{
					Dtdisc_t*	disc;

					if (!(disc = newof(0, Dtdisc_t, 1, 0)))
						exnospace();
					if ((exvsp[(3) - (4)].integer) == INTEGER) {
						disc->key = offsetof(Exassoc_t, key);
						disc->size = sizeof(Extype_t);
						disc->comparf = (Dtcompar_f)cmpKey;
					}
					else
						disc->key = offsetof(Exassoc_t, name);
					if (!((exvsp[(1) - (4)].id)->local.pointer = (char*)dtopen(disc, Dtoset)))
						exerror("%s: cannot initialize associative array", (exvsp[(1) - (4)].id)->name);
					(exvsp[(1) - (4)].id)->index_type = (exvsp[(3) - (4)].integer); /* -1 indicates no typechecking */
				}
				if ((exvsp[(4) - (4)].expr))
				{
					if ((exvsp[(4) - (4)].expr)->type != (exvsp[(1) - (4)].id)->type)
					{
						(exvsp[(4) - (4)].expr)->type = (exvsp[(1) - (4)].id)->type;
						(exvsp[(4) - (4)].expr)->data.operand.right = excast(expr.program, (exvsp[(4) - (4)].expr)->data.operand.right, (exvsp[(1) - (4)].id)->type, NiL, 0);
					}
					(exvsp[(4) - (4)].expr)->data.operand.left = exnewnode(expr.program, DYNAMIC, 0, (exvsp[(1) - (4)].id)->type, NiL, NiL);
					(exvsp[(4) - (4)].expr)->data.operand.left->data.variable.symbol = (exvsp[(1) - (4)].id);
					(exval.expr) = (exvsp[(4) - (4)].expr);
#if UNUSED
					if (!expr.program->frame && !expr.program->errors)
					{
						expr.assigned++;
						exeval(expr.program, (exval.expr), NiL);
					}
#endif
				}
				else if (!(exvsp[(3) - (4)].integer))
					(exvsp[(1) - (4)].id)->value->data.value = exzero((exvsp[(1) - (4)].id)->type);
			}
		}
    break;

  case 45:
/* Line 1787 of yacc.c  */
#line 584 "../../lib/expr/exparse.y"
    {
			(exval.expr) = 0;
		}
    break;

  case 46:
/* Line 1787 of yacc.c  */
#line 588 "../../lib/expr/exparse.y"
    {
			(exval.expr) = (exvsp[(2) - (2)].expr);
		}
    break;

  case 47:
/* Line 1787 of yacc.c  */
#line 594 "../../lib/expr/exparse.y"
    {
			(exval.expr) = 0;
		}
    break;

  case 49:
/* Line 1787 of yacc.c  */
#line 601 "../../lib/expr/exparse.y"
    {
			(exval.expr) = (exvsp[(2) - (3)].expr);
		}
    break;

  case 50:
/* Line 1787 of yacc.c  */
#line 605 "../../lib/expr/exparse.y"
    {
			(exval.expr) = ((exvsp[(4) - (4)].expr)->type == (exvsp[(2) - (4)].id)->type) ? (exvsp[(4) - (4)].expr) : excast(expr.program, (exvsp[(4) - (4)].expr), (exvsp[(2) - (4)].id)->type, NiL, 0);
		}
    break;

  case 51:
/* Line 1787 of yacc.c  */
#line 609 "../../lib/expr/exparse.y"
    {
			int	rel;

		relational:
			rel = INTEGER;
			goto coerce;
		binary:
			rel = 0;
		coerce:
			if (!(exvsp[(1) - (3)].expr)->type)
			{
				if (!(exvsp[(3) - (3)].expr)->type)
					(exvsp[(1) - (3)].expr)->type = (exvsp[(3) - (3)].expr)->type = rel ? STRING : INTEGER;
				else
					(exvsp[(1) - (3)].expr)->type = (exvsp[(3) - (3)].expr)->type;
			}
			else if (!(exvsp[(3) - (3)].expr)->type)
				(exvsp[(3) - (3)].expr)->type = (exvsp[(1) - (3)].expr)->type;
			if ((exvsp[(1) - (3)].expr)->type != (exvsp[(3) - (3)].expr)->type)
			{
				if ((exvsp[(1) - (3)].expr)->type == STRING)
					(exvsp[(1) - (3)].expr) = excast(expr.program, (exvsp[(1) - (3)].expr), (exvsp[(3) - (3)].expr)->type, (exvsp[(3) - (3)].expr), 0);
				else if ((exvsp[(3) - (3)].expr)->type == STRING)
					(exvsp[(3) - (3)].expr) = excast(expr.program, (exvsp[(3) - (3)].expr), (exvsp[(1) - (3)].expr)->type, (exvsp[(1) - (3)].expr), 0);
				else if ((exvsp[(1) - (3)].expr)->type == FLOATING)
					(exvsp[(3) - (3)].expr) = excast(expr.program, (exvsp[(3) - (3)].expr), FLOATING, (exvsp[(1) - (3)].expr), 0);
				else if ((exvsp[(3) - (3)].expr)->type == FLOATING)
					(exvsp[(1) - (3)].expr) = excast(expr.program, (exvsp[(1) - (3)].expr), FLOATING, (exvsp[(3) - (3)].expr), 0);
			}
			if (!rel)
				rel = ((exvsp[(1) - (3)].expr)->type == STRING) ? STRING : (((exvsp[(1) - (3)].expr)->type == UNSIGNED) ? UNSIGNED : (exvsp[(3) - (3)].expr)->type);
			(exval.expr) = exnewnode(expr.program, (exvsp[(2) - (3)].op), 1, rel, (exvsp[(1) - (3)].expr), (exvsp[(3) - (3)].expr));
			if (!expr.program->errors && (exvsp[(1) - (3)].expr)->op == CONSTANT && (exvsp[(3) - (3)].expr)->op == CONSTANT)
			{
				(exval.expr)->data.constant.value = exeval(expr.program, (exval.expr), NiL);
				(exval.expr)->binary = 0;
				(exval.expr)->op = CONSTANT;
				exfreenode(expr.program, (exvsp[(1) - (3)].expr));
				exfreenode(expr.program, (exvsp[(3) - (3)].expr));
			}
			else if (!BUILTIN((exvsp[(1) - (3)].expr)->type) || !BUILTIN((exvsp[(3) - (3)].expr)->type)) {
				checkBinary(expr.program, (exvsp[(1) - (3)].expr), (exval.expr), (exvsp[(3) - (3)].expr));
			}
		}
    break;

  case 52:
/* Line 1787 of yacc.c  */
#line 654 "../../lib/expr/exparse.y"
    {
			goto binary;
		}
    break;

  case 53:
/* Line 1787 of yacc.c  */
#line 658 "../../lib/expr/exparse.y"
    {
			goto binary;
		}
    break;

  case 54:
/* Line 1787 of yacc.c  */
#line 662 "../../lib/expr/exparse.y"
    {
			goto binary;
		}
    break;

  case 55:
/* Line 1787 of yacc.c  */
#line 666 "../../lib/expr/exparse.y"
    {
			goto binary;
		}
    break;

  case 56:
/* Line 1787 of yacc.c  */
#line 670 "../../lib/expr/exparse.y"
    {
			goto binary;
		}
    break;

  case 57:
/* Line 1787 of yacc.c  */
#line 674 "../../lib/expr/exparse.y"
    {
			goto binary;
		}
    break;

  case 58:
/* Line 1787 of yacc.c  */
#line 678 "../../lib/expr/exparse.y"
    {
			goto relational;
		}
    break;

  case 59:
/* Line 1787 of yacc.c  */
#line 682 "../../lib/expr/exparse.y"
    {
			goto relational;
		}
    break;

  case 60:
/* Line 1787 of yacc.c  */
#line 686 "../../lib/expr/exparse.y"
    {
			goto relational;
		}
    break;

  case 61:
/* Line 1787 of yacc.c  */
#line 690 "../../lib/expr/exparse.y"
    {
			goto relational;
		}
    break;

  case 62:
/* Line 1787 of yacc.c  */
#line 694 "../../lib/expr/exparse.y"
    {
			goto relational;
		}
    break;

  case 63:
/* Line 1787 of yacc.c  */
#line 698 "../../lib/expr/exparse.y"
    {
			goto binary;
		}
    break;

  case 64:
/* Line 1787 of yacc.c  */
#line 702 "../../lib/expr/exparse.y"
    {
			goto binary;
		}
    break;

  case 65:
/* Line 1787 of yacc.c  */
#line 706 "../../lib/expr/exparse.y"
    {
			goto binary;
		}
    break;

  case 66:
/* Line 1787 of yacc.c  */
#line 710 "../../lib/expr/exparse.y"
    {
			goto binary;
		}
    break;

  case 67:
/* Line 1787 of yacc.c  */
#line 714 "../../lib/expr/exparse.y"
    {
		logical:
			if ((exvsp[(1) - (3)].expr)->type == STRING)
				(exvsp[(1) - (3)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (exvsp[(1) - (3)].expr), NiL);
			else if (!BUILTIN((exvsp[(1) - (3)].expr)->type))
				(exvsp[(1) - (3)].expr) = excast(expr.program, (exvsp[(1) - (3)].expr), INTEGER, NiL, 0);
			if ((exvsp[(3) - (3)].expr)->type == STRING)
				(exvsp[(3) - (3)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (exvsp[(3) - (3)].expr), NiL);
			else if (!BUILTIN((exvsp[(3) - (3)].expr)->type))
				(exvsp[(3) - (3)].expr) = excast(expr.program, (exvsp[(3) - (3)].expr), INTEGER, NiL, 0);
			goto binary;
		}
    break;

  case 68:
/* Line 1787 of yacc.c  */
#line 727 "../../lib/expr/exparse.y"
    {
			goto logical;
		}
    break;

  case 69:
/* Line 1787 of yacc.c  */
#line 731 "../../lib/expr/exparse.y"
    {
			if ((exvsp[(1) - (3)].expr)->op == CONSTANT)
			{
				exfreenode(expr.program, (exvsp[(1) - (3)].expr));
				(exval.expr) = (exvsp[(3) - (3)].expr);
			}
			else
				(exval.expr) = exnewnode(expr.program, ',', 1, (exvsp[(3) - (3)].expr)->type, (exvsp[(1) - (3)].expr), (exvsp[(3) - (3)].expr));
		}
    break;

  case 70:
/* Line 1787 of yacc.c  */
#line 740 "../../lib/expr/exparse.y"
    {expr.nolabel=1;}
    break;

  case 71:
/* Line 1787 of yacc.c  */
#line 740 "../../lib/expr/exparse.y"
    {expr.nolabel=0;}
    break;

  case 72:
/* Line 1787 of yacc.c  */
#line 741 "../../lib/expr/exparse.y"
    {
			if (!(exvsp[(4) - (7)].expr)->type)
			{
				if (!(exvsp[(7) - (7)].expr)->type)
					(exvsp[(4) - (7)].expr)->type = (exvsp[(7) - (7)].expr)->type = INTEGER;
				else
					(exvsp[(4) - (7)].expr)->type = (exvsp[(7) - (7)].expr)->type;
			}
			else if (!(exvsp[(7) - (7)].expr)->type)
				(exvsp[(7) - (7)].expr)->type = (exvsp[(4) - (7)].expr)->type;
			if ((exvsp[(1) - (7)].expr)->type == STRING)
				(exvsp[(1) - (7)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (exvsp[(1) - (7)].expr), NiL);
			else if (!INTEGRAL((exvsp[(1) - (7)].expr)->type))
				(exvsp[(1) - (7)].expr) = excast(expr.program, (exvsp[(1) - (7)].expr), INTEGER, NiL, 0);
			if ((exvsp[(4) - (7)].expr)->type != (exvsp[(7) - (7)].expr)->type)
			{
				if ((exvsp[(4) - (7)].expr)->type == STRING || (exvsp[(7) - (7)].expr)->type == STRING)
					exerror("if statement string type mismatch");
				else if ((exvsp[(4) - (7)].expr)->type == FLOATING)
					(exvsp[(7) - (7)].expr) = excast(expr.program, (exvsp[(7) - (7)].expr), FLOATING, NiL, 0);
				else if ((exvsp[(7) - (7)].expr)->type == FLOATING)
					(exvsp[(4) - (7)].expr) = excast(expr.program, (exvsp[(4) - (7)].expr), FLOATING, NiL, 0);
			}
			if ((exvsp[(1) - (7)].expr)->op == CONSTANT)
			{
				if ((exvsp[(1) - (7)].expr)->data.constant.value.integer)
				{
					(exval.expr) = (exvsp[(4) - (7)].expr);
					exfreenode(expr.program, (exvsp[(7) - (7)].expr));
				}
				else
				{
					(exval.expr) = (exvsp[(7) - (7)].expr);
					exfreenode(expr.program, (exvsp[(4) - (7)].expr));
				}
				exfreenode(expr.program, (exvsp[(1) - (7)].expr));
			}
			else
				(exval.expr) = exnewnode(expr.program, '?', 1, (exvsp[(4) - (7)].expr)->type, (exvsp[(1) - (7)].expr), exnewnode(expr.program, ':', 1, (exvsp[(4) - (7)].expr)->type, (exvsp[(4) - (7)].expr), (exvsp[(7) - (7)].expr)));
		}
    break;

  case 73:
/* Line 1787 of yacc.c  */
#line 782 "../../lib/expr/exparse.y"
    {
		iunary:
			if ((exvsp[(2) - (2)].expr)->type == STRING)
				(exvsp[(2) - (2)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (exvsp[(2) - (2)].expr), NiL);
			else if (!INTEGRAL((exvsp[(2) - (2)].expr)->type))
				(exvsp[(2) - (2)].expr) = excast(expr.program, (exvsp[(2) - (2)].expr), INTEGER, NiL, 0);
		unary:
			(exval.expr) = exnewnode(expr.program, (exvsp[(1) - (2)].op), 1, (exvsp[(2) - (2)].expr)->type == UNSIGNED ? INTEGER : (exvsp[(2) - (2)].expr)->type, (exvsp[(2) - (2)].expr), NiL);
			if ((exvsp[(2) - (2)].expr)->op == CONSTANT)
			{
				(exval.expr)->data.constant.value = exeval(expr.program, (exval.expr), NiL);
				(exval.expr)->binary = 0;
				(exval.expr)->op = CONSTANT;
				exfreenode(expr.program, (exvsp[(2) - (2)].expr));
			}
			else if (!BUILTIN((exvsp[(2) - (2)].expr)->type)) {
				checkBinary(expr.program, (exvsp[(2) - (2)].expr), (exval.expr), 0);
			}
		}
    break;

  case 74:
/* Line 1787 of yacc.c  */
#line 802 "../../lib/expr/exparse.y"
    {
			if ((exvsp[(2) - (2)].id)->local.pointer == 0)
              			exerror("cannot apply '#' operator to non-array %s", (exvsp[(2) - (2)].id)->name);
			(exval.expr) = exnewnode(expr.program, '#', 0, INTEGER, NiL, NiL);
			(exval.expr)->data.variable.symbol = (exvsp[(2) - (2)].id);
		}
    break;

  case 75:
/* Line 1787 of yacc.c  */
#line 809 "../../lib/expr/exparse.y"
    {
			goto iunary;
		}
    break;

  case 76:
/* Line 1787 of yacc.c  */
#line 813 "../../lib/expr/exparse.y"
    {
			goto unary;
		}
    break;

  case 77:
/* Line 1787 of yacc.c  */
#line 817 "../../lib/expr/exparse.y"
    {
			(exval.expr) = (exvsp[(2) - (2)].expr);
		}
    break;

  case 78:
/* Line 1787 of yacc.c  */
#line 821 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, ADDRESS, 0, T((exvsp[(2) - (2)].expr)->type), (exvsp[(2) - (2)].expr), NiL);
		}
    break;

  case 79:
/* Line 1787 of yacc.c  */
#line 825 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, ARRAY, 1, T((exvsp[(1) - (4)].id)->type), call(0, (exvsp[(1) - (4)].id), (exvsp[(3) - (4)].expr)), (exvsp[(3) - (4)].expr));
		}
    break;

  case 80:
/* Line 1787 of yacc.c  */
#line 829 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, FUNCTION, 1, T((exvsp[(1) - (4)].id)->type), call(0, (exvsp[(1) - (4)].id), (exvsp[(3) - (4)].expr)), (exvsp[(3) - (4)].expr));
#ifdef UNUSED
			if (!expr.program->disc->getf)
				exerror("%s: function references not supported", (exval.expr)->data.operand.left->data.variable.symbol->name);
			else if (expr.program->disc->reff)
				(*expr.program->disc->reff)(expr.program, (exval.expr)->data.operand.left, (exval.expr)->data.operand.left->data.variable.symbol, 0, NiL, EX_CALL, expr.program->disc);
#endif
		}
    break;

  case 81:
/* Line 1787 of yacc.c  */
#line 839 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewsub (expr.program, (exvsp[(3) - (4)].expr), GSUB);
		}
    break;

  case 82:
/* Line 1787 of yacc.c  */
#line 843 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewsub (expr.program, (exvsp[(3) - (4)].expr), SUB);
		}
    break;

  case 83:
/* Line 1787 of yacc.c  */
#line 847 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewsubstr (expr.program, (exvsp[(3) - (4)].expr));
		}
    break;

  case 84:
/* Line 1787 of yacc.c  */
#line 851 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewsplit (expr.program, (exvsp[(1) - (6)].id)->index, (exvsp[(5) - (6)].id), (exvsp[(3) - (6)].expr), NiL);
		}
    break;

  case 85:
/* Line 1787 of yacc.c  */
#line 855 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewsplit (expr.program, (exvsp[(1) - (8)].id)->index, (exvsp[(5) - (8)].id), (exvsp[(3) - (8)].expr), (exvsp[(7) - (8)].expr));
		}
    break;

  case 86:
/* Line 1787 of yacc.c  */
#line 859 "../../lib/expr/exparse.y"
    {
			if (!INTEGRAL((exvsp[(3) - (4)].expr)->type))
				(exvsp[(3) - (4)].expr) = excast(expr.program, (exvsp[(3) - (4)].expr), INTEGER, NiL, 0);
			(exval.expr) = exnewnode(expr.program, EXIT, 1, INTEGER, (exvsp[(3) - (4)].expr), NiL);
		}
    break;

  case 87:
/* Line 1787 of yacc.c  */
#line 865 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, RAND, 0, FLOATING, NiL, NiL);
		}
    break;

  case 88:
/* Line 1787 of yacc.c  */
#line 869 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, SRAND, 0, INTEGER, NiL, NiL);
		}
    break;

  case 89:
/* Line 1787 of yacc.c  */
#line 873 "../../lib/expr/exparse.y"
    {
			if (!INTEGRAL((exvsp[(3) - (4)].expr)->type))
				(exvsp[(3) - (4)].expr) = excast(expr.program, (exvsp[(3) - (4)].expr), INTEGER, NiL, 0);
			(exval.expr) = exnewnode(expr.program, SRAND, 1, INTEGER, (exvsp[(3) - (4)].expr), NiL);
		}
    break;

  case 90:
/* Line 1787 of yacc.c  */
#line 879 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, CALL, 1, (exvsp[(1) - (4)].id)->type, NiL, (exvsp[(3) - (4)].expr));
			(exval.expr)->data.call.procedure = (exvsp[(1) - (4)].id);
		}
    break;

  case 91:
/* Line 1787 of yacc.c  */
#line 884 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exprint(expr.program, (exvsp[(1) - (4)].id), (exvsp[(3) - (4)].expr));
		}
    break;

  case 92:
/* Line 1787 of yacc.c  */
#line 888 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, (exvsp[(1) - (4)].id)->index, 0, (exvsp[(1) - (4)].id)->type, NiL, NiL);
			if ((exvsp[(3) - (4)].expr) && (exvsp[(3) - (4)].expr)->data.operand.left->type == INTEGER)
			{
				(exval.expr)->data.print.descriptor = (exvsp[(3) - (4)].expr)->data.operand.left;
				(exvsp[(3) - (4)].expr) = (exvsp[(3) - (4)].expr)->data.operand.right;
			}
			else 
				switch ((exvsp[(1) - (4)].id)->index)
				{
				case QUERY:
					(exval.expr)->data.print.descriptor = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
					(exval.expr)->data.print.descriptor->data.constant.value.integer = 2;
					break;
				case PRINTF:
					(exval.expr)->data.print.descriptor = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
					(exval.expr)->data.print.descriptor->data.constant.value.integer = 1;
					break;
				case SPRINTF:
					(exval.expr)->data.print.descriptor = 0;
					break;
				}
			(exval.expr)->data.print.args = preprint((exvsp[(3) - (4)].expr));
		}
    break;

  case 93:
/* Line 1787 of yacc.c  */
#line 913 "../../lib/expr/exparse.y"
    {
			register Exnode_t*	x;

			(exval.expr) = exnewnode(expr.program, (exvsp[(1) - (4)].id)->index, 0, (exvsp[(1) - (4)].id)->type, NiL, NiL);
			if ((exvsp[(3) - (4)].expr) && (exvsp[(3) - (4)].expr)->data.operand.left->type == INTEGER)
			{
				(exval.expr)->data.scan.descriptor = (exvsp[(3) - (4)].expr)->data.operand.left;
				(exvsp[(3) - (4)].expr) = (exvsp[(3) - (4)].expr)->data.operand.right;
			}
			else 
				switch ((exvsp[(1) - (4)].id)->index)
				{
				case SCANF:
					(exval.expr)->data.scan.descriptor = 0;
					break;
				case SSCANF:
					if ((exvsp[(3) - (4)].expr) && (exvsp[(3) - (4)].expr)->data.operand.left->type == STRING)
					{
						(exval.expr)->data.scan.descriptor = (exvsp[(3) - (4)].expr)->data.operand.left;
						(exvsp[(3) - (4)].expr) = (exvsp[(3) - (4)].expr)->data.operand.right;
					}
					else
						exerror("%s: string argument expected", (exvsp[(1) - (4)].id)->name);
					break;
				}
			if (!(exvsp[(3) - (4)].expr) || !(exvsp[(3) - (4)].expr)->data.operand.left || (exvsp[(3) - (4)].expr)->data.operand.left->type != STRING)
				exerror("%s: format argument expected", (exvsp[(1) - (4)].id)->name);
			(exval.expr)->data.scan.format = (exvsp[(3) - (4)].expr)->data.operand.left;
			for (x = (exval.expr)->data.scan.args = (exvsp[(3) - (4)].expr)->data.operand.right; x; x = x->data.operand.right)
			{
				if (x->data.operand.left->op != ADDRESS)
					exerror("%s: address argument expected", (exvsp[(1) - (4)].id)->name);
				x->data.operand.left = x->data.operand.left->data.operand.left;
			}
		}
    break;

  case 94:
/* Line 1787 of yacc.c  */
#line 949 "../../lib/expr/exparse.y"
    {
			if ((exvsp[(2) - (2)].expr))
			{
				if ((exvsp[(1) - (2)].expr)->op == ID && !expr.program->disc->setf)
					exerror("%s: variable assignment not supported", (exvsp[(1) - (2)].expr)->data.variable.symbol->name);
				else
				{
					if (!(exvsp[(1) - (2)].expr)->type)
						(exvsp[(1) - (2)].expr)->type = (exvsp[(2) - (2)].expr)->type;
#if 0
					else if ((exvsp[(2) - (2)].expr)->type != (exvsp[(1) - (2)].expr)->type && (exvsp[(1) - (2)].expr)->type >= 0200)
#else
					else if ((exvsp[(2) - (2)].expr)->type != (exvsp[(1) - (2)].expr)->type)
#endif
					{
						(exvsp[(2) - (2)].expr)->type = (exvsp[(1) - (2)].expr)->type;
						(exvsp[(2) - (2)].expr)->data.operand.right = excast(expr.program, (exvsp[(2) - (2)].expr)->data.operand.right, (exvsp[(1) - (2)].expr)->type, NiL, 0);
					}
					(exvsp[(2) - (2)].expr)->data.operand.left = (exvsp[(1) - (2)].expr);
					(exval.expr) = (exvsp[(2) - (2)].expr);
				}
			}
		}
    break;

  case 95:
/* Line 1787 of yacc.c  */
#line 973 "../../lib/expr/exparse.y"
    {
		pre:
			if ((exvsp[(2) - (2)].expr)->type == STRING)
				exerror("++ and -- invalid for string variables");
			(exval.expr) = exnewnode(expr.program, (exvsp[(1) - (2)].op), 0, (exvsp[(2) - (2)].expr)->type, (exvsp[(2) - (2)].expr), NiL);
			(exval.expr)->subop = PRE;
		}
    break;

  case 96:
/* Line 1787 of yacc.c  */
#line 981 "../../lib/expr/exparse.y"
    {
		pos:
			if ((exvsp[(1) - (2)].expr)->type == STRING)
				exerror("++ and -- invalid for string variables");
			(exval.expr) = exnewnode(expr.program, (exvsp[(2) - (2)].op), 0, (exvsp[(1) - (2)].expr)->type, (exvsp[(1) - (2)].expr), NiL);
			(exval.expr)->subop = POS;
		}
    break;

  case 97:
/* Line 1787 of yacc.c  */
#line 989 "../../lib/expr/exparse.y"
    {
			if ((exvsp[(3) - (3)].id)->local.pointer == 0)
              			exerror("cannot apply IN to non-array %s", (exvsp[(3) - (3)].id)->name);
			if (((exvsp[(3) - (3)].id)->index_type > 0) && ((exvsp[(1) - (3)].expr)->type != (exvsp[(3) - (3)].id)->index_type))
            		    exerror("%s indices must have type %s, not %s", 
				(exvsp[(3) - (3)].id)->name, extypename(expr.program, (exvsp[(3) - (3)].id)->index_type),extypename(expr.program, (exvsp[(1) - (3)].expr)->type));
			(exval.expr) = exnewnode(expr.program, IN_OP, 0, INTEGER, NiL, NiL);
			(exval.expr)->data.variable.symbol = (exvsp[(3) - (3)].id);
			(exval.expr)->data.variable.index = (exvsp[(1) - (3)].expr);
		}
    break;

  case 98:
/* Line 1787 of yacc.c  */
#line 1000 "../../lib/expr/exparse.y"
    {
			goto pre;
		}
    break;

  case 99:
/* Line 1787 of yacc.c  */
#line 1004 "../../lib/expr/exparse.y"
    {
			goto pos;
		}
    break;

  case 103:
/* Line 1787 of yacc.c  */
#line 1014 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, CONSTANT, 0, (exvsp[(1) - (1)].id)->type, NiL, NiL);
			if (!expr.program->disc->reff)
				exerror("%s: identifier references not supported", (exvsp[(1) - (1)].id)->name);
			else
				(exval.expr)->data.constant.value = (*expr.program->disc->reff)(expr.program, (exval.expr), (exvsp[(1) - (1)].id), NiL, NiL, EX_SCALAR, expr.program->disc);
		}
    break;

  case 104:
/* Line 1787 of yacc.c  */
#line 1022 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, CONSTANT, 0, FLOATING, NiL, NiL);
			(exval.expr)->data.constant.value.floating = (exvsp[(1) - (1)].floating);
		}
    break;

  case 105:
/* Line 1787 of yacc.c  */
#line 1027 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
			(exval.expr)->data.constant.value.integer = (exvsp[(1) - (1)].integer);
		}
    break;

  case 106:
/* Line 1787 of yacc.c  */
#line 1032 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, CONSTANT, 0, STRING, NiL, NiL);
			(exval.expr)->data.constant.value.string = (exvsp[(1) - (1)].string);
		}
    break;

  case 107:
/* Line 1787 of yacc.c  */
#line 1037 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, CONSTANT, 0, UNSIGNED, NiL, NiL);
			(exval.expr)->data.constant.value.integer = (exvsp[(1) - (1)].integer);
		}
    break;

  case 113:
/* Line 1787 of yacc.c  */
#line 1053 "../../lib/expr/exparse.y"
    {
			(exval.expr) = makeVar(expr.program, (exvsp[(1) - (2)].id), 0, 0, (exvsp[(2) - (2)].reference));
		}
    break;

  case 114:
/* Line 1787 of yacc.c  */
#line 1057 "../../lib/expr/exparse.y"
    {
			Exnode_t*   n;

			n = exnewnode(expr.program, DYNAMIC, 0, (exvsp[(1) - (3)].id)->type, NiL, NiL);
			n->data.variable.symbol = (exvsp[(1) - (3)].id);
			n->data.variable.reference = 0;
			if (((n->data.variable.index = (exvsp[(2) - (3)].expr)) == 0) != ((exvsp[(1) - (3)].id)->local.pointer == 0))
				exerror("%s: is%s an array", (exvsp[(1) - (3)].id)->name, (exvsp[(1) - (3)].id)->local.pointer ? "" : " not");
			if ((exvsp[(1) - (3)].id)->local.pointer && ((exvsp[(1) - (3)].id)->index_type > 0)) {
				if ((exvsp[(2) - (3)].expr)->type != (exvsp[(1) - (3)].id)->index_type)
					exerror("%s: indices must have type %s, not %s", 
						(exvsp[(1) - (3)].id)->name, extypename(expr.program, (exvsp[(1) - (3)].id)->index_type),extypename(expr.program, (exvsp[(2) - (3)].expr)->type));
			}
			if ((exvsp[(3) - (3)].reference)) {
				n->data.variable.dyna =exnewnode(expr.program, 0, 0, 0, NiL, NiL);
				(exval.expr) = makeVar(expr.program, (exvsp[(1) - (3)].id), (exvsp[(2) - (3)].expr), n, (exvsp[(3) - (3)].reference));
			}
			else (exval.expr) = n;
		}
    break;

  case 115:
/* Line 1787 of yacc.c  */
#line 1077 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, ID, 0, STRING, NiL, NiL);
			(exval.expr)->data.variable.symbol = (exvsp[(1) - (1)].id);
			(exval.expr)->data.variable.reference = 0;
			(exval.expr)->data.variable.index = 0;
			(exval.expr)->data.variable.dyna = 0;
			if (!(expr.program->disc->flags & EX_UNDECLARED))
				exerror("unknown identifier");
		}
    break;

  case 116:
/* Line 1787 of yacc.c  */
#line 1089 "../../lib/expr/exparse.y"
    {
			(exval.integer) = 0;
		}
    break;

  case 117:
/* Line 1787 of yacc.c  */
#line 1093 "../../lib/expr/exparse.y"
    {
			(exval.integer) = -1;
		}
    break;

  case 118:
/* Line 1787 of yacc.c  */
#line 1097 "../../lib/expr/exparse.y"
    {
			/* If DECLARE is VOID, its type is 0, so this acts like
			 * the empty case.
			 */
			if (INTEGRAL((exvsp[(2) - (3)].id)->type))
				(exval.integer) = INTEGER;
			else
				(exval.integer) = (exvsp[(2) - (3)].id)->type;
				
		}
    break;

  case 119:
/* Line 1787 of yacc.c  */
#line 1110 "../../lib/expr/exparse.y"
    {
			(exval.expr) = 0;
		}
    break;

  case 120:
/* Line 1787 of yacc.c  */
#line 1114 "../../lib/expr/exparse.y"
    {
			(exval.expr) = (exvsp[(2) - (3)].expr);
		}
    break;

  case 121:
/* Line 1787 of yacc.c  */
#line 1120 "../../lib/expr/exparse.y"
    {
			(exval.expr) = 0;
		}
    break;

  case 122:
/* Line 1787 of yacc.c  */
#line 1124 "../../lib/expr/exparse.y"
    {
			(exval.expr) = (exvsp[(1) - (1)].expr)->data.operand.left;
			(exvsp[(1) - (1)].expr)->data.operand.left = (exvsp[(1) - (1)].expr)->data.operand.right = 0;
			exfreenode(expr.program, (exvsp[(1) - (1)].expr));
		}
    break;

  case 123:
/* Line 1787 of yacc.c  */
#line 1132 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, ',', 1, 0, exnewnode(expr.program, ',', 1, (exvsp[(1) - (1)].expr)->type, (exvsp[(1) - (1)].expr), NiL), NiL);
			(exval.expr)->data.operand.right = (exval.expr)->data.operand.left;
		}
    break;

  case 124:
/* Line 1787 of yacc.c  */
#line 1137 "../../lib/expr/exparse.y"
    {
			(exvsp[(1) - (3)].expr)->data.operand.right = (exvsp[(1) - (3)].expr)->data.operand.right->data.operand.right = exnewnode(expr.program, ',', 1, (exvsp[(1) - (3)].expr)->type, (exvsp[(3) - (3)].expr), NiL);
		}
    break;

  case 125:
/* Line 1787 of yacc.c  */
#line 1143 "../../lib/expr/exparse.y"
    {
			(exval.expr) = 0;
		}
    break;

  case 126:
/* Line 1787 of yacc.c  */
#line 1147 "../../lib/expr/exparse.y"
    {
			(exval.expr) = 0;
			if ((exvsp[(1) - (1)].id)->type)
				exerror("(void) expected");
		}
    break;

  case 128:
/* Line 1787 of yacc.c  */
#line 1156 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, ',', 1, (exvsp[(1) - (1)].expr)->type, (exvsp[(1) - (1)].expr), NiL);
		}
    break;

  case 129:
/* Line 1787 of yacc.c  */
#line 1160 "../../lib/expr/exparse.y"
    {
			register Exnode_t*	x;
			register Exnode_t*	y;

			(exval.expr) = (exvsp[(1) - (3)].expr);
			for (x = (exvsp[(1) - (3)].expr); (y = x->data.operand.right); x = y);
			x->data.operand.right = exnewnode(expr.program, ',', 1, (exvsp[(3) - (3)].expr)->type, (exvsp[(3) - (3)].expr), NiL);
		}
    break;

  case 130:
/* Line 1787 of yacc.c  */
#line 1170 "../../lib/expr/exparse.y"
    {expr.declare=(exvsp[(1) - (1)].id)->type;}
    break;

  case 131:
/* Line 1787 of yacc.c  */
#line 1171 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, ID, 0, (exvsp[(1) - (3)].id)->type, NiL, NiL);
			(exval.expr)->data.variable.symbol = (exvsp[(3) - (3)].id);
			(exvsp[(3) - (3)].id)->lex = DYNAMIC;
			(exvsp[(3) - (3)].id)->type = (exvsp[(1) - (3)].id)->type;
			(exvsp[(3) - (3)].id)->value = exnewnode(expr.program, 0, 0, 0, NiL, NiL);
			expr.procedure->data.procedure.arity++;
			expr.declare = 0;
		}
    break;

  case 132:
/* Line 1787 of yacc.c  */
#line 1183 "../../lib/expr/exparse.y"
    {
			(exval.reference) = expr.refs = expr.lastref = 0;
		}
    break;

  case 133:
/* Line 1787 of yacc.c  */
#line 1187 "../../lib/expr/exparse.y"
    {
			Exref_t*	r;

			r = ALLOCATE(expr.program, Exref_t);
			r->symbol = (exvsp[(1) - (1)].id);
			expr.refs = r;
			expr.lastref = r;
			r->next = 0;
			r->index = 0;
			(exval.reference) = expr.refs;
		}
    break;

  case 134:
/* Line 1787 of yacc.c  */
#line 1199 "../../lib/expr/exparse.y"
    {
			Exref_t*	r;
			Exref_t*	l;

			r = ALLOCATE(expr.program, Exref_t);
			r->symbol = (exvsp[(3) - (3)].id);
			r->index = 0;
			r->next = 0;
			l = ALLOCATE(expr.program, Exref_t);
			l->symbol = (exvsp[(2) - (3)].id);
			l->index = 0;
			l->next = r;
			expr.refs = l;
			expr.lastref = r;
			(exval.reference) = expr.refs;
		}
    break;

  case 135:
/* Line 1787 of yacc.c  */
#line 1218 "../../lib/expr/exparse.y"
    {
			(exval.id) = (exvsp[(2) - (2)].id);
		}
    break;

  case 136:
/* Line 1787 of yacc.c  */
#line 1222 "../../lib/expr/exparse.y"
    {
			(exval.id) = (exvsp[(2) - (2)].id);
		}
    break;

  case 137:
/* Line 1787 of yacc.c  */
#line 1227 "../../lib/expr/exparse.y"
    {
			(exval.expr) = 0;
		}
    break;

  case 138:
/* Line 1787 of yacc.c  */
#line 1231 "../../lib/expr/exparse.y"
    {
			(exval.expr) = exnewnode(expr.program, '=', 1, (exvsp[(2) - (2)].expr)->type, NiL, (exvsp[(2) - (2)].expr));
			(exval.expr)->subop = (exvsp[(1) - (2)].op);
		}
    break;

  case 140:
/* Line 1787 of yacc.c  */
#line 1238 "../../lib/expr/exparse.y"
    {
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
			}
    break;

  case 141:
/* Line 1787 of yacc.c  */
#line 1255 "../../lib/expr/exparse.y"
    {
				expr.id->lex = PROCEDURE;
				expr.id->type = expr.procedure->type;
				expr.program->formals = 0;
				expr.declare = 0;
			}
    break;

  case 142:
/* Line 1787 of yacc.c  */
#line 1261 "../../lib/expr/exparse.y"
    {
			(exval.expr) = expr.procedure;
			expr.procedure = 0;
			if (expr.program->frame)
			{
				expr.program->symbols = expr.program->frame->view;
				dtview(expr.program->frame, NiL);
				expr.program->frame = 0;
			}
			(exval.expr)->data.operand.left = (exvsp[(3) - (8)].expr);
			(exval.expr)->data.operand.right = excast(expr.program, (exvsp[(7) - (8)].expr), (exval.expr)->type, NiL, 0);

			/*
			 * NOTE: procedure definition was slipped into the
			 *	 declaration initializer statement production,
			 *	 therefore requiring the statement terminator
			 */

			exunlex(expr.program, ';');
		}
    break;


/* Line 1787 of yacc.c  */
#line 3510 "y.tab.c"
      default: break;
    }
  /* User semantic actions sometimes alter exchar, and that requires
     that extoken be updated with the new translation.  We take the
     approach of translating immediately before every use of extoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     EXABORT, EXACCEPT, or EXERROR immediately after altering exchar or
     if it invokes EXBACKUP.  In the case of EXABORT or EXACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of EXERROR or EXBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  EX_SYMBOL_PRINT ("-> $$ =", exr1[exn], &exval, &exloc);

  EXPOPSTACK (exlen);
  exlen = 0;
  EX_STACK_PRINT (exss, exssp);

  *++exvsp = exval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  exn = exr1[exn];

  exstate = expgoto[exn - EXNTOKENS] + *exssp;
  if (0 <= exstate && exstate <= EXLAST && excheck[exstate] == *exssp)
    exstate = extable[exstate];
  else
    exstate = exdefgoto[exn - EXNTOKENS];

  goto exnewstate;


/*------------------------------------.
| exerrlab -- here on detecting error |
`------------------------------------*/
exerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  extoken = exchar == EXEMPTY ? EXEMPTY : EXTRANSLATE (exchar);

  /* If not already recovering from an error, report this error.  */
  if (!exerrstatus)
    {
      ++exnerrs;
#if ! EXERROR_VERBOSE
      exerror (EX_("syntax error"));
#else
# define EXSYNTAX_ERROR exsyntax_error (&exmsg_alloc, &exmsg, \
                                        exssp, extoken)
      {
        char const *exmsgp = EX_("syntax error");
        int exsyntax_error_status;
        exsyntax_error_status = EXSYNTAX_ERROR;
        if (exsyntax_error_status == 0)
          exmsgp = exmsg;
        else if (exsyntax_error_status == 1)
          {
            if (exmsg != exmsgbuf)
              EXSTACK_FREE (exmsg);
            exmsg = (char *) EXSTACK_ALLOC (exmsg_alloc);
            if (!exmsg)
              {
                exmsg = exmsgbuf;
                exmsg_alloc = sizeof exmsgbuf;
                exsyntax_error_status = 2;
              }
            else
              {
                exsyntax_error_status = EXSYNTAX_ERROR;
                exmsgp = exmsg;
              }
          }
        exerror (exmsgp);
        if (exsyntax_error_status == 2)
          goto exexhaustedlab;
      }
# undef EXSYNTAX_ERROR
#endif
    }



  if (exerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (exchar <= EXEOF)
	{
	  /* Return failure if at end of input.  */
	  if (exchar == EXEOF)
	    EXABORT;
	}
      else
	{
	  exdestruct ("Error: discarding",
		      extoken, &exlval);
	  exchar = EXEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto exerrlab1;


/*---------------------------------------------------.
| exerrorlab -- error raised explicitly by EXERROR.  |
`---------------------------------------------------*/
exerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     EXERROR and the label exerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto exerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this EXERROR.  */
  EXPOPSTACK (exlen);
  exlen = 0;
  EX_STACK_PRINT (exss, exssp);
  exstate = *exssp;
  goto exerrlab1;


/*-------------------------------------------------------------.
| exerrlab1 -- common code for both syntax error and EXERROR.  |
`-------------------------------------------------------------*/
exerrlab1:
  exerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      exn = expact[exstate];
      if (!expact_value_is_default (exn))
	{
	  exn += EXTERROR;
	  if (0 <= exn && exn <= EXLAST && excheck[exn] == EXTERROR)
	    {
	      exn = extable[exn];
	      if (0 < exn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (exssp == exss)
	EXABORT;


      exdestruct ("Error: popping",
		  exstos[exstate], exvsp);
      EXPOPSTACK (1);
      exstate = *exssp;
      EX_STACK_PRINT (exss, exssp);
    }

  *++exvsp = exlval;


  /* Shift the error token.  */
  EX_SYMBOL_PRINT ("Shifting", exstos[exn], exvsp, exlsp);

  exstate = exn;
  goto exnewstate;


/*-------------------------------------.
| exacceptlab -- EXACCEPT comes here.  |
`-------------------------------------*/
exacceptlab:
  exresult = 0;
  goto exreturn;

/*-----------------------------------.
| exabortlab -- EXABORT comes here.  |
`-----------------------------------*/
exabortlab:
  exresult = 1;
  goto exreturn;

#if !defined exoverflow || EXERROR_VERBOSE
/*-------------------------------------------------.
| exexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
exexhaustedlab:
  exerror (EX_("memory exhausted"));
  exresult = 2;
  /* Fall through.  */
#endif

exreturn:
  if (exchar != EXEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      extoken = EXTRANSLATE (exchar);
      exdestruct ("Cleanup: discarding lookahead",
                  extoken, &exlval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this EXABORT or EXACCEPT.  */
  EXPOPSTACK (exlen);
  EX_STACK_PRINT (exss, exssp);
  while (exssp != exss)
    {
      exdestruct ("Cleanup: popping",
		  exstos[*exssp], exvsp);
      EXPOPSTACK (1);
    }
#ifndef exoverflow
  if (exss != exssa)
    EXSTACK_FREE (exss);
#endif
#if EXERROR_VERBOSE
  if (exmsg != exmsgbuf)
    EXSTACK_FREE (exmsg);
#endif
  /* Make sure EXID is used.  */
  return EXID (exresult);
}


/* Line 2048 of yacc.c  */
#line 1283 "../../lib/expr/exparse.y"


#include "exgram.h"

