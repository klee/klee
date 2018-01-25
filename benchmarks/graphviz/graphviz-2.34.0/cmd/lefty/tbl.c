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

long Ttime = 0;
int Tstringoffset, Tcodeoffset, Tkvoffset;
Tobj Ttrue, Tfalse;

#define ISEQIK(ik, ko) ( \
    T_ISNUMBER (ko) && Tgetnumber (ko) == (ik) \
)
#define ISEQRK(rk, ko) ( \
    T_ISNUMBER (ko) && Tgetnumber (ko) == (rk) \
)
#define ISEQSK(sk, ko) ( \
    T_ISSTRING (ko)  && strcmp (((Tstring_t *) (ko))->s, (sk)) == 0 \
)

#define GETIKINDEX(tp, ik) (unsigned long) ik  % tp->ln
#define GETRKINDEX(tp, rk) (unsigned long) rk  % tp->ln
#define GETSKINDEX(tp, sk) (unsigned long) *sk % tp->ln

typedef struct mapentry_t {
    struct mapentry_t *next;
    Tobj fmo, too;
} mapentry_t;
#define MAPENTRYSIZE sizeof (mapentry_t)
#define MAPLISTN 100
typedef struct Map_t {
    struct mapentry_t *list[MAPLISTN];
} Map_t;
static Map_t map;
static long mapentrybyte2size;

static long truem, falsem;

static Tinteger_t keyi;
static Treal_t keyr;
static Tstring_t keys;

static void insert (Ttable_t *, Tobj, char *, Tobj);
static Tobj find (Ttable_t *, Tobj, char *);
static void delete (Ttable_t *, Tobj, char *);
static void copytable (Ttable_t *, long);
static void reccopytable (Ttable_t *, Ttable_t *);
static void mapinit (void);
static void mapterm (void);
static void mapinsert (Tobj, Tobj);
static Tobj mapfind (Tobj);

void Tinit (void) {
    Tstring_t s;
    Tcode_t c;
    Tkvlist_t kvl;

    Mhaspointers[T_INTEGER] = FALSE;
    Mhaspointers[T_REAL] = FALSE;
    Mhaspointers[T_STRING] = FALSE;
    Mhaspointers[T_CODE] = FALSE;
    Mhaspointers[T_TABLE] = TRUE;
    Ttrue = Tinteger (1);
    truem = Mpushmark (Ttrue);
    Tfalse = Tinteger (0);
    falsem = Mpushmark (Tfalse);
    Tstringoffset = (char *) &s.s[0] - (char *) &s + 1;
    /* the + 1 above accounts for the null character */
    Tcodeoffset = (char *) &c.c[0] - (char *) &c;
    Tkvoffset = (char *) &kvl.kv[0] - (char *) &kvl;
    keyi.head.type = T_INTEGER;
    keyr.head.type = T_REAL;
    keys.head.type = T_STRING;
    mapentrybyte2size = M_BYTE2SIZE (MAPENTRYSIZE);
}

void Tterm (void) {
    Mpopmark (falsem);
    Tfalse = NULL;
    Mpopmark (truem);
    Ttrue = NULL;
    Mdogc (M_GCFULL);
    Mdogc (M_GCFULL);
}

void Tgchelper (void *p) {
    Ttable_t *tp;
    Tkvlist_t *kvlp;
    long i, j;

    /* must be a table */
    tp = (Ttable_t *) p;
    for (i = 0; i < tp->ln; i++)
        if ((kvlp = tp->lp[i]))
            for (j = 0; j < kvlp->i; j++)
                Mmkcurr (kvlp->kv[j].ko), Mmkcurr (kvlp->kv[j].vo);
}

void Tfreehelper (void *p) {
    Ttable_t *tp;
    Tkvlist_t *kvlp;
    long i;

    /* must be a table */
    tp = (Ttable_t *) p;
    for (i = 0; i < tp->ln; i++)
        if ((kvlp = tp->lp[i]))
            Mfree (kvlp, M_BYTE2SIZE (T_KVLISTSIZE (kvlp->n)));
    Mfree (tp->lp, M_BYTE2SIZE (tp->ln * T_KVLISTPTRSIZE));
}

