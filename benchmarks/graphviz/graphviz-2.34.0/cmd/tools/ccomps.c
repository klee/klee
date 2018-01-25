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
 * Written by Stephen North
 * Updated by Emden Gansner
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <stdlib.h>
#include "cgraph.h"

#define N_NEW(n,t)       (t*)malloc((n)*sizeof(t))
#define NEW(t)           (t*)malloc(sizeof(t))

typedef struct {
    Agrec_t h;
    char cc_subg;   /* true iff subgraph corresponds to a component */
} Agraphinfo_t;

typedef struct {
    Agrec_t h;
    char mark;
    Agobj_t* ptr;
} Agnodeinfo_t;

#define GD_cc_subg(g)  (((Agraphinfo_t*)(g->base.data))->cc_subg)
#define ND_mark(n)  (((Agnodeinfo_t*)(n->base.data))->mark)
#define ND_ptr(n)  (((Agnodeinfo_t*)(n->base.data))->ptr)
#define ND_dn(n)  ((Agnode_t*)ND_ptr(n))
#define ND_clust(n)  ((Agraph_t*)ND_ptr(n))
#define agfindnode(G,N) (agnode(G, N, 0))

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "compat_getopt.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include "ingraphs.h"

  /* internals of libgraph */
#define TAG_NODE            1

#define INTERNAL 0       /* Basically means all components need to be 
                          * generated before output
                          */
#define EXTERNAL 1
#define SILENT   2
#define EXTRACT  3

#define BY_INDEX 1
#define BY_SIZE  2

char* Cmd;
char **Files;
int verbose;
int printMode = INTERNAL;
int useClusters = 0;
int doEdges = 1;		/* induce edges */
int doAll = 1;			/* induce subgraphs */
char *suffix = 0;
char *outfile = 0;
char *path = 0;
int sufcnt = 0;
int sorted = 0;
int sortIndex = 0;
int sortFinal;
int x_index = -1;
int x_final = -1;  /* require 0 <= x_index <= x_final or x_final= -1 */ 
int x_mode;
char *x_node;

static char *useString =
    "Usage: ccomps [-svenCx?] [-X[#%]s[-f]] [-o<out template>] <files>\n\
  -s - silent\n\
  -x - external\n\
  -X - extract component\n\
  -C - use clusters\n\
  -e - do not induce edges\n\
  -n - do not induce subgraphs\n\
  -v - verbose\n\
  -o - output file template\n\
  -z - sort by size, largest first\n\
  -? - print usage\n\
If no files are specified, stdin is used\n";

static void usage(int v)
{
    printf("%s",useString);
    exit(v);
}

static void split(char *name)
{
    char *sfx = 0;
    int size;

    sfx = strrchr(name, '.');
    if (sfx) {
	suffix = sfx + 1;
	size = sfx - name;
	path = (char *) malloc(size + 1);
	strncpy(path, name, size);
	*(path + size) = '\0';
    } else {
	path = name;
    }
}

/* isCluster:
 * Return true if graph is a cluster
 */
static int isCluster(Agraph_t * g)
{
    return (strncmp(agnameof(g), "cluster", 7) == 0);
}

