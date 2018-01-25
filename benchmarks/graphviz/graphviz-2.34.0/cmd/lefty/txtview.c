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
#include "g.h"
#include "mem.h"
#include "code.h"
#include "tbl.h"
#include "parse.h"
#include "exec.h"
#include "str.h"
#include "txtview.h"

#define TXT_SEEN     0
#define TXT_ABSTRACT 1
#define TXT_FULL     2

typedef struct txtnode_t {
    int mode;
    int ttype;
    Tobj ko, vo;
    long time;
    char *path;
    union {
        struct {
            struct txtnode_t *txtnode;
            Tobj ko;
            char *text;
            int wi;
        } s;
        struct {
            char *text;
            int wi;
        } a;
        union {
            struct {
                char *text;
                int wi;
            } s;
            struct {
                char *ftext, *ltext;
                int fwi, mwi, lwi;
                struct txtnode_t *list;
                int n;
            } t;
        } f;
    } u;
} txtnode_t;
static txtnode_t *txtroot;
static txtnode_t defnode;

static txtnode_t **seenp;
static long seeni, seenn;
#define SEENINCR 100
#define SEENSIZE sizeof (txtnode_t)

static Tobj txtvo2toggle = NULL;

static Gwattr_t arraydata[2], buttondata[5];

static int txton, txtx, txty, txtwidth, txtheight;
static int txtwi, queryws[G_QWMODES + 1], listwi, scrollwi, arraywi, cmdwi;

static Psrc_t src;

#define __max(a, b) (((a) >= (b)) ? (a) : (b))

static void viewon (void);
static void viewoff (void);
static void update (txtnode_t *, long);
static void buildlist (txtnode_t *);
static void rebuildlist (txtnode_t *);
static void fillnode (txtnode_t *, txtnode_t *);
static void unfillnode (txtnode_t *);
static int cmp (const void *, const void *);
static txtnode_t *findseen (txtnode_t *);
static void add2seen (txtnode_t *);
static void orderfunc (void *, Gawdata_t *);
static void coordsfunc (int, Gawdata_t *);
static void coords2func (int, Gawdata_t *);

void TXTinit (Grect_t r) {
    Gwattr_t data[1];
    int i;

    txton = TRUE;
    txtwi = -1;
    txtx = r.o.x, txty = r.o.y;
    txtwidth = r.c.x - r.o.x + 1, txtheight = r.c.y - r.o.y + 1;

    buttondata[0].id = G_ATTRBUTTONCB;
    buttondata[0].u.func = TXTtoggle;
    buttondata[1].id = G_ATTRSIZE;
    buttondata[1].u.s.x = buttondata[1].u.s.y = 0;
    buttondata[2].id = G_ATTRBORDERWIDTH;
    buttondata[2].u.i = 0;
    buttondata[3].id = G_ATTRTEXT;
    buttondata[4].id = G_ATTRUSERDATA;

    arraydata[0].id = G_ATTRRESIZECB;
    arraydata[0].u.func = coordsfunc;
    arraydata[1].id = G_ATTRSIZE;
    arraydata[1].u.s.x = arraydata[1].u.s.y = 0;

    defnode.mode = TXT_ABSTRACT;
    defnode.ttype = T_TABLE;
    defnode.ko = defnode.vo = NULL;
    defnode.time = -1;
    defnode.path = NULL;
    defnode.u.s.txtnode = NULL;
    defnode.u.s.ko = NULL;
    defnode.u.s.text = NULL;
    defnode.u.s.wi = 0;
    defnode.u.a.text = NULL;
    defnode.u.a.wi = 0;
    defnode.u.f.s.text = NULL;
    defnode.u.f.s.wi = 0;
    defnode.u.f.t.ftext = defnode.u.f.t.ltext = NULL;
    defnode.u.f.t.fwi = 0;
    defnode.u.f.t.mwi = 0;
    defnode.u.f.t.lwi = 0;
    defnode.u.f.t.list = NULL;
    defnode.u.f.t.n = 0;

    for (i = 1; i <= G_QWMODES; i++) {
        data[0].id = G_ATTRMODE;
        switch (i) {
        case G_QWSTRING:
            data[0].u.t = "string";
            break;
        case G_QWFILE:
            data[0].u.t = "file";
            break;
        case G_QWCHOICE:
            data[0].u.t = "choice";
            break;
        }
        queryws[i] = Gcreatewidget (-1, G_QUERYWIDGET, 1, &data[0]);
    }

    src.flag = CHARSRC, src.fp = NULL, src.tok = -1, src.lnum = 1;
}

