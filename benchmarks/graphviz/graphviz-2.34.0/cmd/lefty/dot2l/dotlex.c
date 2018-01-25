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

/* the graph lexer */

typedef void *Tobj;
#include "common.h"
#include "dotparse.h"
#include "dot2l.h"
#include "io.h"
#include "triefa.c"

static int syntax_errors;
static int lexer_fd;
#define LEXBUFSIZ 10240
static char *lexbuf, *lexptr;
static int lexsiz;
static int in_comment;
static int comment_start;
int line_number;

static char *lex_gets (int);
static int lex_token (char *);
static void error_context (void);
static char *skip_wscomments (char *);
static char *scan_token (char *);
static char *scan_num (char *);
static char *quoted_string (char *);
static char *html_string (char *);

int lex_begin (int ioi) {
    lexer_fd = ioi;
    lexptr = NULL;
    if (!(lexbuf = malloc (LEXBUFSIZ))) {
        fprintf (stderr, "cannot allocate buffer\n");
        return -1;
    }
    lexsiz = LEXBUFSIZ;
    return 0;
}

int myyylex (void) {        /* for debugging */
    int rv = myyylex ();

    fprintf (stderr, "returning %d\n", rv);
    if (rv == T_id)
        fprintf (stderr, "string val is %s\n", yylval.s);
    return rv;
}

int yylex (void) {
    int token;
    char *p;

    /* if the parser has accepted a graph, reset and return EOF */
    if (yaccdone) {
        yaccdone = FALSE;
        return EOF;
    }

    /* get a nonempty lex buffer */
    do {
        if ((lexptr == NULL) || (lexptr[0] == '\0'))
            if ((lexptr = lex_gets (0)) == NULL) {
                if (in_comment)
                    fprintf (
                        stderr,
                        "warning, nonterminated comment in line %d\n",
                        comment_start
                    );
                return EOF;
            }
        lexptr = skip_wscomments (lexptr);
    } while (lexptr[0] == '\0');

    /* scan quoted strings */
    if (lexptr[0] == '\"') {
        lexptr = quoted_string (lexptr);
        yylval.s = (char *) strdup (lexbuf);
        return T_id;
    }

    /* scan html strings */
    if (lexptr[0] == '<') {
        lexptr = html_string (lexptr);
        yylval.s = (char *) strdup (lexbuf);
        return T_id;
    }

    /* scan edge operator */
    if (etype && (strncmp (lexptr, etype, strlen (etype)) == 0)) {
        lexptr += strlen (etype);
        return T_edgeop;
    }

    /* scan numbers */
    if ((p = scan_num (lexptr))) {
        lexptr = p;
        yylval.s =  strdup (lexbuf);
        return T_id;
    }
    else {
        if (ispunct (lexptr[0]) && (lexptr[0] != '_'))
            return *lexptr++;
        else
            lexptr = scan_token (lexptr);
    }

    /* scan other tokens */
    token = lex_token (lexbuf);
    if (token == -1) {
        yylval.s = strdup (lexbuf);
        token = T_id;
    }
    return token;
}

void
yyerror (char *fmt, char *s) {
    if (syntax_errors++)
        return;
    fprintf (stderr, "graph parser: ");
    fprintf (stderr, fmt, s);
    fprintf (stderr, " near line %d\n", line_number);
    error_context ();
}

static char *lex_gets (int curlen) {
    char *clp;
    int len;

    do {
        /* off by one so we can back up in LineBuf */
        if (IOreadline (
            lexer_fd, lexbuf + curlen + 1, lexsiz - curlen - 1
        ) == -1)
            break;
        clp = lexbuf + curlen + 1;
        len = strlen (clp);
        clp[len++] = '\n';
        clp[len] = 0;

        if (clp == lexbuf + 1 && clp[0] == '#') {
            /* comment line or cpp line sync */
            if (sscanf (clp+1, "%d", &line_number) == 0)
                line_number++;
            len = 0;
            clp[len] = 0;
            continue;
        }

        line_number++;
        if ((len = strlen (clp)) > 1) {
            if (clp[len - 2] == '\\') {
                len = len - 2;
                clp[len] = '\0';
            }
        }
        curlen += len;
        if (lexsiz - curlen - 1 < 1024) {
            if (!(lexbuf = realloc (lexbuf, lexsiz * 2))) {
                fprintf (stderr, "cannot grow buffer\n");
                return NULL;
            }
            lexsiz *= 2;
        }
    } while (clp[len - 1] != '\n');

    if (curlen > 0)
        return lexbuf + 1;
    else
        return NULL;
}