static void init(int argc, char *argv[])
{
    int c;
    char* endp;

    Cmd = argv[0];
    opterr = 0;
    while ((c = getopt(argc, argv, ":zo:xCX:nesv")) != -1) {
	switch (c) {
	case 'o':
	    outfile = optarg;
	    split(outfile);
	    break;
	case 'C':
	    useClusters = 1;
	    break;
	case 'e':
	    doEdges = 0;
	    break;
	case 'n':
	    doAll = 0;
	    break;
	case 'x':
	    printMode = EXTERNAL;
	    break;
	case 's':
	    printMode = SILENT;
	    break;
	case 'X':
	    if ((*optarg == '#') || (*optarg == '%')) {
		char *p = optarg + 1;
		if (*optarg == '#') x_mode = BY_INDEX;
		else x_mode = BY_SIZE;
		if (isdigit(*p)) {
		    x_index = (int)strtol (p, &endp, 10);
		    printMode = EXTRACT;
		    if (*endp == '-') {
			p = endp + 1;
			if (isdigit(*p)) {
			    x_final = atoi (p);
			    if (x_final < x_index) {
				printMode = INTERNAL;
				fprintf(stderr,
				    "ccomps: final index %d < start index %d in -X%s flag - ignored\n",
				    x_final, x_index, optarg);
			    }
			}
			else if (*p) {
			    printMode = INTERNAL;
			    fprintf(stderr,
				"ccomps: number expected in -X%s flag - ignored\n",
				optarg);
			}
		    }
		    else
			x_final = x_index;
		} else
		    fprintf(stderr,
			    "ccomps: number expected in -X%s flag - ignored\n",
			    optarg);
	    } else {
		x_node = optarg;
		printMode = EXTRACT;
	    }
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'z':
	    sorted = 1;
	    break;
	case ':':
	    fprintf(stderr,
		"ccomps: option -%c missing argument - ignored\n", optopt);
	    break;
	case '?':
	    if (optopt == '?')
		usage(0);
	    else
		fprintf(stderr,
			"ccomps: option -%c unrecognized - ignored\n", optopt);
	    break;
	}
    }
    argv += optind;
    argc -= optind;

    if (sorted) {
	if ((printMode == EXTRACT) && (x_index >= 0)) {
	    printMode = INTERNAL;
	    sortIndex = x_index;
	    sortFinal = x_final;
	}
	else if (printMode == EXTERNAL) {
	    sortIndex = -1;
	    printMode = INTERNAL;
	}
	else
	    sorted = 0;    /* not relevant; turn off */
    }
    if (argc > 0)
	Files = argv;
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
		fprintf(stderr, "%s: Out of memory\n", Cmd);
		exit(1);
	    }
	    bp->endp = bp->data + BIGBUF;
	    Stk.curblk->next = bp;
	}
	Stk.curblk = Stk.curblk->next;
	Stk.curp = Stk.curblk->data;
    }
    ND_mark(np) = -1;
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

static int dfs(Agraph_t * g, Agnode_t * n, Agraph_t * out)
{
    Agedge_t *e;
    Agnode_t *other;
    int cnt = 0;

    push(n);
    while ((n = pop())) {
	ND_mark(n) = 1;
	cnt++;
	agsubnode(out, n, 1);
	for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
	    if ((other = agtail(e)) == n)
		other = aghead(e);
	    if (ND_mark(other) == 0)
		push (other);
	}
    }
    return cnt;
}

/* nodeInduce:
 * Using the edge set of eg, add to g any edges
 * with both endpoints in g.
 */
static int nodeInduce(Agraph_t * g, Agraph_t * eg)
{
    Agnode_t *n;
    Agedge_t *e;
    int e_cnt = 0;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(eg, n); e; e = agnxtout(eg, e)) {
	    if (agsubnode(g, aghead(e), 0)) {
		agsubedge(g, e, 1);
		e_cnt++;
	    }
	}
    }
    return e_cnt;
}

static char *getName(void)
{
    char *name;
    static char *buf = 0;

    if (sufcnt == 0)
	name = outfile;
    else {
	if (!buf)
	    buf = (char *) malloc(strlen(outfile) + 20);	/* enough to handle '_number' */
	if (suffix)
	    sprintf(buf, "%s_%d.%s", path, sufcnt, suffix);
	else
	    sprintf(buf, "%s_%d", path, sufcnt);
	name = buf;
    }
    sufcnt++;
    return name;
}

static void gwrite(Agraph_t * g)
{
    FILE *outf;
    char *name;

    if (!outfile) {
	agwrite(g, stdout);
	fflush(stdout);
    } else {
	name = getName();
	outf = fopen(name, "w");
	if (!outf) {
	    fprintf(stderr, "Could not open %s for writing\n", name);
	    perror("ccomps");
	}
	agwrite(g, outf);
	fflush(outf);
	fclose(outf);
    }
}

/* getBuf
 * Return pointer to buffer containing at least n bytes.
 * Non-reentrant.
 */
