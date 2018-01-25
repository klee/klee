/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/


#include <stdarg.h>
#include <stdlib.h>
#include "libgraph.h"
#include "parser.h"
#include "triefa.cP"
#include "agxbuf.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define InfileName (InputFile?InputFile:"<unknown>")

static FILE *Lexer_fp;
static char *LexPtr, *TokenBuf;
static int LineBufSize;
static unsigned char In_comment;
static unsigned char Comment_start;
static unsigned char Start_html_string;
int Line_number;
static char *InputFile;
static int agmaxerr;

static void
storeFileName (char* fname, int len)
{
    static int cnt;
    static char* buf;

    if (len > cnt) {
	if (cnt) buf = (char*)realloc (buf, len+1);
	else buf = (char*)malloc (len+1);
	cnt = len;
    }
    strcpy (buf, fname);
    InputFile = buf;
}

  /* Reset line number.
   * Argument n is indexed from 1, so we decrement it.
   */
void agreadline(int n)
{
    Line_number = n - 1;
}

int aglinenumber ()
{
    return Line_number;
}

  /* (Re)set file:
   */
void agsetfile(char *f)
{
    InputFile = f;
    Line_number = 0;
}

void aglexinit(FILE * fp, gets_f mygets)
{
    if (Lexer_fp != fp)
	LexPtr = NULL;
    Lexer_fp = fp;
    if (mygets)
        AG.fgets = mygets;
    if (AG.fgets == NULL)
	AG.fgets = fgets;
    if (AG.linebuf == NULL) {
	LineBufSize = BUFSIZ;
	AG.linebuf = N_NEW(LineBufSize, char);
	TokenBuf = N_NEW(LineBufSize, char);
    }
    AG.fgets (AG.linebuf, 0, fp);	/* reset mygets */
    AG.syntax_errors = 0;
}

#define ISSPACE(c) ((c != 0) && ((isspace(c) || iscntrl(c))))

/* skip leading white space and comments in a string p
 * whitespace includes control characters
 */
static char *skip_wscomments(char *pp)
{
    unsigned char *p = (unsigned char *) pp;
    do {
	while (ISSPACE(*p))
	    p++;
	while (In_comment && p[0]) {
	    while (p[0] && (p[0] != '*'))
		p++;
	    if (p[0]) {
		if (p[1] == '/') {
		    In_comment = FALSE;
		    p += 2;
		    break;
		} else
		    p++;
	    }
	}
	if (p[0] == '/') {
	    if (p[1] == '/')
		while (*p)
		    p++;	/* skip to end of line */
	    else {
		if (p[1] == '*') {
		    In_comment = TRUE;
		    Comment_start = Line_number;
		    p += 2;
		    continue;
		} else
		    break;	/* return a slash */
	    }
	} else {
	    if (!ISSPACE(*p))
		break;
	}
    } while (p[0]);
    return (char *) p;
}

/* scan an unquoted token and return the position after its terminator */
static char *scan_token(unsigned char *p, unsigned char *token)
{
    unsigned char *q;

    q = token;
    if (p == '\0')
	return NULL;
    while (ISALNUM(*p)) {
	*q++ = *p++;
    }
    *q = '\0';
    return p;
}

static char *scan_num(char *p, char *token)
{
    unsigned char *q, *z;
    int saw_rp = FALSE;
    int saw_digit = FALSE;

    z = (unsigned char *) p;
    q = (unsigned char *) token;
    if (*z == '-')
	*q++ = *z++;
    if (*z == '.') {
	saw_rp = TRUE;
	*q++ = *z++;
    }
    while (isdigit(*z)) {
	saw_digit = TRUE;
	*q++ = *z++;
    }
    if ((*z == '.') && (saw_rp == FALSE)) {
	saw_rp = TRUE;
	*q++ = *z++;
	while (isdigit(*z)) {
	    saw_digit = TRUE;
	    *q++ = *z++;
	}
    }
    *q = '\0';
    if (saw_digit && *z && ((isalpha(*z)) || (*z == '_'))) {
	unsigned char *endp = z + 1;
	unsigned char c;
	while ((c = *endp) && ((isalpha(c)) || (c == '_')))
	    endp++;
	*endp = '\0';
	agerr(AGWARN,
	      "%s:%d: ambiguous \"%s\" splits into two names: \"%s\" and \"%s\"\n",
	      InfileName, Line_number, p, token, z);
	*endp = c;
    }

    if (saw_digit == FALSE)
	z = NULL;
    return (char *) z;
}