Tobj Tinteger (long i) {
    Tinteger_t *ip;

    ip = Mnew (T_INTEGERSIZE, T_INTEGER);
    ip->i = i;
    return ip;
}

Tobj Treal (double d) {
    Treal_t *rp;

    if (d == (double) (long) d)
        return Tinteger ((long) d);
    rp = Mnew (T_REALSIZE, T_REAL);
    rp->d = d;
    return rp;
}

Tobj Tstring (char *s) {
    Tstring_t *sp;

    sp = Mnew ((long) T_STRINGSIZE (strlen (s)), T_STRING);
    strcpy (&sp->s[0], s);
    return sp;
}

Tobj Tcode (Code_t *cp, int ci, int cl) {
    Tcode_t *codep;
    Code_t *cp2;
    char *s;
    int i, j, cn;

    codep = Mnew ((long) T_CODESIZE (cl), T_CODE);
    cp2 = codep->c;
    for (i = 0; i < cl; i++) {
        switch (cp[i].ctype) {
        case C_INTEGER:
            cp2[i] = cp[i];
            if (cp2[i].next != C_NULL)
                cp2[i].next -= ci;
            break;
        case C_REAL:
            cp2[i] = cp[i];
            if (cp2[i].next != C_NULL)
                cp2[i].next -= ci;
            break;
        case C_STRING:
            cp2[i] = cp[i];
            if (cp2[i].next != C_NULL)
                cp2[i].next -= ci;
            s = (char *) &cp[i].u.s;
            while (*s)
                s++;
            cn = (long) (s - (char *) &cp[i]) / sizeof (Code_t);
            for (j = 0; j < cn; j++)
                i++, cp2[i] = cp[i];
            break;
        default:
            cp2[i] = cp[i];
            if (cp2[i].next != C_NULL)
                cp2[i].next -= ci;
            if (cp2[i].u.fp != C_NULL)
                cp2[i].u.fp -= ci;
            break;
        }
    }
    return codep;
}

Tobj Ttable (long sizehint) {
    Ttable_t *tp;
    Tkvlist_t **lp;
    long i;

    sizehint = (sizehint < 2) ? 2 : sizehint;
    tp = Mnew (T_TABLESIZE, T_TABLE);
    lp = Mallocate ((long) (sizehint * T_KVLISTPTRSIZE));
    tp->lp = lp;
    tp->ln = sizehint;
    tp->n = 0;
    tp->time = Ttime;
    for (i = 0; i < sizehint; i++)
        lp[i] = NULL;
    return tp;
}


void Tinsi (Tobj to, long ik, Tobj vo) {
    long tm;

    if (!to || !T_ISTABLE (to))
        panic1 (POS, "Tinsi", "insert attempted on non-table");
    tm = Mpushmark (to);
    if (vo)
        Mpushmark (vo);
    keyi.i = ik;
    insert (to, &keyi, NULL, vo);
    Mpopmark (tm);
}

void Tinsr (Tobj to, double rk, Tobj vo) {
    long tm;

    if (!to || !T_ISTABLE (to))
        panic1 (POS, "Tinsr", "insert attempted on non-table");
    tm = Mpushmark (to);
    if (vo)
        Mpushmark (vo);
    keyr.d = rk;
    insert (to, &keyr, NULL, vo);
    Mpopmark (tm);
}

void Tinss (Tobj to, char *sk, Tobj vo) {
    long tm;

    if (!to || !T_ISTABLE (to))
        panic1 (POS, "Tinss", "insert attempted on non-table");
    tm = Mpushmark (to);
    if (vo)
        Mpushmark (vo);
    insert (to, &keys, sk, vo);
    Mpopmark (tm);
}