void TXTterm (void) {
    int i;

    for (i = 1; i <= G_QWMODES; i++)
        Gdestroywidget (queryws[i]);
    if (txton)
        viewoff (), txton = FALSE;
}

/* LEFTY builtin */
int TXTmode (int argc, lvar_t *argv) {
    char *s;

    if (!argv[0].o || Tgettype (argv[0].o) != T_STRING)
        return L_FAILURE;
    s = Tgetstring (argv[0].o);
    if (strcmp (s, "on") == 0) {
        if (txtwi == -1)
            viewon ();
    } else if (strcmp (s, "off") == 0) {
        if (txtwi != -1)
            viewoff ();
        else
            txton = FALSE;
    }
    return L_SUCCESS;
}

static void viewon (void) {
    Gwattr_t data[5];

    data[0].id = G_ATTRNAME;
    data[0].u.t = "LEFTY text view";
    data[1].id = G_ATTRORIGIN;
    data[1].u.p.x = txtx, data[1].u.p.y = txty;
    data[2].id = G_ATTRSIZE;
    data[2].u.s.x = txtwidth, data[2].u.s.y = txtheight;
    txtwi = Gcreatewidget (-1, G_VIEWWIDGET, 3, &data[0]);

    data[0].id = G_ATTRSIZE;
    data[0].u.s.x = txtwidth, data[0].u.s.y = txtheight;
    data[1].id = G_ATTRBORDERWIDTH;
    data[1].u.i = 1;
    data[2].id = G_ATTRRESIZECB;
    data[2].u.func = coords2func;
    listwi = Gcreatewidget (txtwi, G_ARRAYWIDGET, 3, &data[0]);
    Gawsetmode (&Gwidgets[listwi], TRUE);

    data[0].id = G_ATTRSIZE;
    data[0].u.s.x = txtwidth, data[0].u.s.y = txtheight / 3;
    data[1].id = G_ATTRBORDERWIDTH;
    data[1].u.i = 0;
    data[2].id = G_ATTRTEXT;
    data[2].u.t = NULL;
    data[3].id = G_ATTRNEWLINECB;
    data[3].u.func = TXTprocess;
    data[4].id = G_ATTRMODE;
    data[4].u.t = "oneline";
    cmdwi = Gcreatewidget (listwi, G_TEXTWIDGET, 5, &data[0]);

    data[0].id = G_ATTRSIZE;
    data[0].u.s.x = txtwidth, data[0].u.s.y = txtheight * 2 / 3;
    data[1].id = G_ATTRMODE;
    data[1].u.t = "forcebars";
    scrollwi = Gcreatewidget (listwi, G_SCROLLWIDGET, 2, &data[0]);

    data[0].id = G_ATTRRESIZECB;
    data[0].u.func = coordsfunc;
    data[1].id = G_ATTRSIZE;
    data[1].u.s.x = txtwidth, data[1].u.s.y = txtheight;
    data[2].id = G_ATTRBORDERWIDTH;
    data[2].u.i = 0;
    arraywi = Gcreatewidget (scrollwi, G_ARRAYWIDGET, 3, &data[0]);

    Gawsetmode (&Gwidgets[listwi], FALSE);

    if (!(txtroot = malloc (sizeof (txtnode_t))))
        panic1 (POS, "TXTinit", "txtroot malloc failed");
    *txtroot = defnode;
    txtroot->mode = TXT_FULL;
    txtroot->vo = root;
    txtroot->path = NULL;
    txtroot->u.f.t.mwi = arraywi;

    seenp = Marrayalloc ((long) SEENINCR * SEENSIZE);
    seeni = 0;
    seenn = SEENINCR;
    txton = TRUE;
}

