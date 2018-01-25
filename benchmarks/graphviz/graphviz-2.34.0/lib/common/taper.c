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

/*
 * Tapered edges, based on lines.ps written by Denis Moskowitz.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>
#include <memory.h>
#include <logic.h>
#include <agxbuf.h>
#include <utils.h>

  /* sample point size; should be dynamic based on dpi or under user control */
#define BEZIERSUBDIVISION 20

  /* initial guess of array size */
#define INITSZ 2000

  /* convert degrees to radians and vice versa */
#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif
#define D2R(d)    (M_PI*(d)/180.0)
#define R2D(r)    (180.0*(r)/M_PI)

static double currentmiterlimit = 10.0;

#define moveto(p,x,y) addto(p,x,y)
#define lineto(p,x,y) addto(p,x,y)

static void addto (stroke_t* p, double x, double y)
{
    pointf pt;

    if (p->nvertices >= p->flags) {
	p->flags =+ INITSZ;
	p->vertices = RALLOC(p->flags,p->vertices,pointf);
    }
    pt.x = x;
    pt.y = y;
    p->vertices[p->nvertices++] = pt;
}

static void arcn (stroke_t* p, double x, double y, double r, double a1, double a2)
{
    double theta;
    int i;

    addto (p, x+r*cos(a1), y+r*sin(a1));
    if (r == 0) return;
    while (a2 > a1) a2 -= 2*M_PI;
    theta = a1 - a2; 
    while (theta > 2*M_PI) theta -= 2*M_PI;
    theta /= (BEZIERSUBDIVISION-1);
    for (i = 1; i < BEZIERSUBDIVISION; i++)
	addto (p, x+r*cos(a1-i*theta), y+r*sin(a1-i*theta));
}

#if 0
static void closepath (stroke_t* p)
{
    pointf pt = p->vertices[0];

    addto (p, pt.x, pt.y);
    if (p->flags > p->nvertices)
	p->vertices = RALLOC(p->nvertices,p->vertices,pointf);
}
#endif

/*
 * handle zeros
 */
static double myatan (double y, double x)
{
    double v;
    if ((x == 0) && (y == 0))
	return 0;
    else {
	v = atan2 (y, x);
	if (v >= 0) return v;
	else return (v + 2*M_PI);
    }
}

/*
 *  mod that accepts floats and makes negatives positive
 */
static double mymod (double original, double modulus)
{
    double v;
    if ((original < 0) || (original >= modulus)) {
	v = -floor(original/modulus);
	return ((v*modulus) + original);
    }
    return original;
}

/*
 * allow division by zero
 */
static double zdiv (double num, double denom)
{
    if (denom == 0) return 0;
    else return (num/denom);
}

typedef struct {
    double x;
    double y;
    double lengthsofar;
    char type;
    double dir;
    double lout;
    int bevel;
    double dir2;
} pathpoint;

typedef struct {
    pathpoint* pts;
    int cnt;
    int sz;
} vararr_t;


static vararr_t*
newArr ()
{
    vararr_t* arr = NEW(vararr_t);

    arr->cnt = 0;
    arr->sz = INITSZ;
    arr->pts = N_NEW(INITSZ,pathpoint);

    return arr;
}

static void
insertArr (vararr_t* arr, pointf p, double l)
{
    if (arr->cnt >= arr->sz) {
	arr->sz *= 2;
	arr->pts = RALLOC(arr->sz,arr->pts,pathpoint);
    }

    arr->pts[arr->cnt].x = p.x; 
    arr->pts[arr->cnt].y = p.y; 
    arr->pts[arr->cnt++].lengthsofar = l; 
}

#ifdef DEBUG
static void
printArr (vararr_t* arr, FILE* fp)
{
    int i;
    pathpoint pt;

    fprintf (fp, "cnt %d sz %d\n", arr->cnt, arr->sz);
    for (i = 0; i < arr->cnt; i++) {
	pt = arr->pts[i];
	fprintf (fp, "  [%d] x %.02f y  %.02f d %.02f\n", i, pt.x, pt.y, pt.lengthsofar);
    }
}
#endif

static void
fixArr (vararr_t* arr)
{
    if (arr->sz > arr->cnt)
	arr->pts = RALLOC(arr->cnt,arr->pts,pathpoint);
}

static void
freeArr (vararr_t* arr)
{
    free (arr->pts);
    free (arr);
}

