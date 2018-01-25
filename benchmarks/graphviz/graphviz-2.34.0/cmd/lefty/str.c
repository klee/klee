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
#include "mem.h"
#include "code.h"
#include "tbl.h"
#include "str.h"
#include "internal.h"

static int highci;
static int indent;
#define INDINC(i) (indent += (i))
#define INDDEC(i) (indent -= (i))

static char *sbufp;
static int sbufi, sbufn;
#define SBUFINCR 1000
#define SBUFSIZE sizeof (char)

static void scalarstr (Tobj);
static void codestr (Tobj, int);
static void appends (char *);
static void appendi (long);
static void appendd (double);
static void appendnl (void);
static void growsbuf (int);
static char *copysbuf (void);

void Sinit (void) {
    if (!(sbufp = malloc (SBUFINCR * SBUFSIZE)))
        panic1 (POS, "Sinit", "sbuf malloc failed");
    sbufi = 0;
    sbufn = SBUFINCR;
    indent = 0;
    highci = -1;
}

void Sterm (void) {
    indent = 0;
    free (sbufp), sbufp = NULL;
    sbufn = sbufi = 0;
}

char *Spath (char *path, Tobj ko) {
    sbufp[(sbufi = 0)] = '\000';
    appends ((path) ? path : "");
    scalarstr (ko);
    return copysbuf ();
}

char *Sseen (Tobj ko, char *path) {
    sbufp[(sbufi = 0)] = '\000';
    scalarstr (ko), appends (" = "), appends (path), appends (";");
    return copysbuf ();
}

char *Sabstract (Tobj ko, Tobj vo) {
    sbufp[(sbufi = 0)] = '\000';
    scalarstr (ko), appends (" = ");
    switch (Tgettype (vo)) {
    case T_STRING:
    case T_INTEGER:
    case T_REAL:
        scalarstr (vo);
        break;
    case T_CODE:
        appends ("function (...) { ... }");
        break;
    case T_TABLE:
        appends ("[ ... ]");
        break;
    }
    appends (";");
    return copysbuf ();
}

char *Stfull (Tobj ko) {
    sbufp[(sbufi = 0)] = '\000';
    scalarstr (ko), appends (" = [");
    return copysbuf ();
}

char *Ssfull (Tobj ko, Tobj vo) {
    sbufp[(sbufi = 0)] = '\000';
    if (ko)
        scalarstr (ko), appends (" = ");
    switch (Tgettype (vo)) {
    case T_STRING:
    case T_INTEGER:
    case T_REAL:
    case T_CODE:
        scalarstr (vo);
        break;
    }
    appends (";");
    return copysbuf ();
}

char *Scfull (Tobj co, int ci, int mci) {
    sbufp[(sbufi = 0)] = '\000';
    highci = mci;
    codestr (co, ci);
    highci = -1;
    return copysbuf ();
}

static void scalarstr (Tobj to) {
    switch (Tgettype (to)) {
    case T_INTEGER:
        appendi (Tgetinteger (to));
        break;
    case T_REAL:
        appendd (Tgetreal (to));
        break;
    case T_STRING:
        appends ("\""), appends (Tgetstring (to)), appends ("\"");
        break;
    case T_CODE:
        codestr (to, 0);
        break;
    }
}

