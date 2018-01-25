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

#include "glcomplabel.h"
#include "glcompfont.h"
#include "glcompset.h"
#include "glutils.h"
#include "memory.h"

glCompLabel *glCompLabelNew(glCompObj * par, GLfloat x, GLfloat y,
			    char *text)
{
    glCompLabel *p;
//      glCompCommon* parent=&par->common;
    p = NEW(glCompLabel);
    glCompInitCommon((glCompObj *) p, par, x, y);
    p->objType = glLabelObj;
    p->transparent=1;
    //typedef enum {glPanelObj,glbuttonObj,glLabelObj,glImageObj}glObjType;

    p->text = strdup(text);
    p->common.font = glNewFontFromParent ((glCompObj*)p, text);
    p->common.functions.draw = (glcompdrawfunc_t)glCompLabelDraw;

    return p;
}


int glCompLabelDraw(glCompLabel * p)
{
    glCompCommon ref;
    ref = p->common;
    glCompCalcWidget((glCompCommon *) p->common.parent, &p->common, &ref);
    /*draw background */
    if(!p->transparent)
    {
	glCompSetColor(&p->common.color);
	glBegin(GL_QUADS);
	glVertex3d(ref.refPos.x, ref.refPos.y, ref.refPos.z);
	glVertex3d(ref.refPos.x + ref.width, ref.refPos.y, ref.refPos.z);
	glVertex3d(ref.refPos.x + ref.width, ref.refPos.y + ref.height,
	           ref.refPos.z);
	glVertex3d(ref.refPos.x, ref.refPos.y + ref.height, ref.refPos.z);
	glEnd();
    }
    glCompRenderText(p->common.font, (glCompObj *) p);
    return 1;

}
static void update_font(glCompLabel * p,char* text,char* desc,int fs)
{

    glCompFont* temp=p->common.font;
    if (strlen(text) >512)
	return ;

    p->common.font=glNewFont (p->common.compset,text,&p->common.color,temp->type,desc,fs,temp->is2D);
    if(temp)
	glDeleteFont(temp);
    if(p->text)
	free(p->text);
    p->text = strdup(text);


}

void glCompLabelSetText(glCompLabel * p, char *text)
{
    glCompFont* temp=p->common.font;
    update_font(p,text,temp->fontdesc,temp->size);
}
void glCompLabelSetFontSize(glCompLabel * p, int size)
{
    glCompFont* temp=p->common.font;
    update_font(p,p->text,temp->fontdesc,size);
}
void glCompLabelSetFontName(glCompLabel * p, char* fontName)
{
    glCompFont* temp=p->common.font;
    update_font(p,p->text,fontName,temp->size);
}


void glCompLabelClick(glCompObj * o, GLfloat x, GLfloat y,
		      glMouseButtonType t)
{
    if (o->common.callbacks.click)
	o->common.callbacks.click(o, x, y, t);
}

void glCompLabelDoubleClick(glCompObj * obj, GLfloat x, GLfloat y,
			    glMouseButtonType t)
{
    /*Put your internal code here */
    if (((glCompLabel *) obj)->common.callbacks.doubleclick)
	((glCompLabel *) obj)->common.callbacks.doubleclick(obj, x, y, t);
}

void glCompLabelMouseDown(glCompObj * obj, GLfloat x, GLfloat y,
			  glMouseButtonType t)
{
    /*Put your internal code here */
    if (((glCompLabel *) obj)->common.callbacks.mousedown)
	((glCompLabel *) obj)->common.callbacks.mousedown(obj, x, y, t);
}

void glCompLabelMouseIn(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompLabel *) obj)->common.callbacks.mousein)
	((glCompLabel *) obj)->common.callbacks.mousein(obj, x, y);
}

void glCompLabelMouseOut(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompLabel *) obj)->common.callbacks.mouseout)
	((glCompLabel *) obj)->common.callbacks.mouseout(obj, x, y);
}

void glCompLabelMouseOver(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompLabel *) obj)->common.callbacks.mouseover)
	((glCompLabel *) obj)->common.callbacks.mouseover(obj, x, y);
}

void glCompLabelMouseUp(glCompObj * obj, GLfloat x, GLfloat y,
			glMouseButtonType t)
{
    /*Put your internal code here */
    if (((glCompLabel *) obj)->common.callbacks.mouseup)
	((glCompLabel *) obj)->common.callbacks.mouseup(obj, x, y, t);
}
