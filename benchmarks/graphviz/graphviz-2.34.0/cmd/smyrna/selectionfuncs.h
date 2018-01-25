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

#ifndef SELECTIONFUNCS_H
#define SELECTIONFUNCS_H

#include "draw.h"
#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void pick_objects_rect(Agraph_t* g) ;
extern void deselect_all(Agraph_t* g);
extern void add_selpoly(Agraph_t* g,glCompPoly* selPoly,glCompPoint pt);
extern void clear_selpoly(glCompPoly* sp);
#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
