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

#ifndef TOPVIEWFUNCS_H
#define TOPVIEWFUNCS_H

#include "smyrnadefs.h"

#ifdef __cplusplus
extern "C" {
#endif
extern void pick_object_xyz(Agraph_t* g,topview* t,GLfloat x,GLfloat y,GLfloat z) ;
extern void initSmGraph(Agraph_t * g,topview* rv);
extern void updateSmGraph(Agraph_t * g,topview* t);
extern void renderSmGraph(Agraph_t * g,topview* t);
extern void freeSmGraph(Agraph_t * g,topview* t);
extern void cacheSelectedEdges(Agraph_t * g,topview* t);
extern void cacheSelectedNodes(Agraph_t * g,topview* t);
extern void renderSmGraph(Agraph_t * g,topview* t);
extern void updateSmGraph(Agraph_t * g,topview* t);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