static void codestr (Tobj co, int ci) {
    int ct, ct1;
    int ci1, ci2;

    if (highci == ci)
        appends (" >> ");
    switch ((ct = TCgettype (co, ci))) {
    case C_ASSIGN:
        codestr (co, (ci1 = TCgetfp (co, ci)));
        appends (" = ");
        codestr (co, TCgetnext (co, ci1));
        break;
    case C_OR:
    case C_AND:
    case C_EQ:
    case C_NE:
    case C_LT:
    case C_LE:
    case C_GT:
    case C_GE:
    case C_PLUS:
    case C_MINUS:
    case C_MUL:
    case C_DIV:
    case C_MOD:
        codestr (co, (ci1 = TCgetfp (co, ci)));
        switch (ct) {
        case C_OR:    appends (" | ");  break;
        case C_AND:   appends (" & ");  break;
        case C_EQ:    appends (" == "); break;
        case C_NE:    appends (" ~= "); break;
        case C_LT:    appends (" < ");  break;
        case C_LE:    appends (" <= "); break;
        case C_GT:    appends (" > ");  break;
        case C_GE:    appends (" >= "); break;
        case C_PLUS:  appends (" + ");  break;
        case C_MINUS: appends (" - ");  break;
        case C_MUL:   appends (" * ");  break;
        case C_DIV:   appends (" / ");  break;
        case C_MOD:   appends (" % ");  break;
        }
        codestr (co, TCgetnext (co, ci1));
        break;
    case C_NOT:
        appends ("~");
        codestr (co, TCgetfp (co, ci));
        break;
    case C_UMINUS:
        appends ("-");
        codestr (co, TCgetfp (co, ci));
        break;
    case C_PEXPR:
        appends ("(");
        codestr (co, TCgetfp (co, ci));
        appends (")");
        break;
    case C_FCALL:
        codestr (co, (ci1 = TCgetfp (co, ci)));
        appends (" (");
        codestr (co, TCgetnext (co, ci1));
        appends (")");
        break;
    case C_INTEGER:
        appendi (TCgetinteger (co, ci));
        break;
    case C_REAL:
        appendd (TCgetreal (co, ci));
        break;
    case C_STRING:
        appends ("\""), appends (TCgetstring (co, ci)), appends ("\"");
        break;
    case C_GVAR:
    case C_LVAR:
        ci1 = TCgetfp (co, ci);
        appends (TCgetstring (co, ci1));
        if (ct == C_LVAR)
            ci1 = TCgetnext (co, ci1);
        for (
            ci1 = TCgetnext (co, ci1); ci1 != C_NULL;
            ci1 = TCgetnext (co, ci1)
        ) {
            switch (TCgettype (co, ci1)) {
            case C_STRING:
                appends ("."), appends (TCgetstring (co, ci1));
                break;
            case C_INTEGER:
                appends ("[");
                appendi (TCgetinteger (co, ci1));
                appends ("]");
                break;
            case C_REAL:
                appends ("[");
                appendd (TCgetreal (co, ci1));
                appends ("]");
                break;
            default:
                appends ("[");
                codestr (co, ci1);
                appends ("]");
            }
        }
        break;
    case C_PVAR:
        appends ("<var>");
        break;
    case C_FUNCTION:
        ci1 = TCgetnext (co, TCgetnext (co, TCgetfp (co, ci)));
        appends ("function (");
        codestr (co, ci1);
        ci1 = TCgetnext (co, ci1);
        if (TCgettype (co, ci1) == C_INTERNAL) {
            appends (") internal \"");
            appends (Ifuncs[TCgetinteger (co, TCgetfp (co, ci1))].name);
            appends ("\"");
        } else {
            appends (") {");
            INDINC (2);
            for (; ci1 != C_NULL; ci1 = TCgetnext (co, ci1)) {
                appendnl ();
                if (TCgettype (co, ci1) == C_DECL)
                    appends ("local "), codestr (co, ci1), appends (";");
                else
                    codestr (co, ci1);
            }
            INDDEC (2);
            appendnl ();
            appends ("}");
        }
        break;
    case C_TCONS:
        appends ("[");
        INDINC (2);
        ci1 = TCgetfp (co, ci);
        while (ci1 != C_NULL) {
            appendnl ();
            codestr (co, ci1);
            appends (" = ");
            ci1 = TCgetnext (co, ci1);
            codestr (co, ci1);
            appends (";");
            ci1 = TCgetnext (co, ci1);
        }
        INDDEC (2);
        appendnl ();
        appends ("]");
        break;
    case C_DECL:
        ci1 = TCgetfp (co, ci);
        while (ci1 != C_NULL) {
            appends (TCgetstring (co, ci1));
            ci1 = TCgetnext (co, ci1);
            if (ci1 != C_NULL)
                appends (", ");
        }
        break;
    case C_STMT:
        ci1 = TCgetfp (co, ci);
        if (ci1 == C_NULL) {
            appends (";");
            break;
        }
        if (TCgetnext (co, ci1) == C_NULL) {
            codestr (co, ci1);
            ct1 = TCgettype (co, ci1);
            if (!C_ISSTMT (ct1))
                appends (";");
        } else {
            appends (" {");
            INDINC (2);
            for (; ci1 != C_NULL; ci1 = TCgetnext (co, ci1)) {
                appendnl ();
                codestr (co, ci1);
            }
            INDDEC (2);
            appendnl ();
            appends ("}");
        }
        break;
    case C_IF:
        ci1 = TCgetfp (co, ci);
        appends ("if (");
        codestr (co, ci1);
        appends (")");
        ci1 = TCgetnext (co, ci1);
        ci2 = TCgetfp (co, ci1);
        if (ci2 == C_NULL || TCgetnext (co, ci2) == C_NULL) {
            INDINC (2);
            appendnl ();
            codestr (co, ci1);
            INDDEC (2);
        } else {
            codestr (co, ci1);
        }
        ci1 = TCgetnext (co, ci1);
        if (ci1 == C_NULL)
            break;
        if (ci2 == C_NULL || TCgetnext (co, ci2) == C_NULL) {
            appendnl ();
            appends ("else");
        } else {
            appends (" else");
        }
        ci2 = TCgetfp (co, ci1);
        if (ci2 == C_NULL || TCgetnext (co, ci2) == C_NULL) {
            INDINC (2);
            appendnl ();
            codestr (co, ci1);
            INDDEC (2);
        } else {
            codestr (co, ci1);
        }
        break;
    case C_WHILE:
        ci1 = TCgetfp (co, ci);
        appends ("while (");
        codestr (co, ci1);
        ci1 = TCgetnext (co, ci1);
        ci2 = TCgetfp (co, ci1);
        if (ci2 == C_NULL || TCgetnext (co, ci2) == C_NULL) {
            appends (")");
            INDINC (2);
            appendnl ();
            codestr (co, ci1);
            INDDEC (2);
        } else {
            appends (")");
            codestr (co, ci1);
        }
        break;
    case C_FOR:
        ci1 = TCgetfp (co, ci);
        appends ("for (");
        codestr (co, ci1);
        appends ("; ");
        ci1 = TCgetnext (co, ci1);
        codestr (co, ci1);
        appends ("; ");
        ci1 = TCgetnext (co, ci1);
        codestr (co, ci1);
        ci1 = TCgetnext (co, ci1);
        ci2 = TCgetfp (co, ci1);
        if (ci2 == C_NULL || TCgetnext (co, ci2) == C_NULL) {
            appends (")");
            INDINC (2);
            appendnl ();
            codestr (co, ci1);
            INDDEC (2);
        } else {
            appends (")");
            codestr (co, ci1);
        }
        break;
    case C_FORIN:
        ci1 = TCgetfp (co, ci);
        appends ("for (");
        codestr (co, ci1);
        appends (" in ");
        ci1 = TCgetnext (co, ci1);
        codestr (co, ci1);
        ci1 = TCgetnext (co, ci1);
        ci2 = TCgetfp (co, ci1);
        if (ci2 == C_NULL || TCgetnext (co, ci2) == C_NULL) {
            appends (")");
            INDINC (2);
            appendnl ();
            codestr (co, ci1);
            INDDEC (2);
        } else {
            appends (")");
            codestr (co, ci1);
        }
        break;
    case C_BREAK:
        appends ("break;");
        break;
    case C_CONTINUE:
        appends ("continue;");
        break;
    case C_RETURN:
        ci1 = TCgetfp (co, ci);
        appends ("return");
        if (ci1 != C_NULL) {
            appends (" ");
            codestr (co, ci1);
        }
        appends (";");
        break;
    case C_ARGS:
        ci1 = TCgetfp (co, ci);
        while (ci1 != C_NULL) {
            codestr (co, ci1);
            ci1 = TCgetnext (co, ci1);
            if (ci1 != C_NULL)
                appends (", ");
        }
        break;
    default:
        panic1 (POS, "codestr", "bad object type: %d", ct);
    }
}

