/* vim:set shiftwidth=4 ts=8: */

%{
#include <stdlib.h>
#include <gml2gv.h>
#include <gmlparse.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef HAVE_UNISTD_H
#define YY_NO_UNISTD_H
#endif

#define GRAPH_EOF_TOKEN     '@'     /* lex class must be defined below */

static int line_num = 1;
static int errors;
static FILE* Ifile;

void initgmlscan(FILE *ifile) 
{ 
    if (ifile) {
	Ifile = ifile; 
	line_num = 1;
    }
    errors = 0;
}

#ifndef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
        if ((result = fread(buf, 1, max_size, Ifile)) < 0) \
                YY_FATAL_ERROR( "input in flex scanner failed" )
#endif

/* buffer for arbitrary length strings (longer than BUFSIZ) */
static char     *Sbuf,*Sptr,*Send;
static void beginstr(void) 
{
    if (Sbuf == NULL) {
	Sbuf = malloc(BUFSIZ);
	Send = Sbuf + BUFSIZ;
    }
    Sptr = Sbuf;
    *Sptr = 0;
}

static void addstr(char *src) 
{
    char    c;
    if (Sptr > Sbuf) Sptr--;
    do {
	do {c = *Sptr++ = *src++;} while (c && (Sptr < Send));
	if (c) {
	    long    sz = Send - Sbuf;
	    long    off = Sptr - Sbuf;
	    sz *= 2;
	    Sbuf = (char*)realloc(Sbuf,sz);
	    Send = Sbuf + sz;
	    Sptr = Sbuf + off;
	}
    } while (c);
}

static void endstr(void) {
    yylval.str = strdup(Sbuf);
}

%}
GRAPH_EOF_TOKEN                         [@]
ASCII     [ !#$%\047-\177]
DIGIT     [0-9]
L_INT     [-+]?{DIGIT}+
MANTISSA  E[-+]?{DIGIT}
L_REAL    [-+]?{DIGIT}*\.{DIGIT}*{MANTISSA}?
L_ID      [a-zA-Z_][_a-zA-Z0-9]*
%x qstring
%%
{GRAPH_EOF_TOKEN}           return(EOF);
<INITIAL,qstring>\n         line_num++;
^"#".*                      /* ignore # line */
[ \t\r]                     /* ignore whitespace */

"graph"	                    return (GRAPH);
"node"                      return (NODE);
"edge"                      return (EDGE);
"directed"                  return (DIRECTED);
"id"                        return (ID);
"source"                    return (SOURCE);
"target"                    return (TARGET);
"x"                         return (XVAL);
"y"                         return (YVAL);
"w"                         return (WVAL);
"h"                         return (HVAL);
"label"                     return (LABEL);
"graphics"                  return (GRAPHICS);;
"LabelGraphics"             return (LABELGRAPHICS);
"type"                      return (TYPE);
"fill"                      return (FILL);
"outline"                   return (OUTLINE);
"outlineStyle"              return (OUTLINESTYLE);
"outlineWidth"              return (OUTLINEWIDTH);
"width"                     return (WIDTH);
"style"                     return (STYLE);
"Line"                      return (LINE);
"point"                     return (POINT);
"text"                      return (TEXT);
"fontSize"                  return (FONTSIZE);
"fontName"                  return (FONTNAME);
"color"                     return (COLOR);
{L_INT}                     { yylval.str = strdup(yytext); return (INTEGER); }
{L_REAL}                    { yylval.str = strdup(yytext); return (REAL); }
{L_ID}                      { yylval.str = strdup(yytext); return (NAME); }
["]                         BEGIN(qstring); beginstr();

<qstring>["]                BEGIN(INITIAL); endstr(); return (STRING);
<qstring>([^"]+)            addstr(yytext);

.                           return (yytext[0]);

%%

void yyerror(char *str)
{
    char    buf[BUFSIZ];
    if (errors)
	return;
    errors = 1;
    sprintf(buf," %s in line %d near '%s'\n", str,line_num,yytext);
    agerr(AGWARN,buf);
}

int gmlerrors()
{
    return errors;
}
 
void gmllexeof() { unput(GRAPH_EOF_TOKEN); }

#ifndef YY_CALL_ONLY_ARG
# define YY_CALL_ONLY_ARG void
#endif

int yywrap(YY_CALL_ONLY_ARG)
{
    return 1;
}

