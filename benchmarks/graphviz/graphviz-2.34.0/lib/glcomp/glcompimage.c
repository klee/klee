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

#include "glcompimage.h"
#include "glcompfont.h"
#include "glcompset.h"
#include "glutils.h"
#include "glcomptexture.h"
#include "memory.h"

glCompImage *glCompImageNew(glCompObj * par, GLfloat x, GLfloat y)
{
    glCompImage *p;
    p = NEW(glCompImage);
    glCompInitCommon((glCompObj *) p, par, x, y);
    p->objType = glImageObj;
    //typedef enum {glPanelObj,glbuttonObj,glLabelObj,glImageObj}glObjType;

    p->objType = glImageObj;
    p->stretch = 0;
#if 0
    p->pngFile = (char *) 0;
#endif
    p->texture = NULL;
    p->common.functions.draw = glCompImageDraw;
    return p;
}

/* glCompImageNewFile:
 * Creates image from given input file.
 * At present, we assume png input.
 * Return 0 on failure.
 */
glCompImage *glCompImageNewFile (glCompObj * par, GLfloat x, GLfloat y, char* imgfile, int is2D)
{
    int imageWidth, imageHeight;
    unsigned char *data = glCompLoadPng (imgfile, &imageWidth, &imageHeight);
    glCompImage *p;

    if (!data) return NULL;
    p = glCompImageNew (par, x, y);
    if (!glCompImageLoad (p, data, imageWidth, imageHeight, is2D)) {
	glCompImageDelete (p);
	return NULL;
    }
    return p;
}

void glCompImageDelete(glCompImage * p)
{
    glCompEmptyCommon(&p->common);
#if 0
    if (p->pngFile)
	free(p->pngFile);
#endif
    if (p->texture)
	glCompDeleteTexture(p->texture);
    free(p);
}

int glCompImageLoad(glCompImage * i, unsigned char *data, int width,
		    int height,int is2D)
{
    if (data != NULL) {		/*valid image data */
	glCompDeleteTexture(i->texture);
	i->texture =
	    glCompSetAddNewTexImage(i->common.compset, width, height, data,
				    is2D);
	if (i->texture) {
	    i->common.width = width;
	    i->common.height = height;
	    return 1;
	}

    }
    return 0;
}



int glCompImageLoadPng(glCompImage * i, char *pngFile,int is2D)
{
    int imageWidth, imageHeight;
    unsigned char *data;
    data = glCompLoadPng (pngFile, &imageWidth, &imageHeight);
    return glCompImageLoad(i, data, imageWidth, imageHeight,is2D);
}

#if 0
int glCompImageLoadRaw(glCompSet * s, glCompImage * i, char *rawFile,int is2D)
{
    int imageWidth, imageHeight;
    unsigned char *data;
    data = glCompLoadPng (rawFile, &imageWidth, &imageHeight);
    return glCompImageLoad(i, data, imageWidth, imageHeight,is2D);
}
#endif

void glCompImageDraw(void *obj)
{
    glCompImage *p = (glCompImage *) obj;
    glCompCommon ref = p->common;
    GLfloat w,h,d;

    glCompCalcWidget((glCompCommon *) p->common.parent, &p->common, &ref);
    if (!p->common.visible)
	return;
    if (!p->texture)
	return;

    if(p->texture->id <=0)
    {
	glRasterPos2f(ref.pos.x, ref.pos.y);
	glDrawPixels(p->texture->width, p->texture->height, GL_RGBA,GL_UNSIGNED_BYTE, p->texture->data);
    }
    else
    {
#if 0
	w=ref.width;
	h=ref.height;
#endif
	w = p->width;
	h = p->height;
	d=(GLfloat)p->common.layer* (GLfloat)GLCOMPSET_BEVEL_DIFF;
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glBindTexture(GL_TEXTURE_2D,p->texture->id);
	glBegin(GL_QUADS);
		glTexCoord2d(0.0f, 1.0f);glVertex3d(ref.pos.x,ref.pos.y,d);
		glTexCoord2d(1.0f, 1.0f);glVertex3d(ref.pos.x+w,ref.pos.y,d);
		glTexCoord2d(1.0f, 0.0f);glVertex3d(ref.pos.x+w,ref.pos.y+h,d);
		glTexCoord2d(0.0f, 0.0f);glVertex3d(ref.pos.x,ref.pos.y+h,d);
	glEnd();


	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
    }

}

void glCompImageClick(glCompObj * o, GLfloat x, GLfloat y,
		      glMouseButtonType t)
{
    if (o->common.callbacks.click)
	o->common.callbacks.click(o, x, y, t);
}

void glCompImageDoubleClick(glCompObj * obj, GLfloat x, GLfloat y,
			    glMouseButtonType t)
{
    /*Put your internal code here */
    if (((glCompImage *) obj)->common.callbacks.doubleclick)
	((glCompImage *) obj)->common.callbacks.doubleclick(obj, x, y, t);
}

void glCompImageMouseDown(glCompObj * obj, GLfloat x, GLfloat y,
			  glMouseButtonType t)
{
    /*Put your internal code here */
    if (((glCompImage *) obj)->common.callbacks.mousedown)
	((glCompImage *) obj)->common.callbacks.mousedown(obj, x, y, t);
}

void glCompImageMouseIn(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompImage *) obj)->common.callbacks.mousein)
	((glCompImage *) obj)->common.callbacks.mousein(obj, x, y);
}

void glCompImageMouseOut(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompImage *) obj)->common.callbacks.mouseout)
	((glCompImage *) obj)->common.callbacks.mouseout(obj, x, y);
}

void glCompImageMouseOver(glCompObj * obj, GLfloat x, GLfloat y)
{
    /*Put your internal code here */
    if (((glCompImage *) obj)->common.callbacks.mouseover)
	((glCompImage *) obj)->common.callbacks.mouseover(obj, x, y);
}

void glCompImageMouseUp(glCompObj * obj, GLfloat x, GLfloat y,
			glMouseButtonType t)
{
    /*Put your internal code here */
    if (((glCompImage *) obj)->common.callbacks.mouseup)
	((glCompImage *) obj)->common.callbacks.mouseup(obj, x, y, t);
}
