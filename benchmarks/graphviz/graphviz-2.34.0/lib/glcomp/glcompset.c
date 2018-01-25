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

#include "glcompset.h"
#include "memory.h"
#include "glcomppanel.h"
#include "glcomplabel.h"
#include "glcompbutton.h"
#include "glcompmouse.h"

#include "glutils.h"
//typedef enum {glPanelObj,glbuttonObj,glLabelObj,glImageObj}glObjType;

static GLfloat startX, startY;



void glCompGetObjectType(glCompObj * p)
{
    switch (p->objType) {
    case glPanelObj:
	printf("Panel\n");
	break;
    case glButtonObj:
	printf("Button\n");
	break;
    case glImageObj:
	printf("Image\n");
	break;
    case glLabelObj:
	printf("Label\n");
	break;
    default:
	printf("undefined object\n");
	break;

    }

}

static int glCompPointInObject(glCompObj * p, float x, float y)
{
    return ((x > p->common.refPos.x)
	    && (x < p->common.refPos.x + p->common.width)
	    && (y > p->common.refPos.y)
	    && (y < p->common.refPos.y + p->common.height));
}

glCompObj *glCompGetObjByMouse(glCompSet * s, glCompMouse * m,
			       int onlyClickable)
{
    int ind;
    glCompObj *rv = NULL;
    if (!s || !m)
	return NULL;
    for (ind = 0; ind < s->objcnt; ind++) {
	if ((s->obj[ind]->common.visible)
	    && (glCompPointInObject(s->obj[ind], m->pos.x, m->pos.y))) {
	    if ((!rv) || (s->obj[ind]->common.layer >= rv->common.layer)) {
		if (((onlyClickable)
		     && (s->obj[ind]->common.functions.click))
		    || (!onlyClickable))
		    rv = s->obj[ind];
	    }
	}
    }

    return rv;
}


static void glCompMouseMove(void *obj, GLfloat x, GLfloat y)
{
    ((glCompSet *) obj)->mouse.pos.x = x;
    ((glCompSet *) obj)->mouse.pos.y =
	((glCompObj *) obj)->common.height - y;
    ((glCompSet *) obj)->mouse.pos.z = 0;
    ((glCompSet *) obj)->mouse.dragY =
	((glCompSet *) obj)->mouse.pos.y - startY;
    ((glCompSet *) obj)->mouse.dragX =
	((glCompSet *) obj)->mouse.pos.x - startX;
    if (((glCompSet *) obj)->common.callbacks.mouseover)
	((glCompSet *) obj)->common.callbacks.mouseover(obj, x, y);
/*	if (((glCompSet*)obj)->mouse.down)
		printf ("%f %f \n",((glCompSet*)obj)->mouse.dragX,((glCompSet*)obj)->mouse.dragX);*/
}



