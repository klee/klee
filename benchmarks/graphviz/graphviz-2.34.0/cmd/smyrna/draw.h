/* $Id$ $Revision$ */
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

#ifndef DRAW_H
#define DRAW_H
#include "smyrnadefs.h"
#include <gtk/gtkgl.h>
#include "xdot.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "glcompfont.h"

#ifdef __cplusplus
extern "C" {
#endif
/* DRAWING FUNCTIONS 
 * these are opengl based xdot drawing functions 
 * topview drawings are not here
 */
extern drawfunc_t OpFns[];
extern void drawGraph(Agraph_t * g);
#if 0
extern void scanGraph(Agraph_t * g);
extern int randomize_color(glCompColor * c, int brightness);
extern void draw_sphere(float x, float y, float z, float r);
extern int draw_node_hintbox(GLfloat x, GLfloat y, GLfloat z, GLfloat fs, char *text);
extern glCompColor GetglCompColor(char *color);
extern void draw_magnifier(ViewInfo * view);
extern void draw_fisheye_magnifier(ViewInfo * view);
#endif
extern void draw_selection_box(ViewInfo * view);
extern void drawCircle(float x, float y, float radius, float zdepth);
extern void drawBorders(ViewInfo * view);
extern void drawEllipse(float xradius, float yradius, int angle1, int angle2);
extern void draw_selpoly(glCompPoly* selPoly);

#if 0
	/*xdot drawing functions*/
extern void DrawBeziers(sdot_op* o, int param);
extern void DrawEllipse(sdot_op * op, int param);
extern void DrawPolygon(sdot_op * op, int param);
extern void DrawPolyline(sdot_op * op, int param);
extern void SetFillColor(sdot_op*  o, int param);
extern void SetPenColor(sdot_op* o, int param);
extern void SetStyle(sdot_op* o, int param);
extern void SetFont(sdot_op * o, int param);
extern void InsertImage(sdot_op * o, int param);
extern void EmbedText(sdot_op * o, int param);
#endif

typedef struct {
    glCompColor color;
    float width;
} xdotstyle;

typedef struct {
    glCompColor penColor;
    glCompColor fillColor;
    xdotstyle style;
} xdotstate;	


#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