static char *getBuf(int n)
{
    static int len = 0;
    static char *buf = 0;
    int sz;

    if (n > len) {
	sz = n + 100;
	if (len == 0)
	    buf = (char *) malloc(sz);
	else
	    buf = (char *) realloc(buf, sz);
	len = sz;
    }
    return buf;
}

/* projectG:
 * If any nodes of subg are in g, create a subgraph of g
 * and fill it with all nodes of subg in g and their induced
 * edges in subg. Copy the attributes of subg to g. Return the subgraph.
 * If not, return null.
 */
static Agraph_t *projectG(Agraph_t * subg, Agraph_t * g, int inCluster)
{
    Agraph_t *proj = 0;
    Agnode_t *n;
    Agnode_t *m;

    for (n = agfstnode(subg); n; n = agnxtnode(subg, n)) {
	if ((m = agfindnode(g, agnameof(n)))) {
	    if (proj == 0) {
		proj = agsubg(g, agnameof(subg), 1);
	    }
	    agsubnode(proj, m, 1);
	}
    }
    if (!proj && inCluster) {
	proj = agsubg(g, agnameof(subg), 1);
    }
    if (proj) {
	if (doEdges) nodeInduce(proj, subg);
	agcopyattr(subg, proj);
    }

    return proj;
}

/* subgInduce:
 * Project subgraphs of root graph on subgraph.
 * If non-empty, add to subgraph.
 */
static void
subgInduce(Agraph_t * root, Agraph_t * g, int inCluster)
{
    Agraph_t *subg;
    Agraph_t *proj;
    int in_cluster;

/* fprintf (stderr, "subgInduce %s inCluster %d\n", agnameof(root), inCluster); */
    for (subg = agfstsubg(root); subg; subg = agnxtsubg(subg)) {
	if (GD_cc_subg(subg))
	    continue;
	if ((proj = projectG(subg, g, inCluster))) {
	    in_cluster = inCluster || (useClusters && isCluster(subg));
	    subgInduce(subg, proj, in_cluster);
	}
    }
}

static void
subGInduce(Agraph_t* g, Agraph_t * out)
{
    subgInduce(g, out, 0);
}

#define PFX1 "%s_cc"
#define PFX2 "%s_cc_%ld"

/* deriveClusters:
 * Construct nodes in derived graph corresponding top-level clusters.
 * Since a cluster might be wrapped in a subgraph, we need to traverse
 * down into the tree of subgraphs
 */
static void deriveClusters(Agraph_t* dg, Agraph_t * g)
{
    Agraph_t *subg;
    Agnode_t *dn;
    Agnode_t *n;

    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	if (!strncmp(agnameof(subg), "cluster", 7)) {
	    dn = agnode(dg, agnameof(subg), 1);
	    agbindrec (dn, "nodeinfo", sizeof(Agnodeinfo_t), TRUE);
	    ND_ptr(dn) = (Agobj_t*)subg;
	    for (n = agfstnode(subg); n; n = agnxtnode(subg, n)) {
		if (ND_ptr(n)) {
		   fprintf (stderr, "Error: node \"%s\" belongs to two non-nested clusters \"%s\" and \"%s\"\n",
			agnameof (n), agnameof(subg), agnameof(ND_dn(n)));  
		}
		ND_ptr(n) = (Agobj_t*)dn;
	    }
	}
	else {
	    deriveClusters (dg, subg);
	}
    }
}

/* deriveGraph:
 * Create derived graph dg of g where nodes correspond to top-level nodes 
 * or clusters, and there is an edge in dg if there is an edge in g
 * between any nodes in the respective clusters.
 */
