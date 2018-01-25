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

#include "glutils.h"
#include "stdlib.h"
#include "string.h"
#include "glcompdefs.h"
/* #include "glexpose.h" */

/* at given depth value, tranforms 2d Window location to 3d gl coords*/
int GetFixedOGLPos(int x, int y, float kts, GLfloat * X, GLfloat * Y,
		   GLfloat * Z)
{
    GLdouble wwinX;
    GLdouble wwinY;
    GLdouble wwinZ;

    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY;
    GLdouble posX, posY, posZ;

    glColor4f((GLfloat) 0, (GLfloat) 0, (GLfloat) 0, (GLfloat) 0.001);
    glBegin(GL_POINTS);
    glVertex3f((GLfloat) - 100.00, (GLfloat) - 100.00, (GLfloat) 1.00);
    glEnd();

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);
    gluProject(-100.0, -100.0, 1.00, modelview, projection, viewport,
	       &wwinX, &wwinY, &wwinZ);

    winX = (float) x;
    winY = (float) viewport[3] - (float) y;
    gluUnProject(winX, winY, wwinZ, modelview, projection, viewport, &posX,
		 &posY, &posZ);
    *X = (GLfloat) posX;
    *Y = (GLfloat) posY;
    *Z = (GLfloat) posZ;

    return 1;

}

/*transforms 2d windows location to 3d gl coords but depth is calculated unlike the previous function*/
int GetOGLPosRef(int x, int y, float *X, float *Y, float *Z)
{

    GLdouble wwinX;
    GLdouble wwinY;
    GLdouble wwinZ;
    GLdouble posX, posY, posZ;


    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY;
    //glTranslatef (0.0,0.0,0.0);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    //draw a point  to a not important location to get window coordinates
    glColor4f((GLfloat) 0, (GLfloat) 0, (GLfloat) 0, (GLfloat) 0.001);

    glBegin(GL_POINTS);
    glVertex3f(-100.00, -100.00, 0.00);
    glEnd();
    gluProject(-100.0, -100.0, 0.00, modelview, projection, viewport,
	       &wwinX, &wwinY, &wwinZ);
    winX = (float) x;
    winY = (float) viewport[3] - (float) y;
    gluUnProject(winX, winY, wwinZ, modelview, projection, viewport, &posX,
		 &posY, &posZ);

    *X = (float) posX;
    *Y = (float) posY;
    *Z = (float) posZ;
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//      printf("==>(%d,%d,%d) -> (%f,%f,%f)\n",x,y,wwinZ,*X,*Y,*Z);


    return 1;

}


float GetOGLDistance(int l)
{

    int x, y;
    GLdouble wwinX;
    GLdouble wwinY;
    GLdouble wwinZ;
    GLdouble posX, posY, posZ;
    GLdouble posXX, posYY, posZZ;



    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY;




    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    //draw a point  to a not important location to get window coordinates
    glColor4f((GLfloat) 0, (GLfloat) 0, (GLfloat) 0, (GLfloat) 0.001);

    glBegin(GL_POINTS);
    glVertex3f(10.00, 10.00, 1.00);
    glEnd();
    gluProject(10.0, 10.0, 1.00, modelview, projection, viewport, &wwinX,
	       &wwinY, &wwinZ);
    x = 50;
    y = 50;
    winX = (float) x;
    winY = (float) viewport[3] - (float) y;
    gluUnProject(winX, winY, wwinZ, modelview, projection, viewport, &posX,
		 &posY, &posZ);
    x = x + l;
    y = 50;
    winX = (float) x;
    winY = (float) viewport[3] - (float) y;
    gluUnProject(winX, winY, wwinZ, modelview, projection, viewport,
		 &posXX, &posYY, &posZZ);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return ((float) (posXX - posX));

}

/*
	functions def: returns opengl coordinates of firt hit object by using screen coordinates
	x,y; 2D screen coordiantes (usually received from mouse events
	X,Y,Z; pointers to coordinates values to be calculated
	return value: no return value


*/

