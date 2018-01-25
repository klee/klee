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
#include "exec.h"
#include "internal.h"

static lvar_t *lvarp;
static int lvarn, llvari, flvari;
#define LVARINCR 1000
#define LVARSIZE sizeof (lvar_t)

Tobj root, null;
Tobj rtno;
int Erun;
int Eerrlevel, Estackdepth, Eshowbody, Eshowcalls, Eoktorun;

#define PUSHJMP(op, np, b) op = (volatile jmp_buf *) np, np = (jmp_buf *) &b
#define POPJMP(op, np) np = (jmp_buf *) op

/* longjmps for normal program execution */
#define PLJ_BREAK    0
#define PLJ_CONTINUE 1
#define PLJ_RETURN   2
#define PLJ_SIZE     3

static jmp_buf *pljbufp1, *pljbufp2;
static int pljtype;

/* longjmp for error handling */
static jmp_buf *eljbufp;

/* error levels and types */
#define ERR0 0
#define ERR1 1
#define ERR2 2
#define ERR3 3
#define ERR4 4
#define ERR5 5

#define ERRNOLHS        0
#define ERRNORHS        1
#define ERRNOSUCHFUNC   2
#define ERRBADARG       3
#define ERRARGMIS       4
#define ERRNOTATABLE    5
#define ERRIFUNCERR     6
#define ERRRECRUN       7
#define ERRTABLECHANGED 8

static char *errnam[] = {
    "no variable",
    "no value",
    "no such function",
    "bad argument",
    "argument number mismatch",
    "not a table",
    "internal function call error",
    "recursive run attempt",
    "table changed during a forin loop",
};

static int errdo;

/* stack information */
typedef struct sinfo_t {
    Tobj co, fco;
    int ci, fci;
    int flvari, llvari;
} sinfo_t;
#define SINFOSIZE sizeof (sinfo_t)
#define SINFOINCR 100
static sinfo_t *sinfop;
static int sinfoi, sinfon;

#define TNK_LI 0
#define TNK_O  1
#define TNK_S  2

typedef struct tnk_t {
    int type;
    union {
        int li;
        struct {
            Tobj to, ko;
        } tnko;
        struct {
            int kt;
            Tobj to, co;
            int vi;
        } tnks;
    } u;
} tnk_t;

typedef struct Num_t {
    int type;
    union {
        long i;
        double d;
        Tobj no;
    } u;
} Num_t;

static long rootm;
static int running;

static Tobj eeval (Tobj, int);
static Tobj efcall (Tobj, int);
static void ewhilest (Tobj, int);
static void eforst (Tobj, int);
static void eforinst (Tobj, int);

static Tobj getval (Tobj, int);
static int getvar (Tobj, int, tnk_t *);
static void setvar (tnk_t, Tobj);
static int boolop (Tobj);
static int orderop (Tobj, int, Tobj);
static Tobj arithop (Num_t *, int, Num_t *);
static void err (int, int, Tobj, int);
static void printbody (char *, int);

void Einit (void) {
    root = Ttable (100);
    rootm = Mpushmark (root);
    Tinss (root, "null", (null = Ttable (2)));
    rtno = NULL;
    pljbufp1 = pljbufp2 = NULL, pljtype = 0;
    eljbufp = NULL;
    lvarp = Marrayalloc ((long) LVARINCR * LVARSIZE);
    lvarn = LVARINCR;
    llvari = 0;
    flvari = 0;
    sinfop = Marrayalloc ((long) SINFOINCR * SINFOSIZE);
    sinfon = SINFOINCR;
    sinfoi = 0;
    Erun = FALSE;
    running = 0;
    Eoktorun = FALSE;
}

void Eterm (void) {
    Marrayfree (sinfop), sinfop = NULL, sinfon = 0, sinfoi = 0;
    Marrayfree (lvarp), lvarp = NULL, lvarn = 0, llvari = 0, flvari = 0;
    rtno = NULL;
    null = NULL;
    Mpopmark (rootm);
}

