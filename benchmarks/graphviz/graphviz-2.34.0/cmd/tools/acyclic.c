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




#ifdef WIN32 /*dependencies*/
    #pragma comment( lib, "cgraph.lib" )
    #pragma comment( lib, "ingraphs.lib" )
#endif



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>

#include <stdlib.h>
#include "cgraph.h"
typedef struct {
    Agrec_t h;
    int mark;
    int onstack;
} Agnodeinfo_t;

#define ND_mark(n) (((Agnodeinfo_t*)((n)->base.data))->mark)
#define ND_onstack(n) (((Agnodeinfo_t*)((n)->base.data))->onstack)
#define graphName(g) (agnameof(g))

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "compat_getopt.h"
#endif

static FILE *inFile;
static FILE *outFile;
static int doWrite = 1;
static int Verbose;
static char *cmd;

/* addRevEdge:
 * Add a reversed version of e. The new edge has the same key.
 * We also copy the attributes, reversing the roles of head and 
 * tail ports.
 * This assumes we've already checked that such an edge does not exist.
 */
static void addRevEdge(Agraph_t * g, Agedge_t * e)
{
    Agsym_t* sym;
    Agedge_t* f = agedge (g, aghead(e), agtail(e), agnameof(e), 1);

    agcopyattr (e, f);

    sym = agattr (g, AGEDGE, TAILPORT_ID, 0);
    if (sym) agsafeset (f, HEADPORT_ID, agxget (e, sym), "");
    sym = agattr (g, AGEDGE, HEADPORT_ID, 0);
    if (sym) agsafeset (f, TAILPORT_ID, agxget (e, sym), "");
}

static int dfs(Agraph_t * g, Agnode_t * t, int hasCycle)
{
    Agedge_t *e;
    Agedge_t *f;
    Agnode_t *h;

    ND_mark(t) = 1;
    ND_onstack(t) = 1;
    for (e = agfstout(g, t); e; e = f) {
	f = agnxtout(g, e);
	if (agtail(e) == aghead(e))
	    continue;
	h = aghead(e);
	if (ND_onstack(h)) {
	    if (agisstrict(g)) {
		if (agedge(g, h, t, 0, 0) == 0)
		    addRevEdge(g, e);
	    } else {
		char* key = agnameof (e);
		if (!key || (agedge(g, h, t, key, 0) == 0))
		    addRevEdge(g, e);
	    }
	    agdelete(g, e);
	    hasCycle = 1;
	} else if (ND_mark(h) == 0)
	    hasCycle = dfs(g, h, hasCycle);
    }
    ND_onstack(t) = 0;
    return hasCycle;
}

static char *useString = "Usage: %s [-nv?] [-o outfile] <file>\n\
  -o <file> - put output in <file>\n\
  -n        - do not output graph\n\
  -v        - verbose\n\
  -?        - print usage\n";

static void usage(int v)
{
    fprintf(stderr, useString, cmd);
    exit(v);
}

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
	exit(-1);
    }
    return (fp);
}

static void init(int argc, char *argv[])
{
    int c;

    cmd = argv[0];
    opterr = 0;
    while ((c = getopt(argc, argv, ":vno:")) != -1)
	switch (c) {
	case 'o':
	    outFile = openFile(optarg, "w");
	    break;
	case 'n':
	    doWrite = 0;
	    break;
	case 'v':
	    Verbose = 1;
	    break;
	case '?':
	    if (optopt == '?')
		usage(0);
	    else {
		fprintf(stderr, "%s: option -%c unrecognized\n", cmd,
			optopt);
		usage(-1);
	    }
	    break;
	case ':':
	    fprintf(stderr, "%s: missing argument for option -%c\n",
		    cmd, optopt);
	    usage(-1);
	    break;
	}
    if (optind < argc) {
	inFile = openFile(argv[optind], "r");
    } else
	inFile = stdin;
    if (!outFile)
	outFile = stdout;

}

int main(int argc, char *argv[])
{
    Agraph_t *g;
    Agnode_t *n;
    int rv = 0;

    init(argc, argv);

    if ((g = agread(inFile,  (Agdisc_t *) 0)) != 0) {
	if (agisdirected (g)) {
	    aginit(g, AGNODE, "info", sizeof(Agnodeinfo_t), TRUE);
	    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
		if (ND_mark(n) == 0)
		    rv |= dfs(g, n, 0);
	    }
	    if (doWrite) {
		agwrite(g, outFile);
		fflush(outFile);
	    }
	    if (Verbose) {
		if (rv)
		    fprintf(stderr, "Graph %s has cycles\n", graphName(g));
		else
		    fprintf(stderr, "Graph %s is acyclic\n", graphName(g));
	    }
	} else {
	    rv = 2;
	    if (Verbose)
		fprintf(stderr, "Graph %s is undirected\n", graphName(g));
	}
	exit(rv);
    } else
	exit(-1);
}