static double l2dist (pointf p0, pointf p1)
{
    double delx = p0.x - p1.x;
    double dely = p0.y - p1.y;
    return sqrt(delx*delx + dely*dely);
}

/* analyze current path, creating pathpoints array
 * turn all curves into lines
 */
static vararr_t* pathtolines (bezier* bez, double initwid)
{
    int i, j, step;
    double seglen, linelen = 0;
    vararr_t* arr = newArr();
    pointf p0, p1, V[4];
    int n = bez->size;
    pointf* A = bez->list;

    insertArr (arr, A[0], 0);
    V[3] = A[0];
    for (i = 0; i + 3 < n; i += 3) {
	V[0] = V[3];
	for (j = 1; j <= 3; j++)
	    V[j] = A[i + j];
	p0 = V[0];
	for (step = 1; step <= BEZIERSUBDIVISION; step++) {
	    p1 = Bezier(V, 3, (double) step / BEZIERSUBDIVISION, NULL, NULL);
	    seglen = l2dist(p0, p1);
	    /* If initwid is large, this may never happen, so turn off. I assume this is to prevent
	     * too man points or too small a movement. Perhaps a better test can be made, but for now
	     * we turn it off. 
	     */
	    /* if (seglen > initwid/10) { */
		linelen += seglen;
		insertArr (arr, p1, linelen); 
	    /* } */
	    p0 = p1;
	}
    }
    fixArr (arr);
    /* printArr (arr, stderr); */
    return arr;
}

static void drawbevel(double x, double y, double lineout, int forward, double dir, double dir2, int linejoin, stroke_t* p)
{
    double a, a1, a2;

    if (forward) {
	a1 = dir;
	a2 = dir2;
    } else {
	a1 = dir2;
	a2 = dir;
    }
    if (linejoin == 1) {
	a = a1 - a2;
	if (a <= D2R(0.1)) a += D2R(360);
	if (a < D2R(180)) {
	    a1 = a + a2;
	    arcn (p,x,y,lineout,a1,a2);
	} else {
	    lineto (p, x + lineout*cos(a2), x + lineout*sin(a2));
	}
    } else {
	lineto (p, x + lineout*cos(a2), x + lineout*sin(a2));
    }
}

typedef double (*radfunc_t) (double curlen, double totallen, double initwid);

/* taper:
 * Given a B-spline bez, returns a polygon that represents spline as a tapered
 * edge, starting with width initwid.
 * The radfunc determines the half-width along the curve. Typically, this will
 * decrease from initwid to 0 as the curlen goes from 0 to totallen.
 * The linejoin and linecap parameters have roughly the same meaning as in postscript.
 *  - linejoin = 0 or 1 
 *  - linecap = 0 or 1 or 2 
 *
 * Calling function needs to free the allocated stoke_t.
 */