Tobj Eunit (Tobj co) {
    volatile jmp_buf *oeljbufp;
    volatile int ownsinfoi;
    volatile long m;
    volatile Tobj lrtno;

    jmp_buf eljbuf;

#if 0
    if (running && !Eoktorun) {
        err (ERRRECRUN, ERR2, NULL, 0);
        return NULL;
    }
#endif
    Eoktorun = FALSE;

    if (!co)
        return NULL;

    if (Tgettype (co) != T_CODE)
        panic1 (POS, "Eunit", "argument type is not T_CODE");

    m = Mpushmark (co);
    PUSHJMP (oeljbufp, eljbufp, eljbuf);
    ownsinfoi = sinfoi++;
    if (sinfoi == sinfon) {
        sinfop = Marraygrow (sinfop, (long) (sinfon + SINFOINCR) * SINFOSIZE);
        sinfon += SINFOINCR;
    }
    sinfop[ownsinfoi].co = co;
    sinfop[ownsinfoi].ci = TCgetfp (co, 0);
    sinfop[ownsinfoi].fco = NULL;
    sinfop[ownsinfoi].flvari = flvari;
    sinfop[ownsinfoi].llvari = llvari;
    running++;
    if (setjmp (*eljbufp))
        lrtno = NULL;
    else
        lrtno = eeval (co, TCgetfp (co, 0));
    running--;
    rtno = NULL;
    flvari = sinfop[ownsinfoi].flvari;
    llvari = sinfop[ownsinfoi].llvari;
    sinfoi = ownsinfoi;
    POPJMP (oeljbufp, eljbufp);
    Mpopmark (m);
    Erun = TRUE;
    return lrtno;
}

/*  shortcut: this function executes a piece of code that corresponds to
    <internal func name> = function () internal "<internal func name>";
*/
Tobj Efunction (Tobj co, char *ifnam) {
    Tobj v1o;
    int fi;

    fi = TCgetnext (co, TCgetfp (co, TCgetfp (co, 0)));
    v1o = Tcode (
        TCgetaddr (co, fi), fi, (int) TCgetinteger (co, TCgetfp (co, fi))
    );
    Tinss (root, ifnam, v1o);
    return v1o;
}

