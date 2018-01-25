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
#ifndef GLCOMPBUTTON_H
#define GLCOMPBUTTON_H

#include "glcompdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern glCompButton *glCompButtonNew(glCompObj * par, GLfloat x,
					 GLfloat y, GLfloat w, GLfloat h,
					 char *caption);
    extern int glCompSetRemoveButton(glCompSet * s, glCompButton * p);
    extern void glCompButtonDraw(glCompButton * p);
    extern void glCompButtonSetText(glCompButton * p, char *str);
    extern int glCompButtonAddPngGlyph(glCompButton * b, char *fileName);
    extern void glCompButtonClick(glCompObj * o, GLfloat x, GLfloat y,
				  glMouseButtonType t);
    extern void glCompButtonDoubleClick(glCompObj * o, GLfloat x,
					GLfloat y, glMouseButtonType t);
    extern void glCompButtonMouseDown(glCompObj * o, GLfloat x, GLfloat y,
				      glMouseButtonType t);
    extern void glCompButtonMouseIn(glCompObj * o, GLfloat x, GLfloat y);
    extern void glCompButtonMouseOut(glCompObj * o, GLfloat x, GLfloat y);
    extern void glCompButtonMouseOver(glCompObj * o, GLfloat x, GLfloat y);
    extern void glCompButtonMouseUp(glCompObj * o, GLfloat x, GLfloat y,
				    glMouseButtonType t);
    extern void glCompButtonHide(glCompButton * p);
    extern void glCompButtonShow(glCompButton * p);

#ifdef __cplusplus
}
#endif
#endif
