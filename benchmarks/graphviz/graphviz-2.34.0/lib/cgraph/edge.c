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

#include <cghdr.h>

#define IN_SET FALSE
#define OUT_SET TRUE
#define ID_ORDER TRUE
#define SEQ_ORDER FALSE

static Agtag_t Tag;		/* to silence warnings about initialization */


/* return first outedge of <n> */
Agedge_t *agfstout(Agraph_t * g, Agnode_t * n)
{
    Agsubnode_t *sn;
    Agedge_t *e = NILedge;

    sn = agsubrep(g, n);
    if (sn) {
		dtrestore(g->e_seq, sn->out_seq);
		e = (Agedge_t *) dtfirst(g->e_seq);
		sn->out_seq = dtextract(g->e_seq);
	}
    return e;
}

/* return outedge that follows <e> of <n> */
Agedge_t *agnxtout(Agraph_t * g, Agedge_t * e)
{
    Agnode_t *n;
    Agsubnode_t *sn;
    Agedge_t *f = NILedge;

    n = AGTAIL(e);
    sn = agsubrep(g, n);
    if (sn) {
		dtrestore(g->e_seq, sn->out_seq);
		f = (Agedge_t *) dtnext(g->e_seq, e);
		sn->out_seq = dtextract(g->e_seq);
	}
    return f;
}

Agedge_t *agfstin(Agraph_t * g, Agnode_t * n)
{
    Agsubnode_t *sn;
    Agedge_t *e = NILedge;

    sn = agsubrep(g, n);
	if (sn) {
		dtrestore(g->e_seq, sn->in_seq);
		e = (Agedge_t *) dtfirst(g->e_seq);
		sn->in_seq = dtextract(g->e_seq);
	}
    return e;
}

Agedge_t *agnxtin(Agraph_t * g, Agedge_t * e)
{
    Agnode_t *n;
    Agsubnode_t *sn;
    Agedge_t *f = NILedge;

    n = AGHEAD(e);
    sn = agsubrep(g, n);
	if (sn) {
		dtrestore(g->e_seq, sn->in_seq);
		f = (Agedge_t *) dtnext(g->e_seq, e);
		sn->in_seq = dtextract(g->e_seq);
	}
	return f;
}

Agedge_t *agfstedge(Agraph_t * g, Agnode_t * n)
{
    Agedge_t *rv;
    rv = agfstout(g, n);
    if (rv == NILedge)
	rv = agfstin(g, n);
    return rv;
}

Agedge_t *agnxtedge(Agraph_t * g, Agedge_t * e, Agnode_t * n)
{
    Agedge_t *rv;

    if (AGTYPE(e) == AGOUTEDGE) {
	rv = agnxtout(g, e);
	if (rv == NILedge) {
	    do {
		rv = !rv ? agfstin(g, n) : agnxtin(g,rv);
	    } while (rv && (rv->node == n));
	}
    } else {
	do {
	    rv = agnxtin(g, e);		/* so that we only see each edge once, */
		e = rv;
	} while (rv && (rv->node == n));	/* ignore loops as in-edges */
    }
    return rv;
}

/* internal edge set lookup */
static Agedge_t *agfindedge_by_key(Agraph_t * g, Agnode_t * t, Agnode_t * h,
			    Agtag_t key)
{
    Agedge_t *e, template;
    Agsubnode_t *sn;

    if ((t == NILnode) || (h == NILnode))
	return NILedge;
    template.base.tag = key;
    template.node = t;		/* guess that fan-in < fan-out */
    sn = agsubrep(g, h);
    if (!sn) e = 0;
    else {
#if 0
	if (t != h) {
#endif
	    dtrestore(g->e_id, sn->in_id);
	    e = (Agedge_t *) dtsearch(g->e_id, &template);
	    sn->in_id = dtextract(g->e_id);
#if 0
	} else {			/* self edge */
	    dtrestore(g->e_id, sn->out_id);
	    e = (Agedge_t *) dtsearch(g->e_id, &template);
	    sn->out_id = dtextract(g->e_id);
	}
#endif
    }
    return e;
}

