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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.6.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




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

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_Y_TAB_H
# define YY_Y_TAB_H
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
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



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
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
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_Y_TAB_H  */

/* Copy the second part of user declarations.  */
/* Line 353 of yacc.c  */
#line 166 "../../lib/expr/exparse.y"


#include "exgram.h"


/* Line 353 of yacc.c  */
#line 344 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
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

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1112

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  107
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  44
/* YYNRULES -- Number of rules.  */
#define YYNRULES  142
/* YYNRULES -- Number of states.  */
#define YYNSTATES  286

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   336

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
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

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
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

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
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

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
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

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
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
  "$@10", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
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

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
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

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
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

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
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

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    49,    95,   205,     2,    50,    96,   206,   200,
     243,   253,   254,   255,    51,   225,   226,   237,   227,   279,
     232,    52,    53,   159,   238,    54,    55,    56,    57,    58,
     246,    63,   127,   128,   271,   272,   273,   274,    70,    71,
     125,   260,   268,   275
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -144
static const yytype_int16 yypact[] =
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

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -144,  -144,  -144,  -144,  -144,   -48,  -143,  -144,  -144,  -144,
    -144,  -144,  -144,    28,  -144,  -144,    48,  -144,  -144,  -144,
    -144,    -9,   -36,  -144,  -144,  -144,    34,  -144,  -144,   101,
    -144,  -144,    24,  -144,  -144,  -144,    12,  -144,   224,   150,
      51,  -144,  -144,  -144
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -127
static const yytype_int16 yytable[] =
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

#define yypact_value_is_default(yystate) \
  ((yystate) == (-144))

#define yytable_value_is_error(yytable_value) \
  ((yytable_value) == (-127))

static const yytype_int16 yycheck[] =
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

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
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

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (YYID (N))                                                     \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (YYID (0))
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])



/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
/* Line 1787 of yacc.c  */
#line 175 "../../lib/expr/exparse.y"
    {
			if ((yyvsp[(1) - (2)].expr) && !(expr.program->disc->flags & EX_STRICT))
			{
				if (expr.program->main.value && !(expr.program->disc->flags & EX_RETAIN))
					exfreenode(expr.program, expr.program->main.value);
				if ((yyvsp[(1) - (2)].expr)->op == S2B)
				{
					Exnode_t*	x;

					x = (yyvsp[(1) - (2)].expr);
					(yyvsp[(1) - (2)].expr) = x->data.operand.left;
					x->data.operand.left = 0;
					exfreenode(expr.program, x);
				}
				expr.program->main.lex = PROCEDURE;
				expr.program->main.value = exnewnode(expr.program, PROCEDURE, 1, (yyvsp[(1) - (2)].expr)->type, NiL, (yyvsp[(1) - (2)].expr));
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
				(yyvsp[(1) - (2)].id)->lex = PROCEDURE;
				expr.procedure = (yyvsp[(1) - (2)].id)->value = exnewnode(expr.program, PROCEDURE, 1, (yyvsp[(1) - (2)].id)->type, NiL, NiL);
				expr.procedure->type = INTEGER;
				if (!(disc = newof(0, Dtdisc_t, 1, 0)))
					exnospace();
				disc->key = offsetof(Exid_t, name);
				if (expr.assigned && !streq((yyvsp[(1) - (2)].id)->name, "begin"))
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
			if ((yyvsp[(4) - (4)].expr) && (yyvsp[(4) - (4)].expr)->op == S2B)
			{
				Exnode_t*	x;

				x = (yyvsp[(4) - (4)].expr);
				(yyvsp[(4) - (4)].expr) = x->data.operand.left;
				x->data.operand.left = 0;
				exfreenode(expr.program, x);
			}
			(yyvsp[(1) - (4)].id)->value->data.operand.right = excast(expr.program, (yyvsp[(4) - (4)].expr), (yyvsp[(1) - (4)].id)->type, NiL, 0);
		}
    break;

  case 7:
/* Line 1787 of yacc.c  */
#line 239 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = 0;
		}
    break;

  case 8:
/* Line 1787 of yacc.c  */
#line 243 "../../lib/expr/exparse.y"
    {
			if (!(yyvsp[(1) - (2)].expr))
				(yyval.expr) = (yyvsp[(2) - (2)].expr);
			else if (!(yyvsp[(2) - (2)].expr))
				(yyval.expr) = (yyvsp[(1) - (2)].expr);
			else if ((yyvsp[(1) - (2)].expr)->op == CONSTANT)
			{
				exfreenode(expr.program, (yyvsp[(1) - (2)].expr));
				(yyval.expr) = (yyvsp[(2) - (2)].expr);
			}
#ifdef UNUSED
			else if ((yyvsp[(1) - (2)].expr)->op == ';')
			{
				(yyval.expr) = (yyvsp[(1) - (2)].expr);
				(yyvsp[(1) - (2)].expr)->data.operand.last = (yyvsp[(1) - (2)].expr)->data.operand.last->data.operand.right = exnewnode(expr.program, ';', 1, (yyvsp[(2) - (2)].expr)->type, (yyvsp[(2) - (2)].expr), NiL);
			}
			else
			{
				(yyval.expr) = exnewnode(expr.program, ';', 1, (yyvsp[(1) - (2)].expr)->type, (yyvsp[(1) - (2)].expr), NiL);
				(yyval.expr)->data.operand.last = (yyval.expr)->data.operand.right = exnewnode(expr.program, ';', 1, (yyvsp[(2) - (2)].expr)->type, (yyvsp[(2) - (2)].expr), NiL);
			}
#endif
			else (yyval.expr) = exnewnode(expr.program, ';', 1, (yyvsp[(2) - (2)].expr)->type, (yyvsp[(1) - (2)].expr), (yyvsp[(2) - (2)].expr));
		}
    break;

  case 9:
/* Line 1787 of yacc.c  */
#line 270 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = (yyvsp[(2) - (3)].expr);
		}
    break;

  case 10:
/* Line 1787 of yacc.c  */
#line 274 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = ((yyvsp[(1) - (2)].expr) && (yyvsp[(1) - (2)].expr)->type == STRING) ? exnewnode(expr.program, S2B, 1, INTEGER, (yyvsp[(1) - (2)].expr), NiL) : (yyvsp[(1) - (2)].expr);
		}
    break;

  case 11:
/* Line 1787 of yacc.c  */
#line 277 "../../lib/expr/exparse.y"
    {expr.instatic=(yyvsp[(1) - (1)].integer);}
    break;

  case 12:
/* Line 1787 of yacc.c  */
#line 277 "../../lib/expr/exparse.y"
    {expr.declare=(yyvsp[(3) - (3)].id)->type;}
    break;

  case 13:
