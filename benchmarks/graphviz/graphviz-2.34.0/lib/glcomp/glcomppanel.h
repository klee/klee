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
#ifndef GLCOMPPANEL_H
#define GLCOMPPANEL_H

#include "glcompdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern glCompPanel *glCompPanelNew(glCompObj * parentObj, GLfloat x,
				       GLfloat y, GLfloat w, GLfloat h);
    extern int glCompSetAddPanel(glCompSet * s, glCompPanel * p);
    extern int glCompSetRemovePanel(glCompSet * s, glCompPanel * p);
    extern int glCompPanelShow(glCompPanel * p);
    extern int glCompPanelHide(glCompPanel * p);
    extern void glCompSetPanelText(glCompPanel * p, char *t);

/*events*/
    extern int glCompPanelDraw(glCompObj * o);
    extern void glCompPanelClick(glCompObj * o, GLfloat x, GLfloat y,
				 glMouseButtonType t);
    extern void glCompPanelDoubleClick(glCompObj * obj, GLfloat x,
				       GLfloat y, glMouseButtonType t);
    extern void glCompPanelMouseDown(glCompObj * obj, GLfloat x, GLfloat y,
				     glMouseButtonType t);
    extern void glCompPanelMouseIn(glCompObj * obj, GLfloat x, GLfloat y);
    extern void glCompPanelMouseOut(glCompObj * obj, GLfloat x, GLfloat y);
    extern void glCompPanelMouseOver(glCompObj * obj, GLfloat x,
				     GLfloat y);
    extern void glCompPanelMouseUp(glCompObj * obj, GLfloat x, GLfloat y,
				   glMouseButtonType t);

#ifdef __cplusplus
}
#endif
#endif