static Agedge_t *agfindedge_by_id(Agraph_t * g, Agnode_t * t, Agnode_t * h,
				  unsigned long id)
{
    Agtag_t tag;

    tag = Tag;
    tag.objtype = AGEDGE;
    tag.id = id;
    return agfindedge_by_key(g, t, h, tag);
}

Agsubnode_t *agsubrep(Agraph_t * g, Agnode_t * n)
{
    Agsubnode_t *sn, template;

	if (g == n->root) sn = &(n->mainsub);
	else {
			template.node = n;
			sn = dtsearch(g->n_id, &template);
	}
    return sn;
}

static void ins(Dict_t * d, Dtlink_t ** set, Agedge_t * e)
{
    dtrestore(d, *set);
    dtinsert(d, e);
    *set = dtextract(d);
}

static void del(Dict_t * d, Dtlink_t ** set, Agedge_t * e)
{
    void *x;
    dtrestore(d, *set);
    x = dtdelete(d, e);
    assert(x);
    *set = dtextract(d);
}

static void installedge(Agraph_t * g, Agedge_t * e)
{
    Agnode_t *t, *h;
    Agedge_t *out, *in;
    Agsubnode_t *sn;

    out = AGMKOUT(e);
    in = AGMKIN(e);
    t = agtail(e);
    h = aghead(e);
    while (g) {
	if (agfindedge_by_key(g, t, h, AGTAG(e))) break;
	sn = agsubrep(g, t);
	ins(g->e_seq, &sn->out_seq, out);
	ins(g->e_id, &sn->out_id, out);
	sn = agsubrep(g, h);
	ins(g->e_seq, &sn->in_seq, in);
	ins(g->e_id, &sn->in_id, in);
	g = agparent(g);
    }
}

static void subedge(Agraph_t * g, Agedge_t * e)
{
    installedge(g, e);
    /* might an init method call be needed here? */
}

static Agedge_t *newedge(Agraph_t * g, Agnode_t * t, Agnode_t * h,
			 unsigned long id)
{
    Agedgepair_t *e2;
    Agedge_t *in, *out;
    int seq;

    (void)agsubnode(g,t,TRUE);
    (void)agsubnode(g,h,TRUE);
    e2 = (Agedgepair_t *) agalloc(g, sizeof(Agedgepair_t));
    in = &(e2->in);
    out = &(e2->out);
    seq = agnextseq(g, AGEDGE);
    AGTYPE(in) = AGINEDGE;
    AGTYPE(out) = AGOUTEDGE;
    AGID(in) = AGID(out) = id;
    AGSEQ(in) = AGSEQ(out) = seq;
    in->node = t;
    out->node = h;

    installedge(g, out);
    if (g->desc.has_attrs) {
	(void) agbindrec(out, AgDataRecName, sizeof(Agattr_t), FALSE);
	agedgeattr_init(g, out);
    }
    agmethod_init(g, out);
    return out;
}

/* edge creation predicate */
static int ok_to_make_edge(Agraph_t * g, Agnode_t * t, Agnode_t * h)
{
    Agtag_t key;

    /* protect against self, multi-edges in strict graphs */
    if (agisstrict(g)) {
	if (g->desc.no_loop && (t == h)) /* simple graphs */
	    return FALSE;
	key = Tag;
	key.objtype = 0;	/* wild card */
	if (agfindedge_by_key(g, t, h, key))
	    return FALSE;
    }
    return TRUE;
}

Agedge_t *agidedge(Agraph_t * g, Agnode_t * t, Agnode_t * h,
		   unsigned long id, int cflag)
{
    Agraph_t *root;
    Agedge_t *e;

    e = agfindedge_by_id(g, t, h, id);
    if ((e == NILedge) && agisundirected(g))
	e = agfindedge_by_id(g, h, t, id);
    if ((e == NILedge) && cflag && ok_to_make_edge(g, t, h)) {
	root = agroot(g);
	if ((g != root) && ((e = agfindedge_by_id(root, t, h, id)))) {
	    subedge(g, e);	/* old */
	} else {
	    if (agallocid(g, AGEDGE, id)) {
		e = newedge(g, t, h, id);	/* new */
	    }
	}
    }
    return e;
}

