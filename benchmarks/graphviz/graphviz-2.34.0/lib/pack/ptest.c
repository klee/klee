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

#include <assert.h>
#include "render.h"
#include "neatoprocs.h"
#include "pack.h"

/* Test driver for libpack library.
 * Input consists of graphs in dot format.
 * If -c is not specified, the graphs must have pos information,
 * typically the output of one of the layout programs using -Tdot.
 *  -c computes connected components of input graphs
 *   Otherwise, ptest packs the input graphs.
 *  -s causes all the input graphs to be combined
 *   into a single output graph, ready to be sent to neato -s -n2.
 *   Otherwise, each graph is output separately, but with the
 *   appropriately adjusted coordinates.
 *  -e causes the packing to not use edge splines, if any.
 *   If any input graph does not have spline info, -e goes into 
 *   effect automatically.
 *  -m <i> specifies the margin, in points, about each graph.
 */
char *Info[] = {
    "ptest",			/* Program */
    "1.0",			/* Version */
    DATE			/* Build Date */
};

static int margin = 8;
static int doEdges = 1;
static int doComps = 0;
static int verbose = 0;
static char **Files = 0;
static int nFiles = 0;
static int single = 0;

static char *useString = "Usage: ptest [-cesv?] [-m <margine>] <files>\n\
  -c - components\n\
  -e - no edges\n\
  -m n - set margine\n\
  -v - verbose\n\
  -s - single graph\n\
  -? - print usage\n\
If no files are specified, stdin is used\n";

static void usage(int v)
{
    printf(useString);
    exit(v);
}

static void init(int argc, char *argv[])
{
    int c;

    aginit();
    while ((c = getopt(argc, argv, ":escvm:?")) != -1) {
	switch (c) {
	case 'e':
	    doEdges = 0;
	    break;
	case 'c':
	    doComps = 1;
	    break;
	case 'm':
	    margin = atoi(optarg);
	    break;
	case 's':
	    single = 1;
	    break;
	case 'v':
	    verbose = 1;
	    Verbose = 1;
	    break;
	case '?':
	    if (optopt == '?')
		usage(0);
	    else
		fprintf(stderr,
			"ptest: option -%c unrecognized - ignored\n", c);
	    break;
	}
    }
    argv += optind;
    argc -= optind;

    if (argc) {
	Files = argv;
	nFiles = argc;
    }

}

static int numFields(char *pos)
{
    int cnt = 0;
    char c;

    while (isspace(*pos))
	pos++;
    while (*pos) {
	cnt++;
	while ((c = *pos) && !isspace(c))
	    pos++;		/* skip token */
	while (isspace(*pos))
	    pos++;
    }
    return cnt;
}

static point *user_spline(attrsym_t * symptr, edge_t * e, int *np)
{
    char *pos;
    int i, n, nc;
    point *ps = 0;
    point *pp;
    double x, y;

    if (symptr == NULL)
	return 0;
    pos = agxget(e, symptr->index);
    if (*pos == '\0')
	return 0;
    n = numFields(pos);
    *np = n;
    if (n > 1) {
	ps = ALLOC(n, 0, point);
	pp = ps;
	while (n) {
	    i = sscanf(pos, "%lf,%lf%n", &x, &y, &nc);
	    if (i < 2) {
		free(ps);
		ps = 0;
		break;
	    }
	    pos = pos + nc;
	    pp->x = (int) x;
	    pp->y = (int) y;
	    pp++;
	    n--;
	}
    }
    return ps;
}

static void initPos(Agraph_t * g)
{
    Agnode_t *n;
    Agedge_t *e;
    double *pvec;
    char *p;
    point *sp;
    int pn;
    attrsym_t *N_pos = agfindnodeattr(g, "pos");
    attrsym_t *E_pos = agfindedgeattr(g, "pos");

    assert(N_pos);
    if (!E_pos) {
	if (doEdges)
	    fprintf(stderr, "Warning: turning off doEdges, graph %s\n",
		    g->name);
	doEdges = 0;
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	pvec = ND_pos(n);
	p = agxget(n, N_pos->index);
	if (p[0] && (sscanf(p, "%lf,%lf", pvec, pvec + 1) == 2)) {
	    int i;
	    for (i = 0; i < NDIM; i++)
		pvec[i] = pvec[i] / PSinputscale;
	} else {
	    fprintf(stderr, "could not find pos for node %s in graph %s\n",
		    n->name, g->name);
	    exit(1);
	}
	ND_coord_i(n).x = POINTS(ND_pos(n)[0]);
	ND_coord_i(n).y = POINTS(ND_pos(n)[1]);
    }

    if (doEdges) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
		if ((sp = user_spline(E_pos, e, &pn)) != 0) {
		    clip_and_install(e, sp, pn);
		    free(sp);
		} else {
		    fprintf(stderr,
			    "Missing edge pos for edge %s - %s in graph %s\n",
			    n->name, e->head->name, g->name);
		    exit(1);
		}
	    }
	}
    }
}

static void ptest_nodesize(node_t * n, boolean flip)
{
    int w;

    w = ND_xsize(n) = POINTS(ND_width(n));
    ND_lw(n) = ND_rw(n) = w / 2;
    ND_ht(n) = ND_ysize(n) = POINTS(ND_height(n));
}