static Agraph_t *deriveGraph(Agraph_t * g)
{
    Agraph_t *dg;
    Agnode_t *dn;
    Agnode_t *n;

    dg = agopen("dg", Agstrictundirected, (Agdisc_t *) 0);

    deriveClusters (dg, g);

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (ND_dn(n))
	    continue;
	dn = agnode(dg, agnameof(n), 1);
	agbindrec (dn, "nodeinfo", sizeof(Agnodeinfo_t), TRUE);
	ND_ptr(dn) = (Agobj_t*)n;
	ND_ptr(n) = (Agobj_t*)dn;
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	Agedge_t *e;
	Agnode_t *hd;
	Agnode_t *tl = ND_dn(n);
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    hd = ND_dn(aghead(e));
	    if (hd == tl)
		continue;
	    if (hd > tl)
		agedge(dg, tl, hd, 0, 1);
	    else
		agedge(dg, hd, tl, 0, 1);
	}
    }

    return dg;
}

/* unionNodes:
 * Add all nodes in cluster nodes of dg to g
 */
static void unionNodes(Agraph_t * dg, Agraph_t * g)
{
    Agnode_t *n;
    Agnode_t *dn;
    Agraph_t *clust;

    for (dn = agfstnode(dg); dn; dn = agnxtnode(dg, dn)) {
	if (AGTYPE(ND_ptr(dn)) == AGNODE) {
	    agsubnode(g, ND_dn(dn), 1);
	} else {
	    clust = ND_clust(dn);
	    for (n = agfstnode(clust); n; n = agnxtnode(clust, n))
		agsubnode(g, n, 1);
	}
    }
}

typedef int (*qsort_cmpf) (const void *, const void *);

static int cmp(Agraph_t** p0, Agraph_t** p1)
{
    return (agnnodes(*p1) - agnnodes(*p0));
}

static void
printSorted (Agraph_t* root, int c_cnt)
{
    Agraph_t** ccs = N_NEW(c_cnt, Agraph_t*);
    Agraph_t* subg;
    int i = 0, endi;

    for (subg = agfstsubg(root); subg; subg = agnxtsubg(subg)) {
	if (GD_cc_subg(subg))
	    ccs[i++] = subg;
    }
    /* sort by component size, largest first */
    qsort (ccs, c_cnt, sizeof(Agraph_t*), (qsort_cmpf)cmp);

    if (sortIndex >= 0) {
	if (x_mode == BY_INDEX) {
	    if (sortIndex >= c_cnt) {
		fprintf(stderr,
		    "ccomps: component %d not found in graph %s - ignored\n",
		    sortIndex, agnameof(root));
		return;
	    }
	    if (sortFinal >= sortIndex) {
		endi = sortFinal;
		if (endi >= c_cnt) endi = c_cnt-1;
	    }
	    else
		endi = c_cnt-1;
            for (i = sortIndex; i <= endi ; i++) {
		subg = ccs[i];
		if (doAll)
		    subGInduce(root, subg);
		gwrite(subg);
	    }
	}
	else if (x_mode == BY_SIZE) {
	    if (sortFinal == -1)
		sortFinal = agnnodes(ccs[0]);
            for (i = 0; i < c_cnt ; i++) {
		int sz;
		subg = ccs[i];
		sz = agnnodes(subg);
		if (sz > sortFinal) continue;
		if (sz < sortIndex) break;
		if (doAll)
		    subGInduce(root, subg);
		gwrite(subg);
	    }
	}
    }
    else for (i = 0; i < c_cnt; i++) {
	subg = ccs[i];
	if (doAll)
	    subGInduce(root, subg);
	gwrite(subg);
    }
    free (ccs);
}

/* processClusters:
 * Return 0 if graph is connected.
 */