/* scan a quoted string and return the position after its terminator */
static char *quoted_string(char *p, char *token)
{
    char quote, *q;

    quote = *p++;
    q = token;
    while ((*p) && (*p != quote)) {
	if (*p == '\\') {
	    if (*(p + 1) == quote)
		p++;
	    else {
		if (*(p + 1) == '\\')
		    *q++ = *p++;
	    }
	}
	*q++ = *p++;
    }
    if (*p == '\0')
	agerr(AGWARN, "%s:%d: string ran past end of line\n",
	      InfileName, Line_number);
    else
	p++;
    *q = 0;
    return p;
}

int myaglex(void)
{				/* for debugging */
    int rv = aglex();
    fprintf(stderr, "returning %d\n", rv);
    if (rv == T_symbol)
	fprintf(stderr, "string val is %s\n", aglval.str);
    return rv;
}

/*
 * Return a logical line in AG.linebuf.
 * In particular, the buffer will contain a '\n' as the last non-null char.
 * Ignore lines beginning with '#'; update cpp line number if applicable.
 * Fold long lines, i.e., ignore escaped newlines.
 * Assume the AG.fgets function reads upto newline or buffer length
 * like fgets.
 * Need to be careful that AG.fgets might not return full physical line
 * because buffer is too small to hold it.
 */
static char *lex_gets(void)
{
    char *clp;
    int len, curlen;

    len = curlen = 0;

    do {
	/* make sure there is room for at least another SMALLBUF worth */
	if (curlen + SMALLBUF >= LineBufSize) {
	    LineBufSize += BUFSIZ;
	    AG.linebuf = (char*)realloc(AG.linebuf, LineBufSize);
	    TokenBuf = (char*)realloc(TokenBuf, LineBufSize);
	}

	/* off by one so we can back up in LineBuf */
	clp = AG.fgets (AG.linebuf + curlen + 1,
			  LineBufSize - curlen - 1, Lexer_fp);
	if (clp == NULL)
	    break;


	len = strlen(clp);	/* since clp != NULL, len > 0 */
	if (clp[len - 1] == '\n') {	/* have physical line */
	    if ((clp[0] == '#') && (curlen == 0)) {
		/* comment line or cpp line sync */
		int r, cnt;
		char buf[2];
                char* s = clp + 1;

		if (strncmp(s, "line", 4) == 0) s += 4;
		r = sscanf(s, "%d %1[\"]%n", &Line_number, buf, &cnt);
		if (r <= 0) Line_number++;
		else { /* got line number */ 
		    Line_number--;
		    if (r > 1) { /* saw quote */
			char* p = s + cnt;
			char* e = p;
			while (*e && (*e != '"')) e++; 
			if (e != p) {
		 	    *e = '\0';
			    storeFileName (p, e-p);
			}
		    }
		}
		clp[0] = 0;
		len = 1;    /* this will make the while test below succeed */
		continue;
	    }
	    Line_number++;
		/* Note it is possible len == 1 and last character in
		 * previous read was '\\'
		 * It is also possible to have curlen=0, and read in
		 * "\\\n". 
		 */
	    if (clp[len - 2] == '\\') {	/* escaped newline */
		len = len - 2;
		clp[len] = '\0';
	    }
	}
	curlen += len;
	/* the following test relies on having AG.linebuf[0] == '\0' */
    } while (clp[len - 1] != '\n');

    if (curlen > 0)
	return AG.linebuf + 1;
    else
	return NULL;
}

/* html_pair:
 * Iteratively scan nested "<...>"
 * p points to first character after initial '<'
 * Store characters up to but not including matching '>'
 * Return pointer to matching '>'
 * We do not check for any escape sequences; pure HTML is
 * expected, so special characters need to be HTML escapes.
 * We read them in and allow the HTML parser to convert them.
 */
