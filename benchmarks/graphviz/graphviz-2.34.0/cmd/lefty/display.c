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
#include "display.h"

typedef struct dnode_t {
    int ttype;
    Tobj ko, vo;
    char *path;
} dnode_t;

typedef struct seennode_t {
    Tobj vo;
    char *path;
} seennode_t;
static seennode_t *seenp;
static int seeni, seenn;
#define SEENINCR 100
#define SEENSIZE sizeof (seennode_t)

static int indent, afternl;

static void update (dnode_t *);
static int cmp (const void *, const void *);
static seennode_t *findseen (dnode_t *);
static void add2seen (dnode_t *);
static void pr (char *);

void Dinit (void) {
    seenp = Marrayalloc ((long) SEENINCR * SEENSIZE);
    seeni = 0;
    seenn = SEENINCR;
}

void Dterm (void) {
    Marrayfree (seenp);
    seeni = seenn = 0;
}

void Dtrace (Tobj to, int offset) {
    dnode_t dnode;
    char *s;
    int i;

    indent = offset - 2;
    afternl = TRUE;
    if (Tgettype (to) != T_TABLE) {
        pr ((s = Ssfull (NULL, to))), free (s);
        return;
    }

    seeni = 0;
    dnode.vo = to;
    dnode.path = "";
    update (&dnode);
    for (i = 0; i < seeni; i++)
        free (seenp[i].path), seenp[i].path = NULL;
}

static void update (dnode_t *pnode) {
    Tkvindex_t tkvi;
    dnode_t *list, *cnode;
    seennode_t *seennode;
    char *s;
    long i, n;

    indent += 2;
    n = ((Ttable_t *) pnode->vo)->n;
    if (!(list = malloc (n * sizeof (dnode_t))))
        panic1 (POS, "update", "list malloc failed");
    for (
        cnode = &list[0], Tgetfirst (pnode->vo, &tkvi); tkvi.kvp;
        cnode++, Tgetnext (&tkvi)
    ) {
        cnode->ko = tkvi.kvp->ko;
        cnode->vo = tkvi.kvp->vo;
        cnode->ttype = Tgettype (cnode->vo);
    }
    qsort ((char *) list, n, sizeof (dnode_t), cmp);
    for (i = 0, cnode = &list[0]; i < n; i++, cnode++) {
        cnode->path = Spath (pnode->path, cnode->ko);
        seennode = findseen (cnode);
        if (seennode) {
            pr ((s = Sseen (cnode->ko, seennode->path))), free (s);
        } else {
            add2seen (cnode);
            if (cnode->ttype == T_TABLE) {
                pr ((s = Stfull (cnode->ko))), free (s);
                update (cnode);
                pr ("];");
            } else {
                pr ((s = Ssfull (cnode->ko, cnode->vo))), free (s);
            }
        }
    }
    free (list);
    indent -= 2;
}

static int cmp (const void *a, const void *b) {
    int atype, btype;
    dnode_t *anode, *bnode;
    double d1, d2;

    d1 = 0.0, d2 = 0.0;
    anode = (dnode_t *) a, bnode = (dnode_t *) b;
    atype = Tgettype (anode->ko), btype = Tgettype (bnode->ko);
    if (atype != btype)
        return (atype - btype);
    if (atype == T_STRING)
        return strcmp (Tgetstring (anode->ko), Tgetstring (bnode->ko));
    if (atype == T_INTEGER)
        d1 = Tgetinteger (anode->ko), d2 = Tgetinteger (bnode->ko);
    else if (atype == T_REAL)
        d1 = Tgetreal (anode->ko), d2 = Tgetreal (bnode->ko);
    if (d1 < d2)
        return -1;
    else if (d1 > d2)
        return 1;
    else
        return 0; /* but this should never happen since keys are unique */
}

static seennode_t *findseen (dnode_t *cnode) {
    int i;

    for (i = 0; i < seeni; i++)
        if (seenp[i].vo == cnode->vo)
            return &seenp[i];
    return NULL;
}

static void add2seen (dnode_t *cnode) {
    if (seeni >= seenn) {
        seenp = Marraygrow (seenp, (long) (seenn + SEENINCR) * SEENSIZE);
        seenn += SEENINCR;
    }
    seenp[seeni].vo = cnode->vo;
    seenp[seeni++].path = cnode->path;
}

static void pr (char *s) {
    char *s1;
    int i;

    for (s1 = s; *s1; s1++) {
        if (afternl) {
            for (i = 0; i < indent; i++)
                putchar (' ');
            afternl = FALSE;
        }
        if (*s1 == '\n')
            afternl = TRUE;
        putchar ((*s1)); /* HACK: to keep proto happy */
    }
    putchar ('\n');
    afternl = TRUE;
}
