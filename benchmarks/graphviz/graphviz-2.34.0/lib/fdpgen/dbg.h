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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FDPDBG_H
#define FDPDBG_H

#ifdef DEBUG

#include <fdp.h>
#include <stdio.h>
#include <cgraph.h>

    extern double Scale;
    extern void outputGraph(Agraph_t *, FILE *, int);

    extern void incInd(void);
    extern void decInd(void);
    extern void prIndent(void);

    extern void dump(graph_t * g, int doAll, int doBB);
    extern void dumpE(graph_t * g, int derived);
    extern void dumpG(graph_t * g, char *fname, int);

#endif

#endif

#ifdef __cplusplus
}
#endif