/* Line 1787 of yacc.c  */
#line 278 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = (yyvsp[(5) - (6)].expr);
			expr.declare = 0;
		}
    break;

  case 14:
/* Line 1787 of yacc.c  */
#line 283 "../../lib/expr/exparse.y"
    {
			if (exisAssign ((yyvsp[(3) - (6)].expr)))
				exwarn ("assignment used as boolean in if statement");
			if ((yyvsp[(3) - (6)].expr)->type == STRING)
				(yyvsp[(3) - (6)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (yyvsp[(3) - (6)].expr), NiL);
			else if (!INTEGRAL((yyvsp[(3) - (6)].expr)->type))
				(yyvsp[(3) - (6)].expr) = excast(expr.program, (yyvsp[(3) - (6)].expr), INTEGER, NiL, 0);
			(yyval.expr) = exnewnode(expr.program, (yyvsp[(1) - (6)].id)->index, 1, INTEGER, (yyvsp[(3) - (6)].expr), exnewnode(expr.program, ':', 1, (yyvsp[(5) - (6)].expr) ? (yyvsp[(5) - (6)].expr)->type : 0, (yyvsp[(5) - (6)].expr), (yyvsp[(6) - (6)].expr)));
		}
    break;

  case 15:
/* Line 1787 of yacc.c  */
#line 293 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, ITERATE, 0, INTEGER, NiL, NiL);
			(yyval.expr)->data.generate.array = (yyvsp[(3) - (5)].expr);
			if (!(yyvsp[(3) - (5)].expr)->data.variable.index || (yyvsp[(3) - (5)].expr)->data.variable.index->op != DYNAMIC)
				exerror("simple index variable expected");
			(yyval.expr)->data.generate.index = (yyvsp[(3) - (5)].expr)->data.variable.index->data.variable.symbol;
			if ((yyvsp[(3) - (5)].expr)->op == ID && (yyval.expr)->data.generate.index->type != INTEGER)
				exerror("integer index variable expected");
			exfreenode(expr.program, (yyvsp[(3) - (5)].expr)->data.variable.index);
			(yyvsp[(3) - (5)].expr)->data.variable.index = 0;
			(yyval.expr)->data.generate.statement = (yyvsp[(5) - (5)].expr);
		}
    break;

  case 16:
/* Line 1787 of yacc.c  */
#line 306 "../../lib/expr/exparse.y"
    {
			if (!(yyvsp[(5) - (9)].expr))
			{
				(yyvsp[(5) - (9)].expr) = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
				(yyvsp[(5) - (9)].expr)->data.constant.value.integer = 1;
			}
			else if ((yyvsp[(5) - (9)].expr)->type == STRING)
				(yyvsp[(5) - (9)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (yyvsp[(5) - (9)].expr), NiL);
			else if (!INTEGRAL((yyvsp[(5) - (9)].expr)->type))
				(yyvsp[(5) - (9)].expr) = excast(expr.program, (yyvsp[(5) - (9)].expr), INTEGER, NiL, 0);
			(yyval.expr) = exnewnode(expr.program, (yyvsp[(1) - (9)].id)->index, 1, INTEGER, (yyvsp[(5) - (9)].expr), exnewnode(expr.program, ';', 1, 0, (yyvsp[(7) - (9)].expr), (yyvsp[(9) - (9)].expr)));
			if ((yyvsp[(3) - (9)].expr))
				(yyval.expr) = exnewnode(expr.program, ';', 1, INTEGER, (yyvsp[(3) - (9)].expr), (yyval.expr));
		}
    break;

  case 17:
/* Line 1787 of yacc.c  */
#line 321 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, ITERATER, 0, INTEGER, NiL, NiL);
			(yyval.expr)->data.generate.array = (yyvsp[(3) - (5)].expr);
			if (!(yyvsp[(3) - (5)].expr)->data.variable.index || (yyvsp[(3) - (5)].expr)->data.variable.index->op != DYNAMIC)
				exerror("simple index variable expected");
			(yyval.expr)->data.generate.index = (yyvsp[(3) - (5)].expr)->data.variable.index->data.variable.symbol;
			if ((yyvsp[(3) - (5)].expr)->op == ID && (yyval.expr)->data.generate.index->type != INTEGER)
				exerror("integer index variable expected");
			exfreenode(expr.program, (yyvsp[(3) - (5)].expr)->data.variable.index);
			(yyvsp[(3) - (5)].expr)->data.variable.index = 0;
			(yyval.expr)->data.generate.statement = (yyvsp[(5) - (5)].expr);
		}
    break;

  case 18:
/* Line 1787 of yacc.c  */
#line 334 "../../lib/expr/exparse.y"
    {
			if ((yyvsp[(3) - (4)].id)->local.pointer == 0)
              			exerror("cannot apply unset to non-array %s", (yyvsp[(3) - (4)].id)->name);
			(yyval.expr) = exnewnode(expr.program, UNSET, 0, INTEGER, NiL, NiL);
			(yyval.expr)->data.variable.symbol = (yyvsp[(3) - (4)].id);
			(yyval.expr)->data.variable.index = NiL;
		}
    break;

  case 19:
/* Line 1787 of yacc.c  */
#line 342 "../../lib/expr/exparse.y"
    {
			if ((yyvsp[(3) - (6)].id)->local.pointer == 0)
              			exerror("cannot apply unset to non-array %s", (yyvsp[(3) - (6)].id)->name);
			if (((yyvsp[(3) - (6)].id)->index_type > 0) && ((yyvsp[(5) - (6)].expr)->type != (yyvsp[(3) - (6)].id)->index_type))
            		    exerror("%s indices must have type %s, not %s", 
				(yyvsp[(3) - (6)].id)->name, extypename(expr.program, (yyvsp[(3) - (6)].id)->index_type),extypename(expr.program, (yyvsp[(5) - (6)].expr)->type));
			(yyval.expr) = exnewnode(expr.program, UNSET, 0, INTEGER, NiL, NiL);
			(yyval.expr)->data.variable.symbol = (yyvsp[(3) - (6)].id);
			(yyval.expr)->data.variable.index = (yyvsp[(5) - (6)].expr);
		}
    break;

  case 20:
/* Line 1787 of yacc.c  */
#line 353 "../../lib/expr/exparse.y"
    {
			if (exisAssign ((yyvsp[(3) - (5)].expr)))
				exwarn ("assignment used as boolean in while statement");
			if ((yyvsp[(3) - (5)].expr)->type == STRING)
				(yyvsp[(3) - (5)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (yyvsp[(3) - (5)].expr), NiL);
			else if (!INTEGRAL((yyvsp[(3) - (5)].expr)->type))
				(yyvsp[(3) - (5)].expr) = excast(expr.program, (yyvsp[(3) - (5)].expr), INTEGER, NiL, 0);
			(yyval.expr) = exnewnode(expr.program, (yyvsp[(1) - (5)].id)->index, 1, INTEGER, (yyvsp[(3) - (5)].expr), exnewnode(expr.program, ';', 1, 0, NiL, (yyvsp[(5) - (5)].expr)));
		}
    break;

  case 21:
/* Line 1787 of yacc.c  */
#line 362 "../../lib/expr/exparse.y"
    {expr.declare=(yyvsp[(3) - (3)].expr)->type;}
    break;

  case 22:
/* Line 1787 of yacc.c  */
#line 363 "../../lib/expr/exparse.y"
    {
			register Switch_t*	sw = expr.swstate;

			(yyval.expr) = exnewnode(expr.program, (yyvsp[(1) - (8)].id)->index, 1, INTEGER, (yyvsp[(3) - (8)].expr), exnewnode(expr.program, DEFAULT, 1, 0, sw->defcase, sw->firstcase));
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
			if (!(yyvsp[(2) - (3)].expr))
			{
				(yyvsp[(2) - (3)].expr) = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
				(yyvsp[(2) - (3)].expr)->data.constant.value.integer = 1;
			}
			else if (!INTEGRAL((yyvsp[(2) - (3)].expr)->type))
				(yyvsp[(2) - (3)].expr) = excast(expr.program, (yyvsp[(2) - (3)].expr), INTEGER, NiL, 0);
			(yyval.expr) = exnewnode(expr.program, (yyvsp[(1) - (3)].id)->index, 1, INTEGER, (yyvsp[(2) - (3)].expr), NiL);
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
			if ((yyvsp[(2) - (3)].expr))
			{
				if (expr.procedure && !expr.procedure->type)
					exerror("return in void function");
				(yyvsp[(2) - (3)].expr) = excast(expr.program, (yyvsp[(2) - (3)].expr), expr.procedure ? expr.procedure->type : INTEGER, NiL, 0);
			}
			(yyval.expr) = exnewnode(expr.program, RETURN, 1, (yyvsp[(2) - (3)].expr) ? (yyvsp[(2) - (3)].expr)->type : 0, (yyvsp[(2) - (3)].expr), NiL);
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

			(yyval.expr) = exnewnode(expr.program, CASE, 1, 0, (yyvsp[(2) - (2)].expr), NiL);
			if (sw->cur > sw->base)
			{
				if (sw->lastcase)
					sw->lastcase->data.select.next = (yyval.expr);
				else
					sw->firstcase = (yyval.expr);
				sw->lastcase = (yyval.expr);
				n = sw->cur - sw->base;
				sw->cur = sw->base;
				(yyval.expr)->data.select.constant = (Extype_t**)exalloc(expr.program, (n + 1) * sizeof(Extype_t*));
				memcpy((yyval.expr)->data.select.constant, sw->base, n * sizeof(Extype_t*));
				(yyval.expr)->data.select.constant[n] = 0;
			}
			else
				(yyval.expr)->data.select.constant = 0;
			if (sw->def)
			{
				sw->def = 0;
				if (sw->defcase)
					exerror("duplicate default in switch");
				else
					sw->defcase = (yyvsp[(2) - (2)].expr);
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
				(yyvsp[(2) - (3)].expr) = excast(expr.program, (yyvsp[(2) - (3)].expr), expr.swstate->type, NiL, 0);
				*expr.swstate->cur++ = &((yyvsp[(2) - (3)].expr)->data.constant.value);
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
			(yyval.integer) = 0;
		}
    break;

  case 34:
/* Line 1787 of yacc.c  */
#line 504 "../../lib/expr/exparse.y"
    {
			(yyval.integer) = 1;
		}
    break;

  case 36:
/* Line 1787 of yacc.c  */
#line 511 "../../lib/expr/exparse.y"
    {
			if ((yyvsp[(3) - (3)].expr))
				(yyval.expr) = (yyvsp[(1) - (3)].expr) ? exnewnode(expr.program, ',', 1, (yyvsp[(3) - (3)].expr)->type, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)) : (yyvsp[(3) - (3)].expr);
		}
    break;

  case 37:
/* Line 1787 of yacc.c  */
#line 517 "../../lib/expr/exparse.y"
    {checkName ((yyvsp[(1) - (1)].id)); expr.id=(yyvsp[(1) - (1)].id);}
    break;

  case 38:
/* Line 1787 of yacc.c  */
#line 518 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = 0;
			if (!(yyvsp[(1) - (4)].id)->type || expr.declare)
				(yyvsp[(1) - (4)].id)->type = expr.declare;
			if ((yyvsp[(4) - (4)].expr) && (yyvsp[(4) - (4)].expr)->op == PROCEDURE)
			{
				(yyvsp[(1) - (4)].id)->lex = PROCEDURE;
				(yyvsp[(1) - (4)].id)->type = (yyvsp[(4) - (4)].expr)->type;
				(yyvsp[(1) - (4)].id)->value = (yyvsp[(4) - (4)].expr);
			}
			else
			{
				(yyvsp[(1) - (4)].id)->lex = DYNAMIC;
				(yyvsp[(1) - (4)].id)->value = exnewnode(expr.program, 0, 0, 0, NiL, NiL);
				if ((yyvsp[(3) - (4)].integer) && !(yyvsp[(1) - (4)].id)->local.pointer)
				{
					Dtdisc_t*	disc;

					if (!(disc = newof(0, Dtdisc_t, 1, 0)))
						exnospace();
					if ((yyvsp[(3) - (4)].integer) == INTEGER) {
						disc->key = offsetof(Exassoc_t, key);
						disc->size = sizeof(Extype_t);
						disc->comparf = (Dtcompar_f)cmpKey;
					}
					else
						disc->key = offsetof(Exassoc_t, name);
					if (!((yyvsp[(1) - (4)].id)->local.pointer = (char*)dtopen(disc, Dtoset)))
						exerror("%s: cannot initialize associative array", (yyvsp[(1) - (4)].id)->name);
					(yyvsp[(1) - (4)].id)->index_type = (yyvsp[(3) - (4)].integer); /* -1 indicates no typechecking */
				}
				if ((yyvsp[(4) - (4)].expr))
				{
					if ((yyvsp[(4) - (4)].expr)->type != (yyvsp[(1) - (4)].id)->type)
					{
						(yyvsp[(4) - (4)].expr)->type = (yyvsp[(1) - (4)].id)->type;
						(yyvsp[(4) - (4)].expr)->data.operand.right = excast(expr.program, (yyvsp[(4) - (4)].expr)->data.operand.right, (yyvsp[(1) - (4)].id)->type, NiL, 0);
					}
					(yyvsp[(4) - (4)].expr)->data.operand.left = exnewnode(expr.program, DYNAMIC, 0, (yyvsp[(1) - (4)].id)->type, NiL, NiL);
					(yyvsp[(4) - (4)].expr)->data.operand.left->data.variable.symbol = (yyvsp[(1) - (4)].id);
					(yyval.expr) = (yyvsp[(4) - (4)].expr);
#if UNUSED
					if (!expr.program->frame && !expr.program->errors)
					{
						expr.assigned++;
						exeval(expr.program, (yyval.expr), NiL);
					}
#endif
				}
				else if (!(yyvsp[(3) - (4)].integer))
					(yyvsp[(1) - (4)].id)->value->data.value = exzero((yyvsp[(1) - (4)].id)->type);
			}
		}
    break;

  case 45:
/* Line 1787 of yacc.c  */
#line 584 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = 0;
		}
    break;

  case 46:
/* Line 1787 of yacc.c  */
#line 588 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = (yyvsp[(2) - (2)].expr);
		}
    break;

  case 47:
/* Line 1787 of yacc.c  */
#line 594 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = 0;
		}
    break;

  case 49:
/* Line 1787 of yacc.c  */
#line 601 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = (yyvsp[(2) - (3)].expr);
		}
    break;

  case 50:
/* Line 1787 of yacc.c  */
#line 605 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = ((yyvsp[(4) - (4)].expr)->type == (yyvsp[(2) - (4)].id)->type) ? (yyvsp[(4) - (4)].expr) : excast(expr.program, (yyvsp[(4) - (4)].expr), (yyvsp[(2) - (4)].id)->type, NiL, 0);
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
			if (!(yyvsp[(1) - (3)].expr)->type)
			{
				if (!(yyvsp[(3) - (3)].expr)->type)
					(yyvsp[(1) - (3)].expr)->type = (yyvsp[(3) - (3)].expr)->type = rel ? STRING : INTEGER;
				else
					(yyvsp[(1) - (3)].expr)->type = (yyvsp[(3) - (3)].expr)->type;
			}
			else if (!(yyvsp[(3) - (3)].expr)->type)
				(yyvsp[(3) - (3)].expr)->type = (yyvsp[(1) - (3)].expr)->type;
			if ((yyvsp[(1) - (3)].expr)->type != (yyvsp[(3) - (3)].expr)->type)
			{
				if ((yyvsp[(1) - (3)].expr)->type == STRING)
					(yyvsp[(1) - (3)].expr) = excast(expr.program, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr)->type, (yyvsp[(3) - (3)].expr), 0);
				else if ((yyvsp[(3) - (3)].expr)->type == STRING)
					(yyvsp[(3) - (3)].expr) = excast(expr.program, (yyvsp[(3) - (3)].expr), (yyvsp[(1) - (3)].expr)->type, (yyvsp[(1) - (3)].expr), 0);
				else if ((yyvsp[(1) - (3)].expr)->type == FLOATING)
					(yyvsp[(3) - (3)].expr) = excast(expr.program, (yyvsp[(3) - (3)].expr), FLOATING, (yyvsp[(1) - (3)].expr), 0);
				else if ((yyvsp[(3) - (3)].expr)->type == FLOATING)
					(yyvsp[(1) - (3)].expr) = excast(expr.program, (yyvsp[(1) - (3)].expr), FLOATING, (yyvsp[(3) - (3)].expr), 0);
			}
			if (!rel)
				rel = ((yyvsp[(1) - (3)].expr)->type == STRING) ? STRING : (((yyvsp[(1) - (3)].expr)->type == UNSIGNED) ? UNSIGNED : (yyvsp[(3) - (3)].expr)->type);
			(yyval.expr) = exnewnode(expr.program, (yyvsp[(2) - (3)].op), 1, rel, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
			if (!expr.program->errors && (yyvsp[(1) - (3)].expr)->op == CONSTANT && (yyvsp[(3) - (3)].expr)->op == CONSTANT)
			{
				(yyval.expr)->data.constant.value = exeval(expr.program, (yyval.expr), NiL);
				(yyval.expr)->binary = 0;
				(yyval.expr)->op = CONSTANT;
				exfreenode(expr.program, (yyvsp[(1) - (3)].expr));
				exfreenode(expr.program, (yyvsp[(3) - (3)].expr));
			}
			else if (!BUILTIN((yyvsp[(1) - (3)].expr)->type) || !BUILTIN((yyvsp[(3) - (3)].expr)->type)) {
				checkBinary(expr.program, (yyvsp[(1) - (3)].expr), (yyval.expr), (yyvsp[(3) - (3)].expr));
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
			if ((yyvsp[(1) - (3)].expr)->type == STRING)
				(yyvsp[(1) - (3)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (yyvsp[(1) - (3)].expr), NiL);
			else if (!BUILTIN((yyvsp[(1) - (3)].expr)->type))
				(yyvsp[(1) - (3)].expr) = excast(expr.program, (yyvsp[(1) - (3)].expr), INTEGER, NiL, 0);
			if ((yyvsp[(3) - (3)].expr)->type == STRING)
				(yyvsp[(3) - (3)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (yyvsp[(3) - (3)].expr), NiL);
			else if (!BUILTIN((yyvsp[(3) - (3)].expr)->type))
				(yyvsp[(3) - (3)].expr) = excast(expr.program, (yyvsp[(3) - (3)].expr), INTEGER, NiL, 0);
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
			if ((yyvsp[(1) - (3)].expr)->op == CONSTANT)
			{
				exfreenode(expr.program, (yyvsp[(1) - (3)].expr));
				(yyval.expr) = (yyvsp[(3) - (3)].expr);
			}
			else
				(yyval.expr) = exnewnode(expr.program, ',', 1, (yyvsp[(3) - (3)].expr)->type, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr));
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
			if (!(yyvsp[(4) - (7)].expr)->type)
			{
				if (!(yyvsp[(7) - (7)].expr)->type)
					(yyvsp[(4) - (7)].expr)->type = (yyvsp[(7) - (7)].expr)->type = INTEGER;
				else
					(yyvsp[(4) - (7)].expr)->type = (yyvsp[(7) - (7)].expr)->type;
			}
			else if (!(yyvsp[(7) - (7)].expr)->type)
				(yyvsp[(7) - (7)].expr)->type = (yyvsp[(4) - (7)].expr)->type;
			if ((yyvsp[(1) - (7)].expr)->type == STRING)
				(yyvsp[(1) - (7)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (yyvsp[(1) - (7)].expr), NiL);
			else if (!INTEGRAL((yyvsp[(1) - (7)].expr)->type))
				(yyvsp[(1) - (7)].expr) = excast(expr.program, (yyvsp[(1) - (7)].expr), INTEGER, NiL, 0);
			if ((yyvsp[(4) - (7)].expr)->type != (yyvsp[(7) - (7)].expr)->type)
			{
				if ((yyvsp[(4) - (7)].expr)->type == STRING || (yyvsp[(7) - (7)].expr)->type == STRING)
					exerror("if statement string type mismatch");
				else if ((yyvsp[(4) - (7)].expr)->type == FLOATING)
					(yyvsp[(7) - (7)].expr) = excast(expr.program, (yyvsp[(7) - (7)].expr), FLOATING, NiL, 0);
				else if ((yyvsp[(7) - (7)].expr)->type == FLOATING)
					(yyvsp[(4) - (7)].expr) = excast(expr.program, (yyvsp[(4) - (7)].expr), FLOATING, NiL, 0);
			}
			if ((yyvsp[(1) - (7)].expr)->op == CONSTANT)
			{
				if ((yyvsp[(1) - (7)].expr)->data.constant.value.integer)
				{
					(yyval.expr) = (yyvsp[(4) - (7)].expr);
					exfreenode(expr.program, (yyvsp[(7) - (7)].expr));
				}
				else
				{
					(yyval.expr) = (yyvsp[(7) - (7)].expr);
					exfreenode(expr.program, (yyvsp[(4) - (7)].expr));
				}
				exfreenode(expr.program, (yyvsp[(1) - (7)].expr));
			}
			else
				(yyval.expr) = exnewnode(expr.program, '?', 1, (yyvsp[(4) - (7)].expr)->type, (yyvsp[(1) - (7)].expr), exnewnode(expr.program, ':', 1, (yyvsp[(4) - (7)].expr)->type, (yyvsp[(4) - (7)].expr), (yyvsp[(7) - (7)].expr)));
		}
    break;

  case 73:
/* Line 1787 of yacc.c  */
#line 782 "../../lib/expr/exparse.y"
    {
		iunary:
			if ((yyvsp[(2) - (2)].expr)->type == STRING)
				(yyvsp[(2) - (2)].expr) = exnewnode(expr.program, S2B, 1, INTEGER, (yyvsp[(2) - (2)].expr), NiL);
			else if (!INTEGRAL((yyvsp[(2) - (2)].expr)->type))
				(yyvsp[(2) - (2)].expr) = excast(expr.program, (yyvsp[(2) - (2)].expr), INTEGER, NiL, 0);
		unary:
			(yyval.expr) = exnewnode(expr.program, (yyvsp[(1) - (2)].op), 1, (yyvsp[(2) - (2)].expr)->type == UNSIGNED ? INTEGER : (yyvsp[(2) - (2)].expr)->type, (yyvsp[(2) - (2)].expr), NiL);
			if ((yyvsp[(2) - (2)].expr)->op == CONSTANT)
			{
				(yyval.expr)->data.constant.value = exeval(expr.program, (yyval.expr), NiL);
				(yyval.expr)->binary = 0;
				(yyval.expr)->op = CONSTANT;
				exfreenode(expr.program, (yyvsp[(2) - (2)].expr));
			}
			else if (!BUILTIN((yyvsp[(2) - (2)].expr)->type)) {
				checkBinary(expr.program, (yyvsp[(2) - (2)].expr), (yyval.expr), 0);
			}
		}
    break;

  case 74:
/* Line 1787 of yacc.c  */
#line 802 "../../lib/expr/exparse.y"
    {
			if ((yyvsp[(2) - (2)].id)->local.pointer == 0)
              			exerror("cannot apply '#' operator to non-array %s", (yyvsp[(2) - (2)].id)->name);
			(yyval.expr) = exnewnode(expr.program, '#', 0, INTEGER, NiL, NiL);
			(yyval.expr)->data.variable.symbol = (yyvsp[(2) - (2)].id);
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
			(yyval.expr) = (yyvsp[(2) - (2)].expr);
		}
    break;

  case 78:
/* Line 1787 of yacc.c  */
#line 821 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, ADDRESS, 0, T((yyvsp[(2) - (2)].expr)->type), (yyvsp[(2) - (2)].expr), NiL);
		}
    break;

  case 79:
/* Line 1787 of yacc.c  */
#line 825 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, ARRAY, 1, T((yyvsp[(1) - (4)].id)->type), call(0, (yyvsp[(1) - (4)].id), (yyvsp[(3) - (4)].expr)), (yyvsp[(3) - (4)].expr));
		}
    break;

  case 80:
/* Line 1787 of yacc.c  */
#line 829 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, FUNCTION, 1, T((yyvsp[(1) - (4)].id)->type), call(0, (yyvsp[(1) - (4)].id), (yyvsp[(3) - (4)].expr)), (yyvsp[(3) - (4)].expr));
#ifdef UNUSED
			if (!expr.program->disc->getf)
				exerror("%s: function references not supported", (yyval.expr)->data.operand.left->data.variable.symbol->name);
			else if (expr.program->disc->reff)
				(*expr.program->disc->reff)(expr.program, (yyval.expr)->data.operand.left, (yyval.expr)->data.operand.left->data.variable.symbol, 0, NiL, EX_CALL, expr.program->disc);
#endif
		}
    break;

  case 81:
/* Line 1787 of yacc.c  */
#line 839 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewsub (expr.program, (yyvsp[(3) - (4)].expr), GSUB);
		}
    break;

  case 82:
/* Line 1787 of yacc.c  */
#line 843 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewsub (expr.program, (yyvsp[(3) - (4)].expr), SUB);
		}
    break;

  case 83:
/* Line 1787 of yacc.c  */
#line 847 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewsubstr (expr.program, (yyvsp[(3) - (4)].expr));
		}
    break;

  case 84:
/* Line 1787 of yacc.c  */
#line 851 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewsplit (expr.program, (yyvsp[(1) - (6)].id)->index, (yyvsp[(5) - (6)].id), (yyvsp[(3) - (6)].expr), NiL);
		}
    break;

  case 85:
/* Line 1787 of yacc.c  */
#line 855 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewsplit (expr.program, (yyvsp[(1) - (8)].id)->index, (yyvsp[(5) - (8)].id), (yyvsp[(3) - (8)].expr), (yyvsp[(7) - (8)].expr));
		}
    break;

  case 86:
/* Line 1787 of yacc.c  */
#line 859 "../../lib/expr/exparse.y"
    {
			if (!INTEGRAL((yyvsp[(3) - (4)].expr)->type))
				(yyvsp[(3) - (4)].expr) = excast(expr.program, (yyvsp[(3) - (4)].expr), INTEGER, NiL, 0);
			(yyval.expr) = exnewnode(expr.program, EXIT, 1, INTEGER, (yyvsp[(3) - (4)].expr), NiL);
		}
    break;

  case 87:
/* Line 1787 of yacc.c  */
#line 865 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, RAND, 0, FLOATING, NiL, NiL);
		}
    break;

  case 88:
/* Line 1787 of yacc.c  */
#line 869 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, SRAND, 0, INTEGER, NiL, NiL);
		}
    break;

  case 89:
/* Line 1787 of yacc.c  */
#line 873 "../../lib/expr/exparse.y"
    {
			if (!INTEGRAL((yyvsp[(3) - (4)].expr)->type))
				(yyvsp[(3) - (4)].expr) = excast(expr.program, (yyvsp[(3) - (4)].expr), INTEGER, NiL, 0);
			(yyval.expr) = exnewnode(expr.program, SRAND, 1, INTEGER, (yyvsp[(3) - (4)].expr), NiL);
		}
    break;

  case 90:
/* Line 1787 of yacc.c  */
#line 879 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, CALL, 1, (yyvsp[(1) - (4)].id)->type, NiL, (yyvsp[(3) - (4)].expr));
			(yyval.expr)->data.call.procedure = (yyvsp[(1) - (4)].id);
		}
    break;

  case 91:
/* Line 1787 of yacc.c  */
#line 884 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exprint(expr.program, (yyvsp[(1) - (4)].id), (yyvsp[(3) - (4)].expr));
		}
    break;

  case 92:
/* Line 1787 of yacc.c  */
#line 888 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, (yyvsp[(1) - (4)].id)->index, 0, (yyvsp[(1) - (4)].id)->type, NiL, NiL);
			if ((yyvsp[(3) - (4)].expr) && (yyvsp[(3) - (4)].expr)->data.operand.left->type == INTEGER)
			{
				(yyval.expr)->data.print.descriptor = (yyvsp[(3) - (4)].expr)->data.operand.left;
				(yyvsp[(3) - (4)].expr) = (yyvsp[(3) - (4)].expr)->data.operand.right;
			}
			else 
				switch ((yyvsp[(1) - (4)].id)->index)
				{
				case QUERY:
					(yyval.expr)->data.print.descriptor = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
					(yyval.expr)->data.print.descriptor->data.constant.value.integer = 2;
					break;
				case PRINTF:
					(yyval.expr)->data.print.descriptor = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
					(yyval.expr)->data.print.descriptor->data.constant.value.integer = 1;
					break;
				case SPRINTF:
					(yyval.expr)->data.print.descriptor = 0;
					break;
				}
			(yyval.expr)->data.print.args = preprint((yyvsp[(3) - (4)].expr));
		}
    break;

  case 93:
/* Line 1787 of yacc.c  */
#line 913 "../../lib/expr/exparse.y"
    {
			register Exnode_t*	x;

			(yyval.expr) = exnewnode(expr.program, (yyvsp[(1) - (4)].id)->index, 0, (yyvsp[(1) - (4)].id)->type, NiL, NiL);
			if ((yyvsp[(3) - (4)].expr) && (yyvsp[(3) - (4)].expr)->data.operand.left->type == INTEGER)
			{
				(yyval.expr)->data.scan.descriptor = (yyvsp[(3) - (4)].expr)->data.operand.left;
				(yyvsp[(3) - (4)].expr) = (yyvsp[(3) - (4)].expr)->data.operand.right;
			}
			else 
				switch ((yyvsp[(1) - (4)].id)->index)
				{
				case SCANF:
					(yyval.expr)->data.scan.descriptor = 0;
					break;
				case SSCANF:
					if ((yyvsp[(3) - (4)].expr) && (yyvsp[(3) - (4)].expr)->data.operand.left->type == STRING)
					{
						(yyval.expr)->data.scan.descriptor = (yyvsp[(3) - (4)].expr)->data.operand.left;
						(yyvsp[(3) - (4)].expr) = (yyvsp[(3) - (4)].expr)->data.operand.right;
					}
					else
						exerror("%s: string argument expected", (yyvsp[(1) - (4)].id)->name);
					break;
				}
			if (!(yyvsp[(3) - (4)].expr) || !(yyvsp[(3) - (4)].expr)->data.operand.left || (yyvsp[(3) - (4)].expr)->data.operand.left->type != STRING)
				exerror("%s: format argument expected", (yyvsp[(1) - (4)].id)->name);
			(yyval.expr)->data.scan.format = (yyvsp[(3) - (4)].expr)->data.operand.left;
			for (x = (yyval.expr)->data.scan.args = (yyvsp[(3) - (4)].expr)->data.operand.right; x; x = x->data.operand.right)
			{
				if (x->data.operand.left->op != ADDRESS)
					exerror("%s: address argument expected", (yyvsp[(1) - (4)].id)->name);
				x->data.operand.left = x->data.operand.left->data.operand.left;
			}
		}
    break;

  case 94:
/* Line 1787 of yacc.c  */
#line 949 "../../lib/expr/exparse.y"
    {
			if ((yyvsp[(2) - (2)].expr))
			{
				if ((yyvsp[(1) - (2)].expr)->op == ID && !expr.program->disc->setf)
					exerror("%s: variable assignment not supported", (yyvsp[(1) - (2)].expr)->data.variable.symbol->name);
				else
				{
					if (!(yyvsp[(1) - (2)].expr)->type)
						(yyvsp[(1) - (2)].expr)->type = (yyvsp[(2) - (2)].expr)->type;
#if 0
					else if ((yyvsp[(2) - (2)].expr)->type != (yyvsp[(1) - (2)].expr)->type && (yyvsp[(1) - (2)].expr)->type >= 0200)
#else
					else if ((yyvsp[(2) - (2)].expr)->type != (yyvsp[(1) - (2)].expr)->type)
#endif
					{
						(yyvsp[(2) - (2)].expr)->type = (yyvsp[(1) - (2)].expr)->type;
						(yyvsp[(2) - (2)].expr)->data.operand.right = excast(expr.program, (yyvsp[(2) - (2)].expr)->data.operand.right, (yyvsp[(1) - (2)].expr)->type, NiL, 0);
					}
					(yyvsp[(2) - (2)].expr)->data.operand.left = (yyvsp[(1) - (2)].expr);
					(yyval.expr) = (yyvsp[(2) - (2)].expr);
				}
			}
		}
    break;

  case 95:
/* Line 1787 of yacc.c  */
#line 973 "../../lib/expr/exparse.y"
    {
		pre:
			if ((yyvsp[(2) - (2)].expr)->type == STRING)
				exerror("++ and -- invalid for string variables");
			(yyval.expr) = exnewnode(expr.program, (yyvsp[(1) - (2)].op), 0, (yyvsp[(2) - (2)].expr)->type, (yyvsp[(2) - (2)].expr), NiL);
			(yyval.expr)->subop = PRE;
		}
    break;

  case 96:
/* Line 1787 of yacc.c  */
#line 981 "../../lib/expr/exparse.y"
    {
		pos:
			if ((yyvsp[(1) - (2)].expr)->type == STRING)
				exerror("++ and -- invalid for string variables");
			(yyval.expr) = exnewnode(expr.program, (yyvsp[(2) - (2)].op), 0, (yyvsp[(1) - (2)].expr)->type, (yyvsp[(1) - (2)].expr), NiL);
			(yyval.expr)->subop = POS;
		}
    break;

  case 97:
/* Line 1787 of yacc.c  */
#line 989 "../../lib/expr/exparse.y"
    {
			if ((yyvsp[(3) - (3)].id)->local.pointer == 0)
              			exerror("cannot apply IN to non-array %s", (yyvsp[(3) - (3)].id)->name);
			if (((yyvsp[(3) - (3)].id)->index_type > 0) && ((yyvsp[(1) - (3)].expr)->type != (yyvsp[(3) - (3)].id)->index_type))
            		    exerror("%s indices must have type %s, not %s", 
				(yyvsp[(3) - (3)].id)->name, extypename(expr.program, (yyvsp[(3) - (3)].id)->index_type),extypename(expr.program, (yyvsp[(1) - (3)].expr)->type));
			(yyval.expr) = exnewnode(expr.program, IN_OP, 0, INTEGER, NiL, NiL);
			(yyval.expr)->data.variable.symbol = (yyvsp[(3) - (3)].id);
			(yyval.expr)->data.variable.index = (yyvsp[(1) - (3)].expr);
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
			(yyval.expr) = exnewnode(expr.program, CONSTANT, 0, (yyvsp[(1) - (1)].id)->type, NiL, NiL);
			if (!expr.program->disc->reff)
				exerror("%s: identifier references not supported", (yyvsp[(1) - (1)].id)->name);
			else
				(yyval.expr)->data.constant.value = (*expr.program->disc->reff)(expr.program, (yyval.expr), (yyvsp[(1) - (1)].id), NiL, NiL, EX_SCALAR, expr.program->disc);
		}
    break;

  case 104:
/* Line 1787 of yacc.c  */
#line 1022 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, CONSTANT, 0, FLOATING, NiL, NiL);
			(yyval.expr)->data.constant.value.floating = (yyvsp[(1) - (1)].floating);
		}
    break;

  case 105:
/* Line 1787 of yacc.c  */
#line 1027 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, CONSTANT, 0, INTEGER, NiL, NiL);
			(yyval.expr)->data.constant.value.integer = (yyvsp[(1) - (1)].integer);
		}
    break;

  case 106:
/* Line 1787 of yacc.c  */
#line 1032 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, CONSTANT, 0, STRING, NiL, NiL);
			(yyval.expr)->data.constant.value.string = (yyvsp[(1) - (1)].string);
		}
    break;

  case 107:
/* Line 1787 of yacc.c  */
#line 1037 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, CONSTANT, 0, UNSIGNED, NiL, NiL);
			(yyval.expr)->data.constant.value.integer = (yyvsp[(1) - (1)].integer);
		}
    break;

  case 113:
/* Line 1787 of yacc.c  */
#line 1053 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = makeVar(expr.program, (yyvsp[(1) - (2)].id), 0, 0, (yyvsp[(2) - (2)].reference));
		}
    break;

  case 114:
/* Line 1787 of yacc.c  */
#line 1057 "../../lib/expr/exparse.y"
    {
			Exnode_t*   n;

			n = exnewnode(expr.program, DYNAMIC, 0, (yyvsp[(1) - (3)].id)->type, NiL, NiL);
			n->data.variable.symbol = (yyvsp[(1) - (3)].id);
			n->data.variable.reference = 0;
			if (((n->data.variable.index = (yyvsp[(2) - (3)].expr)) == 0) != ((yyvsp[(1) - (3)].id)->local.pointer == 0))
				exerror("%s: is%s an array", (yyvsp[(1) - (3)].id)->name, (yyvsp[(1) - (3)].id)->local.pointer ? "" : " not");
			if ((yyvsp[(1) - (3)].id)->local.pointer && ((yyvsp[(1) - (3)].id)->index_type > 0)) {
				if ((yyvsp[(2) - (3)].expr)->type != (yyvsp[(1) - (3)].id)->index_type)
					exerror("%s: indices must have type %s, not %s", 
						(yyvsp[(1) - (3)].id)->name, extypename(expr.program, (yyvsp[(1) - (3)].id)->index_type),extypename(expr.program, (yyvsp[(2) - (3)].expr)->type));
			}
			if ((yyvsp[(3) - (3)].reference)) {
				n->data.variable.dyna =exnewnode(expr.program, 0, 0, 0, NiL, NiL);
				(yyval.expr) = makeVar(expr.program, (yyvsp[(1) - (3)].id), (yyvsp[(2) - (3)].expr), n, (yyvsp[(3) - (3)].reference));
			}
			else (yyval.expr) = n;
		}
    break;

  case 115:
/* Line 1787 of yacc.c  */
#line 1077 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, ID, 0, STRING, NiL, NiL);
			(yyval.expr)->data.variable.symbol = (yyvsp[(1) - (1)].id);
			(yyval.expr)->data.variable.reference = 0;
			(yyval.expr)->data.variable.index = 0;
			(yyval.expr)->data.variable.dyna = 0;
			if (!(expr.program->disc->flags & EX_UNDECLARED))
				exerror("unknown identifier");
		}
    break;

  case 116:
/* Line 1787 of yacc.c  */
#line 1089 "../../lib/expr/exparse.y"
    {
			(yyval.integer) = 0;
		}
    break;

  case 117:
/* Line 1787 of yacc.c  */
#line 1093 "../../lib/expr/exparse.y"
    {
			(yyval.integer) = -1;
		}
    break;

  case 118:
/* Line 1787 of yacc.c  */
#line 1097 "../../lib/expr/exparse.y"
    {
			/* If DECLARE is VOID, its type is 0, so this acts like
			 * the empty case.
			 */
			if (INTEGRAL((yyvsp[(2) - (3)].id)->type))
				(yyval.integer) = INTEGER;
			else
				(yyval.integer) = (yyvsp[(2) - (3)].id)->type;
				
		}
    break;

  case 119:
/* Line 1787 of yacc.c  */
#line 1110 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = 0;
		}
    break;

  case 120:
/* Line 1787 of yacc.c  */
#line 1114 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = (yyvsp[(2) - (3)].expr);
		}
    break;

  case 121:
/* Line 1787 of yacc.c  */
#line 1120 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = 0;
		}
    break;

  case 122:
/* Line 1787 of yacc.c  */
#line 1124 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = (yyvsp[(1) - (1)].expr)->data.operand.left;
			(yyvsp[(1) - (1)].expr)->data.operand.left = (yyvsp[(1) - (1)].expr)->data.operand.right = 0;
			exfreenode(expr.program, (yyvsp[(1) - (1)].expr));
		}
    break;

  case 123:
/* Line 1787 of yacc.c  */
#line 1132 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, ',', 1, 0, exnewnode(expr.program, ',', 1, (yyvsp[(1) - (1)].expr)->type, (yyvsp[(1) - (1)].expr), NiL), NiL);
			(yyval.expr)->data.operand.right = (yyval.expr)->data.operand.left;
		}
    break;

  case 124:
/* Line 1787 of yacc.c  */
#line 1137 "../../lib/expr/exparse.y"
    {
			(yyvsp[(1) - (3)].expr)->data.operand.right = (yyvsp[(1) - (3)].expr)->data.operand.right->data.operand.right = exnewnode(expr.program, ',', 1, (yyvsp[(1) - (3)].expr)->type, (yyvsp[(3) - (3)].expr), NiL);
		}
    break;

  case 125:
/* Line 1787 of yacc.c  */
#line 1143 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = 0;
		}
    break;

  case 126:
/* Line 1787 of yacc.c  */
#line 1147 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = 0;
			if ((yyvsp[(1) - (1)].id)->type)
				exerror("(void) expected");
		}
    break;

  case 128:
/* Line 1787 of yacc.c  */
#line 1156 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, ',', 1, (yyvsp[(1) - (1)].expr)->type, (yyvsp[(1) - (1)].expr), NiL);
		}
    break;

  case 129:
/* Line 1787 of yacc.c  */
#line 1160 "../../lib/expr/exparse.y"
    {
			register Exnode_t*	x;
			register Exnode_t*	y;

			(yyval.expr) = (yyvsp[(1) - (3)].expr);
			for (x = (yyvsp[(1) - (3)].expr); (y = x->data.operand.right); x = y);
			x->data.operand.right = exnewnode(expr.program, ',', 1, (yyvsp[(3) - (3)].expr)->type, (yyvsp[(3) - (3)].expr), NiL);
		}
    break;

  case 130:
/* Line 1787 of yacc.c  */
#line 1170 "../../lib/expr/exparse.y"
    {expr.declare=(yyvsp[(1) - (1)].id)->type;}
    break;

  case 131:
/* Line 1787 of yacc.c  */
#line 1171 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, ID, 0, (yyvsp[(1) - (3)].id)->type, NiL, NiL);
			(yyval.expr)->data.variable.symbol = (yyvsp[(3) - (3)].id);
			(yyvsp[(3) - (3)].id)->lex = DYNAMIC;
			(yyvsp[(3) - (3)].id)->type = (yyvsp[(1) - (3)].id)->type;
			(yyvsp[(3) - (3)].id)->value = exnewnode(expr.program, 0, 0, 0, NiL, NiL);
			expr.procedure->data.procedure.arity++;
			expr.declare = 0;
		}
    break;

  case 132:
/* Line 1787 of yacc.c  */
#line 1183 "../../lib/expr/exparse.y"
    {
			(yyval.reference) = expr.refs = expr.lastref = 0;
		}
    break;

  case 133:
/* Line 1787 of yacc.c  */
#line 1187 "../../lib/expr/exparse.y"
    {
			Exref_t*	r;

			r = ALLOCATE(expr.program, Exref_t);
			r->symbol = (yyvsp[(1) - (1)].id);
			expr.refs = r;
			expr.lastref = r;
			r->next = 0;
			r->index = 0;
			(yyval.reference) = expr.refs;
		}
    break;

  case 134:
/* Line 1787 of yacc.c  */
#line 1199 "../../lib/expr/exparse.y"
    {
			Exref_t*	r;
			Exref_t*	l;

			r = ALLOCATE(expr.program, Exref_t);
			r->symbol = (yyvsp[(3) - (3)].id);
			r->index = 0;
			r->next = 0;
			l = ALLOCATE(expr.program, Exref_t);
			l->symbol = (yyvsp[(2) - (3)].id);
			l->index = 0;
			l->next = r;
			expr.refs = l;
			expr.lastref = r;
			(yyval.reference) = expr.refs;
		}
    break;

  case 135:
/* Line 1787 of yacc.c  */
#line 1218 "../../lib/expr/exparse.y"
    {
			(yyval.id) = (yyvsp[(2) - (2)].id);
		}
    break;

  case 136:
/* Line 1787 of yacc.c  */
#line 1222 "../../lib/expr/exparse.y"
    {
			(yyval.id) = (yyvsp[(2) - (2)].id);
		}
    break;

  case 137:
/* Line 1787 of yacc.c  */
#line 1227 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = 0;
		}
    break;

  case 138:
/* Line 1787 of yacc.c  */
#line 1231 "../../lib/expr/exparse.y"
    {
			(yyval.expr) = exnewnode(expr.program, '=', 1, (yyvsp[(2) - (2)].expr)->type, NiL, (yyvsp[(2) - (2)].expr));
			(yyval.expr)->subop = (yyvsp[(1) - (2)].op);
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
			(yyval.expr) = expr.procedure;
			expr.procedure = 0;
			if (expr.program->frame)
			{
				expr.program->symbols = expr.program->frame->view;
				dtview(expr.program->frame, NiL);
				expr.program->frame = 0;
			}
			(yyval.expr)->data.operand.left = (yyvsp[(3) - (8)].expr);
			(yyval.expr)->data.operand.right = excast(expr.program, (yyvsp[(7) - (8)].expr), (yyval.expr)->type, NiL, 0);

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
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


/* Line 2048 of yacc.c  */
#line 1283 "../../lib/expr/exparse.y"


#include "exgram.h"

