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

#include "glcompbutton.h"
#include "glcomplabel.h"
#include "glcompimage.h"
#include "glcompfont.h"
#include "glutils.h"
#include "glcompset.h"
#include "memory.h"
#include <string.h>
#include <GL/glut.h>


glCompButton *glCompButtonNew(glCompObj * par, GLfloat x, GLfloat y,
			      GLfloat w, GLfloat h, char *caption)
{
    glCompButton *p;
//      glCompCommon* parent=&par->common;
    p = NEW(glCompButton);
    glCompInitCommon((glCompObj *) p, par, x, y);
    p->objType = glButtonObj;
    /*customize button color */
    p->common.color.R = GLCOMPSET_BUTTON_COLOR_R;
    p->common.color.G = GLCOMPSET_BUTTON_COLOR_G;
    p->common.color.B = GLCOMPSET_BUTTON_COLOR_B;
    p->common.color.A = GLCOMPSET_BUTTON_COLOR_ALPHA;

    p->common.borderType = glBorderSolid;

    p->common.borderWidth = GLCOMPSET_BUTTON_BEVEL;

    p->common.width = w;
    p->common.height = h;
    p->status = 0;		//0 not pressed 1 pressed;
    p->groupid = 0;
    p->common.callbacks.click = '\0';
    p->customptr = '\0';
    /*set event functions */

    p->common.functions.draw = (glcompdrawfunc_t)glCompButtonDraw;

    p->common.functions.click = glCompButtonClick;
    p->common.functions.doubleclick = glCompButtonDoubleClick;
    p->common.functions.mousedown = glCompButtonMouseDown;
    p->common.functions.mousein = glCompButtonMouseIn;
    p->common.functions.mouseout = glCompButtonMouseOut;
    p->common.functions.mouseover = glCompButtonMouseOver;
    p->common.functions.mouseup = glCompButtonMouseUp;

    /*caption */
    p->common.font = glNewFontFromParent ((glCompObj *) p, NULL);
    p->label = glCompLabelNew((glCompObj *) p, 0, 0, caption);
    p->label->common.font->justify.VJustify = glFontVJustifyCenter;
    p->label->common.font->justify.HJustify = glFontHJustifyCenter;
    p->label->common.align = glAlignParent;
    /*image */
    p->image = (glCompImage *) 0;
    p->glyphPos = glButtonGlyphLeft;
    return p;
}

int glCompButtonAddPngGlyph(glCompButton * b, char *fileName)
{
    int rv;
    /*delete if there is an existing image */
    if (b->image)
	glCompImageDelete(b->image);
    /*image on left for now */
    b->image = glCompImageNew((glCompObj *) b, 0, 0);

    rv = glCompImageLoadPng(b->image, fileName,1);
    if (rv) {
	b->image->common.anchor.leftAnchor = 1;
	b->image->common.anchor.left = 0;

	b->image->common.anchor.topAnchor = 1;
	b->image->common.anchor.top = 0;

	b->image->common.anchor.bottomAnchor = 1;
	b->image->common.anchor.bottom = 0;

	b->label->common.anchor.leftAnchor = 1;
	b->label->common.anchor.left = b->image->common.width;
	b->label->common.anchor.rightAnchor = 1;
	b->label->common.anchor.right = 0;

	b->label->common.anchor.topAnchor = 1;
	b->label->common.anchor.top = 0;

	b->label->common.anchor.bottomAnchor = 1;
	b->label->common.anchor.bottom = 0;

	b->label->common.align = glAlignNone;
    }
    return rv;
}

void glCompButtonHide(glCompButton * p)
{
    p->common.visible = 0;
    if (p->label)
	p->label->common.visible = 0;
    if (p->image)
	p->image->common.visible = 0;
}

void glCompButtonShow(glCompButton * p)
{
    p->common.visible = 1;
    if (p->label)
	p->label->common.visible = 1;
    if (p->image)
	p->image->common.visible = 1;
}

void glCompButtonDraw(glCompButton * p)
{

    glCompCommon ref;
    ref = p->common;
    glCompCalcWidget((glCompCommon *) p->common.parent, &p->common, &ref);
    if (!p->common.visible)
	return;
    /*draw panel */
    glCompDrawRectPrism(&(ref.pos), ref.width, ref.height,
			p->common.borderWidth, 0.01, &(ref.color),
			!p->status);
    if (p->label)
	p->label->common.functions.draw((void *) p->label);
    if (p->image)
	p->image->common.functions.draw((void *) p->image);
    if (p->common.callbacks.draw)
	p->common.callbacks.draw((void *) p);	/*user defined drawing routines are called here. */
}

void glCompButtonClick(glCompObj * o, GLfloat x, GLfloat y,
		       glMouseButtonType t)
{
    glCompButton *p = (glCompButton *) o;
    glCompObj *obj;
    glCompSet *s = o->common.compset;
    int ind = 0;
    ((glCompButton *) o)->status=((glCompButton *) o)->refStatus ;
    if (p->groupid > 0) 
    {
	for (; ind < s->objcnt; ind++) {
	    obj = s->obj[ind];
	    if ((obj->objType == glButtonObj)&&(obj!=o)) {
		if (((glCompButton *) obj)->groupid == p->groupid)
		    ((glCompButton *) obj)->status = 0;
	    }
	}
	p->status = 1;
    }
    else {
	if (p->groupid == -1) {
	    if (p->status == 0)
		p->status = 1;
	    else
		p->status = 0;
	} else
	    p->status = 0;
    }
    if (p->common.callbacks.click)
	p->common.callbacks.click((glCompObj *) p, x, y, t);
}

void glCompButtonDoubleClick(glCompObj * obj, GLfloat x, GLfloat y,
			     glMouseButtonType t)
{
    /*Put your internal code here */
    if (((glCompButton *) obj)->common.callbacks.doubleclick)
	((glCompButton *) obj)->common.callbacks.doubleclick(obj, x, y, t);
}

void glCompButtonMouseDown(glCompObj * obj, GLfloat x, GLfloat y,
			   glMouseButtonType t)
{
    /*Put your internal code here */

    
    ((glCompButton *) obj)->refStatus = ((glCompButton *) obj)->status;
    ((glCompButton *) obj)->status = 1;
    if (((glCompButton *) obj)->common.callbacks.mousedown)
	((glCompButton *) obj)->common.callbacks.mousedown(obj, x, y, t);
}

void glCompButtonMouseIn(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompButton *) obj)->common.callbacks.mousein)
	((glCompButton *) obj)->common.callbacks.mousein(obj, x, y);
}

void glCompButtonMouseOut(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompButton *) obj)->common.callbacks.mouseout)
	((glCompButton *) obj)->common.callbacks.mouseout(obj, x, y);
}

void glCompButtonMouseOver(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompButton *) obj)->common.callbacks.mouseover)
	((glCompButton *) obj)->common.callbacks.mouseover(obj, x, y);
}

void glCompButtonMouseUp(glCompObj * obj, GLfloat x, GLfloat y,
			 glMouseButtonType t)
{
    /*Put your internal code here */

    if (((glCompButton *) obj)->common.callbacks.mouseup)
	((glCompButton *) obj)->common.callbacks.mouseup(obj, x, y, t);
}




void glCompButtonSetText(glCompButton * p, char *str)
{
//    replacestr(str, &p->text);
}
