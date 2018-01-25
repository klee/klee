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



#ifndef SITE_H
#define SITE_H

#include "geometry.h"

    /* Sites are also used as vertices on line segments */
    typedef struct Site {
	Point coord;
	int sitenbr;
	int refcnt;
    } Site;

    extern int siteidx;
    extern Site *bottomsite;

    extern void siteinit(void);
    extern Site *getsite(void);
    extern double dist(Site *, Site *);	/* Distance between two sites */
    extern void deref(Site *);	/* Increment refcnt of site  */
    extern void ref(Site *);	/* Decrement refcnt of site  */
    extern void makevertex(Site *);	/* Transform a site into a vertex */
#endif

#ifdef __cplusplus
}
#endif