static void glCompSetMouseClick(void *obj, GLfloat x, GLfloat y,
				glMouseButtonType t)
{
    if (((glCompSet *) obj)->common.callbacks.click)
	((glCompSet *) obj)->common.callbacks.click(obj, x, y, t);


}
static void glCompSetMouseDown(void *obj, GLfloat x, GLfloat y,
			       glMouseButtonType t)
{
    ((glCompSet *) obj)->mouse.t = t;
    if (t == glMouseLeftButton) {
	((glCompSet *) obj)->mouse.pos.x = x;
	((glCompSet *) obj)->mouse.pos.y =
	    ((glCompObj *) obj)->common.height - y;
	((glCompSet *) obj)->mouse.pos.z = 0;
	((glCompSet *) obj)->mouse.clickedObj =
	    glCompGetObjByMouse(((glCompObj *) obj)->common.compset,
				&((glCompSet *) (((glCompObj *) obj)->
						 common.compset))->mouse,
				1);
	if (((glCompSet *) obj)->mouse.clickedObj)
	    if (((glCompSet *) obj)->mouse.clickedObj->common.functions.
		mousedown)
		((glCompSet *) obj)->mouse.clickedObj->common.functions.
		    mousedown(((glCompSet *) obj)->mouse.clickedObj, x, y,
			      t);
    }
    ((glCompSet *) obj)->mouse.down = 1;
    startX = x;
    startY = ((glCompObj *) obj)->common.height - y;
    if (((glCompSet *) obj)->common.callbacks.mousedown)
	((glCompSet *) obj)->common.callbacks.mousedown(obj, x, y, t);




}
static void glCompSetMouseUp(void *obj, GLfloat x, GLfloat y,
			     glMouseButtonType t)
{

    static GLfloat tempX, tempY;
    tempX = x;
    tempY = ((glCompObj *) obj)->common.height - y;

    ((glCompSet *) obj)->mouse.down = 0;
    if (t == glMouseLeftButton) {
	glCompObj *o = NULL;
	glCompObj *o_clicked = ((glCompSet *) obj)->mouse.clickedObj;
	((glCompSet *) obj)->mouse.pos.x = tempX;
	((glCompSet *) obj)->mouse.pos.y = tempY;
	((glCompSet *) obj)->mouse.pos.z = 0;
	if (o_clicked)
	    o = glCompGetObjByMouse((glCompSet *) obj,
				    &((glCompSet *) obj)->mouse, 1);
	if (!o)
	    return;
	if (o == o_clicked)
	    o->common.functions.click(o, x, y, t);
    }
    if (((glCompSet *) obj)->common.callbacks.mouseup)
	((glCompSet *) obj)->common.callbacks.mouseup(obj, x, y, t);
    /*check if mouse is clicked or dragged */
    if ((startX == (int) tempX) && (startY == tempY))
	glCompSetMouseClick(obj, x, y, t);



}



void glCompInitCommon(glCompObj * childObj, glCompObj * parentObj,
			     GLfloat x, GLfloat y)
{
    glCompCommon *c;
    glCompCommon *parent;
    c = &childObj->common;
    c->align = glAlignNone;
    c->anchor.bottom = 0;
    c->anchor.left = 0;
    c->anchor.top = 0;
    c->anchor.right = 0;
    c->anchor.leftAnchor = 0;
    c->anchor.rightAnchor = 0;
    c->anchor.topAnchor = 0;
    c->anchor.bottomAnchor = 0;
    c->data = 0;
    c->enabled = 1;
    c->height = GLCOMP_DEFAULT_HEIGHT;;
    c->width = GLCOMP_DEFAULT_WIDTH;
    c->visible = 1;
    c->pos.x = x;
    c->pos.y = y;
    c->borderType = glBorderSolid;
    c->borderWidth = GLCOMPSET_BORDERWIDTH;

    /*NULL function pointers */
    childObj->common.callbacks.click = NULL;
    childObj->common.callbacks.doubleclick = NULL;
    childObj->common.callbacks.draw = NULL;
    childObj->common.callbacks.mousedown = NULL;
    childObj->common.callbacks.mousein = NULL;
    childObj->common.callbacks.mouseout = NULL;
    childObj->common.callbacks.mouseover = NULL;
    childObj->common.callbacks.mouseup = NULL;

    childObj->common.functions.click = NULL;
    childObj->common.functions.doubleclick = NULL;
    childObj->common.functions.draw = NULL;
    childObj->common.functions.mousedown = NULL;
    childObj->common.functions.mousein = NULL;
    childObj->common.functions.mouseout = NULL;
    childObj->common.functions.mouseover = NULL;
    childObj->common.functions.mouseup = NULL;



    if (parentObj) {
	c->parent = &parentObj->common;
	parent = &parentObj->common;
	copy_glcomp_color(&parent->color, &c->color);
	c->layer = parent->layer + 1;
	c->pos.z = parent->pos.z;
	glCompSetAddObj((glCompSet *) parent->compset, childObj);
    } else {
	c->parent = NULL;
	c->color.R = GLCOMPSET_PANEL_COLOR_R;
	c->color.G = GLCOMPSET_PANEL_COLOR_G;
	c->color.B = GLCOMPSET_PANEL_COLOR_B;
	c->color.A = GLCOMPSET_PANEL_COLOR_ALPHA;
	c->layer = 0;
	c->pos.z = 0;
    }
    c->font = glNewFontFromParent(childObj, NULL);
}

