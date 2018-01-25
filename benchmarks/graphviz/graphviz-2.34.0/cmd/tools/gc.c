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


/*
 * Written by Emden Gansner
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32 /*dependencies*/
    #pragma comment( lib, "cgraph.lib" )
    #pragma comment( lib, "ingraphs.lib" )
#endif

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>

#define NEW(t)           (t*)malloc(sizeof(t))
#define N_NEW(n,t)       (t*)malloc((n)*sizeof(t))

#include "cgraph.h"
#include "cghdr.h"
typedef struct {
    Agrec_t h;
    int dfs_mark;
} Agnodeinfo_t;

#define ND_dfs_mark(n) (((Agnodeinfo_t*)(n->base.data))->dfs_mark)

#include "ingraphs.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "compat_getopt.h"
#endif

#define NODES 1
#define EDGES 2
#define CC    4
#define CL    8

#define DIRECTED   1
#define UNDIRECTED 2

static int tot_edges;
static int tot_nodes;
static int tot_cc;
static int tot_cl;
static int n_graphs;
static int n_indent;
static int recurse;
static int silent;
static int verbose;
static int gtype;
static int flags;
static char *fname;
static char **Files;
static FILE *outfile;

static char *useString = "Usage: gc [-necCaDUrsv?] <files>\n\
  -n - print number of nodes\n\
  -e - print number of edges\n\
  -c - print number of connected components\n\
  -C - print number of clusters\n\
  -a - print all counts\n\
  -D - only directed graphs\n\
  -U - only undirected graphs\n\
  -r - recursively analyze subgraphs\n\
  -s - silent\n\
  -v - verbose\n\
  -? - print usage\n\
By default, gc prints nodes and edges\n\
If no files are specified, stdin is used\n";

static void usage(int v)
{
    printf("%s",useString);
    exit(v);
}

static void init(int argc, char *argv[])
{
    unsigned int c;

    opterr = 0;
    while ((c = getopt(argc, argv, "necCaDUrsv")) != -1) {
	switch (c) {
	case 'e':
	    flags |= EDGES;
	    break;
	case 'n':
	    flags |= NODES;
	    break;
	case 'c':
	    flags |= CC;
	    break;
	case 'C':
	    flags |= CL;
	    tot_cl = 0;
	    break;
	case 'a':
	    flags = NODES | EDGES | CC | CL;
	    break;
	case 'r':
	    recurse = 1;
	    break;
	case 's':
	    silent = 1;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'D':
	    gtype = DIRECTED;
	    break;
	case 'U':
	    gtype = UNDIRECTED;
	    break;
	case '?':
	    if (optopt == '?')
		usage(0);
	    else
		fprintf(stderr, "gc: option -%c unrecognized - ignored\n",
			optopt);
	    break;
	}
    }
    argv += optind;
    argc -= optind;

    if (argc)
	Files = argv;
    if (flags == 0)
	flags = NODES | EDGES;
    if (gtype == 0)
	gtype = DIRECTED | UNDIRECTED;
    outfile = stdout;
}

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


#define SMALLBUF 1024
#define BIGBUF 1000000

static Agnode_t *base[SMALLBUF];
static blk_t Blk;
static stk_t Stk;

static void initStk()
{
    Blk.data = base;
    Blk.endp = Blk.data + SMALLBUF;
    Stk.curblk = Stk.fstblk = &Blk;
    Stk.curp = Stk.curblk->data;
}

static void push(Agnode_t * np)
{
    if (Stk.curp == Stk.curblk->endp) {
	if (Stk.curblk->next == NULL) {
	    blk_t *bp = NEW(blk_t);
	    if (bp == 0) {
		fprintf(stderr, "gc: Out of memory\n");
		exit(1);
	    }
	    bp->prev = Stk.curblk;
	    bp->next = NULL;
	    bp->data = N_NEW(BIGBUF, Agnode_t *);
	    if (bp->data == 0) {
		fprintf(stderr, "gc: Out of memory\n");
		exit(1);
	    }
	    bp->endp = bp->data + BIGBUF;
	    Stk.curblk->next = bp;
	}
	Stk.curblk = Stk.curblk->next;
	Stk.curp = Stk.curblk->data;
    }
    ND_dfs_mark(np) = -1;
    *Stk.curp++ = np;
}