static void ptest_initNode(node_t * n)
{
    char *str;
    ND_width(n) =
	late_double(n, N_width, DEFAULT_NODEWIDTH, MIN_NODEWIDTH);
    ND_height(n) =
	late_double(n, N_height, DEFAULT_NODEHEIGHT, MIN_NODEWIDTH);
    if (N_label == NULL)
	str = NODENAME_ESC;
    else
	str = agxget(n, N_label->index);
    str = strdup_and_subst(str, NODENAME_ESC, n->name);
    ND_label(n) = make_label(str,
			     late_double(n, N_fontsize, DEFAULT_FONTSIZE,
					 MIN_FONTSIZE), late_nnstring(n,
								      N_fontname,
								      DEFAULT_FONTNAME),
			     late_nnstring(n, N_fontcolor, DEFAULT_COLOR),
			     n->graph);
    ND_shape(n) = bind_shape(late_nnstring(n, N_shape, DEFAULT_NODESHAPE));
    ND_shape(n)->initfn(n);	/* ### need to quantize ? */
    ptest_nodesize(n, n->GD_flip(graph));


}

static void ptest_initGraph(graph_t * g)
{
    node_t *n;
    /* edge_t *e; */

    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	ptest_initNode(n);
/*
  for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
    for (e = agfstout(g,n); e; e = agnxtout(g,e)) ptest_initEdge(e);
  }
*/
}

static void dumpG(graph_t * g)
{
    node_t *n;
    /* point  p; */
    edge_t *e;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	fprintf(stderr, " node %s \n", n->name);

	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    fprintf(stderr, " %s - %s \n", e->tail->name, e->head->name);
	}
#ifdef OLD
	p = coord(n);
	fprintf(stderr, " %s pos (%f,%f) (%d,%d)\n",
		n->name, ND_pos(n)[0], ND_pos(n)[1], p.x, p.y);
	fprintf(stderr, "   width %f height %f xsize %d ysize %d\n",
		ND_width(n), ND_height(n), ND_xsize(n), ND_ysize(n));
#endif
    }
}

static void copyPos(Agraph_t * g)
{
    Agnode_t *n;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	ND_coord_i(n).x = POINTS(ND_pos(n)[0]);
	ND_coord_i(n).y = POINTS(ND_pos(n)[1]);
    }
}

main(int argc, char *argv[])
{
    Agraph_t **gs;
    Agraph_t **ccs;
    Agraph_t *g;
    Agraph_t *gp;
    char *fname;
    FILE *fp;
    int cnt;
    int i;

    init(argc, argv);
    if (!Files) {
	fprintf(stderr, "No input files given\n");
	exit(1);
    }

    PSinputscale = POINTS_PER_INCH;
    if (doComps) {
	if (verbose)
	    fprintf(stderr, "do Comps\n");
	while (fname = *Files++) {
	    fp = fopen(fname, "r");
	    if (!fp) {
		fprintf(stderr, "Could not open %s\n", fname);
		continue;
	    }
	    g = agread(fp);
	    fclose(fp);
	    if (!g) {
		fprintf(stderr, "Could not read graph\n");
		continue;
	    }
	    printf("%s %d nodes %d edges %sconnected\n",
		   g->name, agnnodes(g), agnedges(g),
		   (isConnected(g) ? "" : "not "));
	    gs = ccomps(g, &cnt, "abc");
	    for (i = 0; i < cnt; i++) {
		gp = gs[i];
		printf(" %s %d nodes %d edges\n", gp->name, agnnodes(gp),
		       agnedges(gp));
	    }
	}
    } else {
	gs = N_GNEW(nFiles, Agraph_t *);
	cnt = 0;
	while (fname = Files[cnt]) {
	    fp = fopen(fname, "r");
	    if (!fp) {
		fprintf(stderr, "Could not open %s\n", fname);
		exit(1);
	    }
	    g = agread(fp);
	    fclose(fp);
	    if (!g) {
		fprintf(stderr, "Could not read graph\n");
		exit(1);
	    }
	    if (!single) {
		graph_init(g);
		ptest_initGraph(g);
	    }
	    initPos(g);
	    /* if (Verbose) dumpG (g); */
	    gs[cnt++] = g;
	}
	if (single) {
	    Agraph_t *root;
	    Agnode_t *n;
	    Agnode_t *np;
	    Agnode_t *tp;
	    Agnode_t *hp;
	    Agedge_t *e;
	    Agedge_t *ep;
	    root = agopen("root", 0);
	    agedgeattr(root, "pos", "");
	    for (i = 0; i < cnt; i++) {
		g = gs[i];
		for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
		    if (agfindnode(root, n->name)) {
			fprintf(stderr,
				"Error: node %s in graph %d (%s) previously added\n",
				n->name, i, Files[i]);
			exit(1);
		    }
		    np = agnode(root, n->name);
		    ND_pos(np)[0] = ND_pos(n)[0];
		    ND_pos(np)[1] = ND_pos(n)[1];
		    ND_coord_i(np).x = ND_coord_i(n).x;
		    ND_coord_i(np).y = ND_coord_i(n).y;
		}
		for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
		    tp = agfindnode(root, n->name);
		    for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
			hp = agfindnode(root, e->head->name);
			ep = agedge(root, tp, hp);
			ED_spl(ep) = ED_spl(e);
		    }
		}
	    }
	    graph_init(root);
	    ptest_initGraph(root);
	    ccs = ccomps(root, &cnt, 0);
	    packGraphs(cnt, ccs, root, margin, doEdges);
	    if (!doEdges)
		copyPos(root);
	    else
		State = GVSPLINES;
	    attach_attrs(root);
	    for (i = 0; i < cnt; i++) {
		agdelete(root, ccs[i]);
	    }
	    agwrite(root, stdout);
	} else {
	    packGraphs(cnt, gs, 0, margin, doEdges);
	    if (doEdges)
		State = GVSPLINES;
	    for (i = 0; i < cnt; i++) {
		if (!doEdges)
		    copyPos(gs[i]);
		attach_attrs(gs[i]);
		agwrite(gs[i], stdout);
	    }
	}
    }
}