static Tobj eeval (Tobj co, int ci) {
    Tobj v1o, v2o, v3o;
    int ttype;
    int ctype;
    tnk_t tnk;
    Num_t lnum, rnum;
    long m1, m2;
    int i1, i2, res;

tailrec:
    m1 = 0;
    errdo = TRUE;
    v1o = NULL;
    ctype = TCgettype (co, ci);
    switch (ctype) {
    case C_ASSIGN:
        i1 = TCgetfp (co, ci);
        if ((v1o = eeval (co, TCgetnext (co, i1))) == NULL) {
            err (ERRNORHS, ERR4, co, TCgetnext (co, i1));
            return NULL;
        }
        m1 = Mpushmark (v1o);
        res = getvar (co, i1, &tnk);
        Mpopmark (m1);
        if (res == -1) {
            err (ERRNOLHS, ERR3, co, i1);
            return NULL;
        }
        setvar (tnk, v1o);
        return v1o;
    case C_OR:
    case C_AND:
    case C_NOT:
        i1 = TCgetfp (co, ci);
        if ((v1o = eeval (co, i1)) == NULL)
            err (ERRNORHS, ERR4, co, i1);
        switch (ctype) {
        case C_OR:
            if (boolop (v1o) == TRUE)
                return Ttrue;
            if ((v1o = eeval (co, TCgetnext (co, i1))) == NULL)
                err (ERRNORHS, ERR4, co, TCgetnext (co, i1));
            return (boolop (v1o) == TRUE) ? Ttrue: Tfalse;
        case C_AND:
            if (boolop (v1o) == FALSE)
                return Tfalse;
            if ((v1o = eeval (co, TCgetnext (co, i1))) == NULL)
                err (ERRNORHS, ERR4, co, TCgetnext (co, i1));
            return (boolop (v1o) == FALSE) ? Tfalse: Ttrue;
        case C_NOT:
            return (boolop (v1o) == TRUE) ? Tfalse: Ttrue;
        }
        /* NOT REACHED */
        return Tfalse;
    case C_EQ:
    case C_NE:
    case C_LT:
    case C_LE:
    case C_GT:
    case C_GE:
        i1 = TCgetfp (co, ci);
        if ((v1o = eeval (co, i1)) == NULL)
            err (ERRNORHS, ERR4, co, i1);
        else
            m1 = Mpushmark (v1o);
        if ((v2o = eeval (co, TCgetnext (co, i1))) == NULL)
            err (ERRNORHS, ERR4, co, TCgetnext (co, i1));
        if (v1o)
            Mpopmark (m1);
        return (orderop (v1o, ctype, v2o) == TRUE) ? Ttrue: Tfalse;
    case C_PLUS:
    case C_MINUS:
    case C_MUL:
    case C_DIV:
    case C_MOD:
    case C_UMINUS:
        i1 = TCgetfp (co, ci);
        if ((lnum.type = TCgettype (co, i1)) == C_INTEGER)
            lnum.u.i = TCgetinteger (co, i1);
        else if (lnum.type == C_REAL)
            lnum.u.d = TCgetreal (co, i1);
        else if ((lnum.u.no = eeval (co, i1)) == NULL) {
            err (ERRNORHS, ERR4, co, i1);
            return NULL;
        }
        if (ctype == C_UMINUS) {
            if (!(v1o = arithop (&lnum, ctype, NULL)))
                err (ERRNORHS, ERR4, co, ci);
            return v1o;
        }
        if (lnum.type != C_INTEGER && lnum.type != C_REAL)
            m1 = Mpushmark (lnum.u.no);
        i1 = TCgetnext (co, i1);
        if ((rnum.type = TCgettype (co, i1)) == C_INTEGER)
            rnum.u.i = TCgetinteger (co, i1);
        else if (rnum.type == C_REAL)
            rnum.u.d = TCgetreal (co, i1);
        else if ((rnum.u.no = eeval (co, i1)) == NULL)
            err (ERRNORHS, ERR4, co, i1);
        if (lnum.type != C_INTEGER && lnum.type != C_REAL)
            Mpopmark (m1);
        if (!(v1o = arithop (&lnum, ctype, &rnum)))
            err (ERRNORHS, ERR4, co, ci);
        return v1o;
    case C_PEXPR:
        ci = TCgetfp (co, ci);
        goto tailrec;
    case C_FCALL:
        return efcall (co, ci);
    case C_INTEGER:
        return Tinteger (TCgetinteger (co, ci));
    case C_REAL:
        return Treal (TCgetreal (co, ci));
    case C_STRING:
        return Tstring (TCgetstring (co, ci));
    case C_GVAR:
    case C_LVAR:
    case C_PVAR:
        return getval (co, ci);
    case C_FUNCTION:
        return Tcode (
            TCgetaddr (co, ci), ci, (int) TCgetinteger (co, TCgetfp (co, ci))
        );
    case C_TCONS:
        v1o = Ttable (0);
        m1 = Mpushmark (v1o);
        for (
            i1 = TCgetfp (co, ci); i1 != C_NULL;
            i1 = TCgetnext (co, TCgetnext (co, i1))
        ) {
            if (!(v3o = eeval (co, TCgetnext (co, i1)))) {
                err (ERRNORHS, ERR4, co, TCgetnext (co, i1));
                continue;
            }
            m2 = Mpushmark (v3o);
            if (!(v2o = eeval (co, i1))) {
                err (ERRNOLHS, ERR3, co, i1);
                Mpopmark (m2);
                continue;
            }
            ttype = Tgettype (v2o);
            if (ttype == T_INTEGER || ttype == T_REAL || ttype == T_STRING)
                Tinso (v1o, v2o, v3o);
            else
                err (ERRNOLHS, ERR1, co, i1);
        }
        Mpopmark (m1);
        return v1o;
    case C_STMT:
        for (i1 = TCgetfp (co, ci); i1 != C_NULL; )
            if ((i2 = TCgetnext (co, i1)) != C_NULL) {
                eeval (co, i1);
                i1 = i2;
            } else {
                ci = i1;
                goto tailrec;
            }
        /* NOT REACHED */
        break;
    case C_IF:
        i1 = TCgetfp (co, ci);
        if (!(v1o = eeval (co, i1)))
            err (ERRNORHS, ERR5, co, i1);
        if (boolop (v1o) == TRUE) {
            ci = TCgetnext (co, i1);
            goto tailrec;
        } else if ((ci = TCgetnext (co, TCgetnext (co, i1))) != C_NULL)
            goto tailrec;
        break;
    case C_WHILE:
        ewhilest (co, ci);
        break;
    case C_FOR:
        eforst (co, ci);
        break;
    case C_FORIN:
        eforinst (co, ci);
        break;
    case C_BREAK:
        pljtype = PLJ_BREAK;
        longjmp (*pljbufp1, 1);
        /* NOT REACHED */
        break;
    case C_CONTINUE:
        pljtype = PLJ_CONTINUE;
        longjmp (*pljbufp1, 1);
        /* NOT REACHED */
        break;
    case C_RETURN:
        if ((i1 = TCgetfp (co, ci)) != C_NULL)
            rtno = eeval (co, i1);
        pljtype = PLJ_RETURN;
        longjmp (*pljbufp2, 1);
        /* NOT REACHED */
        break;
    default:
        panic1 (POS, "eeval", "unknown program token type %d", ctype);
    }
    return v1o;
}

