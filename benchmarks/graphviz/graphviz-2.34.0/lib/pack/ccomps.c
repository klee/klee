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


#include <ctype.h>
#include <setjmp.h>
#include "render.h"
#include "pack.h"

static jmp_buf jbuf;

#define MARKED(n) ND_mark(n)
#define MARK(n) (ND_mark(n) = 1)
#define ONSTACK(n) (ND_mark(n) = 1)
#define UNMARK(n) (ND_mark(n) = 0)

typedef void (*dfsfn) (Agnode_t *, void *);

#if 0
static void dfs(Agraph_t * g, Agnode_t * n, dfsfn action, void *state)
{
    Agedge_t *e;
    Agnode_t *other;

    MARK(n);
    action(n, state);
    for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
	if ((other = agtail(e)) == n)
	    other = aghead(e);
	if (!MARKED(other))
	    dfs(g, other, action, state);
    }
}
#endif

typedef struct blk_t {
    Agnode_t **data;
    Agnode_t **endp;
    struct blk_t *prev;
    struct blk_t *next;
} blk_t;

typedef struct {
    blk_t *fstblk;
    blk_t *curblk;
    Agnode_t **curp;
} stk_t;

#define INITBUF 1024
#define BIGBUF 1000000

static void initStk(stk_t* sp, blk_t* bp, Agnode_t** base)
{
    bp->data = base;
    bp->endp = bp->data + INITBUF;
    bp->prev = bp->next = NULL;
    sp->curblk = sp->fstblk = bp;
    sp->curp = sp->curblk->data;
}

static void freeBlk (blk_t* bp)
{
    free (bp->data);
    free (bp);
}

static void freeStk (stk_t* sp)
{
    blk_t* bp;
    blk_t* nxtbp;

    for (bp = sp->fstblk->next; bp; bp = nxtbp) {
	nxtbp = bp->next;
	freeBlk (bp);
    }
}

static void push(stk_t* sp, Agnode_t * np)
{
    if (sp->curp == sp->curblk->endp) {
	if (sp->curblk->next == NULL) {
	    blk_t *bp = GNEW(blk_t);
	    if (bp == 0) {
		agerr(AGERR, "gc: Out of memory\n");
		longjmp(jbuf, 1);
	    }
	    bp->prev = sp->curblk;
	    bp->next = NULL;
	    bp->data = N_GNEW(BIGBUF, Agnode_t *);
	    if (bp->data == 0) {
		agerr(AGERR, "gc: Out of memory\n");
		longjmp(jbuf, 1);
	    }
	    bp->endp = bp->data + BIGBUF;
	    sp->curblk->next = bp;
	}
	sp->curblk = sp->curblk->next;
	sp->curp = sp->curblk->data;
    }
    ONSTACK(np);
    *sp->curp++ = np;
}

static Agnode_t *pop(stk_t* sp)
{
    if (sp->curp == sp->curblk->data) {
	if (sp->curblk == sp->fstblk)
	    return 0;
	sp->curblk = sp->curblk->prev;
	sp->curp = sp->curblk->endp;
    }
    sp->curp--;
    return *sp->curp;
}


static void dfs(Agraph_t * g, Agnode_t * n, dfsfn action, void *state, stk_t* stk)
{
    Agedge_t *e;
    Agnode_t *other;

    push (stk, n);
    while ((n = pop(stk))) {
	MARK(n);
	action(n, state);
        for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
	    if ((other = agtail(e)) == n)
		other = aghead(e);
            if (!MARKED(other))
                push(stk, other);
        }
    }
}

static int isLegal(char *p)
{
    unsigned char c;

    while ((c = *(unsigned char *) p++)) {
	if ((c != '_') && !isalnum(c))
	    return 0;
    }

    return 1;
}

/* insertFn:
 */
static void insertFn(Agnode_t * n, void *state)
{
#ifndef WITH_CGRAPH
    aginsert((Agraph_t *) state, n);
#else /* WITH_CGRAPH */
    agsubnode((Agraph_t *) state,n,1);
#endif /* WITH_CGRAPH */
}

/* pccomps:
 * Return an array of subgraphs consisting of the connected 
 * components of graph g. The number of components is returned in ncc. 
 * All pinned nodes are in one component.
 * If pfx is non-null and a legal graph name, we use it as the prefix
 * for the name of the subgraphs created. If not, a simple default is used.
 * If pinned is non-null, *pinned set to 1 if pinned nodes found
 * and the first component is the one containing the pinned nodes.
 * Note that the component subgraphs do not contain any edges. These must
 * be obtained from the root graph.
 * Return NULL on error or if graph is empty.
 */