void to3D(int x, int y, GLfloat * X, GLfloat * Y, GLfloat * Z)
{

    int const WIDTH = 20;

    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY;
    GLfloat winZ[400];
    GLdouble posX, posY, posZ;
    int idx;
    static float comp;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    winX = (float) x;
    winY = (float) viewport[3] - (float) y;

    glReadPixels(x - WIDTH / 2, (int) winY - WIDTH / 2, WIDTH, WIDTH,
		 GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
    comp = -9999999;
    for (idx = 0; idx < WIDTH * WIDTH; idx++) {
//              printf ("Z value:%f ",winZ[idx]);
	if ((winZ[idx] > comp) && (winZ[idx] < 1))
	    comp = winZ[idx];
    }
//      printf ("\n");

    gluUnProject(winX, winY, comp, modelview, projection, viewport, &posX,
		 &posY, &posZ);

    *X = (GLfloat) posX;
    *Y = (GLfloat) posY;
    *Z = (GLfloat) posZ;
    return;





}


int GetFixedOGLPoslocal(int x, int y, GLfloat * X, GLfloat * Y,
			GLfloat * Z)
{
    GLdouble wwinX;
    GLdouble wwinY;
    GLdouble wwinZ;

    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY;
    GLdouble posX, posY, posZ;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);



    glColor4f((GLfloat) 0, (GLfloat) 0, (GLfloat) 0, (GLfloat) 0.001);
    glBegin(GL_POINTS);
    glVertex3f(10.00, 10.00, 0.00);
    glEnd();

    gluProject(10.0, 10.0, 1.00, modelview, projection, viewport, &wwinX,
	       &wwinY, &wwinZ);

    winX = (float) x;
    winY = (float) viewport[3] - (float) y;
    gluUnProject(winX, winY, wwinZ, modelview, projection, viewport, &posX,
		 &posY, &posZ);
    *X = (GLfloat) posX;
    *Y = (GLfloat) posY;
    *Z = (GLfloat) posZ;

//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return 1;

}
#if 0
void linear_interpolate(float x1, float y1, float x2, float y2, float x3,
			 float *y3)
{

    float a, b;
    a = (y1 - y2) / (x1 - x2);
    b = y1 - a * x1;
    *y3 = a * x3 + b;
}

int glreversecamera(ViewInfo * view)
{

    glLoadIdentity();
    if (view->active_camera == -1) {
	gluLookAt(view->panx, view->pany, 20, view->panx,
		  view->pany, 0.0, 0.0, 1.0, 0.0);
	glScalef(1 * view->zoom * -1, 1 * view->zoom * -1,
		 1 * view->zoom * -1);
    } else {
	glScalef(1 * view->cameras[view->active_camera]->r,
		 1 * view->cameras[view->active_camera]->r,
		 1 * view->cameras[view->active_camera]->r);

    }

    return 1;
}

#endif
#include <math.h>



#if 0
static glCompPoint add(glCompPoint p, glCompPoint q)
{
    p.x += q.x;
    p.y += q.y;
    p.z += q.z;
    return p;
}
#endif

static glCompPoint sub(glCompPoint p, glCompPoint q)
{
    p.x -= q.x;
    p.y -= q.y;
    p.z -= q.z;
    return p;
}

static double dot(glCompPoint p, glCompPoint q)
{
    return (p.x * q.x + p.y * q.y + p.z * q.z);
}

static double len(glCompPoint p)
{
    return sqrt(dot(p, p));
}

#if 0
static glCompPoint scale(double d, glCompPoint p)
{
    p.x *= (float) d;
    p.y *= (float) d;
    p.z *= (float) d;
    return p;
}
#endif

static glCompPoint blend(glCompPoint p, glCompPoint q, float m)
{
    glCompPoint r;

    r.x = p.x + m * (q.x - p.x);
    r.y = p.y + m * (q.y - p.y);
    r.z = p.z + m * (q.z - p.z);
    return r;
}

static double dist(glCompPoint p, glCompPoint q)
{
    return (len(sub(p, q)));
}

#if 0
static glCompPoint normalize(glCompPoint p)
{
    double d = len(p);

    return scale(1 / d, p);
}

static glCompPoint intersect(line l, plane J)
{
    double t = -(J.d + dot(l.u, J.N)) / dot(l.v, J.N);
    return (add(l.u, scale(t, l.v)));
}

/*
 * Given a line l determined by two points a and b, and a 3rd point p,
 * return the distance between the point and the line
 */
