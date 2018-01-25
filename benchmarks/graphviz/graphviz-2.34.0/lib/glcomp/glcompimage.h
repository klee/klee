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
#ifndef GLCOMPIMAGE_H
#define GLCOMPIMAGE_H

#include "glcompdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern glCompImage *glCompImageNewFile(glCompObj * par, GLfloat x,
				       GLfloat y, char* imgfile, int is2D);
    extern glCompImage *glCompImageNew(glCompObj * par, GLfloat x,
				       GLfloat y);
    extern void glCompImageDelete(glCompImage * p);
    extern int glCompImageLoad(glCompImage * i, unsigned char *data,
			       int width, int height,int is2D);
    extern int glCompImageLoadPng(glCompImage * i, char *pngFile,int is2D);
    extern void glCompImageDraw(void *obj);
    extern void glCompImageClick(glCompObj * o, GLfloat x, GLfloat y,
				 glMouseButtonType t);
    extern void glCompImageDoubleClick(glCompObj * obj, GLfloat x,
				       GLfloat y, glMouseButtonType t);
    extern void glCompImageMouseDown(glCompObj * obj, GLfloat x, GLfloat y,
				     glMouseButtonType t);
    extern void glCompImageMouseIn(glCompObj * obj, GLfloat x, GLfloat y);
    extern void glCompImageMouseOut(glCompObj * obj, GLfloat x, GLfloat y);
    extern void glCompImageMouseOver(glCompObj * obj, GLfloat x,
				     GLfloat y);
    extern void glCompImageMouseUp(glCompObj * obj, GLfloat x, GLfloat y,
				   glMouseButtonType t);

#ifdef __cplusplus
}
#endif
#endif
