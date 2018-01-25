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

/* Lefteris Koutsofios - AT&T Labs Research */

#include "common.h"
#include "lex.h"

int Ltok;
char Lstrtok[MAXTOKEN];

Lname_t Lnames[] = {
    /* L_SEMI     */ ";",
    /* L_ASSIGN   */ "=",
    /* L_OR       */ "|",
    /* L_AND      */ "&",
    /* L_EQ       */ "==",
    /* L_NE       */ "~=",
    /* L_LT       */ "<",
    /* L_LE       */ "<=",
    /* L_GT       */ ">",
    /* L_GE       */ ">=",
    /* L_PLUS     */ "+",
    /* L_MINUS    */ "-",
    /* L_MUL      */ "*",
    /* L_DIV      */ "/",
    /* L_MOD      */ "%",
    /* L_NOT      */ "~",
    /* L_STRING   */ "STRING",
    /* L_NUMBER   */ "NUMBER",
    /* L_ID       */ "IDENTIFIER",
    /* L_DOT      */ "DOT",
    /* L_LB       */ "LEFT BRACKET",
    /* L_RB       */ "RIGHT BRACKET",
    /* L_FUNCTION */ "FUNCTION",
    /* L_LP       */ "LEFT PARENTHESIS",
    /* L_RP       */ "RIGHT PARENTHESIS",
    /* L_LCB      */ "LEFT CURLY BRACE",
    /* L_RCB      */ "RIGHT CURLY BRACE",
    /* L_LOCAL    */ "LOCAL",
    /* L_COLON    */ ":",
    /* L_COMMA    */ ",",
    /* L_IF       */ "IF",
    /* L_ELSE     */ "ELSE",
    /* L_WHILE    */ "WHILE",
    /* L_FOR      */ "FOR",
    /* L_IN       */ "IN",
    /* L_BREAK    */ "BREAK",
    /* L_CONTINUE */ "CONTINUE",
    /* L_RETURN   */ "RETURN",
    /* L_INTERNAL */ "INTERNAL",
    /* L_EOF      */ "EOF"
};

static char *unitp, *ucp;
static FILE *lfp;
static int lsrc, seeneof, linenum;

#define MAXBUF 10000
static char unitbuf[MAXBUF];

/* keyword mapping */
static struct keyword {
    char *str;
    int tok;
} keywords[] = {
    { "function", L_FUNCTION, },
    { "local",    L_LOCAL,    },
    { "if",       L_IF,       },
    { "else",     L_ELSE,     },
    { "while",    L_WHILE,    },
    { "for",      L_FOR,      },
    { "in",       L_IN,       },
    { "break",    L_BREAK,    },
    { "continue", L_CONTINUE, },
    { "return",   L_RETURN,   },
    { "internal", L_INTERNAL, },
    { NULL,       0,          },
};

/* single character token mapping */
static struct keychar {
    int chr, tok;
} keychars[] = {
    { ';',    L_SEMI  },
    { '|',    L_OR    },
    { '&',    L_AND   },
    { '+',    L_PLUS  },
    { '-',    L_MINUS },
    { '*',    L_MUL   },
    { '/',    L_DIV   },
    { '%',    L_MOD   },
    { '.',    L_DOT   },
    { '[',    L_LB    },
    { ']',    L_RB    },
    { '(',    L_LP    },
    { ')',    L_RP    },
    { '{',    L_LCB   },
    { '}',    L_RCB   },
    { ':',    L_COLON },
    { ',',    L_COMMA },
    { '\000', 0       }
};

static int gtok (void);
static int sgetc (void);
static void sungetc (void);

void Lsetsrc (int src, char *s, FILE *fp, int tok, int lnum) {
    if (src == CHARSRC)
        unitp = ucp = s;
    else if (src == FILESRC)
        unitp = ucp = unitbuf, unitbuf[0] = '\000', lfp = fp;
    lsrc = src;
    linenum = lnum;
    seeneof = FALSE;
    if (tok == -1 || tok == L_EOF)
        Ltok = gtok ();
    else
        Ltok = tok;
}

