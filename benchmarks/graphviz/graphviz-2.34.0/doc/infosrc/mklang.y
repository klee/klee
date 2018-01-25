/* vim:set shiftwidth=4 ts=8: */

%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct _term term_t;
struct _term {
  int kind;
  union {
    char c;
    char* s;
    term_t* t;
  } u;
  term_t* next;
};

static FILE* outf;
static int yylex ();
static void yyerror (char*);
static void xlate (term_t* ts);

static void
nonTerm (char* s)
{
    fprintf (outf, "<I>%s</I>", s);
}

static void
gen (char* name, term_t* ts)
{
    fprintf (outf, "<TR>\n  <TD ALIGN=RIGHT>");
    nonTerm (name);
    fprintf (outf, "</TD>\n  <TD ALIGN=LEFT>:</TD>\n  <TD ALIGN=LEFT>");
    xlate (ts);
    fprintf (outf, "</TD>\n</TR>\n");

    for (ts = ts->next; ts; ts = ts->next) {
	fprintf (outf, "<TR>\n  <TD ALIGN=RIGHT></TD>\n  <TD ALIGN=LEFT>|</TD>\n  <TD ALIGN=LEFT>");
	xlate (ts);
	fprintf (outf, "</TD>\n</TR>\n");
    }
}

static term_t*
mkLiteral (char c, int kind)
{
    term_t* nt = (term_t*)malloc(sizeof(term_t));
    nt->kind = kind;
    nt->u.c = c;
    nt->next = 0;
    return nt;
}

static term_t*
mkID (char* s, int kind)
{
    term_t* nt = (term_t*)malloc(sizeof(term_t));
    nt->kind = kind;
    nt->u.s = s;
    nt->next = 0;
    return nt;
}

static term_t*
mkSeq (term_t* t1, term_t* t2, int kind)
{
    term_t* nt = (term_t*)malloc(sizeof(term_t));
    nt->kind = kind;
    nt->u.t = t1;
    t1->next = t2;
    nt->next = 0;
    return nt;
}

static term_t*
mkTerm (term_t* t, int kind)
{
    term_t* nt = (term_t*)malloc(sizeof(term_t));
    nt->kind = kind;
    nt->u.t = t;
    nt->next = 0;
    return nt;
}

%}

%union {
  char c;
  char* str;
  term_t* term;
}

%token <c> T_literal
%token <str> T_name T_token T_opt T_seq T_choice
%type <term> atomterm term rule rulelist
%start prods

%%

prods : prods prod
      | prod
      ;

prod : T_name '=' rulelist '\n'
    { gen ($1, $3); }

rulelist : rule '|' rulelist { $1->next = $3; $$ = $1; }
         | rule { $$ = $1; }
         ;

rule : term rule 
      { if ($2->kind == T_seq) { $1->next = $2->u.t; $2->u.t = $1; $$ = $2;}
        else $$ = mkSeq ($1, $2, T_seq); }
     | term { $$ = $1; }
     ;

term : '[' rule ']' { $$ = mkTerm ($2, T_opt); }
     | '(' rulelist ')' { $$ = mkTerm ($2, T_choice); }
     | atomterm { $$ = $1; }
     ;

atomterm : T_literal { $$ = mkLiteral($1, T_literal); }
         | T_token { $$ = mkID($1, T_token); }
         | T_name { $$ = mkID($1, T_name); }
         ;

%%

static void
xlate (term_t* ts)
{
    term_t* t;

    switch (ts->kind) {
    case T_name :
	nonTerm (ts->u.s);
	break;
    case T_literal :
	fprintf (outf, "<B>'%c'</B>", ts->u.c);
	break;
    case T_token :
	fprintf (outf, "<B>%s</B>", ts->u.s);
	break;
    case T_opt :
	fprintf (outf, "[ ");
	xlate (ts->u.t);
	fprintf (outf, " ]");
	break;
    case T_seq :
	t = ts->u.t;
	xlate (t);
	for (t = t->next; t; t = t->next) {
	    fprintf (outf, " ");
	    xlate (t);
	}
	break;
    case T_choice :
	fprintf (outf, "(");
	t = ts->u.t;
	xlate (t);
	for (t = t->next; t; t = t->next) {
	    fprintf (outf, " | ");
	    xlate (t);
	}
	fprintf (outf, ")");
	break;
    }
}

#define BSIZE 2048

static FILE* inf;
static char buf[BSIZE];
static char* lexptr;
static int lineno;

static char*
skipSpace (char* p)
{
    int c;
    while (isspace ((c = *p)) && (c != '\n')) p++;
    return p;
}

static char*
mystrndup (char* p, int sz)
{
    char* s = malloc (sz+1);
    memcpy (s, p, sz);
    s[sz] = '\0';
    return s;
}

static char*
readLiteral (char* p)
{
    int c;
    char* s = p;

    while (((c = *p) != '\'') && (c != '\0')) p++;
    if (c == '\0') {
	fprintf (stderr, "Unclosed literal '%s, line %d\n", s, lineno);
	exit (1);
    }
    yylval.c = *s;
    return (p+1);
}

static char*
readName (char* p)
{
    int c;
    char* s = p;

    while (!isspace ((c = *p)) && (c != '\0')) p++;
    yylval.str = mystrndup (s, p-s);
    return p;
}

static void
yyerror (char* msg)
{
    fprintf (stderr, "%s, line %d\n", msg, lineno);  
}

static void
lexinit ()
{
    lexptr = buf;
}

#ifdef DEBUG
static int _yylex ();
int yylex()
{                               /* for debugging */
    int rv = _yylex();
    fprintf(stderr, "returning %d\n", rv);
    switch (rv) {
    case T_name :
    case T_token :
        fprintf(stderr, "string val is '%s'\n", yylval.str);
        break;
    case T_literal :
        fprintf(stderr, "string val is '%c'\n", yylval.c);
        break;
    }
    return rv;
}
#define yylex _yylex
#endif

static int
yylex ()
{
    int c;
    do {    
	if (*lexptr == '\0') {
            if (!fgets (buf, BSIZE, inf)) return EOF;
	    lineno++;
            lexptr = buf;
	}
	lexptr = skipSpace (lexptr);
    } while (*lexptr == '\0');
    
    switch (c = *lexptr++) {
    case '\n' :
    case '|' :
    case '=' :
    case '(' :
    case ')' :
    case '[' :
    case ']' :
	return c;
	break;
    case '\'' :
	lexptr = readLiteral (lexptr);
	return T_literal;
	break;
    case 'T' :
	if (*lexptr == '_') {
	    lexptr = readName (lexptr+1);
	    return T_token;
	}
	else {
	    lexptr = readName (lexptr-1);
	    return T_name;
	}
	break;
    default :
	lexptr = readName (lexptr-1);
	return T_name;
	break;
    }
}
#ifdef DEBUG
#undef yylex
#endif

static FILE*
openF (char* fname, char* mode)
{
  FILE* f = fopen (fname, mode);

  if (!f) {
    fprintf (stderr, "Could not open %s for %s\n", fname, mode);
    exit(1);
  }
  return f;    
}

main (int argc, char* argv[])
{
  if (argc != 3) {
    fprintf (stderr, "mklang: 2 arguments required\n");
    exit(1);
  }
  inf = openF (argv[1], "r");
  outf = openF (argv[2], "w");
  lexinit();
  yyparse ();
  exit (0);
}