static void viewoff (void) {
    Marrayfree (seenp);
    seeni = seenn = 0;
    unfillnode (txtroot);
    free (txtroot);
    Gdestroywidget (arraywi);
    Gdestroywidget (scrollwi);
    Gdestroywidget (cmdwi);
    Gdestroywidget (listwi);
    Gdestroywidget (txtwi);
    txton = FALSE;
    txtwi = -1;
}

/* LEFTY builtin */
int TXTask (int argc, lvar_t *argv) {
    Tobj so, ao;
    char buf[1200];
    char *sp, *ap;
    int mode;

    mode = 0;
    ap = NULL;
    if (argc < 2)
        mode = G_QWSTRING;
    else {
        so = argv[1].o;
        if (T_ISSTRING (so)) {
            sp = Tgetstring (so);
            if (strcmp (sp, "string") == 0)
                mode = G_QWSTRING;
            else if (strcmp (sp, "file") == 0)
                mode = G_QWFILE;
            else if (strcmp (sp, "choice") == 0)
                mode = G_QWCHOICE;
            else mode = 0;
        }
    }
    if (argc < 3)
        ap = NULL;
    else {
        ao = argv[2].o;
        if (T_ISSTRING (ao))
            ap = Tgetstring (ao);
    }
    if (Gqueryask (queryws[mode], Tgetstring (argv[0].o), ap, buf, 1200) == 0)
        rtno = Tstring (buf);
    else
        rtno = NULL;
#ifndef NOQUERYFIX
    if (mode == G_QWCHOICE) {
        Gwattr_t data[1];

        data[0].id = G_ATTRMODE;
        data[0].u.t = "choice";
        Gdestroywidget (queryws[mode]);
        queryws[mode] = Gcreatewidget (-1, G_QUERYWIDGET, 1, &data[0]);
    }
#endif
    return L_SUCCESS;
}

void TXTprocess (int wi, char *sp) {
    Tobj co;

    src.s = sp;
    if ((co = Punit (&src)))
        Eoktorun = TRUE, Eunit (co);
}

void TXTupdate (void) {
    if (!txton)
        return;
    if (txtwi == -1)
        viewon ();
    seeni = 0;
    update (txtroot, txtroot->time);
    txtroot->time = Ttime;
    Ttime++;
}

void TXTtoggle (int wi, void *data) {
    txtvo2toggle = data;
    TXTupdate ();
    txtvo2toggle = NULL;
}

/* update is called only for TXT_FULL tables */
static void update (txtnode_t *pnode, long ptim) {
    txtnode_t *cnode, *seennode;
    long ctim;
    int i;

    Gawsetmode (&Gwidgets[pnode->u.f.t.mwi], TRUE);
    if (!pnode->u.f.t.list)
        buildlist (pnode);
    else if (ptim < Tgettime (pnode->vo))
        rebuildlist (pnode);
    for (
        i = 0, cnode = &pnode->u.f.t.list[0]; i < pnode->u.f.t.n;
        i++, cnode++
    ) {
        ctim = cnode->time;
        if (txtvo2toggle == cnode->vo) {
            switch (cnode->mode) {
            case TXT_SEEN:
                break;
            case TXT_ABSTRACT:
                unfillnode (cnode);
                cnode->mode = TXT_FULL;
                fillnode (pnode, cnode);
                break;
            case TXT_FULL:
                unfillnode (cnode);
                cnode->mode = TXT_ABSTRACT;
                fillnode (pnode, cnode);
                break;
            }
        }
        if (!(seennode = findseen (cnode)))
            add2seen (cnode);
        if (
            seennode && cnode->mode == TXT_SEEN && seennode->ko != cnode->u.s.ko
        ) {
            unfillnode (cnode);
            cnode->u.s.txtnode = seennode;
            cnode->u.s.ko = seennode->ko;
            fillnode (pnode, cnode);
        } else if (seennode && cnode->mode != TXT_SEEN) {
            unfillnode (cnode);
            cnode->mode = TXT_SEEN;
            cnode->u.s.txtnode = seennode;
            cnode->u.s.ko = seennode->ko;
            fillnode (pnode, cnode);
        } else if (!seennode && cnode->mode == TXT_SEEN) {
            unfillnode (cnode);
            cnode->mode = TXT_ABSTRACT;
            fillnode (pnode, cnode);
        } else if (cnode->time == -1) {
            unfillnode (cnode);
            if (seennode)
                cnode->u.s.txtnode = seennode;
            fillnode (pnode, cnode);
        }
        if (cnode->ttype == T_TABLE && cnode->mode == TXT_FULL)
            update (cnode, ctim);
    }
    Gaworder (&Gwidgets[pnode->u.f.t.mwi], pnode, orderfunc);
    Gawsetmode (&Gwidgets[pnode->u.f.t.mwi], FALSE);
}