static Tobj efcall (Tobj co, int ci) {
    volatile jmp_buf *opljbufp1, *opljbufp2;
    volatile long m;
    volatile int bi, ownsinfoi, li, ln;

    jmp_buf pljbuf;
    Tobj fdo, vo, lrtno;
    int i, fci, ai, di, di1, fid;

    ownsinfoi = sinfoi++;
    if (sinfoi == sinfon) {
        sinfop = Marraygrow (sinfop, (long) (sinfon + SINFOINCR) * SINFOSIZE);
        sinfon += SINFOINCR;
    }
    sinfop[ownsinfoi].co = co;
    sinfop[ownsinfoi].ci = ci;
    sinfop[ownsinfoi].fco = NULL;
    sinfop[ownsinfoi].flvari = flvari;
    sinfop[ownsinfoi].llvari = llvari;
    fci = TCgetfp (co, ci);
    if (!(fdo = getval (co, fci)) || Tgettype (fdo) != T_CODE) {
        err (ERRNOSUCHFUNC, ERR2, co, fci);
        sinfoi = ownsinfoi;
        return NULL;
    }

    m = Mpushmark ((Tobj) fdo);
    ai = TCgetfp (co, TCgetnext (co, fci));
    ln = (int) TCgetinteger (fdo, (li = TCgetnext (fdo, TCgetfp (fdo, 0))));
    di = TCgetnext (fdo, li);
    bi = TCgetnext (fdo, di);
    if (bi != C_NULL && TCgettype (fdo, bi) == C_INTERNAL) {
        for (i = 0; ai != C_NULL; ai = TCgetnext (co, ai), i++) {
            if (!(vo = eeval (co, ai))) {
                err (ERRBADARG, ERR2, co, ai);
                Mpopmark (m);
                llvari = sinfop[ownsinfoi].llvari;
                sinfoi = ownsinfoi;
                return NULL;
            }
            if (llvari + 1 > lvarn) {
                lvarp = Marraygrow (lvarp, (long) (llvari + 1) * LVARSIZE);
                lvarn = llvari + 1;
            }
            lvarp[llvari].m = Mpushmark ((lvarp[llvari].o = vo));
            llvari++;
        }
        fid = (int) TCgetinteger (fdo, TCgetfp (fdo, bi));
        if (Ifuncs[fid].min > i || Ifuncs[fid].max < i) {
            err (ERRARGMIS, ERR2, co, ci);
            Mpopmark (m);
            llvari = sinfop[ownsinfoi].llvari;
            sinfoi = ownsinfoi;
            return NULL;
        }
        flvari = sinfop[ownsinfoi].llvari;
        sinfop[ownsinfoi].fco = fdo;
        sinfop[ownsinfoi].fci = bi;
        if (fid < 0 || fid >= Ifuncn)
            panic1 (POS, "efcall", "no such internal function: %d", fid);
        rtno = Ttrue;
        if ((*Ifuncs[fid].func) (i, &lvarp[flvari]) == L_FAILURE) {
            rtno = NULL;
            err (ERRIFUNCERR, ERR2, co, ci);
        }
    } else {
        if (llvari + ln > lvarn) {
            lvarp = Marraygrow (lvarp, (long) (llvari + ln) * LVARSIZE);
            lvarn = llvari + ln;
        }
        di1 = TCgetfp (fdo, di);
        for (
            i = 0; i < ln && di1 != C_NULL && ai != C_NULL;
            i++, ai = TCgetnext (co, ai)
        ) {
            if (!(vo = eeval (co, ai))) {
                err (ERRBADARG, ERR2, co, ai);
                Mpopmark (m);
                llvari = sinfop[ownsinfoi].llvari;
                sinfoi = ownsinfoi;
                return NULL;
            }
            lvarp[llvari].m = Mpushmark ((lvarp[llvari].o = vo));
            llvari++;
            di1 = TCgetnext (fdo, di1);
        }
        if (di1 != C_NULL || ai != C_NULL) {
            err (ERRARGMIS, ERR2, co, ci);
            Mpopmark (m);
            llvari = sinfop[ownsinfoi].llvari;
            sinfoi = ownsinfoi;
            return NULL;
        }
        for (; i < ln; i++, llvari++)
            lvarp[llvari].m = Mpushmark ((lvarp[llvari].o = NULL));
        flvari = sinfop[ownsinfoi].llvari;
        PUSHJMP (opljbufp2, pljbufp2, pljbuf);
        opljbufp1 = (volatile jmp_buf *) pljbufp1;
        if (setjmp (*pljbufp2)) {
            ;
        } else {
            sinfop[ownsinfoi].fco = fdo;
            for (; bi != C_NULL; bi = TCgetnext (fdo, bi)) {
                sinfop[ownsinfoi].fci = bi;
                if (TCgettype (fdo, bi) != C_DECL)
                    eeval ((Tobj) fdo, bi);
            }
        }
        POPJMP (opljbufp2, pljbufp2);
        pljbufp1 = (jmp_buf *) opljbufp1;
    }
    flvari = sinfop[ownsinfoi].flvari;
    llvari = sinfop[ownsinfoi].llvari;
    sinfoi = ownsinfoi;
    Mpopmark (m);
    lrtno = rtno, rtno = NULL;
    errdo = TRUE;
    return lrtno;
}

