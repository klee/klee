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

/*Open GL basic component set
  includes glPanel,glCompButton,glCompCustomButton,clCompLabel,glCompStyle
*/
#ifndef GLCOMPSET_H
#define GLCOMPSET_H

#include "glcompfont.h"
#include "glcomptextpng.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern void glCompInitCommon(glCompObj * childObj,
				 glCompObj * parentObj, GLfloat x,
				 GLfloat y);
    void glCompEmptyCommon(glCompCommon * c);
    extern glCompSet *glCompSetNew(int w, int h);
    extern void glCompSetClear(glCompSet * s);
    extern int glCompSetDraw(glCompSet * s);
    extern int glCompSetHide(glCompSet * s);
    extern int glCompSetShow(glCompSet * s);
    extern int glCompSetClick(glCompSet * s, int x, int y);
    extern int glCompSetRelease(glCompSet * s, int x, int y);
    extern void glcompsetUpdateBorder(glCompSet * s, int w, int h);
    extern int glcompsetNextGroupId(glCompSet * s);
    extern int glcompsetGetGroupId(glCompSet * s);
    extern void glCompDrawBegin(void);
    extern void glCompDrawEnd(void);
    extern void glCompSetAddObj(glCompSet * s, glCompObj * obj);
    glCompObj *glCompGetObjByMouse(glCompSet * s, glCompMouse * m,
				   int onlyClickable);
    extern void glCompGetObjectType(glCompObj * p);
/*
	change all components's fonts  in s 
	to sourcefont
*/
/* void change_fonts(glCompSet * s,const texFont_t* sourcefont); */

#ifdef __cplusplus
}
#endif
#endif