void Tinso (Tobj to, Tobj ko, Tobj vo) {
    long tm;

    if (!to || !T_ISTABLE (to))
        panic1 (POS, "Tinso", "insert attempted on non-table");
    if (!ko || !(T_ISINTEGER (ko) || T_ISREAL (ko) || T_ISSTRING (ko)))
        panic1 (POS, "Tinso", "bad key");
    tm = Mpushmark (to);
    Mpushmark (ko);
    if (vo)
        Mpushmark (vo);
    insert (to, ko, NULL, vo);
    Mpopmark (tm);
}

Tobj Tfindi (Tobj to, long ik) {
    if (!to)
        return NULL;
    if (!T_ISTABLE (to))
        panic1 (POS, "Tfindi", "find attempted on non-table");
    keyi.i = ik;
    return find (to, &keyi, NULL);
}

Tobj Tfindr (Tobj to, double rk) {
    if (!to)
        return NULL;
    if (!T_ISTABLE (to))
        panic1 (POS, "Tfindr", "find attempted on non-table");
    keyr.d = rk;
    return find (to, &keyr, NULL);
}

Tobj Tfinds (Tobj to, char *sk) {
    if (!to)
        return NULL;
    if (!T_ISTABLE (to))
        panic1 (POS, "Tfinds", "find attempted on non-table");
    return find (to, &keys, sk);
}

Tobj Tfindo (Tobj to, Tobj ko) {
    if (!to || !ko)
        return NULL;
    if (!T_ISTABLE (to))
        panic1 (POS, "Tfindo", "find attempted on non-table");
    if (!(T_ISINTEGER (ko) || T_ISREAL (ko) || T_ISSTRING (ko)))
        panic1 (POS, "Tfindo", "bad key");
    return find (to, ko, NULL);
}

void Tdeli (Tobj to, long ik) {
    if (!to)
        return;
    if (!T_ISTABLE (to))
        panic1 (POS, "Tdeli", "delete attempted on non-table");
    keyi.i = ik;
    delete (to, &keyi, NULL);
}

void Tdelr (Tobj to, double rk) {
    if (!to)
        return;
    if (!T_ISTABLE (to))
        panic1 (POS, "Tdelr", "delete attempted on non-table");
    keyr.d = rk;
    delete (to, &keyr, NULL);
}

void Tdels (Tobj to, char *sk) {
    if (!to)
        return;
    if (!T_ISTABLE (to))
        panic1 (POS, "Tdels", "delete attempted on non-table");
    delete (to, &keys, sk);
}

void Tdelo (Tobj to, Tobj ko) {
    if (!to || !ko)
        return;
    if (!T_ISTABLE (to))
        panic1 (POS, "Tdelo", "delete attempted on non-table");
    if (!(T_ISINTEGER (ko) || T_ISREAL (ko) || T_ISSTRING (ko)))
        panic1 (POS, "Tdelo", "bad key");
    delete (to, ko, NULL);
}

Tobj Tcopy (Tobj fmvo) {
    Tobj tovo;
    long m;

    tovo = NULL;
    switch (M_TYPEOF (fmvo)) {
    case T_INTEGER: case T_REAL: case T_STRING: case T_CODE:
        tovo = fmvo;
        break;
    case T_TABLE:
        mapinit ();
        m = Mpushmark (fmvo);
        tovo = Mnew (T_TABLESIZE, T_TABLE);
        mapinsert (fmvo, tovo);
        reccopytable (fmvo, tovo);
        Mpopmark (m);
        mapterm ();
        break;
    }
    return tovo;
}

void Tgetfirst (Tobj to, Tkvindex_t *p) {
    if (!to || !T_ISTABLE (to))
        return;
    p->tp = to, p->kvp = NULL, p->i = 0, p->j = 0;
    for (; p->i < p->tp->ln; p->i++) {
        if (!p->tp->lp[p->i])
            continue;
        for (; p->j < p->tp->lp[p->i]->i; p->j++) {
            if ((p->kvp = &p->tp->lp[p->i]->kv[p->j]))
                return;
        }
        p->j = 0;
    }
}

