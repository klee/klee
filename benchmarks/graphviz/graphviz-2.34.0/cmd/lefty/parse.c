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
#include "lex.h"
#include "parse.h"
#include "internal.h"

static jmp_buf eljbuf;

#define GTOKIFEQ(t) { \
    if (Ltok == (t)) \
        Lgtok (); \
    else \
        err ("expected token: '%s', found: '%s'", Lnames[t], Lnames[Ltok]); \
}

typedef struct lv_t {
    int si, vi;
} lv_t;
static lv_t *lvp;
static int lvn, flvi, llvi;
#define GETLVSTR(i) (char *) Cgetstring (lvp[i].si)
#define GETLVNUM(i) lvp[i].vi
#define LVINCR 1000
#define LVSIZE sizeof (lv_t)

static int pexpr (void);
static int getop (int, int);
static int pexpi (int);
static int pexp5 (void);
static int pexp6 (void);
static int pexp7 (void);
static int pargs (void);
static int pcons (void);
static int pvar (void);
static int pfunc (void);
static int pdecl (int *);
static int ptcons (void);
static int pstmt (void);
static int pifst (void);
static int pwhilest (void);
static int pforst (void);
static int pbreakst (void);
static int pcontinuest (void);
static int preturnst (void);

static void addlv (int, int);
static void err (char *, ...);

void Pinit (void) {
    lvp = Marrayalloc ((long) LVINCR * LVSIZE);
    lvn = LVINCR;
    flvi = llvi = 0;
}

void Pterm (void) {
    Marrayfree (lvp);
    lvp = NULL;
    lvn = flvi = llvi = 0;
}

Tobj Punit (Psrc_t *sp) {
    int ui, ei;

    Lsetsrc (sp->flag, sp->s, sp->fp, sp->tok, sp->lnum);
    Creset ();
    flvi = llvi = 0;

    if (setjmp (eljbuf) != 0)
        return NULL;

    while (Ltok == L_SEMI)
        Lgtok ();
    if (Ltok == L_EOF)
        return NULL;

    ui = Cnew (C_CODE);
    ei = pexpr ();
    Csetfp (ui, ei);
    Lgetsrc (&sp->flag, &sp->s, &sp->fp, &sp->tok, &sp->lnum);
    return Tcode (cbufp, 0, cbufi);
}

/*  shortcut: this function creates a piece of code that corresponds to
    <internal func name> = function () internal "<internal func name>";
*/
Tobj Pfunction (char *ifnam, int ifnum) {
    int ui, ai, vi, si, fi, li1, li2, di, ifi, ifn;

    Creset ();
    ui = Cnew (C_CODE);
    ai = Cnew (C_ASSIGN);
    Csetfp (ui, ai);
    vi = Cnew (C_GVAR);
    si = Cstring (ifnam);
    Csetfp (vi, si);
    Csetfp (ai, vi);
    fi = Cnew (C_FUNCTION);
    Csetnext (vi, fi);
    li1 = Cinteger (0);
    Csetfp (fi, li1);
    li2 = Cinteger (0);
    Csetnext (li1, li2);
    di = Cnew (C_DECL);
    Csetfp (di, C_NULL);
    Csetnext (li2, di);
    ifi = Cnew (C_INTERNAL);
    ifn = Cinteger ((long) ifnum);
    Csetfp (ifi, ifn);
    Csetnext (di, ifi);
    Csetinteger (li1, (long) (Cgetindex () - fi));
    Csetinteger (li2, 0);
    return Tcode (cbufp, 0, cbufi);
}

/*  shortcut: this function creates a piece of code that corresponds to
    <func name> (<args>); where <args> is the second argument (ao)
*/
Tobj Pfcall (Tobj fo, Tobj ao) {
    int ui, fi, ffi, ai, aai;

    Creset ();
    ui = Cnew (C_CODE);
    fi = Cnew (C_FCALL);
    Csetfp (ui, fi);
    ffi = Cnew (C_PVAR);
    Csetobject (ffi, fo);
    Csetfp (fi, ffi);
    ai = Cnew (C_ARGS);
    Csetnext (ffi, ai);
    if (ao) {
        aai = Cnew (C_PVAR);
        Csetobject (aai, ao);
        Csetfp (ai, aai);
    } else
        Csetfp (ai, C_NULL);
    return Tcode (cbufp, 0, cbufi);
}

static int pexpr (void) {
    int ai, ei0, ei1;

    ei0 = pexpi (0);
    if (Ltok != C_ASSIGN)
        return ei0;

    ai = Cnew (C_ASSIGN);
    Csetfp (ai, ei0);
    Lgtok ();
    ei1 = pexpr ();
    Csetnext (ei0, ei1);
    return ai;
}