static void ewhilest (Tobj co, int ci) {
    volatile jmp_buf *opljbufp;
    volatile jmp_buf pljbuf;
    volatile Tobj c1o;
    volatile int ei, si;

    Tobj v1o;

    c1o = (Tobj) co; /* protect argument from longjmp */
    ei = TCgetfp (c1o, ci);
    si = TCgetnext (c1o, ei);
    PUSHJMP (opljbufp, pljbufp1, pljbuf);
    for (;;) {
        if (!(v1o = eeval ((Tobj) c1o, ei)))
            err (ERRNORHS, ERR5, c1o, ei);
        if (boolop (v1o) == FALSE)
            break;
        if (setjmp (*pljbufp1)) {
            if (pljtype == PLJ_CONTINUE)
                continue;
            else if (pljtype == PLJ_BREAK)
                break;
        }
        eeval ((Tobj) c1o, si);
    }
    POPJMP (opljbufp, pljbufp1);
}

static void eforst (Tobj co, int ci) {
    volatile jmp_buf *opljbufp;
    volatile jmp_buf pljbuf;
    volatile Tobj c1o;
    volatile int ei1, ei2, ei3, si, eisnop1, eisnop2, eisnop3;

    Tobj v1o;

    c1o = (Tobj) co; /* protect argument from longjmp */
    ei1 = TCgetfp (c1o, ci);
    ei2 = TCgetnext (c1o, ei1);
    ei3 = TCgetnext (c1o, ei2);
    si = TCgetnext (c1o, ei3);
    eisnop1 = (TCgettype (c1o, ei1) == C_NOP);
    eisnop2 = (TCgettype (c1o, ei2) == C_NOP);
    eisnop3 = (TCgettype (c1o, ei3) == C_NOP);
    PUSHJMP (opljbufp, pljbufp1, pljbuf);
    if (!eisnop1)
        eeval ((Tobj) c1o, ei1);
    for (;;) {
        if (!eisnop2) {
            if (!(v1o = eeval ((Tobj) c1o, ei2)))
                err (ERRNORHS, ERR5, c1o, ei2);
            if (boolop (v1o) == FALSE)
                break;
        }
        if (setjmp (*pljbufp1) != 0) {
            if (pljtype == PLJ_CONTINUE)
                ;
            else if (pljtype == PLJ_BREAK)
                break;
        } else {
            eeval ((Tobj) c1o, si);
        }
        if (!eisnop3)
            eeval ((Tobj) c1o, ei3);
    }
    POPJMP (opljbufp, pljbufp1);
}