void Tgetnext (Tkvindex_t *p) {
    p->kvp = NULL;
    p->j++;
    for (; p->i < p->tp->ln; p->i++) {
        if (!p->tp->lp[p->i])
            continue;
        for (; p->j < p->tp->lp[p->i]->i; p->j++) {
            if ((p->kvp = &p->tp->lp[p->i]->kv[p->j]))
                return;
        }
        p->j = 0;
    }
}

static void insert (Ttable_t *tp, Tobj ko, char *sk, Tobj vo) {
    Tkvlist_t *kvlp, *nkvlp;
    Tkv_t *kvp;
    int kt;
    long ik, i, ind, nln;
    double rk;

    rk = 0.0;
    kvlp = NULL;
    ind = ik = 0;
    switch ((kt = M_TYPEOF (ko))) {
    case T_INTEGER:
        ik = ((Tinteger_t *) ko)->i;
        if ((kvlp = tp->lp[(ind = GETIKINDEX (tp, ik))]))
            for (i = 0, kvp = &kvlp->kv[0]; i < kvlp->i; i++)
                if (ISEQIK (ik, kvp[i].ko))
                    goto found;
        break;
    case T_REAL:
        rk = ((Treal_t *) ko)->d;
        if ((kvlp = tp->lp[(ind = GETRKINDEX (tp, rk))]))
            for (i = 0, kvp = &kvlp->kv[0]; i < kvlp->i; i++)
                if (ISEQRK (rk, kvp[i].ko))
                    goto found;
        break;
    case T_STRING:
        if (M_AREAOF (ko) != 0)
            sk = ((Tstring_t *) ko)->s;
        if ((kvlp = tp->lp[(ind = GETSKINDEX (tp, sk))]))
            for (i = 0, kvp = &kvlp->kv[0]; i < kvlp->i; i++)
                if (ISEQSK (sk, kvp[i].ko))
                    goto found;
        break;
    }
    if ((nln = tp->n + 1) > 4 * tp->ln && nln < M_SIZEMAX) {
        copytable (tp, nln);
        switch (kt) {
        case T_INTEGER: ind = GETIKINDEX (tp, ik); break;
        case T_REAL:    ind = GETRKINDEX (tp, rk); break;
        case T_STRING:  ind = GETSKINDEX (tp, sk); break;
        }
        kvlp = tp->lp[ind];
    }
    if (!kvlp) {
        tp->lp[ind] = kvlp = Mallocate ((long) T_KVLISTSIZE (1));
        kvlp->i = 0, kvlp->n = 1;
    } else if (kvlp->i == kvlp->n) {
        tp->lp[ind] = nkvlp = Mallocate ((long) T_KVLISTSIZE (kvlp->n * 2));
        nkvlp->n = kvlp->n * 2;
        for (i = 0; i < kvlp->n; i++)
            nkvlp->kv[i] = kvlp->kv[i];
        nkvlp->i = kvlp->i;
        Mfree (kvlp, M_BYTE2SIZE (T_KVLISTSIZE (kvlp->n))), kvlp = nkvlp;
    }
    if (M_AREAOF (ko) == 0) { /* ko must be allocated */
        switch (kt) {
        case T_INTEGER: ko = Tinteger (ik); break;
        case T_REAL:    ko = Treal (rk);    break;
        case T_STRING:  ko = Tstring (sk);  break;
        }
    }

    kvlp->kv[kvlp->i].ko = ko, kvlp->kv[kvlp->i++].vo = vo;
    tp->n++;
    tp->time = Ttime;
    return;

found:
    kvp[i].vo = vo;
    tp->time = Ttime;
}

