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

#include "glcomppanel.h"
#include "glcompfont.h"
#include "glcompset.h"
#include "glcomptexture.h"
#include "glutils.h"
#include "memory.h"

glCompPanel *glCompPanelNew(glCompObj * parentObj, GLfloat x, GLfloat y,
			    GLfloat w, GLfloat h)
{
    glCompPanel *p;
//      glCompCommon* parent=&parentObj->common;
    p = NEW(glCompPanel);
    glCompInitCommon((glCompObj *) p, parentObj, (GLfloat) x, (GLfloat) y);

    p->shadowcolor.R = GLCOMPSET_PANEL_SHADOW_COLOR_R;
    p->shadowcolor.G = GLCOMPSET_PANEL_SHADOW_COLOR_G;
    p->shadowcolor.B = GLCOMPSET_PANEL_SHADOW_COLOR_B;
    p->shadowcolor.A = GLCOMPSET_PANEL_SHADOW_COLOR_A;
    p->shadowwidth = GLCOMPSET_PANEL_SHADOW_WIDTH;
    p->common.borderWidth = GLCOMPSET_PANEL_BORDERWIDTH;


    p->common.width = w;
    p->common.height = h;

    p->common.font = glNewFontFromParent((glCompObj *) p, NULL);
    p->text = (char *) 0;
    p->common.functions.draw = (glcompdrawfunc_t)glCompPanelDraw;
    p->image = (glCompImage *) 0;
    return p;
}
void glCompSetPanelText(glCompPanel * p, char *t)
{
    replacestr(t, &p->text);
    glCompDeleteTexture(p->common.font->tex);
    p->common.font->tex =
	glCompSetAddNewTexLabel(p->common.compset,
				p->common.font->fontdesc,
				p->common.font->size, p->text, 1);
}

int glCompPanelDraw(glCompObj * o)
{
    glCompPanel *p;
    glCompCommon ref;
    glCompRect r;
    p = (glCompPanel *) o;
    ref = p->common;
    glCompCalcWidget((glCompCommon *) p->common.parent, &p->common, &ref);
    p->objType = glPanelObj;
    //typedef enum {glPanelObj,glbuttonObj,glLabelObj,glImageObj}glObjType;


    if (!p->common.visible)
	return 0;
    /*draw shadow */
    glColor4f((GLfloat) p->shadowcolor.R, (GLfloat) p->shadowcolor.G,
	      (GLfloat) p->shadowcolor.B, (GLfloat) p->shadowcolor.A);
    r.h = p->shadowwidth;
    r.w = ref.width;
    r.pos.x = ref.pos.x + p->shadowwidth;
    r.pos.y = ref.pos.y - p->shadowwidth;
    r.pos.z = -0.001;
    glCompDrawRectangle(&r);
    r.h = ref.height;
    r.w = p->shadowwidth;
    r.pos.x = ref.pos.x + ref.width;
    r.pos.y = ref.pos.y - p->shadowwidth;
    r.pos.z = -0.001;
    glCompDrawRectangle(&r);
    /*draw panel */
    glCompDrawRectPrism(&(ref.pos), ref.width, ref.height,
			p->common.borderWidth, 0.01, &(ref.color), 1);
    /*draw image if there is */
    if (p->image) {
	p->image->common.callbacks.draw((void *) p->image);
    }
    if (p->text) {


    }
    return 1;
}


int glCompPanelHide(glCompPanel * p)
{
    /* int ind = 0; */
    p->common.visible = 0;
    return 1;


}

int glCompPanelShow(glCompPanel * p)
{
    /* int ind = 0; */
    p->common.visible = 1;
    return 1;

}

void glCompPanelClick(glCompObj * o, GLfloat x, GLfloat y,
		      glMouseButtonType t)
{
    if (o->common.callbacks.click)
	o->common.callbacks.click(o, x, y, t);
}

void glCompPanelDoubleClick(glCompObj * obj, GLfloat x, GLfloat y,
			    glMouseButtonType t)
{
    /*Put your internal code here */
    if (((glCompPanel *) obj)->common.callbacks.doubleclick)
	((glCompPanel *) obj)->common.callbacks.doubleclick(obj, x, y, t);
}

void glCompPanelMouseDown(glCompObj * obj, GLfloat x, GLfloat y,
			  glMouseButtonType t)
{
    /*Put your internal code here */
    if (((glCompPanel *) obj)->common.callbacks.mousedown)
	((glCompPanel *) obj)->common.callbacks.mousedown(obj, x, y, t);
}

void glCompPanelMouseIn(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompPanel *) obj)->common.callbacks.mousein)
	((glCompPanel *) obj)->common.callbacks.mousein(obj, x, y);
}

void glCompPanelMouseOut(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompPanel *) obj)->common.callbacks.mouseout)
	((glCompPanel *) obj)->common.callbacks.mouseout(obj, x, y);
}

void glCompPanelMouseOver(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompPanel *) obj)->common.callbacks.mouseover)
	((glCompPanel *) obj)->common.callbacks.mouseover(obj, x, y);
}

void glCompPanelMouseUp(glCompObj * obj, GLfloat x, GLfloat y,
			glMouseButtonType t)
{
    /*Put your internal code here */
    if (((glCompPanel *) obj)->common.callbacks.mouseup)
	((glCompPanel *) obj)->common.callbacks.mouseup(obj, x, y, t);
}


#if 0

void glCompPanelSetText(glCompPanel * p, char *str)
{
//    replacestr(str, &p->text);
}
#endif