static char *html_pair(char *p, agxbuf * tokp)
{
    unsigned char c;
    int rc, depth = 1;

    while (1) {
	while ((c = *p)) {
	    if (c == '>') {
		depth--;
		if (depth == 0)
		    return p;	/* p points to closing > */
	    } else if (c == '<')
		depth++;
	    rc = agxbputc(tokp, c);
	    p++;
	}
	if ((p = lex_gets()) == NULL) {
	    agerr(AGWARN,
		  "non-terminated HTML string starting line %d, file %s\n",
		  Start_html_string, InfileName);
	    return 0;
	}
    }
}

/* html_string:
 * scan an html string and return the position after its terminator 
 * The string is stored in token.
 * p points to the opening <.
 */

static char *html_string(char *p, agxbuf * token)
{
    Start_html_string = Line_number;
    p = html_pair(p + 1, token);
    if (p)
	p++;			/* skip closing '>' */
    return p;
}

int agtoken(char *p)
{
    char ch;
    TFA_Init();
    while ((ch = *p)) {
	/* any non-ascii characters converted to ascii DEL (127) */
	TFA_Advance(ch & ~127 ? 127 : ch);
	p++;
    }
    return TFA_Definition();
}

int aglex(void)
{
    int token;
    char *tbuf, *p;
    static unsigned char BOM[] = { 0xEF, 0xBB, 0xBF };	/* UTF-8 byte order marker */

    /* if the parser has accepted a graph, reset and return EOF */
    if (AG.accepting_state) {
	AG.accepting_state = FALSE;
	return EOF;
    }

    /* get a nonempty lex buffer */
    do {
	if ((LexPtr == NULL) || (LexPtr[0] == '\0'))
	    if ((LexPtr = lex_gets()) == NULL) {
		if (In_comment)
		    agerr(AGWARN, "nonterminated comment in line %d\n",
			  Comment_start);
		return EOF;
	    }
	/* skip UTF-8 Byte Order Marker if at beginning of file */
	if ((Line_number == 1) && !strncmp(LexPtr, (char *) BOM, 3))
	    LexPtr += 3;
	LexPtr = (char *) skip_wscomments(LexPtr);
    } while (LexPtr[0] == '\0');

    tbuf = TokenBuf;

    /* scan quoted strings */
    if (LexPtr[0] == '\"') {
	LexPtr = quoted_string(LexPtr, tbuf);
	aglval.str = agstrdup(tbuf);
	return T_qsymbol;
    }

    /* scan HTML strings */
    if (LexPtr[0] == '<') {
	agxbuf xb;
	unsigned char htmlbuf[BUFSIZ];
	agxbinit(&xb, BUFSIZ, htmlbuf);
	LexPtr = html_string(LexPtr, &xb);
	aglval.str = agstrdup_html(agxbuse(&xb));
	agxbfree(&xb);
	return T_symbol;
    }

    /* scan edge operator */
    if (AG.edge_op
	&& (strncmp(LexPtr, AG.edge_op, strlen(AG.edge_op)) == 0)) {
	LexPtr += strlen(AG.edge_op);
	return T_edgeop;
    }

    /* scan numbers */
    if ((p = scan_num(LexPtr, tbuf))) {
	LexPtr = p;
	aglval.str = agstrdup(tbuf);
	return T_symbol;
    } else {
	unsigned char uc = *(unsigned char *) LexPtr;
	if (ispunct(uc) && (uc != '_'))
	    return *LexPtr++;
	else
	    LexPtr = scan_token(LexPtr, tbuf);
    }

    /* scan other tokens */
    token = agtoken(tbuf);
    if (token == -1) {
	aglval.str = agstrdup(tbuf);
	token = T_symbol;
    }
    return token;
}

static void error_context(void)
{
    char *p;
    char c;
    char *buf = AG.linebuf + 1;	/* characters are always put at AG.linebuf[1] */
    /* or later; AG.linebuf[0] = '\0' */

    if (LexPtr == NULL)
	return;
    agerr(AGPREV, "context: ");
    for (p = LexPtr - 1; (p > buf) && (!isspace(*(unsigned char *) p));
	 p--);
    if (buf < p) {
	c = *p;
	*p = '\0';
	agerr(AGPREV, buf);
	*p = c;
    }
    agerr(AGPREV, " >>> ");
    c = *LexPtr;
    *LexPtr = '\0';
    agerr(AGPREV, p);
    *LexPtr = c;
    agerr(AGPREV, " <<< ");
    agerr(AGPREV, LexPtr);
}