double point_to_line_dist(glCompPoint p, glCompPoint a, glCompPoint b)
{
    line l;
    plane J;
    glCompPoint q;

    l.u = a;
    l.v = normalize(sub(b, a));

    J.N = l.v;
    J.d = -dot(p, l.v);

    q = intersect(l, J);

    return (dist(p, q));
}
#endif


/*
 * Given a line segment determined by two points a and b, and a 3rd point p,
 * return the distance between the point and the segment.
 * If the perpendicular from p to the line a-b is outside of the segment,
 * return the distance to the closer of a or b.
 */
double point_to_lineseg_dist(glCompPoint p, glCompPoint a, glCompPoint b)
{
    float U;
    glCompPoint q;
    glCompPoint ba = sub(b, a);
    glCompPoint pa = sub(p, a);

    U = (float) (dot(pa, ba) / dot(ba, ba));

    if (U > 1)
	q = b;
    else if (U < 0)
	q = a;
    else
	q = blend(a, b, U);

    return dist(p, q);

}

#if 0
/*
	Calculates the parameters of a plane via given 3 points on it
*/

void make_plane(glCompPoint a, glCompPoint b, glCompPoint c, plane * P)
{
    P->N.x = a.y * (b.z - c.z) + b.y * (c.z - a.z) + c.y * (a.z - b.z);	//+
    P->N.y = a.z * (b.x - c.x) + b.z * (c.x - a.x) + c.z * (a.x - b.x);	//+
    P->N.z = a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y);	//+
    P->d =
	(a.x * (b.y * c.z - c.y * b.z) + b.x * (c.y * a.z - a.y * c.z) +
	 c.x * (a.y * b.z - b.y * a.z)) * -1;
}
#endif
void replacestr(char *source, char **target)
{

    if (*target)
	free(*target);
    *target = strdup(source);
}

#if 0
/*
	move a point on the great circle of it (spherical)

*/

#define G_PI    3.1415926535897932384626433832795028841971693993751
#define DEG2RAD  G_PI/180
int rot_spherex(plane J, float tet, glCompPoint P, glCompPoint * P2)
{
    if (tet > 0) {
	tet = 5;
	tet = (float) DEG2RAD *tet;
	P2->x =
	    (float) (J.N.x * J.N.x +
		     (float) cos(tet) * (1 - J.N.x * J.N.x)) * P.x +
	    (J.N.x * J.N.y * (1 - (float) cos(tet)) -
	     J.N.z * (float) sin(tet))
	    + (J.N.z * J.N.x * (1 - (float) cos(tet)) +
	       J.N.y * (float) sin(tet)) * P.z;
	P2->y =
	    (float) (J.N.x * J.N.y * (1 - (float) cos(tet)) +
		     J.N.z * (float) sin(tet)) * P.x + (J.N.y * J.N.y +
							(float) cos(tet) *
							(1 -
							 J.N.y * J.N.y)) *
	    P.y + (J.N.y * J.N.z * (1 - (float) cos(tet)) -
		   J.N.x * (float) sin(tet)) * P.z;
	P2->z =
	    (float) (J.N.z * J.N.x * (1 - (float) cos(tet)) -
		     J.N.y * (float) sin(tet)) * P.x +
	    (J.N.y * J.N.z * (1 - (float) cos(tet))
	     + J.N.x * (float) sin(tet)) * P.y + (J.N.z * J.N.z +
						  (float) cos(tet) * (1 -
								      J.N.
								      z *
								      J.N.
								      z)) *
	    P.z;
	return 1;
    } else
	return 0;

}
#endif

void glCompSelectionBox(glCompSet * s)
{
    static GLfloat x, y, w, h;
/*	if (( h < 0)  || (w < 0))
	{
	    glEnable(GL_LINE_STIPPLE);
		glLineStipple(1, 15);
	}*/

    x = s->mouse.pos.x;
    y = s->mouse.pos.y;
    w = s->mouse.dragX;
    h = s->mouse.dragY;
    printf("%f %f  %f  %f \n", x, y, w, h);
    glColor4f(1, 1, 1, 1);
/*	glBegin(GL_POLYGON);
		glVertex2f(x,y);
		glVertex2f(x, y+h);
		glVertex2f(x-w, y+h);
		glVertex2f(x-w, y);
		glVertex2f(x-w, y);

	glEnd();*/

    glBegin(GL_POLYGON);
    glVertex2f(0, 0);
    glVertex2f(250, 0);
    glVertex2f(250, 250);
    glVertex2f(0, 250);
    glVertex2f(0, 0);

    glEnd();


    glDisable(GL_LINE_STIPPLE);



}