void Lgetsrc (int *srcp, char **sp, FILE **fpp, int *tokp, int *lnump) {
    *srcp = lsrc;
    *sp = ucp;
    *fpp = lfp;
    *tokp = Ltok;
    if (*ucp && *ucp == '\n')
        linenum++;
    *lnump = linenum;
}

void Lprintpos (void) {
    char *s1, *s2;
    char c;

    printf ("at line %d: ", linenum);
    for (s1 = ucp; s1 > unitp && *s1 != '\n'; s1--)
        ;
    for (s2 = ucp; *s2 && *s2 != '\n'; s2++)
        ;
    c = *s2, *s2 = '\000';
    printf ("%s\n", s1);
    *s2 = c;
}

void Lgtok (void) {
    Ltok = gtok ();
}

static int gtok (void) {
    struct keyword *kwp;
    struct keychar *kcp;
    int c, qc, nc;
    char *p;

    while ((c = sgetc ()) != EOF) {
        if (c == '#')
            while ((c = sgetc ()) != '\n')
                ;
        if (c != ' ' && c != '\t' && c != '\n')
            break;
    }
    if (c == EOF)
        return L_EOF;
    /* check for keywords and identifiers */
    if (isalpha (c) || c == '_') {
        p = &Lstrtok[0], *p++ = c;
        while (isalpha ((c = sgetc ())) || isdigit (c) || c == '_')
            *p++ = c;
        sungetc ();
        *p = '\000';
        for (kwp = &keywords[0]; kwp->str; kwp++)
            if (strcmp (kwp->str, Lstrtok) == 0)
                return kwp->tok;
        return L_ID;
    }
    /* check for number constant */
    if (isdigit (c)) {
        p = &Lstrtok[0], *p++ = c;
        while (isdigit ((c = sgetc ())))
            *p++ = c;
        if (c == '.') {
            *p++ = c;
            while (isdigit ((c = sgetc ())))
                *p++ = c;
        }
        sungetc ();
        *p = '\000';
        return L_NUMBER;
    }
    /* check for string constants */
    if (c == '"' || c == '\'') {
        p = &Lstrtok[0];
        qc = c;
        while ((c = sgetc ()) != EOF && c != qc)
            *p++ = c; /* FIXME: deal with \'s */
        if (c == EOF)
            return L_EOF;
        *p = '\000';
        return L_STRING;
    }
    /* check for single letter keywords */
    for (kcp = &keychars[0]; kcp->chr; kcp++)
        if (kcp->chr == c)
            return kcp->tok;
    /* check for 2/1 letter keywords */
    if (c == '=' || c == '~' || c == '<' || c == '>') {
        nc = sgetc ();
        if (nc == '=') {
            switch (c) {
            case '=': return L_EQ;
            case '~': return L_NE;
            case '<': return L_LE;
            case '>': return L_GE;
            }
        } else {
            sungetc ();
            switch (c) {
            case '=': return L_ASSIGN;
            case '~': return L_NOT;
            case '<': return L_LT;
            case '>': return L_GT;
            }
        }
    }
    return L_EOF;
}

static int sgetc (void) {
    if (seeneof)
        return EOF;
    if (*ucp == '\000') {
        if (lsrc == CHARSRC) {
            seeneof = TRUE;
            linenum++;
            return EOF;
        } else if (lsrc == FILESRC) {
            if (ucp != unitp)
                *unitp = *(ucp - 1), ucp = unitp + 1;
            else
                ucp = unitp;
            *ucp = '\000';
            if (!fgets (ucp, MAXBUF - (ucp - unitp), lfp)) {
                seeneof = TRUE;
                return EOF;
            }
        }
    }
    if (*ucp == '\n')
        linenum++;
    return *ucp++;
}

static void sungetc (void) {
    if (seeneof) {
        seeneof = FALSE;
        return;
    }
    if (ucp == unitp)
        panic1 (POS, "sungetc", "unget before start of string");
    ucp--;
    if (*ucp == '\n')
        linenum--;
}