static int processClusters(Agraph_t * g, char* graphName)
{
    Agraph_t *dg;
    long n_cnt, c_cnt, e_cnt;
    char *name;
    Agraph_t *out;
    Agnode_t *n;
    Agraph_t *dout;
    Agnode_t *dn;
    int extracted = 0;

    dg = deriveGraph(g);

    if (x_node) {
	n = agfindnode(g, x_node);
	if (!n) {
	    fprintf(stderr, "ccomps: node %s not found in graph %s\n",
		    x_node, agnameof(g));
	    return 1;
	}
	name = getBuf(sizeof(PFX1) + strlen(graphName));
	sprintf(name, PFX1, graphName);
	dout = agsubg(dg, name, 1);
	out = agsubg(g, name, 1);
	aginit(out, AGRAPH, "graphinfo", sizeof(Agraphinfo_t), TRUE);
	GD_cc_subg(out) = 1;
	dn = ND_dn(n);
	n_cnt = dfs(dg, dn, dout);
	unionNodes(dout, out);
	if (doEdges)
	    e_cnt = nodeInduce(out, out->root);
	else
	    e_cnt = 0;
	if (doAll)
	    subGInduce(g, out);
	gwrite(out);
	if (verbose)
	    fprintf(stderr, " %7ld nodes %7ld edges\n", n_cnt, e_cnt);
	return 0;
    }

    c_cnt = 0;
    for (dn = agfstnode(dg); dn; dn = agnxtnode(dg, dn)) {
	if (ND_mark(dn))
	    continue;
	name = getBuf(sizeof(PFX2) + strlen(graphName) + 32);
	sprintf(name, PFX2, graphName, c_cnt);
	dout = agsubg(dg, name, 1);
	out = agsubg(g, name, 1);
	aginit(out, AGRAPH, "graphinfo", sizeof(Agraphinfo_t), TRUE);
	GD_cc_subg(out) = 1;
	n_cnt = dfs(dg, dn, dout);
	unionNodes(dout, out);
	if (doEdges)
	    e_cnt = nodeInduce(out, out->root);
	else
	    e_cnt = 0;
	if (printMode == EXTERNAL) {
	    if (doAll)
		subGInduce(g, out);
	    gwrite(out);
	} else if (printMode == EXTRACT) {
	    if (x_mode == BY_INDEX) {
		if (x_index <= c_cnt) {
		    extracted = 1;
		    if (doAll)
			subGInduce(g, out);
		    gwrite(out);
		    if (c_cnt == x_final)
			return 0;
	        }
	    }
	    else if (x_mode == BY_SIZE) {
		int sz = agnnodes(out);
		if ((x_index <= sz) && ((x_final == -1) || (sz <= x_final))) {
		    extracted = 1;
		    if (doAll)
			subGInduce(g, out);
		    gwrite(out);
	        }
	    }
	}
	if (printMode != INTERNAL)
	    agdelete(g, out);
	agdelete(dg, dout);
	if (verbose)
	    fprintf(stderr, "(%4ld) %7ld nodes %7ld edges\n",
		    c_cnt, n_cnt, e_cnt);
	c_cnt++;
    }
    if ((printMode == EXTRACT) && !extracted && (x_mode == BY_INDEX))  {
	fprintf(stderr,
		"ccomps: component %d not found in graph %s - ignored\n",
		x_index, agnameof(g));
	return 1;
    }

    if (sorted) 
	printSorted (g, c_cnt);
    else if (printMode == INTERNAL)
	gwrite(g);

    if (verbose)
	fprintf(stderr, "       %7d nodes %7d edges %7ld components %s\n",
		agnnodes(g), agnedges(g), c_cnt, agnameof(g));

    agclose(dg);

    return (c_cnt ? 1 : 0);
}

static void
bindGraphinfo (Agraph_t * g)
{
    Agraph_t *subg;

    aginit(g, AGRAPH, "graphinfo", sizeof(Agraphinfo_t), TRUE);
    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	bindGraphinfo (subg);
    }
}

/* process:
 * Return 0 if graph is connected.
 */
