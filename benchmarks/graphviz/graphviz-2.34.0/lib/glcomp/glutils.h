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

#ifndef GLUTILS_H
#define GLUTILS_H
#ifdef _WIN32
#include "windows.h"
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include "glcompdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
	glCompPoint u, v;
    } line;
    typedef struct {
	glCompPoint N;		/* normal */
	double d;		/* offset */
    } plane;


    int GetFixedOGLPos(int, int, float, GLfloat *, GLfloat *, GLfloat * Z);
    int GetOGLPosRef(int x, int y, float *X, float *Y, float *Z);
    float GetOGLDistance(int l);
    int GetFixedOGLPoslocal(int x, int y, GLfloat * X, GLfloat * Y,
			    GLfloat * Z);
    void to3D(int x, int y, GLfloat * X, GLfloat * Y, GLfloat * Z);
    double point_to_lineseg_dist(glCompPoint p, glCompPoint a, glCompPoint b);
#if 0
    void linear_interpolate(float, float, float, float, float, float *);
    double point_to_line_dist(glCompPoint p, glCompPoint u, glCompPoint v);
    int rot_spherex(plane J, float tet, glCompPoint P, glCompPoint * P2);
    void make_plane(glCompPoint a, glCompPoint b, glCompPoint c, plane * P);
    int lines_intersect (glCompPoint* a, glCompPoint* b, glCompPoint* c, glCompPoint* d);
#endif
    void replacestr(char *source, char **target);
    extern void glCompCalcWidget(glCompCommon * parent,
				 glCompCommon * child, glCompCommon * ref);
    extern void glCompDrawRectangle(glCompRect * r);
    extern void glCompDrawRectPrism(glCompPoint * p, GLfloat w, GLfloat h,
				    GLfloat b, GLfloat d, glCompColor * c,
				    int bumped);
    extern void copy_glcomp_color(glCompColor * source,
				  glCompColor * target);
    extern void glCompSetColor(glCompColor * c);

    void glCompSelectionBox(glCompSet * s);
    extern GLfloat distBetweenPts(glCompPoint A,glCompPoint B,float R);
    extern int is_point_in_rectangle(float X, float Y, float RX, float RY, float RW,float RH);

#ifdef __cplusplus
}
#endif
#endif