stroke_t* taper (bezier* bez, radfunc_t radfunc, double initwid, int linejoin, int linecap)
{
    int i, l, n;
    int pathcount, bevel;
    double direction, direction_2;
    vararr_t* arr = pathtolines (bez, initwid);
    pathpoint* pathpoints;
    pathpoint cur_point, last_point, next_point;
    double x, y, dist;
    double nx, ny, ndir;
    double lx, ly, ldir;
    double lineout, linerad, linelen;
    double theta, phi;
    stroke_t* p;

    pathcount = arr->cnt;
    pathpoints = arr->pts;
    linelen = pathpoints[pathcount-1].lengthsofar;

    /* determine miter and bevel points and directions */
    for (i = 0; i < pathcount; i++) {
	l = mymod(i-1,pathcount);
	n = mymod(i+1,pathcount);

	cur_point = pathpoints[i];
	x = cur_point.x;
	y = cur_point.y;
	dist = cur_point.lengthsofar;

	next_point = pathpoints[n];
	nx = next_point.x;
	ny = next_point.y;
	ndir = myatan (ny-y, nx-x);

	last_point = pathpoints[l];
	lx = last_point.x;
	ly = last_point.y;
	ldir = myatan (ly-y, lx-x);

	bevel = FALSE;
	direction_2 = 0;

	    /* effective line radius at this point */
	linerad = radfunc(dist, linelen, initwid);

 	if ((i == 0) || (i == pathcount-1)) {
	    lineout = linerad;
	    if (i == 0) {
		direction = ndir + D2R(90);
		if (linecap == 2) {
		    x -= cos(ndir)*lineout;
		    y -= sin(ndir)*lineout;
		}
	    } else {
		direction = ldir - D2R(90);
		if (linecap == 2) {
		    x -= cos(ldir)*lineout;
		    y -= sin(ldir)*lineout;
		}
	    }
	    direction_2 = direction;
	} else {
	    theta = ndir-ldir;
	    if (theta < 0) {
		theta += D2R(360);
	    }
	    phi = D2R(90)-(theta/2);
		 /* actual distance to junction point */
	    if (cos(phi) == 0) {
		lineout = 0;
	    } else {
		lineout = linerad/(cos(phi));
	    }
		 /* direction to junction point */
	    direction = ndir+D2R(90)+phi;
	    if ((0 != linejoin) || (zdiv(lineout,linerad) > currentmiterlimit)) {
		bevel = TRUE;
		lineout = linerad;
		direction = mymod(ldir-D2R(90),D2R(360));
		direction_2 = mymod(ndir+D2R(90),D2R(360));
		if (i == pathcount-1) {
		    bevel = FALSE;
		}
	    } else {
		direction_2 = direction;
	    }
	}
	pathpoints[i].x = x;
	pathpoints[i].y = y;
	pathpoints[i].lengthsofar = dist;
	pathpoints[i].type = 'l';
	pathpoints[i].dir = direction;
	pathpoints[i].lout = lineout;
	pathpoints[i].bevel = bevel;
	pathpoints[i].dir2 = direction_2;
    }

	 /* draw line */
    p = NEW(stroke_t);
	 /* side 1 */
    for (i = 0; i < pathcount; i++) {
	cur_point = pathpoints[i];
	x = cur_point.x;
	y = cur_point.y;
	direction = cur_point.dir;
	lineout = cur_point.lout;
	bevel = cur_point.bevel;
	direction_2 = cur_point.dir2;
	if (i == 0) {
	    moveto (p, x+cos(direction)*lineout, y+sin(direction)*lineout);
	} else {
	    lineto (p, x+cos(direction)*lineout, y+sin(direction)*lineout);
	}
	if (bevel) {
	    drawbevel (x, y, lineout, TRUE, direction, direction_2, linejoin, p);
	}
    }
	 /* end circle as needed */
    if (linecap == 1) {
	arcn (p, x,y,lineout,direction,direction+D2R(180));
    } else {
	direction += D2R(180);
	lineto (p, x+cos(direction)*lineout, y+sin(direction)*lineout);
    }
	 /* side 2 */
    for (i = pathcount-2; i >= 0; i--) {
	cur_point = pathpoints[i];
	x = cur_point.x;
	y = cur_point.y;
	direction = cur_point.dir + D2R(180);
	lineout = cur_point.lout;
	bevel = cur_point.bevel;
	direction_2 = cur_point.dir2 + D2R(180);
	lineto (p, x+cos(direction_2)*lineout, y+sin(direction_2)*lineout);
	if (bevel) { 
	    drawbevel (x, y, lineout, FALSE, direction, direction_2, linejoin, p);
	}
    }
	 /* start circle if needed */
    if (linecap == 1) {
	arcn (p, x,y,lineout,direction,direction+D2R(180));
    }
    /* closepath (p); */
    freeArr (arr);
    return p;
}

static double halffunc (double curlen, double totallen, double initwid)
{
    return ((1 - (curlen/totallen))*initwid/2.0);
}

stroke_t* taper0 (bezier* bez, double initwid)
{
    return taper(bez, halffunc, initwid, 0, 0);
}

#ifdef TEST
static pointf pts[] = {
  {100,100},
  {150,150},
  {200,100},
  {250,200},
};

main ()
{
    stroke_t* sp;
    bezier bez;
    int i;

    bez.size = sizeof(pts)/sizeof(pointf);
    bez.list = pts;
    sp = taper0 (&bez, 20);
    printf ("newpath\n");
    printf ("%.02f %.02f moveto\n", sp->vertices[0].x, sp->vertices[0].y);
    for (i=1; i<sp->nvertices; i++)
        printf ("%.02f %.02f lineto\n", sp->vertices[i].x, sp->vertices[i].y);
    printf ("fill showpage\n");
}
#endif