Agedge_t *agedge(Agraph_t * g, Agnode_t * t, Agnode_t * h, char *name,
		 int cflag)
{
    Agedge_t *e;
    unsigned long id;
    int have_id;

    have_id = agmapnametoid(g, AGEDGE, name, &id, FALSE);
    if (have_id || ((name == NILstr) && (NOT(cflag) || agisstrict(g)))) {
	/* probe for pre-existing edge */
	Agtag_t key;
	key = Tag;
	if (have_id) {
	    key.id = id;
	    key.objtype = AGEDGE;
	} else {
	    key.id = key.objtype = 0;
	}

	/* might already exist locally */
	e = agfindedge_by_key(g, t, h, key);
	if ((e == NILedge) && agisundirected(g))
	    e = agfindedge_by_key(g, h, t, key);
	if (e)
	    return e;
	if (cflag) {
	    e = agfindedge_by_key(agroot(g), t, h, key);
	    if ((e == NILedge) && agisundirected(g))
		e = agfindedge_by_key(agroot(g), h, t, key);
	    if (e) {
		subedge(g,e);
		return e;
	    }
	}
    }

    if (cflag && ok_to_make_edge(g, t, h)
	&& agmapnametoid(g, AGEDGE, name, &id, TRUE)) { /* reserve id */
	e = newedge(g, t, h, id);
	agregister(g, AGEDGE, e); /* register new object in external namespace */
    }
    else
	e = NILedge;
    return e;
}

void agdeledgeimage(Agraph_t * g, Agedge_t * e, void *ignored)
{
    Agedge_t *in, *out;
    Agnode_t *t, *h;
    Agsubnode_t *sn;

    NOTUSED(ignored);
    if (AGTYPE(e) == AGINEDGE) {
	in = e;
	out = AGIN2OUT(e);
    } else {
	out = e;
	in = AGOUT2IN(e);
    }
    t = in->node;
    h = out->node;
    sn = agsubrep(g, t);
    del(g->e_seq, &sn->out_seq, out);
    del(g->e_id, &sn->out_id, out);
    sn = agsubrep(g, h);
    del(g->e_seq, &sn->in_seq, in);
    del(g->e_id, &sn->in_id, in);
#ifdef DEBUG
    for (e = agfstin(g,h); e; e = agnxtin(g,e))
	assert(e != in);
    for (e = agfstout(g,t); e; e = agnxtout(g,e))
	assert(e != out);
#endif
}

int agdeledge(Agraph_t * g, Agedge_t * e)
{
    e = AGMKOUT(e);
    if (agfindedge_by_key(g, agtail(e), aghead(e), AGTAG(e)) == NILedge)
	return FAILURE;

    if (g == agroot(g)) {
	if (g->desc.has_attrs)
	    agedgeattr_delete(e);
	agmethod_delete(g, e);
	agrecclose((Agobj_t *) e);
	agfreeid(g, AGEDGE, AGID(e));
    }
    if (agapply (g, (Agobj_t *) e, (agobjfn_t) agdeledgeimage, NILedge, FALSE) == SUCCESS) {
	if (g == agroot(g))
		agfree(g, e);
	return SUCCESS;
    } else
	return FAILURE;
}

Agedge_t *agsubedge(Agraph_t * g, Agedge_t * e, int cflag)
{
    Agnode_t *t, *h;
    Agedge_t *rv;

    rv = NILedge;
    t = agsubnode(g, AGTAIL(e), cflag);
    h = agsubnode(g, AGHEAD(e), cflag);
    if (t && h) {
	rv = agfindedge_by_key(g, t, h, AGTAG(e));
	if (cflag && (rv == NILedge)) {
#ifdef OLD_OBSOLETE
	    rv = agfindedge_by_id(g, t, h, AGID(e));
	    if (!rv)
		rv = newedge(g, t, h, AGID(e));
#else
	installedge(g, e);
	rv = e;
#endif
	}
	if (rv && (AGTYPE(rv) != AGTYPE(e)))
	    rv = AGOPP(rv);
    }
    return rv;
}