static Agnode_t *pop()
{
    if (Stk.curp == Stk.curblk->data) {
	if (Stk.curblk == Stk.fstblk)
	    return 0;
	Stk.curblk = Stk.curblk->prev;
	Stk.curp = Stk.curblk->endp;
    }
    Stk.curp--;
    return *Stk.curp;
}

static void cc_dfs(Agraph_t * g, Agnode_t * n)
{
    Agedge_t *e;
    Agnode_t *nxt;

    push(n);
    while ((n = pop())) {
	ND_dfs_mark(n) = 1;
	for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
	    if (n == agtail(e))
		nxt = aghead(e);
	    else
		nxt = agtail(e);
	    if (ND_dfs_mark(nxt) == 0)
		push(nxt);
	}
    }
}

static void cntCluster(Agraph_t * g, Agobj_t * sg, void *arg)
{
    char *sgname = agnameof((Agraph_t *) sg);

    if (strncmp(sgname, "cluster", 7) == 0)
	*(int *) (arg) += 1;
}

static int cc_decompose(Agraph_t * g)
{
    int c_cnt = 0;
    Agnode_t *n;

    c_cnt = 0;
    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	ND_dfs_mark(n) = 0;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (ND_dfs_mark(n))
	    continue;
	c_cnt++;
	cc_dfs(g, n);
    }

    return c_cnt;
}

static void ipr(long num)
{
    printf(" %7ld", num);
}

static void
wcp(int nnodes, int nedges, int ncc, int ncl, char *gname, char *fname)
{
    int i;

    if (silent)
	return;
    for (i = 0; i < n_indent; i++)
	fputs("  ", outfile);
    if (flags & NODES)
	ipr(nnodes);
    if (flags & EDGES)
	ipr(nedges);
    if (flags & CC)
	ipr(ncc);
    if (flags & CL)
	ipr(ncl);
    if (fname)
	printf(" %s (%s)\n", gname, fname);
    else
	printf(" %s\n", gname);
}

static void emit(Agraph_t * g, int root, int cl_count)
{
    int n_edges = agnedges(g);
    int n_nodes = agnnodes(g);
    int n_cc = 0;
    int n_cl = 0;
    char *file = 0;

    if (flags & CC)
	n_cc = cc_decompose(g);

    if (flags & CL)
	n_cl = cl_count;

    if (root)
	file = fname;
    wcp(n_nodes, n_edges, n_cc, n_cl, agnameof(g), file);

    if (root) {
	n_graphs++;
	tot_edges += n_edges;
	tot_nodes += n_nodes;
	tot_cc += n_cc;
	tot_cl += n_cl;
    }
}

#define GTYPE(g) (agisdirected(g)?DIRECTED:UNDIRECTED)

static int eval(Agraph_t * g, int root)
{
    Agraph_t *subg;
    int cl_count;

    if (root && !(GTYPE(g) & gtype))
	return 1;

    if (root) {
	aginit(g, AGNODE, "nodeinfo", sizeof(Agnodeinfo_t), TRUE);
    }

    if ((flags & CL) && root) {
	cl_count = 0;
	agapply(g, (Agobj_t *) g, cntCluster, &cl_count, 0);
    }

    emit(g, root, cl_count);

    if (recurse) {
	n_indent++;
	for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg))
	    eval(subg, 0);
	n_indent--;
    }
    return 0;
}

static Agraph_t *gread(FILE * fp)
{
    return agread(fp, (Agdisc_t *) 0);
}

int main(int argc, char *argv[])
{
    Agraph_t *g;
    Agraph_t *prev = NULL;
    ingraph_state ig;
    int rv = 0;

    init(argc, argv);
    newIngraph(&ig, Files, gread);

    if (flags & CC)
	initStk();

    while ((g = nextGraph(&ig)) != 0) {
	if (prev)
	    agclose(prev);
	prev = g;
	fname = fileName(&ig);
	if (verbose)
	    fprintf(stderr, "Process graph %s in file %s\n", agnameof(g),
		    fname);
	rv |= eval(g, 1);
    }

    if (n_graphs > 1)
	wcp(tot_nodes, tot_edges, tot_cc, tot_cl, "total", 0);

    return rv;
}