void glCompCalcWidget(glCompCommon * parent, glCompCommon * child,
		      glCompCommon * ref)
{
    /*check alignments first , alignments overrides anchors */
    GLfloat borderWidth;
    ref->height = child->height;
    ref->width = child->width;
    if(!parent)
    {
	child->refPos.x = child->pos.x;
	child->refPos.y = child->pos.y;
	return;
    }
    if (parent->borderType == glBorderNone)
	borderWidth = 0;
    else
	borderWidth = parent->borderWidth;
    if (child->align != glAlignNone)	//if alignment, make sure width and height is no greater than parent
    {
	if (child->width > parent->width)
	    ref->width = parent->width - (float) 2.0 *borderWidth;
	if (child->height > parent->height)
	    ref->height = parent->height - (float) 2.0 *borderWidth;;

    }

    ref->pos.x = parent->refPos.x + ref->pos.x + borderWidth;
    ref->pos.y = parent->refPos.y + ref->pos.y + borderWidth;


    switch (child->align) {
    case glAlignLeft:
	ref->pos.x = parent->refPos.x + borderWidth;
	ref->pos.y = parent->refPos.y + borderWidth;
	ref->height = parent->height - 2 * borderWidth;
	break;
    case glAlignRight:
	ref->pos.x =
	    parent->refPos.x + parent->width - child->width - borderWidth;
	ref->pos.y = parent->refPos.y + borderWidth;
	ref->height = parent->height - 2 * borderWidth;
	break;

    case glAlignTop:
	ref->pos.y =
	    parent->refPos.y + parent->height - child->height -
	    borderWidth;
	ref->pos.x = parent->refPos.x;
	ref->width = parent->width - 2 * borderWidth;
	break;

    case glAlignBottom:
	ref->pos.y = parent->refPos.y + borderWidth;
	ref->pos.x = parent->refPos.x + borderWidth;
	ref->width = parent->width - 2 * borderWidth;
	break;
    case glAlignParent:
	ref->pos.y = parent->refPos.y + borderWidth;
	ref->pos.x = parent->refPos.x + borderWidth;;
	ref->width = parent->width - 2 * borderWidth;;
	ref->height = parent->height - 2 * borderWidth;
	break;
    case glAlignCenter:
    case glAlignNone:
	break;
    }
    if (child->align == glAlignNone)	// No alignment , chekc anchors
    {
	ref->pos.x = parent->refPos.x + child->pos.x + borderWidth;
	ref->pos.y = parent->refPos.y + child->pos.y + borderWidth;

	if (child->anchor.leftAnchor)
	    ref->pos.x =
		parent->refPos.x + child->anchor.left + borderWidth;
	if (child->anchor.bottomAnchor)
	    ref->pos.y =
		parent->refPos.y + child->anchor.bottom + borderWidth;

	if (child->anchor.topAnchor)
	    ref->height =
		parent->refPos.y + parent->height - ref->pos.y -
		child->anchor.top - borderWidth;
	if (child->anchor.rightAnchor)
	    ref->width =
		parent->refPos.x + parent->width - ref->pos.x -
		child->anchor.right - borderWidth;
    }
    child->refPos.x = ref->pos.x;
    child->refPos.y = ref->pos.y;
    child->width = ref->width;
    child->height = ref->height;
}

#if 0
// compiler reports this function is not used

static void glCompVertex(glCompPoint * p)
{
    glVertex3f(p->x, p->y, p->z);
}
#endif

static void glCompQuadVertex(glCompPoint * p0, glCompPoint * p1,
			     glCompPoint * p2, glCompPoint * p3)
{
    glVertex3f(p0->x, p0->y, p0->z);
    glVertex3f(p1->x, p1->y, p1->z);
    glVertex3f(p2->x, p2->y, p2->z);
    glVertex3f(p3->x, p3->y, p3->z);
}

