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

#ifndef GRID_H
#define GRID_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <render.h>
#include <cdt.h>

    typedef struct _grid Grid;

    typedef struct _node_list {
	Agnode_t *node;
	struct _node_list *next;
    } node_list;

    typedef struct {
	int i, j;
    } gridpt;

    typedef struct {
	gridpt p;		/* index of cell */
	node_list *nodes;	/* nodes in cell */
	Dtlink_t link;		/* cdt data */
    } cell;

    extern Grid *mkGrid(int);
    extern void adjustGrid(Grid * g, int nnodes);
    extern void clearGrid(Grid *);
    extern void addGrid(Grid *, int, int, Agnode_t *);
    extern void walkGrid(Grid *, int (*)(Dt_t *, cell *, Grid *));
    extern cell *findGrid(Grid *, int, int);
    extern void delGrid(Grid *);
    extern int gLength(cell * p);

#endif

#ifdef __cplusplus
}
#endif
