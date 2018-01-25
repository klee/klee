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
 * Written by Stephen North and Eleftherios Koutsofios.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gvc.h"
#include "gvio.h"

#ifdef WIN32_DLL
__declspec(dllimport) boolean MemTest;
__declspec(dllimport) int GvExitOnUsage;
/*gvc.lib cgraph.lib*/
#ifdef WITH_CGRAPH
    #pragma comment( lib, "cgraph.lib" )
#else
    #pragma comment( lib, "graph.lib" )
#endif
    #pragma comment( lib, "gvc.lib" )
#else   /* not WIN32_DLL */
#include "globals.h"
#endif

#include <stdlib.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(HAVE_FENV_H) && defined(HAVE_FEENABLEEXCEPT)
/* _GNU_SOURCE is needed for feenableexcept to be defined in fenv.h on GNU
 * systems.   Presumably it will do no harm on other systems. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
# include <fenv.h>
#elif HAVE_FPU_CONTROL_H
# include <fpu_control.h>
#elif HAVE_SYS_FPU_H
# include <sys/fpu.h>
#endif

static GVC_t *Gvc;
static graph_t * G;

#ifndef WIN32
static void intr(int s)
{
/* if interrupted we try to produce a partial rendering before exiting */
    if (G)
	gvRenderJobs(Gvc, G);
/* Note that we don't call gvFinalize() so that we don't start event-driven
 * devices like -Tgtk or -Txlib */
    exit (gvFreeContext(Gvc));
}

#ifndef NO_FPERR
static void fperr(int s)
{
    fprintf(stderr, "caught SIGFPE %d\n", s);
    /* signal (s, SIG_DFL); raise (s); */
    exit(1);
}

static void fpinit(void)
{
#if defined(HAVE_FENV_H) && defined(HAVE_FEENABLEEXCEPT)
    int exc = 0;
# ifdef FE_DIVBYZERO
    exc |= FE_DIVBYZERO;
# endif
# ifdef FE_OVERFLOW
    exc |= FE_OVERFLOW;
# endif
# ifdef FE_INVALID
    exc |= FE_INVALID;
# endif
    feenableexcept(exc);

#ifdef HAVE_FESETENV
#ifdef FE_NONIEEE_ENV
    fesetenv (FE_NONIEEE_ENV);
#endif
#endif

#elif  HAVE_FPU_CONTROL_H
    /* On s390-ibm-linux, the header exists, but the definitions
     * of the masks do not.  I assume this is temporary, but until
     * there's a real implementation, it's probably safest to not
     * adjust the FPU on this platform.
     */
# if defined(_FPU_MASK_IM) && defined(_FPU_MASK_DM) && defined(_FPU_MASK_ZM) && defined(_FPU_GETCW)
    fpu_control_t fpe_flags = 0;
    _FPU_GETCW(fpe_flags);
    fpe_flags &= ~_FPU_MASK_IM;	/* invalid operation */
    fpe_flags &= ~_FPU_MASK_DM;	/* denormalized operand */
    fpe_flags &= ~_FPU_MASK_ZM;	/* zero-divide */
    /*fpe_flags &= ~_FPU_MASK_OM;        overflow */
    /*fpe_flags &= ~_FPU_MASK_UM;        underflow */
    /*fpe_flags &= ~_FPU_MASK_PM;        precision (inexact result) */
    _FPU_SETCW(fpe_flags);
# endif
#endif
}
#endif
#endif

static graph_t *create_test_graph(void)
{
#define NUMNODES 5

    Agnode_t *node[NUMNODES];
    Agedge_t *e;
    Agraph_t *g;
    Agraph_t *sg;
    int j, k;
    char name[10];

    /* Create a new graph */
#ifndef WITH_CGRAPH
    aginit();
    agsetiodisc(NULL, gvfwrite, gvferror);
    g = agopen("new_graph", AGDIGRAPH);
#else /* WITH_CGRAPH */
    g = agopen("new_graph", Agdirected,NIL(Agdisc_t *));
#endif /* WITH_CGRAPH */

    /* Add nodes */
    for (j = 0; j < NUMNODES; j++) {
	sprintf(name, "%d", j);
#ifndef WITH_CGRAPH
	node[j] = agnode(g, name);
#else /* WITH_CGRAPH */
	node[j] = agnode(g, name, 1);
	agbindrec(node[j], "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
    }

    /* Connect nodes */
    for (j = 0; j < NUMNODES; j++) {
	for (k = j + 1; k < NUMNODES; k++) {
#ifndef WITH_CGRAPH
	    agedge(g, node[j], node[k]);
#else /* WITH_CGRAPH */
	    e = agedge(g, node[j], node[k], NULL, 1);
	    agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);	//edge custom data
#endif /* WITH_CGRAPH */
	}
    }

#ifndef WITH_CGRAPH
    sg = agsubg (g, "cluster1");
    aginsert (sg, node[0]);
#else /* WITH_CGRAPH */
    sg = agsubg (g, "cluster1", 1);
    agsubnode (sg, node[0], 1);
#endif /* WITH_CGRAPH */

    return g;
}

int main(int argc, char **argv)
{
    graph_t *prev = NULL;
    int r, rc = 0;

    Gvc = gvContextPlugins(lt_preloaded_symbols, DEMAND_LOADING);
    GvExitOnUsage = 1;
    gvParseArgs(Gvc, argc, argv);
#ifndef WIN32
    signal(SIGUSR1, gvToggle);
    signal(SIGINT, intr);
#ifndef NO_FPERR
    fpinit();
    signal(SIGFPE, fperr);
#endif
#endif

    if (MemTest) {
	while (MemTest--) {
	    /* Create a test graph */
	    G = create_test_graph();

	    /* Perform layout and cleanup */
	    gvLayoutJobs(Gvc, G);  /* take layout engine from command line */
	    gvFreeLayout(Gvc, G);
	    agclose (G);
	}
    }
    else if ((G = gvPluginsGraph(Gvc))) {
	    gvLayoutJobs(Gvc, G);  /* take layout engine from command line */
	    gvRenderJobs(Gvc, G);
    }
    else {
	while ((G = gvNextInputGraph(Gvc))) {
	    if (prev) {
		gvFreeLayout(Gvc, prev);
		agclose(prev);
	    }
	    gvLayoutJobs(Gvc, G);  /* take layout engine from command line */
	    gvRenderJobs(Gvc, G);
	    r = agreseterrors();
	    rc = MAX(rc,r);
	    prev = G;
	}
    }
    gvFinalize(Gvc);
    
    r = gvFreeContext(Gvc);
    return (MAX(rc,r));
}