void agerror(char *msg)
{
    if (AG.syntax_errors++)
	return;
    agerr(AGERR, "%s:%d: %s near line %d\n",
	  InfileName, Line_number, msg, Line_number);
    error_context();
}

agerrlevel_t agerrno;		/* Last error */
static agerrlevel_t agerrlevel = AGWARN;	/* Report errors >= agerrlevel */
static long aglast;		/* Last message */
static FILE *agerrout;		/* Message file */
static agusererrf usererrf;     /* User-set error function */

agusererrf 
agseterrf (agusererrf newf)
{
    agusererrf oldf = usererrf;
    usererrf = newf;
    return oldf;
}

void agseterr(agerrlevel_t lvl)
{
    agerrlevel = lvl;
}

int agerrors(void)
{
    return MAX(agmaxerr, AG.syntax_errors);
}

int agreseterrors(void)
{
    int rc = MAX(agmaxerr, AG.syntax_errors);
    agmaxerr = 0;
    return rc;
}

char *aglasterr()
{
    long endpos;
    long len;
    char *buf;

    if (!agerrout)
	return 0;
    fflush(agerrout);
    endpos = ftell(agerrout);
    len = endpos - aglast;
    buf = (char*)malloc(len + 1);
    fseek(agerrout, aglast, SEEK_SET);
    fread(buf, sizeof(char), len, agerrout);
    buf[len] = '\0';
    fseek(agerrout, endpos, SEEK_SET);

    return buf;
}

static void
userout (agerrlevel_t level, const char *fmt, va_list args)
{
    static char* buf;
    static int bufsz = 1024;
    char* np;
    int n;

    if (!buf) {
	buf = (char*)malloc(bufsz);
	if (!buf) {
	    fputs("userout: could not allocate memory\n", stderr );
	    return;
	}
    }

    if (level != AGPREV) {
	usererrf ((level == AGERR) ? "Error" : "Warning");
	usererrf (": ");
    }

    while (1) {
	n = vsnprintf(buf, bufsz, fmt, args);
	if ((n > -1) && (n < bufsz)) {
	    usererrf (buf);
	    break;
	}
	bufsz = MAX(bufsz*2,n+1);
	if ((np = (char*)realloc(buf, bufsz)) == NULL) {
	    fputs("userout: could not allocate memory\n", stderr );
	    return;
	}
    }
    va_end(args);
}

/* agerr_va:
 * Main error reporting function
 */
static int agerr_va(agerrlevel_t level, const char *fmt, va_list args)
{
    agerrlevel_t lvl;

    /* Use previous error level if continuation message;
     * Convert AGMAX to AGERROR;
     * else use input level
     */
    lvl = (level == AGPREV ? agerrno : (level == AGMAX) ? AGERR : level);

    /* store this error level and maximum error level used */
    agerrno = lvl;
    agmaxerr = MAX(agmaxerr, agerrno);

    /* We report all messages whose level is bigger than the user set agerrlevel
     * Setting agerrlevel to AGMAX turns off immediate error reporting.
     */
    if (lvl >= agerrlevel) {
	if (usererrf)
	    userout (level, fmt, args);
	else {
	    if (level != AGPREV)
		fprintf(stderr, "%s: ", (level == AGERR) ? "Error" : "Warning");
	    vfprintf(stderr, fmt, args);
	    va_end(args);
	}
	return 0;
    }

    /* If error is not immediately reported, store in log file */
    if (!agerrout) {
	agerrout = tmpfile();
	if (!agerrout)
	    return 1;
    }

    if (level != AGPREV)
	aglast = ftell(agerrout);
    vfprintf(agerrout, fmt, args);
    va_end(args);
    return 0;
}

/* agerr:
 * Varargs function for reporting errors with level argument
 */
int agerr(agerrlevel_t level, char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    return agerr_va(level, fmt, args);
}

/* agerrorf:
 * Varargs function for reporting errors
 */
void agerrorf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    agerr_va(AGERR, fmt, args);
}

/* agwarningf:
 * Varargs function for reporting warnings
 */
void agwarningf(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    agerr_va(AGWARN, fmt, args);
}