static void buildlist (txtnode_t *pnode) {
    Tkvindex_t tkvi;
    Tobj ko, vo;
    int vtype;
    txtnode_t *cnode;

    pnode->u.f.t.n = ((Ttable_t *) pnode->vo)->n;
    if (!(pnode->u.f.t.list = malloc (
        __max (pnode->u.f.t.n, 1) * sizeof (txtnode_t)
    )))
        panic1 (POS, "buildlist", "list malloc failed");

    for (
        cnode = &pnode->u.f.t.list[0], Tgetfirst (pnode->vo, &tkvi);
        tkvi.kvp; Tgetnext (&tkvi)
    ) {
        ko = tkvi.kvp->ko, vo = tkvi.kvp->vo;
        vtype = Tgettype (vo);
        if (vtype == T_CODE && TCgettype (vo, TCgetnext (
            vo, TCgetnext (vo, TCgetnext (vo, TCgetfp (vo, 0)))
        )) == C_INTERNAL) {
            pnode->u.f.t.n--;
            continue;
        }
        *cnode = defnode;
        cnode->vo = vo;
        cnode->ko = ko;
        cnode->ttype = vtype;
        cnode++;
    }
    qsort (
        (char *) pnode->u.f.t.list, pnode->u.f.t.n, sizeof (txtnode_t), cmp
    );
}

static void rebuildlist (txtnode_t *pnode) {
    Tkvindex_t tkvi;
    Tobj ko, vo;
    int vtype;
    txtnode_t *cnode;
    txtnode_t *olist, *nlist;
    txtnode_t tmpnode;
    int on, nn, i, j, cmpval;

    cmpval = 0;
    olist = pnode->u.f.t.list;
    on = pnode->u.f.t.n;
    pnode->u.f.t.n = ((Ttable_t *) pnode->vo)->n;
    if (!(pnode->u.f.t.list = malloc (
        __max (pnode->u.f.t.n, 1) * sizeof (txtnode_t)
    )))
        panic1 (POS, "rebuildlist", "list malloc failed");

    for (
        cnode = &pnode->u.f.t.list[0], Tgetfirst (pnode->vo, &tkvi);
        tkvi.kvp; Tgetnext (&tkvi)
    ) {
        ko = tkvi.kvp->ko, vo = tkvi.kvp->vo;
        vtype = Tgettype (vo);
        if (vtype == T_CODE && TCgettype (vo, TCgetnext (
            vo, TCgetnext (vo, TCgetnext (vo, TCgetfp (vo, 0)))
        )) == C_INTERNAL) {
            pnode->u.f.t.n--;
            continue;
        }
        *cnode = defnode;
        cnode->vo = vo;
        cnode->ko = ko;
        cnode->ttype = vtype;
        cnode++;
    }
    qsort ((char *) pnode->u.f.t.list, pnode->u.f.t.n, sizeof (txtnode_t), cmp);
    nlist = pnode->u.f.t.list;
    nn = pnode->u.f.t.n;
    for (i = 0, j = 0; i < nn; i++) {
        while (j < on && (cmpval = cmp (&olist[j], &nlist[i])) < 0)
            j++;
        if (j < on && cmpval == 0 && nlist[i].vo == olist[j].vo)
            tmpnode = olist[j], olist[j] = nlist[i], nlist[i] = tmpnode, j++;
    }
    for (j = 0; j < on; j++)
        unfillnode (&olist[j]);
    free (olist);
}