static int lextab[][7] = {
    {   L_OR,       0,     0,    0,    0,    0, 0 },
    {  L_AND,       0,     0,    0,    0,    0, 0 },
    {   L_EQ,    L_NE,  L_LT, L_LE, L_GT, L_GE, 0 },
    { L_PLUS, L_MINUS,     0,    0,    0,    0, 0 },
    {  L_MUL,   L_DIV, L_MOD,    0,    0,    0, 0 },
    {      0,       0,     0,    0,    0,    0, 0 }
};

static int parsetab[][7] = {
    {   C_OR,       0,     0,    0,    0,    0, 0 },
    {  C_AND,       0,     0,    0,    0,    0, 0 },
    {   C_EQ,    C_NE,  C_LT, C_LE, C_GT, C_GE, 0 },
    { C_PLUS, C_MINUS,     0,    0,    0,    0, 0 },
    {  C_MUL,   C_DIV, C_MOD,    0,    0,    0, 0 },
    {      0,       0,     0,    0,    0,    0, 0 }
};

static int getop (int t, int i) {
    int j;

    for (j = 0; lextab[i][j] != 0; j++)
        if (t == lextab[i][j])
            return parsetab[i][j];
    return -1;
}

static int pexpi (int k) {
    int ei0, ei1, ei2, ptok;

    if (lextab[k][0] == 0)
        return pexp5 ();

    ei0 = pexpi (k + 1);
    while ((ptok = getop (Ltok, k)) != -1) {
        ei1 = Cnew (ptok);
        Csetfp (ei1, ei0);
        Lgtok ();
        ei2 = pexpi (k + 1);
        Csetnext (ei0, ei2);
        ei0 = ei1;
    }
    return ei0;
}

static int pexp5 (void) {
    int ei0, ei1;

    if (Ltok == L_MINUS) {
        ei0 = Cnew (C_UMINUS);
        Lgtok ();
        ei1 = pexp5 ();
        Csetfp (ei0, ei1);
        return ei0;
    }
    return pexp6 ();
}

static int pexp6 (void) {
    int ei0, ei1;

    if (Ltok == L_NOT) {
        ei0 = Cnew (C_NOT);
        Lgtok ();
        ei1 = pexp6 ();
        Csetfp (ei0, ei1);
        return ei0;
    }
    return pexp7 ();
}

static int pexp7 (void) {
    int ei0, ei1, ei2;

    ei0 = 0;
    switch (Ltok) {
    case L_FUNCTION:
        Lgtok ();
        ei0 =  pfunc ();
        break;
    case L_LP:
        ei0 = Cnew (C_PEXPR);
        Lgtok ();
        ei1 = pexpr ();
        GTOKIFEQ (L_RP);
        Csetfp (ei0, ei1);
        break;
    case L_LB:
        ei0 = ptcons ();
        break;
    case L_STRING:
    case L_NUMBER:
        ei0 = pcons ();
        break;
    case L_ID:
        ei0 = pvar ();
        if (Ltok == L_LP) { /* ie: it's really a function call */
            ei1 = ei0;
            ei0 = Cnew (C_FCALL);
            Csetfp (ei0, ei1);
            Lgtok ();
            ei2 = pargs ();
            Csetnext (ei1, ei2);
            GTOKIFEQ (L_RP);
        }
        break;
    default:
        err ("expected EXP7 type token, found: %s", Lnames[Ltok]);
    }
    return ei0;
}

static int pargs (void) {
    int ai, ei0, ei1;

    ai = Cnew (C_ARGS);
    if (Ltok == L_RP) {
        Csetfp (ai, C_NULL);
        return ai;
    }
    ei0 = pexpr ();
    Csetfp (ai, ei0);
    while (Ltok != L_RP) {
        GTOKIFEQ (L_COMMA);
        if (Ltok == L_RP)
            err ("expected expression, found: %s", Lnames[Ltok]);

        ei1 = pexpr ();
        Csetnext (ei0, ei1);
        ei0 = ei1;
    }
    return ai;
}

static int pcons (void) {
    int ci;
    double d;

    ci = 0;
    switch (Ltok) {
    case L_NUMBER:
        d = atof (Lstrtok);
        ci = (d == (double) (long) d) ? Cinteger ((long) d) : Creal (d);
        break;
    case L_STRING:
        ci = Cstring (Lstrtok);
        break;
    default:
        err ("expected scalar constant, found: %s", Lnames[Ltok]);
    }
    Lgtok ();
    return ci;
}