static void eforinst (Tobj co, int ci) {
    volatile jmp_buf *opljbufp;
    volatile jmp_buf pljbuf;
    volatile Tobj tblo, c1o;
    volatile Tkvindex_t tkvi;
    volatile tnk_t tnk;
    volatile long tm, km, t;
    volatile int ei1, ei2, si;

    c1o = (Tobj) co; /* protect argument from longjmp */
    ei1 = TCgetfp (c1o, ci);
    ei2 = TCgetnext (c1o, ei1);
    si = TCgetnext (c1o, ei2);
    if (getvar ((Tobj) c1o, ei1, (tnk_t *) &tnk) == -1) {
        err (ERRNOLHS, ERR3, c1o, ei1);
        return;
    }
    if (tnk.type == TNK_O)
        km = Mpushmark (tnk.u.tnko.ko);
    if (!(tblo = (Tobj) eeval ((Tobj) c1o, ei2))) {
        if (tnk.type == TNK_O)
            Mpopmark (km);
        err (ERRNORHS, ERR4, c1o, ei2);
        return;
    }
    if (Tgettype (tblo) != T_TABLE) {
        err (ERRNOTATABLE, ERR1, c1o, ei2);
        return;
    }
    tm = Mpushmark (tblo);
    PUSHJMP (opljbufp, pljbufp1, pljbuf);
    t = Tgettime (tblo);
    Ttime++;
    for (
        Tgetfirst ((Tobj) tblo, (Tkvindex_t *) &tkvi); tkvi.kvp;
        Tgetnext ((Tkvindex_t *) &tkvi)
    ) {
        setvar (tnk, tkvi.kvp->ko);
        if (setjmp (*pljbufp1) != 0) {
            if (pljtype == PLJ_CONTINUE)
                continue;
            else if (pljtype == PLJ_BREAK)
                break;
        }
        eeval ((Tobj) c1o, si);
        if (t != Tgettime (tblo)) {
            err (ERRTABLECHANGED, ERR1, c1o, ei2);
            break;
        }
    }
    POPJMP (opljbufp, pljbufp1);
    Mpopmark (tm);
    if (tnk.type == TNK_O)
        Mpopmark (km);
}

static Tobj getval (Tobj co, int ci) {
    Tobj cvo, cko, cto;
    int ct, vt;
    int vi, ni, nn;

    cvo = NULL;
    if ((ct = TCgettype (co, ci)) == C_LVAR) {
        nn = (int) TCgetinteger (co, (ni = TCgetnext (co, TCgetfp (co, ci))));
        cto = cvo = lvarp[flvari + nn].o;
        if (!cto)
            return NULL;
        vi = TCgetnext (co, ni);
    } else if (ct == C_GVAR) {
        cto = root;
        vi = TCgetfp (co, ci);
    } else if (ct == C_PVAR)
        return TCgetobject (co, ci);
    else
        return NULL;

    while (vi != C_NULL) {
        if (Tgettype (cto) != T_TABLE)
            return NULL;
        if ((vt = TCgettype (co, vi)) == C_STRING) {
            if (!(cvo = Tfinds (cto, TCgetstring (co, vi))))
                return NULL;
        } else if (vt == C_INTEGER) {
            if (!(cvo = Tfindi (cto, TCgetinteger (co, vi))))
                return NULL;
        } else if (vt == C_REAL) {
            if (!(cvo = Tfindr (cto, TCgetreal (co, vi))))
                return NULL;
        } else {
            if (!(cko = eeval (co, vi)) || !(cvo = Tfindo (cto, cko)))
                return NULL;
        }
        cto = cvo;
        vi = TCgetnext (co, vi);
    }
    return cvo;
}

static int getvar (Tobj co, int ci, tnk_t *tnkp) {
    Tobj cvo, cko, cto;
    int ct, vt;
    long m;
    int vi, ovi, nn, ni;

    nn = 0;
    vt = 0;
    cko = cto = NULL;
    if ((ct = TCgettype (co, ci)) == C_LVAR) {
        nn = (int) TCgetinteger (co, (ni = TCgetnext (co, TCgetfp (co, ci))));
        cvo = cto = lvarp[flvari + nn].o;
        vi = TCgetnext (co, ni);
        if (vi != C_NULL && (!cvo || Tgettype (cvo) != T_TABLE))
            Mresetmark (
                lvarp[flvari + nn].m,
                (lvarp[flvari + nn].o = cvo = cto = Ttable (0))
            );
    } else if (ct == C_GVAR) { /* else it's a global variable */
        cvo = root;
        vi = TCgetfp (co, ci);
    } else {
        return -1;
    }

    ovi = -1;
    while (vi != C_NULL) {
        cto = cvo;
        if ((vt = TCgettype (co, vi)) == C_STRING) {
            cvo = Tfinds (cto, TCgetstring (co, vi));
        } else if (vt == C_INTEGER) {
            cvo = Tfindi (cto, TCgetinteger (co, vi));
        } else if (vt == C_REAL) {
            cvo = Tfindr (cto, TCgetreal (co, vi));
        } else {
            if (
                !(cko = eeval (co, vi)) || !(T_ISSTRING (cko) ||
                T_ISNUMBER (cko))
            )
                return -1;
            cvo = Tfindo (cto, cko);
        }
        ovi = vi, vi = TCgetnext (co, vi);
        if (vi != C_NULL && (!cvo || Tgettype (cvo) != T_TABLE)) {
            if (vt == C_STRING)
                Tinss (cto, TCgetstring (co, ovi), (cvo = Ttable (0)));
            else if (vt == C_INTEGER)
                Tinsi (cto, TCgetinteger (co, ovi), (cvo = Ttable (0)));
            else if (vt == C_REAL)
                Tinsr (cto, TCgetreal (co, ovi), (cvo = Ttable (0)));
            else {
                m = Mpushmark (cko);
                Tinso (cto, cko, (cvo = Ttable (0))),
                Mpopmark (m);
            }
        }
    }
    if (ct == C_LVAR && ovi == -1) {
        tnkp->type = TNK_LI;
        tnkp->u.li = nn;
    } else {
        switch (vt) {
        case C_STRING:
        case C_INTEGER:
        case C_REAL:
            tnkp->type = TNK_S;
            tnkp->u.tnks.kt = vt;
            tnkp->u.tnks.to = cto;
            tnkp->u.tnks.co = co;
            tnkp->u.tnks.vi = ovi;
            break;
        default:
            tnkp->type = TNK_O;
            tnkp->u.tnko.to = cto;
            tnkp->u.tnko.ko = cko;
            break;
        }
    }
    return 0;
}