static void appends (char *s) {
    int n;

    n = strlen (s) + 1;
    if (sbufi + n > sbufn)
        growsbuf (n);
    strcpy (&sbufp[sbufi], s);
    sbufi += (n - 1);
}

static void appendi (long i) {
    char buf[40];
    int n;

    sprintf (buf, "%ld", i);
    n = strlen (buf) + 1;
    if (sbufi + n > sbufn)
        growsbuf (n);
    strcpy (&sbufp[sbufi], buf);
    sbufi += (n - 1);
}

static void appendd (double d) {
    char buf[40];
    int n;

    sprintf (buf, "%lf", d);
    n = strlen (buf) + 1;
    if (sbufi + n > sbufn)
        growsbuf (n);
    strcpy (&sbufp[sbufi], buf);
    sbufi += (n - 1);
}

static void appendnl (void) {
    int i, n;

    n = indent + 1;
    if (sbufi + n > sbufn)
        growsbuf (n);
    sbufp[sbufi++] = '\n';
    for (i = 0; i < indent; i++)
        sbufp[sbufi++] = ' ';
}

static void growsbuf (int ssize) {
    int nsize;

    nsize = ((sbufn + ssize) / SBUFINCR + 1) * SBUFINCR;
    if (!(sbufp = realloc (sbufp, nsize * SBUFSIZE)))
        panic1 (POS, "growsbuf", "sbuf realloc failed");
    sbufn = nsize;
}

static char *copysbuf (void) {
    char *newsbufp;

    sbufp[sbufi++] = '\000';
    if (!(newsbufp = malloc (sbufi * sizeof (char))))
        panic1 (POS, "copysbuf", "newsbuf malloc failed");
    strcpy (newsbufp, sbufp);
    return newsbufp;
}