static void fillnode (txtnode_t *pnode, txtnode_t *cnode) {
    cnode->time = Ttime;
    cnode->path = Spath (pnode->path, cnode->ko);
    switch (cnode->mode) {
    case TXT_SEEN:
        cnode->u.s.text = Sseen (cnode->ko, cnode->u.s.txtnode->path);
        buttondata[3].u.t = cnode->u.s.text;
        buttondata[4].u.u = (unsigned long) cnode->vo;
        cnode->u.s.wi = Gcreatewidget (
            pnode->u.f.t.mwi, G_BUTTONWIDGET, 5, &buttondata[0]
        );
        break;
    case TXT_ABSTRACT:
        cnode->u.a.text = Sabstract (cnode->ko, cnode->vo);
        buttondata[3].u.t = cnode->u.a.text;
        buttondata[4].u.u = (unsigned long) cnode->vo;
        cnode->u.a.wi = Gcreatewidget (
            pnode->u.f.t.mwi, G_BUTTONWIDGET, 5, &buttondata[0]
        );
        break;
    case TXT_FULL:
        if (cnode->ttype == T_TABLE) {
            cnode->u.f.t.ftext = Stfull (cnode->ko);
            cnode->u.f.t.ltext = "];";
            buttondata[3].u.t = cnode->u.f.t.ftext;
            buttondata[4].u.u = (unsigned long) cnode->vo;
            cnode->u.f.t.fwi = Gcreatewidget (
                pnode->u.f.t.mwi, G_BUTTONWIDGET, 5, &buttondata[0]
            );
            cnode->u.f.t.mwi = Gcreatewidget (
                pnode->u.f.t.mwi, G_ARRAYWIDGET, 2, &arraydata[0]
            );
            buttondata[3].u.t = cnode->u.f.t.ltext;
            buttondata[4].u.u = (unsigned long) cnode->vo;
            cnode->u.f.t.lwi = Gcreatewidget (
                pnode->u.f.t.mwi, G_BUTTONWIDGET, 5, &buttondata[0]
            );
        } else {
            cnode->u.f.s.text = Ssfull (cnode->ko, cnode->vo);
            buttondata[3].u.t = cnode->u.f.s.text;
            buttondata[4].u.u = (unsigned long) cnode->vo;
            cnode->u.f.s.wi = Gcreatewidget (
                pnode->u.f.t.mwi, G_BUTTONWIDGET, 5, &buttondata[0]
            );
        }
        break;
    }
}