static void setvar (tnk_t tnk, Tobj vo) {
    switch (tnk.type) {
    case TNK_LI:
        Mresetmark (
            lvarp[flvari + tnk.u.li].m, (lvarp[flvari + tnk.u.li].o = vo)
        );
        break;
    case TNK_O:
        Tinso (tnk.u.tnko.to, tnk.u.tnko.ko, vo);
        break;
    default:
        switch (tnk.u.tnks.kt) {
        case C_STRING:
            Tinss (
                tnk.u.tnks.to, TCgetstring (tnk.u.tnks.co, tnk.u.tnks.vi), vo
            );
            break;
        case C_INTEGER:
            Tinsi (
                tnk.u.tnks.to, TCgetinteger (tnk.u.tnks.co, tnk.u.tnks.vi), vo
            );
            break;
        case C_REAL:
            Tinsr (tnk.u.tnks.to, TCgetreal (tnk.u.tnks.co, tnk.u.tnks.vi), vo);
            break;
        }
        break;
    }
}

static int boolop (Tobj vo) {
    long i;
    double d;

    if (!vo)
        return FALSE;

    switch (Tgettype (vo)) {
    case T_INTEGER:
        i = Tgetinteger (vo);
        return (i == 0) ? FALSE : TRUE;
    case T_REAL:
        d = Tgetreal (vo);
        return (d == 0.0) ? FALSE : TRUE;
    case T_TABLE:
        if (vo == null)
            return FALSE;
        return TRUE;
    default:
        return TRUE;
    }
}

static int orderop (Tobj v1o, int op, Tobj v2o) {
    int t1, t2;
    long i1, i2;
    int r;
    double d1, d2;

    if (!v1o || !v2o) {
        if ((v1o || v2o) && op == C_NE)
            return TRUE;
        return FALSE;
    }
    t1 = Tgettype (v1o), t2 = Tgettype (v2o);
    if (t1 == T_STRING && t2 == T_STRING) {
        r = strcmp (Tgetstring (v1o), Tgetstring (v2o));
    } else if (t1 == T_INTEGER && t2 == T_INTEGER) {
        i1 = Tgetinteger (v1o), i2 = Tgetinteger (v2o);
        r = (i1 == i2) ? 0 : ((i1 < i2) ? -1 : 1);
    } else if (t1 == T_INTEGER && t2 == T_REAL) {
        i1 = Tgetinteger (v1o), d2 = Tgetreal (v2o);
        r = (i1 == d2) ? 0 : ((i1 < d2) ? -1 : 1);
    } else if (t1 == T_REAL && t2 == T_INTEGER) {
        d1 = Tgetreal (v1o), i2 = Tgetinteger (v2o);
        r = (d1 == i2) ? 0 : ((d1 < i2) ? -1 : 1);
    } else if (t1 == T_REAL && t2 == T_REAL) {
        d1 = Tgetreal (v1o), d2 = Tgetreal (v2o);
        r = (d1 == d2) ? 0 : ((d1 < d2) ? -1 : 1);
    } else if (t1 == t2) {
        if (op != C_EQ && op != C_NE)
            return FALSE;
        r = (v1o == v2o) ? 0 : 1;
    } else {
        return FALSE;
    }
    switch (op) {
    case C_EQ: return (r == 0) ? TRUE : FALSE;
    case C_NE: return (r != 0) ? TRUE : FALSE;
    case C_LT: return (r <  0) ? TRUE : FALSE;
    case C_LE: return (r <= 0) ? TRUE : FALSE;
    case C_GT: return (r >  0) ? TRUE : FALSE;
    case C_GE: return (r >= 0) ? TRUE : FALSE;
    }
    panic1 (POS, "orderop", "bad op code");
    return FALSE; /* NOT REACHED */
}