static int lex_token (char *p) {
    TFA_Init ();
    while (*p)
        TFA_Advance (*p++);
    return
        TFA_Definition ();
}

static void error_context (void) {
    char *p, *q;

    if (lexptr == NULL)
        return;
    fprintf (stderr, "context: ");
    for (p = lexptr - 1; (p > lexbuf) && (isspace (*p) == FALSE); p--)
        ;
    for (q = lexbuf; q < p; q++)
        fputc (*q, stderr);
    fputs (" >>> ", stderr);
    for (; q < lexptr; q++)
        fputc (*q, stderr);
    fputs (" <<< ", stderr);
    fputs (lexptr, stderr);
}

/* i wrote this and it still frightens me */
/* skip white space and comments in p */
static char *skip_wscomments (char *p) {
    do {
        while (isspace (*p))
            p++;
        while (in_comment && p[0]) {
            while (p[0] && (p[0] != '*'))
                p++;
            if (p[0]) {
                if (p[1] == '/') {
                    in_comment = FALSE;
                    p += 2;
                    break;
                } else
                    p++;
            }
        }
        if (p[0] == '/') {
            if (p[1] == '/')
                while (*p)
                    p++;    /* skip to end of line */
            else {
                if (p[1] == '*') {
                    in_comment = TRUE;
                    comment_start = line_number;
                    p += 2;
                    continue;
                }
                else
                    break;    /* return a slash */
            }
        } else {
            if (!isspace (*p))
                break;
        }
    } while (p[0]);
    return p;
}

/* scan an unquoted token and return the position after its terminator */
static char *scan_token (char *p) {
    char *q;

    q = lexbuf;
    if (p == '\0')
        return NULL;
    while (isalnum (*p) || (*p == '_') || (!isascii (*p)))
        *q++ = *p++;
    *q = '\0';
    return p;
}

static char *scan_num (char *p) {
    char *q, *z;
    int saw_rp = FALSE;
    int saw_digit = FALSE;

    z = p;
    q = lexbuf;
    if (*z == '-')
        *q++ = *z++;
    if (*z == '.') {
        saw_rp = TRUE;
        *q++ = *z++;
    }
    while (isdigit (*z)) {
        saw_digit = TRUE;
        *q++ = *z++;
    }
    if ((*z == '.') && (saw_rp == FALSE)) {
        saw_rp = TRUE;
        *q++ = *z++;
        while (isdigit (*z)) {
            saw_digit = TRUE;
            *q++ = *z++;
        }
    }
    *q = '\0';
    if (saw_digit && *z && (isalpha (*z)))
        yyerror ("badly formed number %s", lexbuf);

    if (saw_digit == FALSE)
        z = NULL;
    return z;
}

/* scan a quoted string and return the position after its terminator */
static char *quoted_string (char *p) {
    char quote, *q;

    quote = *p++;
    q = lexbuf;
    while ((*p) && (*p != quote)) {
        if (*p == '\\') {
            if (*(p+1) == quote)
                p++;
            else {
                if (*(p+1) == '\\')
                    *q++ = *p++;
            }
        }
        *q++ = *p++;
    }
    if (*p == '\0')
        yyerror ("string ran past end of line", "");
    else
        p++;
    *q = 0;
    return p;
}

/* scan a html string and return the position after its terminator */
static char *html_string (char *p) {
    char *q, *pbuf;
    int bal;

    p++;
    bal = 1;
    q = lexbuf;
    *q++ = '>';
    while (*p && *p != '<' && *p != '>')
        p++;
    for (;;) {
        while (*p) {
            if (*p == '<')
                bal++;
            else if (*p == '>') {
                bal--;
                if (bal == 0) {
                    *q++ = '<';
                    *q = 0;
                    return p + 1;
                }
            }
            *q++ = *p++;
        }
        pbuf = lexbuf;
        if ((lexptr = lex_gets (p - lexbuf - 1)) == NULL) {
            fprintf (
                stderr,
                "warning, unterminated html label in line %d\n",
                line_number
            );
            return NULL;
        }
        q += (lexbuf - pbuf);
        p += (lexbuf - pbuf);
    }
    return NULL;
}
