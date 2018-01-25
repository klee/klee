/* $Id$Revision: */
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

#ifdef WIN32 /*dependencies*/
    #pragma comment( lib, "cgraph.lib" )
    #pragma comment( lib, "ingraphs.lib" )
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "compat_getopt.h"
#endif
#include "graph_generator.h"

typedef enum { unknown, grid, circle, complete, completeb, 
    path, tree, torus, cylinder, mobius, randomg, randomt, ball,
    sierpinski, hypercube, star, wheel, trimesh
} GraphType;

typedef struct {
    int graphSize1;
    int graphSize2;
    int cnt;
    int parm1;
    int parm2;
    int Verbose;
    int isPartial;
    int foldVal;
    int directed;
    FILE *outfile;
    char* pfx;
    char* name;
} opts_t;

static char *cmd;

static FILE *openFile(char *name, char *mode)
{
    FILE *fp;
    char *modestr;

    fp = fopen(name, mode);
    if (!fp) {
	if (*mode == 'r')
	    modestr = "reading";
	else
	    modestr = "writing";
	fprintf(stderr, "%s: could not open file %s for %s\n",
		cmd, name, modestr);
	exit(1);
    }
    return fp;
}

static char *Usage = "Usage: %s [-dv?] [options]\n\
 -c<n>         : cycle \n\
 -C<x,y>       : cylinder \n\
 -g[f]<h,w>    : grid (folded if f is used)\n\
 -G[f]<h,w>    : partial grid (folded if f is used)\n\
 -h<x>         : hypercube \n\
 -k<x>         : complete \n\
 -b<x,y>       : complete bipartite\n\
 -B<x,y>       : ball\n\
 -i<n>         : generate <n> random\n\
 -m<x>         : triangular mesh\n\
 -M<x,y>       : x by y Moebius strip\n\
 -n<prefix>    : use <prefix> in node names (\"\")\n\
 -N<name>      : use <name> for the graph (\"\")\n\
 -o<outfile>   : put output in <outfile> (stdout)\n\
 -p<x>         : path \n\
 -r<x>,<n>     : random graph\n\
 -R<n>         : random rooted tree on <n> vertices\n\
 -s<x>         : star\n\
 -S<x>         : sierpinski\n\
 -t<x>         : binary tree \n\
 -t<x>,<n>     : n-ary tree \n\
 -T<x,y>       : torus \n\
 -T<x,y,t1,t2> : twisted torus \n\
 -w<x>         : wheel\n\
 -d            : directed graph\n\
 -v            : verbose mode\n\
 -?            : print usage\n";

static void usage(int v)
{
    fprintf(v ? stderr : stdout, Usage, cmd);
    exit(v);
}

static void errexit(char opt)
{
    fprintf(stderr, "in flag -%c\n", opt);
    usage(1);
}

/* readPos:
 * Read and return a single int from s, guaranteed to be >= min.
 * A pointer to the next available character from s is stored in e.
 * Return -1 on error.
 */
static int readPos(char *s, char **e, int min)
{
    int d;

    d = strtol(s, e, 10);
    if (s == *e) {
	fprintf(stderr, "ill-formed integer \"%s\" ", s);
	return -1;
    }
    if (d < min) {
	fprintf(stderr, "integer \"%s\" less than %d", s, min);
	return -1;
    }
    return d;
}

/* readOne:
 * Return non-zero on error.
 */
static int readOne(char *s, int* ip)
{
    int d;
    char *next;

    d = readPos(s, &next, 1);
    if (d > 0) {
	*ip = d;
	return 0;
    }
    else return d;
}

/* setOne:
 * Return non-zero on error.
 */
static int setOne(char *s, opts_t* opts)
{
    int d;
    char *next;

    d = readPos(s, &next, 1);
    if (d > 0) {
	opts->graphSize1 = d;
	return 0;
    }
    else return d;
}

/* setTwo:
 * Return non-zero on error.
 */
static int setTwo(char *s, opts_t* opts)
{
    int d;
    char *next;

    d = readPos(s, &next, 1);
    if (d < 0)
	return d;
    opts->graphSize1 = d;

    if (*next != ',') {
	fprintf(stderr, "ill-formed int pair \"%s\" ", s);
	return -1;
    }

    s = next + 1;
    d = readPos(s, &next, 1);
    if (d > 1) {
	opts->graphSize2 = d;
	return 0;
    }
    else return d;
}