static Tobj arithop (Num_t *lnum, int op, Num_t *rnum) {
    double d1, d2, d3;

    d3 = 0.0;
    if (!rnum && op != C_UMINUS)
        return NULL;
    if (lnum->type == C_INTEGER)
        d1 = lnum->u.i;
    else if (lnum->type == C_REAL)
        d1 = lnum->u.d;
    else if (!lnum->u.no)
        return NULL;
    else if (Tgettype (lnum->u.no) == T_INTEGER)
        d1 = Tgetinteger (lnum->u.no);
    else if (Tgettype (lnum->u.no) == T_REAL)
        d1 = Tgetreal (lnum->u.no);
    else
        return NULL;
    if (op == C_UMINUS) {
        d3 = -d1;
        goto result;
    }
    if (rnum->type == C_INTEGER)
        d2 = rnum->u.i;
    else if (rnum->type == C_REAL)
        d2 = rnum->u.d;
    else if (!rnum->u.no)
        return NULL;
    else if (Tgettype (rnum->u.no) == T_INTEGER)
        d2 = Tgetinteger (rnum->u.no);
    else if (Tgettype (rnum->u.no) == T_REAL)
        d2 = Tgetreal (rnum->u.no);
    else
        return NULL;
    switch (op) {
    case C_PLUS:  d3 = d1 + d2;               break;
    case C_MINUS: d3 = d1 - d2;               break;
    case C_MUL:   d3 = d1 * d2;               break;
    case C_DIV:   d3 = d1 / d2;               break;
    case C_MOD:   d3 = (long) d1 % (long) d2; break;
    }
result:
    if (d3 == (double) (long) d3)
        return Tinteger ((long) d3);
    return Treal (d3);
}

static void err (int errnum, int level, Tobj co, int ci) {
    char *s;
    int si, i;

    if (level > Eerrlevel || !errdo)
        return;
    s = "";
    fprintf (stderr, "runtime error: %s\n", errnam[errnum]);
    if (!co)
        return;
    if (Estackdepth < 1)
        return;
    if (!sinfop[(si = sinfoi - 1)].fco && si > 0)
        si--;
    if (Eshowbody > 0) {
        if (co == sinfop[si].fco)
            s = Scfull (co, 0, ci);
        else if (co == sinfop[si].co)
            s = Scfull (co, TCgetfp (co, 0), ci);
        printbody (s, Eshowbody), free (s);
        if (Estackdepth == 1) {
            fprintf (stderr, "\n");
            errdo = FALSE;
        }
        for (i = si; i >= 0; i--) {
            if (sinfop[i].fco) {
                s = Scfull (sinfop[i].fco, 0, sinfop[i].fci);
                printbody (s, Eshowbody), free (s);
            }
        }
        s = Scfull (sinfop[0].co, TCgetfp (sinfop[0].co, 0), sinfop[0].ci);
        printbody (s, Eshowbody), free (s);
    }
    fprintf (stderr, "\n");
    errdo = FALSE;
}

static void printbody (char *s, int mode) {
    char *s1, *s2;
    char c;

    if (mode == 2) {
        fprintf (stderr, "%s\n", s);
        return;
    }
    c = '\000';
    for (s1 = s; *s1; s1++)
        if (*s1 == '>' && *(s1 + 1) && *(s1 + 1) == '>')
            break;
    if (!*s1)
        return;
    for (; s1 != s; s1--)
        if (*(s1 - 1) == '\n')
            break;
    for (s2 = s1; *s2; s2++)
        if (*s2 == '\n')
            break;
    if (*s2)
        c = *s2, *s2 = '\000';
    fprintf (stderr, "%s\n", s1);
    if (c)
        *s2 = c;
}