void glCompSetColor(glCompColor * c)
{
    glColor4f(c->R, c->G, c->B, c->A);
}

void glCompDrawRectangle(glCompRect * r)
{
    glBegin(GL_QUADS);
    glVertex3f(r->pos.x, r->pos.y, r->pos.z);
    glVertex3f(r->pos.x + r->w, r->pos.y, r->pos.z);
    glVertex3f(r->pos.x + r->w, r->pos.y + r->h, r->pos.z);
    glVertex3f(r->pos.x, r->pos.y + r->h, r->pos.z);
    glEnd();
}
void glCompDrawRectPrism(glCompPoint * p, GLfloat w, GLfloat h, GLfloat b,
			 GLfloat d, glCompColor * c, int bumped)
{
    static GLfloat color_fac;
    static glCompPoint A, B, C, D, E, F, G, H;
    GLfloat dim = 1.00;
    if (!bumped) {
	color_fac = (GLfloat) 1.3;
	b = b - 2;
	dim = 0.5;
    } else
	color_fac = (GLfloat) 1 / (GLfloat) 1.3;


    A.x = p->x;
    A.y = p->y;
    A.z = p->z;
    B.x = p->x + w;
    B.y = p->y;
    B.z = p->z;
    C.x = p->x + w;
    C.y = p->y + h;
    C.z = p->z;
    D.x = p->x;
    D.y = p->y + h;
    D.z = p->z;
    G.x = p->x + b;
    G.y = p->y + b;
    G.z = p->z + d;
    H.x = p->x + w - b;
    H.y = p->y + b;
    H.z = p->z + d;
    E.x = p->x + b;
    E.y = p->y + h - b;
    E.z = p->z + d;
    F.x = p->x + w - b;
    F.y = p->y + h - b;
    F.z = p->z + d;
    glBegin(GL_QUADS);
    glColor4f(c->R * dim, c->G * dim, c->B * dim, c->A);
    glCompQuadVertex(&G, &H, &F, &E);

    glColor4f(c->R * color_fac * dim, c->G * color_fac * dim,
	      c->B * color_fac * dim, c->A);
    glCompQuadVertex(&A, &B, &H, &G);
    glCompQuadVertex(&B, &H, &F, &C);

    glColor4f(c->R / color_fac * dim, c->G / color_fac * dim,
	      c->B / color_fac * dim, c->A);
    glCompQuadVertex(&A, &G, &E, &D);
    glCompQuadVertex(&E, &F, &C, &D);
    glEnd();

}

void copy_glcomp_color(glCompColor * source, glCompColor * target)
{
    target->R = source->R;
    target->G = source->G;
    target->B = source->B;
    target->A = source->A;

}

#if 0
static double area2(glCompPoint * p1p, glCompPoint * p2p, glCompPoint * p3p) 
{
    double d;

    d = ((p1p->y - p2p->y) * (p3p->x - p2p->x)) -
        ((p3p->y - p2p->y) * (p1p->x - p2p->x));
    return d;
}


enum {ISCCW, ISON, ISCW};  /* counterclockwise; collinear; clockwise */ 
static int sideOf (glCompPoint * p1p, glCompPoint * p2p, glCompPoint * p3p) {
    double d = area2 (p1p,p2p,p3p);
    if (d < 0) return ISCCW;
    else if (d > 0) return ISCW;
    else return ISON;
}

int lines_intersect (glCompPoint* a, glCompPoint* b, glCompPoint* c, glCompPoint* d) 
{
  return ((sideOf(a,b,c) != sideOf(a,b,d)) && (sideOf(c,d,a) != sideOf(c,d,b))); 
}
#endif
GLfloat distBetweenPts(glCompPoint A,glCompPoint B,float R)
{
    GLfloat rv=0;	
    rv=(A.x-B.x)*(A.x-B.x) + (A.y-B.y)*(A.y-B.y) +(A.z-B.z)*(A.z-B.z);
    rv=sqrt(rv);
    if (rv <=R)
	return 0;
    return rv;
}

int is_point_in_rectangle(float X, float Y, float RX, float RY, float RW,float RH)
{
    if ((X >= RX) && (X <= (RX + RW)) && (Y >= RY) && (Y <= (RY + RH)))
	return 1;
    else
	return 0;
}