/* setTwoTwoOpt:
 * Read 2 numbers
 * Read 2 more optional numbers
 * Return non-zero on error.
 */
static int setTwoTwoOpt(char *s, opts_t* opts, int dflt)
{
    int d;
    char *next;

    d = readPos(s, &next, 1);
    if (d < 0)
	return d;
    opts->graphSize1 = d;

    if (*next != ',') {
	fprintf(stderr, "ill-formed int pair \"%s\" ", s);
	return -1;
    }

    s = next + 1;
    d = readPos(s, &next, 1);
    if (d < 1) {
	return 0;
    }
    opts->graphSize2 = d;

    if (*next != ',') {
	opts->parm1 = opts->parm2 = dflt;
	return 0;
    }

    s = next + 1;
    d = readPos(s, &next, 1);
    if (d < 0)
	return d;
    opts->parm1 = d;

    if (*next != ',') {
	opts->parm2 = dflt;
	return 0;
    }

    s = next + 1;
    d = readPos(s, &next, 1);
    if (d < 0) {
	return d;
    }
    opts->parm2 = d;
    return 0;
}

/* setTwoOpt:
 * Return non-zero on error.
 */
static int setTwoOpt(char *s, opts_t* opts, int dflt)
{
    int d;
    char *next;

    d = readPos(s, &next, 1);
    if (d < 0)
	return d;
    opts->graphSize1 = d;

    if (*next != ',') {
	opts->graphSize2 = dflt;
	return 0;
    }

    s = next + 1;
    d = readPos(s, &next, 1);
    if (d > 1) {
	opts->graphSize2 = d;
	return 0;
    }
    else return d;
}

static char* setFold(char *s, opts_t* opts)
{
    char *next;

    if (*s == 'f') {
	next = s+1;
	opts->foldVal = 1;
    }
    else
	next = s;

    return next;
}

static char *optList = ":i:M:m:n:N:c:C:dg:G:h:k:b:B:o:p:r:R:s:S:t:T:vw:";

static GraphType init(int argc, char *argv[], opts_t* opts)
{
    int c;
    GraphType graphType = unknown;

    cmd = argv[0];
    opterr = 0;
    while ((c = getopt(argc, argv, optList)) != -1) {
	switch (c) {
	case 'c':
	    graphType = circle;
	    if (setOne(optarg, opts))
		errexit(c);
	    break;
	case 'C':
	    graphType = cylinder;
	    if (setTwo(optarg, opts))
		errexit(c);
	    break;
	case 'M':
	    graphType = mobius;
	    if (setTwo(optarg, opts))
		errexit(c);
	    break;
	case 'd':
	    opts->directed = 1;
	    break;
	case 'G':
	    opts->isPartial = 1;
	case 'g':
	    graphType = grid;
	    optarg = setFold (optarg, opts);
	    if (setTwo(optarg, opts))
		errexit(c);
	    break;
	case 'h':
	    graphType = hypercube;
	    if (setOne(optarg, opts))
		errexit(c);
	    break;
	case 'k':
	    graphType = complete;
	    if (setOne(optarg, opts))
		errexit(c);
	    break;
	case 'b':
	    graphType = completeb;
	    if (setTwo(optarg, opts))
		errexit(c);
	    break;
	case 'B':
	    graphType = ball;
	    if (setTwo(optarg, opts))
		errexit(c);
	    break;
	case 'm':
	    graphType = trimesh;
	    if (setOne(optarg, opts))
		errexit(c);
	    break;
	case 'r':
	    graphType = randomg;
	    if (setTwo(optarg, opts))
		errexit(c);
	    break;
	case 'R':
	    graphType = randomt;
	    if (setOne(optarg, opts))
		errexit(c);
	    break;
	case 'n':
	    opts->pfx = optarg;
	    break;
	case 'N':
	    opts->name = optarg;
	    break;
	case 'o':
	    opts->outfile = openFile(optarg, "w");
	    break;
	case 'p':
	    graphType = path;
	    if (setOne(optarg, opts))
		errexit(c);
	    break;
	case 'S':
	    graphType = sierpinski;
	    if (setOne(optarg, opts))
		errexit(c);
	    break;
	case 's':
	    graphType = star;
	    if (setOne(optarg, opts))
		errexit(c);
	    break;
	case 't':
	    graphType = tree;
	    if (setTwoOpt(optarg, opts, 2))
		errexit(c);
	    break;
	case 'T':
	    graphType = torus;
	    if (setTwoTwoOpt(optarg, opts, 0))
		errexit(c);
	    break;
	case 'i':
	    if (readOne(optarg,&(opts->cnt)))
		errexit(c);
	    break;
	case 'v':
	    opts->Verbose = 1;
	    break;
	case 'w':
	    graphType = wheel;
	    if (setOne(optarg, opts))
		errexit(c);
	    break;
	case '?':
	    if (optopt == '?')
		usage(0);
	    else
		fprintf(stderr, "Unrecognized flag \"-%c\" - ignored\n",
			optopt);
	    break;
	}
    }

    argc -= optind;
    argv += optind;
    if (!opts->outfile)
	opts->outfile = stdout;
    if (graphType == unknown) {
	fprintf(stderr, "Graph type not set\n");
	usage(1);
    }

    return graphType;
}