/* edge comparison.  OBJTYPE(e) == 0 means ID is a wildcard. */
int agedgeidcmpf(Dict_t * d, void *arg_e0, void *arg_e1, Dtdisc_t * disc)
{
    long v;
    Agedge_t *e0, *e1;

    NOTUSED(d);
    e0 = arg_e0;
    e1 = arg_e1;
    NOTUSED(disc);
    v = AGID(e0->node) - AGID(e1->node);
    if (v == 0) {		/* same node */
	if ((AGTYPE(e0) == 0) || (AGTYPE(e1) == 0))
	    v = 0;
	else
	    v = AGID(e0) - AGID(e1);
    }
    return ((v==0)?0:(v<0?-1:1));
}

/* edge comparison.  for ordered traversal. */
int agedgeseqcmpf(Dict_t * d, void *arg_e0, void *arg_e1, Dtdisc_t * disc)
{
    long v;
    Agedge_t *e0, *e1;

    NOTUSED(d);
    e0 = arg_e0;
    e1 = arg_e1;
    NOTUSED(disc);
	assert(arg_e0 && arg_e1);
	if (e0->node != e1->node) v = AGSEQ(e0->node) - AGSEQ(e1->node);
	else v = (AGSEQ(e0) - AGSEQ(e1));
    return ((v==0)?0:(v<0?-1:1));
}

/* indexing for ordered traversal */
Dtdisc_t Ag_mainedge_seq_disc = {
    0,				/* pass object ptr      */
    0,				/* size (ignored)       */
    offsetof(Agedge_t,seq_link),/* use internal links	*/
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    agedgeseqcmpf,
    NIL(Dthash_f),
    agdictobjmem,
    NIL(Dtevent_f)
};

Dtdisc_t Ag_subedge_seq_disc = {
    0,				/* pass object ptr      */
    0,				/* size (ignored)       */
    -1,				/* use external holder objects */
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    agedgeseqcmpf,
    NIL(Dthash_f),
    agdictobjmem,
    NIL(Dtevent_f)
};

/* indexing for random search */
Dtdisc_t Ag_mainedge_id_disc = {
    0,				/* pass object ptr      */
    0,				/* size (ignored)       */
    offsetof(Agedge_t,id_link),	/* use internal links	*/
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    agedgeidcmpf,
    NIL(Dthash_f),
    agdictobjmem,
    NIL(Dtevent_f)
};

Dtdisc_t Ag_subedge_id_disc = {
    0,				/* pass object ptr      */
    0,				/* size (ignored)       */
    -1,				/* use external holder objects */
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    agedgeidcmpf,
    NIL(Dthash_f),
    agdictobjmem,
    NIL(Dtevent_f)
};

/* debug functions */
#ifdef agtail
#undef agtail
#endif
Agnode_t *agtail(Agedge_t * e)
{
    return AGTAIL(e);
}

#ifdef aghead
#undef aghead
#endif
Agnode_t *aghead(Agedge_t * e)
{
    return AGHEAD(e);
}

#ifdef agopp
#undef agopp
#endif
Agedge_t *agopp(Agedge_t * e)
{
    return AGOPP(e);
}

#ifdef NOTDEF
	/* could be useful if we write relabel_edge */
static Agedge_t *agfindedge_by_name(Agraph_t * g, Agnode_t * t,
				    Agnode_t * h, char *name)
{
    unsigned long id;

    if (agmapnametoid(agraphof(t), AGEDGE, name, &id, FALSE))
	return agfindedge_by_id(g, t, h, id);
    else
	return NILedge;
}
#endif
