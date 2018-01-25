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



#ifndef EDGES_H
#define EDGES_H

#include "site.h"

    typedef struct Edge {
	double a, b, c;		/* edge on line ax + by = c */
	Site *ep[2];		/* endpoints (vertices) of edge; initially NULL */
	Site *reg[2];		/* sites forming edge */
	int edgenbr;
    } Edge;

#define le 0
#define re 1

    extern double pxmin, pxmax, pymin, pymax;	/* clipping window */
    extern void edgeinit(void);
    extern void endpoint(Edge *, int, Site *);
    extern void clip_line(Edge * e);
    extern Edge *bisect(Site *, Site *);

#endif

#ifdef __cplusplus
}
#endif