static Tobj find (Ttable_t *tp, Tobj ko, char *sk) {
    Tkvlist_t *kvlp;
    Tkv_t *kvp;
    long ik, i;
    double rk;

    switch (M_TYPEOF (ko)) {
    case T_INTEGER:
        ik = ((Tinteger_t *) ko)->i;
        if ((kvlp = tp->lp[GETIKINDEX (tp, ik)]))
            for (i = 0, kvp = &kvlp->kv[0]; i < kvlp->i; i++)
                if (ISEQIK (ik, kvp[i].ko))
                    goto found;
        break;
    case T_REAL:
        rk = ((Treal_t *) ko)->d;
        if ((kvlp = tp->lp[GETRKINDEX (tp, rk)]))
            for (i = 0, kvp = &kvlp->kv[0]; i < kvlp->i; i++)
                if (ISEQRK (rk, kvp[i].ko))
                    goto found;
        break;
    case T_STRING:
        if (M_AREAOF (ko) != 0)
            sk = ((Tstring_t *) ko)->s;
        if ((kvlp = tp->lp[GETSKINDEX (tp, sk)]))
            for (i = 0, kvp = &kvlp->kv[0]; i < kvlp->i; i++)
                if (ISEQSK (sk, kvp[i].ko))
                    goto found;
        break;
    }
    return NULL;

found:
    return kvp[i].vo;
}

static void delete (Ttable_t *tp, Tobj ko, char *sk) {
    Tkvlist_t *kvlp;
    Tkv_t *kvp;
    long ik, i, j;
    double rk;

    switch (M_TYPEOF (ko)) {
    case T_INTEGER:
        ik = ((Tinteger_t *) ko)->i;
        if ((kvlp = tp->lp[GETIKINDEX (tp, ik)]))
            for (i = 0, kvp = &kvlp->kv[0]; i < kvlp->i; i++)
                if (ISEQIK (ik, kvp[i].ko))
                    goto found;
        break;
    case T_REAL:
        rk = ((Treal_t *) ko)->d;
        if ((kvlp = tp->lp[GETRKINDEX (tp, rk)]))
            for (i = 0, kvp = &kvlp->kv[0]; i < kvlp->i; i++)
                if (ISEQRK (rk, kvp[i].ko))
                    goto found;
        break;
    case T_STRING:
        if (M_AREAOF (ko) != 0)
            sk = ((Tstring_t *) ko)->s;
        if ((kvlp = tp->lp[GETSKINDEX (tp, sk)]))
            for (i = 0, kvp = &kvlp->kv[0]; i < kvlp->i; i++)
                if (ISEQSK (sk, kvp[i].ko))
                    goto found;
        break;
    }
    return;

found:
    for (j = i, kvp = &kvlp->kv[0]; j < kvlp->i - 1; j++)
        kvp[j] = kvp[j + 1];
    kvlp->i--;
    tp->n--;
    tp->time = Ttime;
}

static void copytable (Ttable_t *tp, long ln) {
    Tkvlist_t **olp, **lp;
    Tkvlist_t *okvlp, *kvlp, *nkvlp;
    Tkv_t *kvp;
    long ik, oln, i, j, k, ind;
    double rk;
    char *sk;

    lp = Mallocate ((long) (ln * T_KVLISTPTRSIZE));
    oln = tp->ln, tp->ln = ln;
    olp = tp->lp, tp->lp = lp;
    kvlp = NULL;
    ind = 0;
    for (i = 0; i < ln; i++)
        lp[i] = NULL;
    for (i = 0; i < oln; i++) {
        if (!(okvlp = olp[i]))
            continue;
        for (j = 0; j < okvlp->i; j++) {
            kvp = &okvlp->kv[j];
            switch (M_TYPEOF (kvp->ko)) {
            case T_INTEGER:
                ik = ((Tinteger_t *) kvp->ko)->i;
                kvlp = lp[(ind = GETIKINDEX (tp, ik))];
                break;
            case T_REAL:
                rk = ((Treal_t *) kvp->ko)->d;
                kvlp = lp[(ind = GETRKINDEX (tp, rk))];
                break;
            case T_STRING:
                sk = ((Tstring_t *) kvp->ko)->s;
                kvlp = lp[(ind = GETSKINDEX (tp, sk))];
                break;
            }
            if (!kvlp) {
                lp[ind] = kvlp = Mallocate ((long) T_KVLISTSIZE (1));
                kvlp->i = 0, kvlp->n = 1;
            } else if (kvlp->i == kvlp->n) {
                lp[ind] = nkvlp = Mallocate ((long) T_KVLISTSIZE (kvlp->n * 2));
                nkvlp->n = kvlp->n * 2;
                for (k = 0; k < kvlp->i; k++)
                    nkvlp->kv[k] = kvlp->kv[k];
                nkvlp->i = kvlp->i;
                Mfree (kvlp, M_BYTE2SIZE (T_KVLISTSIZE (kvlp->n)));
                kvlp = nkvlp;
            }
            kvlp->kv[kvlp->i].ko = kvp->ko, kvlp->kv[kvlp->i++].vo = kvp->vo;
        }
        Mfree (okvlp, M_BYTE2SIZE (T_KVLISTSIZE (okvlp->n)));
    }
    Mfree (olp, M_BYTE2SIZE (oln * T_KVLISTPTRSIZE));
}

