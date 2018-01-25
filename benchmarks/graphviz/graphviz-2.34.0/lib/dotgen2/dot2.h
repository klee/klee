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


#ifndef         DOT2_H
#define         DOT2_H

#include        "render.h"
#include        "dot2procs.h"

/* tags for representative (model) objects */
/* 0 is reserved for undefined */
#define STDNODE         1       /* a real node on only one rank */
#define TALLNODE        2       /* a real node on two or more ranks */
#define EXTNODE         3       /* a node external to a cluster */
#define SKELETON        4
#define PATH            5
#define INTNODE         6       /*internal temporary nodes to fill rank gaps*/
#define LBLNODE         7       /*internal temp nodes for edge labels*/

#endif                          /* DOT_H */