static int process(Agraph_t * g, char* graphName)
{
    long n_cnt, c_cnt, e_cnt;
    char *name;
    Agraph_t *out;
    Agnode_t *n;
    int extracted = 0;

    aginit(g, AGNODE, "nodeinfo", sizeof(Agnodeinfo_t), TRUE);
    bindGraphinfo (g);
    initStk();

    if (useClusters)
	return processClusters(g, graphName);

    if (x_node) {
	n = agfindnode(g, x_node);
	if (!n) {
	    fprintf(stderr,
		    "ccomps: node %s not found in graph %s - ignored\n",
		    x_node, agnameof(g));
	    return 1;
	}
	name = getBuf(sizeof(PFX1) + strlen(graphName));
	sprintf(name, PFX1, graphName);
	out = agsubg(g, name, 1);
	aginit(out, AGRAPH, "graphinfo", sizeof(Agraphinfo_t), TRUE);
	GD_cc_subg(out) = 1;
	n_cnt = dfs(g, n, out);
	if (doEdges)
	    e_cnt = nodeInduce(out, out->root);
	else
	    e_cnt = 0;
	if (doAll)
	    subGInduce(g, out);
	gwrite(out);
	if (verbose)
	    fprintf(stderr, " %7ld nodes %7ld edges\n", n_cnt, e_cnt);
	return 0;
    }

    c_cnt = 0;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (ND_mark(n))
	    continue;
	name = getBuf(sizeof(PFX2) + strlen(graphName) + 32);
	sprintf(name, PFX2, graphName, c_cnt);
	out = agsubg(g, name, 1);
	aginit(out, AGRAPH, "graphinfo", sizeof(Agraphinfo_t), TRUE);
	GD_cc_subg(out) = 1;
	n_cnt = dfs(g, n, out);
	if (doEdges)
	    e_cnt = nodeInduce(out, out->root);
	else
	    e_cnt = 0;
	if (printMode == EXTERNAL) {
	    if (doAll)
		subGInduce(g, out);
	    gwrite(out);
	} else if (printMode == EXTRACT) {
	    if (x_mode == BY_INDEX) {
		if (x_index <= c_cnt) {
		    extracted = 1;
		    if (doAll)
			subGInduce(g, out);
		    gwrite(out);
		    if (c_cnt == x_final)
			return 0;
	        }
	    }
	    else if (x_mode == BY_SIZE) {
		int sz = agnnodes(out);
		if ((x_index <= sz) && ((x_final == -1) || (sz <= x_final))) {
		    extracted = 1;
		    if (doAll)
			subGInduce(g, out);
		    gwrite(out);
	        }
	    }
	}
	if (printMode != INTERNAL)
	    agdelete(g, out);
	if (verbose)
	    fprintf(stderr, "(%4ld) %7ld nodes %7ld edges\n",
		    c_cnt, n_cnt, e_cnt);
	c_cnt++;
    }
    if ((printMode == EXTRACT) && !extracted && (x_mode == BY_INDEX))  {
	fprintf(stderr,
		"ccomps: component %d not found in graph %s - ignored\n",
		x_index, agnameof(g));
	return 1;
    }

    if (sorted)
	printSorted (g, c_cnt);
    else if (printMode == INTERNAL)
	gwrite(g);

    if (verbose)
	fprintf(stderr, "       %7d nodes %7d edges %7ld components %s\n",
		agnnodes(g), agnedges(g), c_cnt, agnameof(g));
    return (c_cnt > 1);
}

static Agraph_t *gread(FILE * fp)
{
    return agread(fp, (Agdisc_t *) 0);
}

/* chkGraphName:
 * If the graph is anonymous, its name starts with '%'.
 * If we use this as the prefix for subgraphs, they will not be
 * emitted in agwrite() because cgraph assumes these were anonymous
 * structural subgraphs all of whose properties are attached directly
 * to the nodes and edges involved.
 *
 * This function checks for an initial '%' and if found, prepends '_'
 * the the graph name.
 * NB: static buffer is used.
 */
static char*
chkGraphName (Agraph_t* g)
{
    static char* buf = NULL;
    static int buflen = 0;
    char* s = agnameof(g);
    int len;

    if (*s != '%') return s;
    len = strlen(s) + 2;   /* plus '\0' and '_' */
    if (len > buflen) {
	buf = realloc (buf, len);
	buflen = len;
    }
    buf[0] = '_';
    strcpy (buf+1, s);

    return buf;
}

int main(int argc, char *argv[])
{
    Agraph_t *g;
    ingraph_state ig;
    int r = 0;
    init(argc, argv);
    newIngraph(&ig, Files, gread);

    while ((g = nextGraph(&ig)) != 0) {
	r += process(g, chkGraphName(g));
	agclose(g);
    }

    return r;
}
