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



#ifndef HEDGES_H
#define HEDGES_H

#include "site.h"
#include "edges.h"

    typedef struct Halfedge {
	struct Halfedge *ELleft, *ELright;
	Edge *ELedge;
	int ELrefcnt;
	char ELpm;
	Site *vertex;
	double ystar;
	struct Halfedge *PQnext;
    } Halfedge;

    extern Halfedge *ELleftend, *ELrightend;

    extern void ELinitialize(void);
    extern void ELcleanup(void);
    extern int right_of(Halfedge *, Point *);
    extern Site *hintersect(Halfedge *, Halfedge *);
    extern Halfedge *HEcreate(Edge *, char);
    extern void ELinsert(Halfedge *, Halfedge *);
    extern Halfedge *ELleftbnd(Point *);
    extern void ELdelete(Halfedge *);
    extern Halfedge *ELleft(Halfedge *), *ELright(Halfedge *);
    extern Halfedge *ELleftbnd(Point *);
    extern Site *leftreg(Halfedge *), *rightreg(Halfedge *);

#endif

#ifdef __cplusplus
}
#endif