static int pvar (void) {
    int vi, ci0, ci1, i;

    vi = Cnew (C_GVAR);
    ci0 = Cstring (Lstrtok);
    Csetfp (vi, ci0);
    for (i = flvi; i < llvi; i++) {
        if (strcmp (GETLVSTR (i), Lstrtok) == 0) {
            Csettype (vi, C_LVAR);
            ci1 = Cinteger ((long) GETLVNUM (i));
            Csetnext (ci0, ci1);
            ci0 = ci1;
            break;
        }
    }
    Lgtok ();
    if (Ltok != L_DOT && Ltok != L_LB)
        return vi;

    while (Ltok == L_DOT || Ltok == L_LB) {
        if (Ltok == L_DOT) {
            Lgtok ();
            if (Ltok != L_ID)
                err ("expected identifier, found: %s", Lnames[Ltok]);
            ci1 = Cstring (Lstrtok);
            Csetnext (ci0, ci1);
            Lgtok ();
        } else {
            Lgtok ();
            ci1 = pexpr ();
            Csetnext (ci0, ci1);
            GTOKIFEQ (L_RB);
        }
        ci0 = ci1;
    }
    return vi;
}

static int pfunc (void) {
    int fi, di, si, ifi, ifn, ldi, i, li1, li2;
    int owncbufi, ownflvi, ownllvi, flvn, ifnum;

    owncbufi = Cgetindex ();
    ownflvi = flvi, ownllvi = llvi;
    flvi = llvi;
    flvn = 0;

    fi = Cnew (C_FUNCTION);
    GTOKIFEQ (L_LP);
    li1 = Cinteger (0);
    Csetfp (fi, li1);
    li2 = Cinteger (0);
    Csetnext (li1, li2);
    di = pdecl (&flvn);
    Csetnext (li2, di);
    i = di;
    GTOKIFEQ (L_RP);
    if (Ltok == L_INTERNAL) {
        Lgtok ();
        if (Ltok == L_STRING) {
            if ((ifnum = Igetfunc (Lstrtok)) == -1)
                err ("no such internal function: %s", Lstrtok);
            ifi = Cnew (C_INTERNAL);
            ifn = Cinteger ((long) ifnum);
            Csetfp (ifi, ifn);
            Csetnext (i, ifi);
            Lgtok ();
        } else
            err ("expected token: STRING, found: '%s'", Lnames[Ltok]);
    } else {
        GTOKIFEQ (L_LCB);
        while (Ltok == L_LOCAL) {
            Lgtok ();
            ldi = pdecl (&flvn);
            Csetnext (i, ldi);
            i = ldi;
            GTOKIFEQ (L_SEMI);
        }
        while (Ltok != L_RCB) {
            si = pstmt ();
            Csetnext (i, si);
            i = si;
        }
        GTOKIFEQ (L_RCB);
    }
    Csetinteger (li1, (long) (Cgetindex () - owncbufi));
    Csetinteger (li2, (long) flvn);
    flvi = ownflvi, llvi = ownllvi;
    return fi;
}

static int pdecl (int *lvnp) {
    int di, si, i;

    di = Cnew (C_DECL);
    if (Ltok != L_ID) {
        Csetfp (di, C_NULL);
        return di;
    }
    si = Cstring (Lstrtok);
    addlv (si, (*lvnp)++);
    Csetfp (di, si);
    i = si;
    Lgtok ();
    if (Ltok != L_COMMA)
        return di;
    Lgtok ();
    while (Ltok == L_ID) {
        si = Cstring (Lstrtok);
        addlv (si, (*lvnp)++);
        Lgtok ();
        Csetnext (i, si);
        i = si;
        if (Ltok == L_COMMA) {
            Lgtok ();
            if (Ltok != L_ID)
                err ("expected identifier, found %s", Lnames[Ltok]);
        }
    }
    return di;
}

static int ptcons (void) {
    int ti, ei0, ei1;

    ti = Cnew (C_TCONS);
    Lgtok ();
    if (Ltok == L_RB) {
        Csetfp (ti, C_NULL);
        Lgtok ();
        return ti;
    }
    ei1 = pexpi (0);
    Csetfp (ti, ei1);
    ei0 = ei1;
    GTOKIFEQ (L_ASSIGN);
    ei1 = pexpr ();
    Csetnext (ei0, ei1);
    ei0 = ei1;
    GTOKIFEQ (L_SEMI);
    while (Ltok != L_RB) {
        ei1 = pexpi (0);
        Csetnext (ei0, ei1);
        ei0 = ei1;
        GTOKIFEQ (L_ASSIGN);
        ei1 = pexpr ();
        Csetnext (ei0, ei1);
        ei0 = ei1;
        GTOKIFEQ (L_SEMI);
    }
    Lgtok ();
    return ti;
}