void glCompEmptyCommon(glCompCommon * c)
{
    glDeleteFont(c->font);
}
glCompSet *glCompSetNew(int w, int h)
{

    glCompSet *s = NEW(glCompSet);
    glCompInitCommon((glCompObj *) s, NULL, (GLfloat) 0, (GLfloat) 0);
    s->common.width = (GLfloat) w;
    s->common.height = (GLfloat) h;
    s->groupCount = 0;
    s->objcnt = 0;
    s->obj = (glCompObj **) 0;
    s->textureCount = 0;
    s->textures = (glCompTex **) 0;
    s->common.font = glNewFontFromParent((glCompObj *) s, NULL);
    s->common.compset = (glCompSet *) s;
    s->common.functions.mouseover = (glcompmouseoverfunc_t)glCompMouseMove;
    s->common.functions.mousedown = (glcompmousedownfunc_t)glCompSetMouseDown;
    s->common.functions.mouseup = (glcompmouseupfunc_t)glCompSetMouseUp;
    glCompMouseInit(&s->mouse);
    return s;
}



void glCompSetAddObj(glCompSet * s, glCompObj * obj)
{
    s->objcnt++;
    s->obj = realloc(s->obj, sizeof(glCompObj *) * s->objcnt);
    s->obj[s->objcnt - 1] = obj;
    obj->common.compset = s;
}

#if 0
// compiler reports this function is not used.

//converts screen location to opengl coordinates
static void glCompSetGetPos(int x, int y, float *X, float *Y, float *Z)
{
    GLdouble wwinX;
    GLdouble wwinY;
    GLdouble wwinZ;
    GLdouble posX, posY, posZ;


    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY;
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    //draw a point  to a not important location to get window coordinates
    glBegin(GL_POINTS);
    glVertex3f(10.00, 10.00, 0.00);
    glEnd();
    gluProject(10.0, 10.0, 0.00, modelview, projection, viewport, &wwinX,
	       &wwinY, &wwinZ);
    winX = (float) x;
    winY = (float) viewport[3] - (float) y;
    gluUnProject(winX, winY, wwinZ, modelview, projection, viewport, &posX,
		 &posY, &posZ);

    *X = (float) posX;
    *Y = (float) posY;
    *Z = (float) posZ;
}
#endif

void glCompDrawBegin(void)	//pushes a gl stack 
{
    int vPort[4];

    glGetIntegerv(GL_VIEWPORT, vPort);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();


    glOrtho(0, vPort[2], 0, vPort[3], -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_BLEND);

    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

}

void glCompDrawEnd(void)	//pops the gl stack 
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);


}

void glCompSetClear(glCompSet * s)
{
/*    int ind = 0;
    for (ind = 0; ind < s->buttoncount; ind++) {
	glCompSetRemoveButton(s, s->buttons[ind]);
    }
    free(s->buttons);
    for (ind = 0; ind < s->labelcount; ind++) {
	free(s->labels[ind]->text);
	free(s->labels[ind]);
    }
    free(s->labels);
    for (ind = 0; ind < s->panelcount; ind++) {
	free(s->panels[ind]);
    }
    free(s->panels);
    free(s);*/
}



int glCompSetDraw(glCompSet * s)
{
    int ind = 0;
    glCompDrawBegin();
    for (; ind < s->objcnt; ind++) {
	s->obj[ind]->common.functions.draw((void *) s->obj[ind]);
    }
    glCompDrawEnd();
    return 1;
}

void glcompsetUpdateBorder(glCompSet * s, int w, int h)
{
    if (w > 0 && h > 0) {
	s->common.width = (GLfloat) w;
	s->common.height = (GLfloat) h;
    }
}
extern int glcompsetGetGroupId(glCompSet * s)
{
    return s->groupCount;
}
extern int glcompsetNextGroupId(glCompSet * s)
{
    int rv = s->groupCount;
    s->groupCount++;
    return rv;
}




#if 0
static void change_fonts(glCompSet * s, const texFont_t * sourcefont)
{
    int ind;

    for (ind = 0; ind < s->buttoncount; ind++) {
	copy_font((s->buttons[ind]->font), sourcefont);
    }
    for (ind = 0; ind < s->labelcount; ind++) {
	copy_font((s->labels[ind]->font), sourcefont);

    }
    for (ind = 0; ind < s->panelcount; ind++) {
	copy_font((s->panels[ind]->font), sourcefont);
    }
}
#endif
