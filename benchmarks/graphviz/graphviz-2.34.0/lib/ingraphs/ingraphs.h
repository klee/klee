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

#ifndef INGRAPHS_H
#define INGRAPHS_H

/* The ingraphs library works with both libagraph and with 
 * libgraph, with all user-supplied data. For this to work,
 * the include file relies upon its context to supply a
 * definition of Agraph_t.
 */

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef Agraph_t *(*opengfn) (FILE *);

    typedef struct {
	void *(*openf) (char *);
	Agraph_t *(*readf) (void *);
	int (*closef) (void *);
	void *dflt;
    } ingdisc;

    typedef struct {
	union {
	    char**     Files;
	    Agraph_t** Graphs;
	} u;
	int ctr;
	int ingraphs;
	void *fp;
	ingdisc *fns;
	char heap;
	int errors;
    } ingraph_state;

    extern ingraph_state *newIngraph(ingraph_state *, char **, opengfn);
    extern ingraph_state *newIng(ingraph_state *, char **, ingdisc *);
    extern ingraph_state *newIngGraphs(ingraph_state *, Agraph_t**, ingdisc *);
    extern void closeIngraph(ingraph_state * sp);
    extern Agraph_t *nextGraph(ingraph_state *);
    extern char *fileName(ingraph_state *);

#ifdef __cplusplus
}
#endif
#endif