static void reccopytable (Ttable_t *fmtp, Ttable_t *totp) {
    Tkv_t *fmkvp, *tokvp;
    long i, j, m;

    totp->lp = Mallocate ((long) (fmtp->ln * T_KVLISTPTRSIZE));
    totp->ln = fmtp->ln;
    totp->n = fmtp->n;
    totp->time = Ttime;
    for (i = 0; i < totp->ln; i++) {
        if (!fmtp->lp[i]) {
            totp->lp[i] = NULL;
            continue;
        }
        totp->lp[i] = Mallocate ((long) T_KVLISTSIZE (fmtp->lp[i]->n));
        totp->lp[i]->n = fmtp->lp[i]->n;
        totp->lp[i]->i = 0;
    }
    m = Mpushmark (totp);
    for (i = 0; i < totp->ln; i++) {
        if (!totp->lp[i])
            continue;
        for (j = 0; j < fmtp->lp[i]->i; j++) {
            fmkvp = &fmtp->lp[i]->kv[j], tokvp = &totp->lp[i]->kv[j];
            tokvp->ko = fmkvp->ko;
            switch (M_TYPEOF (fmkvp->vo)) {
            case T_INTEGER: case T_REAL: case T_STRING: case T_CODE:
                tokvp->vo = fmkvp->vo;
                break;
            case T_TABLE:
                if (!(tokvp->vo = mapfind (fmkvp->vo))) {
                    tokvp->vo = Mnew (T_TABLESIZE, T_TABLE);
                    mapinsert (fmkvp->vo, tokvp->vo);
                    reccopytable (fmkvp->vo, tokvp->vo);
                }
                break;
            }
            totp->lp[i]->i++;
        }
    }
    Mpopmark (m);
}

static void mapinit (void) {
    long li;

    for (li = 0; li < MAPLISTN; li++)
        map.list[li] = NULL;
}

static void mapterm (void) {
    mapentry_t **lp;
    mapentry_t *cep, *nep;
    long li;

    for (li = 0; li < MAPLISTN; li++) {
        lp = &map.list[li];
        for (cep = *lp; cep; cep = nep) {
            nep = cep->next;
            Mfree (cep, mapentrybyte2size);
        }
        *lp = NULL;
    }
}

static void mapinsert (Tobj fmo, Tobj too) {
    mapentry_t **lp;
    mapentry_t *cep;

    lp = &map.list[(unsigned long) fmo % MAPLISTN];
    if (!(cep = Mallocate (MAPENTRYSIZE)))
        panic1 (POS, "mapinsert", "cannot allocate mapentry");
    cep->fmo = fmo, cep->too = too;
    cep->next = *lp, *lp = cep;
}

static Tobj mapfind (Tobj fmo) {
    mapentry_t **lp;
    mapentry_t *cep;

    lp = &map.list[(unsigned long) fmo % MAPLISTN];
    for (cep = *lp; cep; cep = cep->next)
        if (cep->fmo == fmo)
            return cep->too;
    return NULL;
}