static int pstmt (void) {
    int si, i0, i1;

    si = Cnew (C_STMT);
    switch (Ltok) {
    case L_SEMI:
        Csetfp (si, C_NULL);
        Lgtok ();
        break;
    case L_LCB:
        Lgtok ();
        if (Ltok == L_RCB) {
            Csetfp (si, C_NULL);
        } else {
            i1 = pstmt ();
            Csetfp (si, i1);
            i0 = i1;
            while (Ltok != L_RCB) {
                i1 = pstmt ();
                Csetnext (i0, i1);
                i0 = i1;
            }
        }
        Lgtok ();
        break;
    case L_IF:
        i0 = pifst ();
        Csetfp (si, i0);
        break;
    case L_WHILE:
        i0 = pwhilest ();
        Csetfp (si, i0);
        break;
    case L_FOR:
        i0 = pforst ();
        Csetfp (si, i0);
        break;
    case L_BREAK:
        i0 = pbreakst ();
        Csetfp (si, i0);
        break;
    case L_CONTINUE:
        i0 = pcontinuest ();
        Csetfp (si, i0);
        break;
    case L_RETURN:
        i0 = preturnst ();
        Csetfp (si, i0);
        break;
    default:
        i0 = pexpr ();
        Csetfp (si, i0);
        GTOKIFEQ (L_SEMI);
    }
    return si;
}

static int pifst (void) {
    int isi, ii, ti, ei;

    isi = Cnew (C_IF);
    Lgtok ();
    GTOKIFEQ (L_LP);
    ii = pexpr ();
    Csetfp (isi, ii);
    GTOKIFEQ (L_RP);
    ti = pstmt ();
    Csetnext (ii, ti);
    if (Ltok == L_ELSE) {
        Lgtok ();
        ei = pstmt ();
        Csetnext (ti, ei);
    }
    return isi;
}

static int pwhilest (void) {
    int wi, ei, si;

    wi = Cnew (C_WHILE);
    Lgtok ();
    GTOKIFEQ (L_LP);
    ei = pexpr ();
    Csetfp (wi, ei);
    GTOKIFEQ (L_RP);
    si = pstmt ();
    Csetnext (ei, si);
    return wi;
}

static int pforst (void) {
    int fi, i0, i1, si;

    fi = Cnew (C_FOR);
    Lgtok ();
    GTOKIFEQ (L_LP);
    i0 = (Ltok == L_SEMI) ? Cnew (C_NOP): pexpr ();
    Csetfp (fi, i0);
    if (Ltok == L_IN) {
        Csettype (fi, C_FORIN);
        Lgtok ();
        i1 = pexpr ();
        Csetnext (i0, i1);
        i0 = i1;
    } else {
        GTOKIFEQ (L_SEMI);
        i1 = (Ltok == L_SEMI) ? Cnew (C_NOP): pexpr ();
        Csetnext (i0, i1);
        i0 = i1;
        GTOKIFEQ (L_SEMI);
        i1 = (Ltok == L_SEMI) ? Cnew (C_NOP): pexpr ();
        Csetnext (i0, i1);
        i0 = i1;
    }
    GTOKIFEQ (L_RP);
    si = pstmt ();
    Csetnext (i0, si);
    return fi;
}

static int pbreakst (void) {
    int bi;

    bi = Cnew (C_BREAK);
    Csetfp (bi, C_NULL);
    Lgtok ();
    GTOKIFEQ (L_SEMI);
    return bi;
}

static int pcontinuest (void) {
    int ci;

    ci = Cnew (C_CONTINUE);
    Csetfp (ci, C_NULL);
    Lgtok ();
    GTOKIFEQ (L_SEMI);
    return ci;
}

static int preturnst (void) {
    int ri, ei;

    ri = Cnew (C_RETURN);
    Lgtok ();
    if (Ltok == L_SEMI) {
        Csetfp (ri, C_NULL);
        GTOKIFEQ (L_SEMI);
        return ri;
    }
    ei = pexpr ();
    Csetfp (ri, ei);
    GTOKIFEQ (L_SEMI);
    return ri;
}

static void addlv (int si, int vi) {
    int i;

    if (llvi >= lvn) {
        lvp = Marraygrow (lvp, (long) (lvn + LVINCR) + LVSIZE);
        lvn += LVINCR;
    }
    lvp[llvi].si = si, lvp[llvi].vi = vi, llvi++;
    for (i = llvi - 2; i >= flvi; i--)
        if (strcmp (GETLVSTR (i), GETLVSTR (llvi - 1)) == 0)
            err ("local variable %s multiply defined", GETLVSTR (i));
}

static void err (char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    Lprintpos ();
    vfprintf (stderr, fmt, args);
    fprintf (stderr, "\n");
    fflush (stdout);
    longjmp (eljbuf, 1);
}