static void unfillnode (txtnode_t *cnode) {
    int i;

    if (cnode->path)
        free (cnode->path), cnode->path = NULL;
    switch (cnode->mode) {
    case TXT_SEEN:
        if (cnode->u.s.text)
            free (cnode->u.s.text);
        if (cnode->u.s.wi)
            Gdestroywidget (cnode->u.s.wi);
        break;
    case TXT_ABSTRACT:
        if (cnode->u.a.text)
            free (cnode->u.a.text);
        if (cnode->u.a.wi)
            Gdestroywidget (cnode->u.a.wi);
        break;
    case TXT_FULL:
        if (cnode->ttype == T_TABLE) {
            for (i = 0; i < cnode->u.f.t.n; i++)
                unfillnode (&cnode->u.f.t.list[i]);
            if (cnode->u.f.t.list)
                free (cnode->u.f.t.list);
            if (cnode->u.f.t.ftext)
                free (cnode->u.f.t.ftext);
            if (cnode->u.f.t.fwi) {
                Gdestroywidget (cnode->u.f.t.fwi);
                Gdestroywidget (cnode->u.f.t.mwi);
                Gdestroywidget (cnode->u.f.t.lwi);
            }
        } else {
            if (cnode->u.f.s.text)
                free (cnode->u.f.s.text);
            if (cnode->u.f.s.wi)
                Gdestroywidget (cnode->u.f.s.wi);
        }
        break;
    }
    cnode->u.s.text = NULL;
    cnode->u.s.wi = 0;
    cnode->u.a.text = NULL;
    cnode->u.a.wi = 0;
    cnode->u.f.t.list = NULL;
    cnode->u.f.t.ftext = NULL;
    cnode->u.f.t.fwi = 0;
    cnode->u.f.t.mwi = 0;
    cnode->u.f.t.lwi = 0;
    cnode->u.f.s.text = NULL;
    cnode->u.f.s.wi = 0;
}

static int cmp (const void *a, const void *b) {
    int atype, btype;
    txtnode_t *anode, *bnode;
    double d1, d2;

    d1 = 0.0, d2 = 0.0;
    anode = (txtnode_t *) a, bnode = (txtnode_t *) b;
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

static txtnode_t *findseen (txtnode_t *cnode) {
    int i;

    for (i = 0; i < seeni; i++)
        if (seenp[i]->vo == cnode->vo)
            return seenp[i];
    return NULL;
}

static void add2seen (txtnode_t *cnode) {
    if (seeni >= seenn) {
        seenp = Marraygrow (seenp, (long) (seenn + SEENINCR) * SEENSIZE);
        seenn += SEENINCR;
    }
    seenp[seeni++] = cnode;
}

static void orderfunc (void *data, Gawdata_t *dp) {
    txtnode_t *pnode, *cnode;
    int i;

    pnode = data;
    dp->cj = 0;
    for (i = 0, cnode = &pnode->u.f.t.list[0]; i < pnode->u.f.t.n; i++, cnode++)
        switch (cnode->mode) {
        case TXT_SEEN:
            dp->carray[dp->cj++].w = Gwidgets[cnode->u.s.wi].w;
            break;
        case TXT_ABSTRACT:
            dp->carray[dp->cj++].w = Gwidgets[cnode->u.a.wi].w;
            break;
        case TXT_FULL:
            if (cnode->ttype == T_TABLE) {
                dp->carray[dp->cj++].w = Gwidgets[cnode->u.f.t.fwi].w;
                dp->carray[dp->cj++].w = Gwidgets[cnode->u.f.t.mwi].w;
                dp->carray[dp->cj++].w = Gwidgets[cnode->u.f.t.lwi].w;
            } else
                dp->carray[dp->cj++].w = Gwidgets[cnode->u.f.s.wi].w;
            break;
        }
}

static void coordsfunc (int wi, Gawdata_t *dp) {
    Gawcarray_t *cp;
    int cox, coy;
    int ci;

    cox = coy = 0;
    for (ci = 0; ci < dp->cj; ci++) {
        cp = &dp->carray[ci];
        if (!cp->flag)
            continue;
        cp->ox = cox, cp->oy = coy;
        cp->sx = dp->sx - 2 * cp->bs;
        coy += cp->sy + 2 * cp->bs;
    }
    dp->sy = coy;
}

static void coords2func (int wi, Gawdata_t *dp) {
    Gawcarray_t *cp;
    int cox, coy;
    int ci, cj;

    cox = coy = 0;
    for (ci = 0, cj = 0; ci < dp->cj; ci++) {
        cp = &dp->carray[ci];
        if (!cp->flag)
            continue;
        cp->ox = cox, cp->oy = coy;
        cj++;
        cp->sx = dp->sx - 2 * cp->bs;
        if (cj == 2)
            cp->sy = dp->sy - coy - 2 * cp->bs;
        coy += cp->sy + 2 * cp->bs;
    }
}
