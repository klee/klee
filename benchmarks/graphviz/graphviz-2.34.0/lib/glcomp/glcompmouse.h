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
#ifndef GLCOMPMOUSE_H
#define GLCOMPMOUSE_H

#include "glcompdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*events*/
    extern void glCompMouseInit(glCompMouse * m);
    extern void glCompClick(glCompObj * o, GLfloat x, GLfloat y,
			    glMouseButtonType t);
    extern void glCompDoubleClick(glCompObj * obj, GLfloat x, GLfloat y,
				  glMouseButtonType t);
    extern void glCompMouseDown(glCompObj * obj, GLfloat x, GLfloat y,
				glMouseButtonType t);
    extern void glCompMouseIn(glCompObj * obj, GLfloat x, GLfloat y);
    extern void glCompMouseOut(glCompObj * obj, GLfloat x, GLfloat y);
    extern void glCompMouseOver(glCompObj * obj, GLfloat x, GLfloat y);
    extern void glCompMouseUp(glCompObj * obj, GLfloat x, GLfloat y,
			      glMouseButtonType t);
    extern void glCompMouseDrag(glCompObj * obj, GLfloat dx, GLfloat dy,
				glMouseButtonType t);

#ifdef __cplusplus
}
#endif
#endif