Agraph_t **pccomps(Agraph_t * g, int *ncc, char *pfx, boolean * pinned)
{
    int c_cnt = 0;
    char buffer[SMALLBUF];
    char *name;
    Agraph_t *out = 0;
    Agnode_t *n;
    Agraph_t **ccs;
    int len;
    int bnd = 10;
    boolean pin = FALSE;
    stk_t stk;
    blk_t blk;
    Agnode_t* base[INITBUF];
    int error = 0;

    if (agnnodes(g) == 0) {
	*ncc = 0;
	return 0;
    }
    if (!pfx || !isLegal(pfx)) {
	pfx = "_cc_";
    }
    len = strlen(pfx);
    if (len + 25 <= SMALLBUF)
	name = buffer;
    else
	name = (char *) gmalloc(len + 25);
    strcpy(name, pfx);

    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	UNMARK(n);

    ccs = N_GNEW(bnd, Agraph_t *);

    initStk (&stk, &blk, base);
    if (setjmp(jbuf)) {
	error = 1;
	goto packerror;
    }
    /* Component with pinned nodes */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (MARKED(n) || !isPinned(n))
	    continue;
	if (!out) {
	    sprintf(name + len, "%d", c_cnt);
#ifndef WITH_CGRAPH
	    out = agsubg(g, name);
#else /* WITH_CGRAPH */
	    out = agsubg(g, name,1);
	    agbindrec(out, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
	    ccs[c_cnt] = out;
	    c_cnt++;
	    pin = TRUE;
	}
	dfs (g, n, insertFn, out, &stk);
    }

    /* Remaining nodes */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (MARKED(n))
	    continue;
	sprintf(name + len, "%d", c_cnt);
#ifndef WITH_CGRAPH
	out = agsubg(g, name);
#else /* WITH_CGRAPH */
	out = agsubg(g, name,1);
	agbindrec(out, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
	dfs(g, n, insertFn, out, &stk);
	if (c_cnt == bnd) {
	    bnd *= 2;
	    ccs = RALLOC(bnd, ccs, Agraph_t *);
	}
	ccs[c_cnt] = out;
	c_cnt++;
    }
packerror:
    freeStk (&stk);
    if (name != buffer)
	free(name);
    if (error) {
	int i;
	*ncc = 0;
	for (i=0; i < c_cnt; i++) {
	    agclose (ccs[i]);
	}
	free (ccs);
	ccs = NULL;
    }
    else {
	ccs = RALLOC(c_cnt, ccs, Agraph_t *);
	*ncc = c_cnt;
	*pinned = pin;
    }
    return ccs;
}

/* ccomps:
 * Return an array of subgraphs consisting of the connected
 * components of graph g. The number of components is returned in ncc.
 * If pfx is non-null and a legal graph name, we use it as the prefix
 * for the name of the subgraphs created. If not, a simple default is used.
 * Note that the component subgraphs do not contain any edges. These must
 * be obtained from the root graph.
 * Returns NULL on error or if graph is empty.
 */
Agraph_t **ccomps(Agraph_t * g, int *ncc, char *pfx)
{
    int c_cnt = 0;
    char buffer[SMALLBUF];
    char *name;
    Agraph_t *out;
    Agnode_t *n;
    Agraph_t **ccs;
    int len;
    int bnd = 10;
    stk_t stk;
    blk_t blk;
    Agnode_t* base[INITBUF];

    if (agnnodes(g) == 0) {
	*ncc = 0;
	return 0;
    }
    if (!pfx || !isLegal(pfx)) {
	pfx = "_cc_";
    }
    len = strlen(pfx);
    if (len + 25 <= SMALLBUF)
	name = buffer;
    else {
	if (!(name = (char *) gmalloc(len + 25))) return NULL;
    }
    strcpy(name, pfx);

    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	UNMARK(n);

    ccs = N_GNEW(bnd, Agraph_t *);
    initStk (&stk, &blk, base);
    if (setjmp(jbuf)) {
	freeStk (&stk);
	free (ccs);
	if (name != buffer)
	    free(name);
	*ncc = 0;
	return NULL;
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (MARKED(n))
	    continue;
	sprintf(name + len, "%d", c_cnt);
#ifndef WITH_CGRAPH
	out = agsubg(g, name);
#else /* WITH_CGRAPH */
	out = agsubg(g, name,1);
	agbindrec(out, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
	dfs(g, n, insertFn, out, &stk);
	if (c_cnt == bnd) {
	    bnd *= 2;
	    ccs = RALLOC(bnd, ccs, Agraph_t *);
	}
	ccs[c_cnt] = out;
	c_cnt++;
    }
    freeStk (&stk);
    ccs = RALLOC(c_cnt, ccs, Agraph_t *);
    if (name != buffer)
	free(name);
    *ncc = c_cnt;
    return ccs;
}

/* cntFn:
 */
static void cntFn(Agnode_t * n, void *s)
{
    *(int *) s += 1;
}

/* isConnected:
 * Returns 1 if the graph is connected.
 * Returns 0 if the graph is not connected.
 * Returns -1 if the graph is error.
 */
int isConnected(Agraph_t * g)
{
    Agnode_t *n;
    int ret = 1;
    int cnt = 0;
    stk_t stk;
    blk_t blk;
    Agnode_t* base[INITBUF];

    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	UNMARK(n);

    n = agfstnode(g);
    if (n) {
	initStk (&stk, &blk, base);
	if (setjmp(jbuf)) {
	    freeStk (&stk);
	    return -1;
        }
	dfs(g, n, cntFn, &cnt, &stk);
	if (cnt != agnnodes(g))
	    ret = 0;
	freeStk (&stk);
    }
    return ret;
}

/* nodeInduce:
 * Given a subgraph, adds all edges in the root graph both of whose
 * endpoints are in the subgraph.
 * If g is a connected component, this will be all edges attached to
 * any node in g.
 * Returns the number of edges added.
 */
int nodeInduce(Agraph_t * g)
{
    Agnode_t *n;
    Agraph_t *root = g->root;
    Agedge_t *e;
    int e_cnt = 0;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(root, n); e; e = agnxtout(root, e)) {
	    if (agcontains(g, aghead(e))) {	/* test will always be true */
#ifndef WITH_CGRAPH
		aginsert(g, e);	/* for connected component  */
#else /* WITH_CGRAPH */
		agsubedge(g,e,1);
#endif /* WITH_CGRAPH */
		e_cnt++;
	    }
	}
    }
    return e_cnt;
}