static opts_t opts;

static void dirfn (int t, int h)
{
    if (h > 0)
	fprintf (opts.outfile, "  %s%d -> %s%d\n", opts.pfx, t, opts.pfx, h);
    else
	fprintf (opts.outfile, "  %s%d\n", opts.pfx, t);
}

static void undirfn (int t, int h)
{
    if (h > 0)
	fprintf (opts.outfile, "  %s%d -- %s%d\n", opts.pfx, t, opts.pfx, h);
    else
	fprintf (opts.outfile, "  %s%d\n", opts.pfx, t);
}

static void
closeOpen ()
{
    if (opts.directed)
	fprintf(opts.outfile, "}\ndigraph {\n");
    else
	fprintf(opts.outfile, "}\ngraph {\n");
}

int main(int argc, char *argv[])
{
    GraphType graphType;
    edgefn ef;

    opts.pfx = "";
    opts.name = "";
    opts.cnt = 1;
    graphType = init(argc, argv, &opts);
    if (opts.directed) {
	fprintf(opts.outfile, "digraph %s{\n", opts.name);
	ef = dirfn;
    }
    else {
	fprintf(opts.outfile, "graph %s{\n", opts.name);
	ef = undirfn;
    }

    switch (graphType) {
    case grid:
	makeSquareGrid(opts.graphSize1, opts.graphSize2,
		       opts.foldVal, opts.isPartial, ef);
	break;
    case circle:
	makeCircle(opts.graphSize1, ef);
	break;
    case path:
	makePath(opts.graphSize1, ef);
	break;
    case tree:
	if (opts.graphSize2 == 2)
	    makeBinaryTree(opts.graphSize1, ef);
	else
	    makeTree(opts.graphSize1, opts.graphSize2, ef);
	break;
    case trimesh:
	makeTriMesh(opts.graphSize1, ef);
	break;
    case ball:
	makeBall(opts.graphSize1, opts.graphSize2, ef);
	break;
    case torus:
	if ((opts.parm1 == 0) && (opts.parm2 == 0))
	    makeTorus(opts.graphSize1, opts.graphSize2, ef);
	else
	    makeTwistedTorus(opts.graphSize1, opts.graphSize2, opts.parm1, opts.parm2, ef);
	break;
    case cylinder:
	makeCylinder(opts.graphSize1, opts.graphSize2, ef);
	break;
    case mobius:
	makeMobius(opts.graphSize1, opts.graphSize2, ef);
	break;
    case sierpinski:
	makeSierpinski(opts.graphSize1, ef);
	break;
    case complete:
	makeComplete(opts.graphSize1, ef);
	break;
    case randomg:
	makeRandom (opts.graphSize1, opts.graphSize2, ef);
	break;
    case randomt:
	{
	    int i;
	    treegen_t* tg = makeTreeGen (opts.graphSize1);
	    for (i = 1; i <= opts.cnt; i++) {
		makeRandomTree (tg, ef);
		if (i != opts.cnt) closeOpen ();
	    }
	    freeTreeGen (tg);
	}
	makeRandom (opts.graphSize1, opts.graphSize2, ef);
	break;
    case completeb:
	makeCompleteB(opts.graphSize1, opts.graphSize2, ef);
	break;
    case hypercube:
	makeHypercube(opts.graphSize1, ef);
	break;
    case star:
	makeStar(opts.graphSize1, ef);
	break;
    case wheel:
	makeWheel(opts.graphSize1, ef);
	break;
    default:
	/* can't happen */
	break;
    }
    fprintf(opts.outfile, "}\n");

    exit(0);
}
