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

/*
 * Tcl binding to drive Stephen North's and
 * Emden Gansner's shortest path code.
 *
 * ellson@graphviz.org   October 2nd, 1996
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* avoid compiler warnings with template changes in Tcl8.4 */
/*    specifically just the change to Tcl_CmdProc */
#define USE_NON_CONST

/* for sincos */
#define _GNU_SOURCE 1

#ifdef HAVE_AST
#include                <ast.h>
#include                <vmalloc.h>
#else
#include                <sys/types.h>
#include                <stdlib.h>
#include                <string.h>
#include                <unistd.h>
#endif

#include <assert.h>
#include <math.h>
#include <pathutil.h>
#include <vispath.h>
#include <tri.h>
#include <tcl.h>
#include "tclhandle.h"

#ifndef CONST84
#define CONST84
#endif

#if ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 6)) || ( TCL_MAJOR_VERSION > 8)
#else
#ifndef Tcl_GetStringResult
#define Tcl_GetStringResult(interp) interp->result
#endif
#endif

typedef Ppoint_t point;

typedef struct poly_s {
    int id;
    Ppoly_t boundary;
} poly;

typedef struct vgpane_s {
    int Npoly;			/* number of polygons */
    poly *poly;			/* set of polygons */
    int N_poly_alloc;		/* for allocation */
    vconfig_t *vc;		/* visibility graph handle */
    Tcl_Interp *interp;		/* interpreter that owns the binding */
    char *triangle_cmd;		/* why is this here any more */
} vgpane_t;

#ifdef HAVE_SINCOS
extern void sincos(double x, double *s, double *c);
#else
# define sincos(x,s,c) *s = sin(x); *c = cos(x)
#endif

tblHeader_pt vgpaneTable;

extern void make_CW(Ppoly_t * poly);
extern int Plegal_arrangement(Ppoly_t ** polys, int n_polys);

static int polyid = 0;		/* unique and unchanging id for each poly */

static poly *allocpoly(vgpane_t * vgp, int id, int npts)
{
    poly *rv;
    if (vgp->Npoly >= vgp->N_poly_alloc) {
	vgp->N_poly_alloc *= 2;
	vgp->poly = realloc(vgp->poly, vgp->N_poly_alloc * sizeof(poly));
    }
    rv = &(vgp->poly[vgp->Npoly++]);
    rv->id = id;
    rv->boundary.pn = 0;
    rv->boundary.ps = malloc(npts * sizeof(point));
    return rv;
}

static void vc_stale(vgpane_t * vgp)
{
    if (vgp->vc) {
	Pobsclose(vgp->vc);
	vgp->vc = (vconfig_t *) 0;
    }
}

static int vc_refresh(vgpane_t * vgp)
{
    int i;
    Ppoly_t **obs;

    if (vgp->vc == (vconfig_t *) 0) {
	obs = malloc(vgp->Npoly * sizeof(Ppoly_t));
	for (i = 0; i < vgp->Npoly; i++)
	    obs[i] = &(vgp->poly[i].boundary);
	if (NOT(Plegal_arrangement(obs, vgp->Npoly)))
	    fprintf(stderr, "bad arrangement\n");
	else
	    vgp->vc = Pobsopen(obs, vgp->Npoly);
	free(obs);
    }
    return (vgp->vc != 0);
}

static void dgsprintxy(Tcl_DString * result, int npts, point p[])
{
    int i;
    char buf[20];

    if (npts != 1)
	Tcl_DStringStartSublist(result);
    for (i = 0; i < npts; i++) {
	sprintf(buf, "%g", p[i].x);
	Tcl_DStringAppendElement(result, buf);
	sprintf(buf, "%g", p[i].y);
	Tcl_DStringAppendElement(result, buf);
    }
    if (npts != 1)
	Tcl_DStringEndSublist(result);
}

static void expandPercentsEval(Tcl_Interp * interp,	/* interpreter context */
			       register char *before,	/* Command with percent expressions */
			       char *r,	/* vgpaneHandle string to substitute for "%r" */
			       int npts,	/* number of coordinates */
			       point * ppos	/* Cordinates to substitute for %t */
    )
{
    register char *string;
    Tcl_DString scripts;

    Tcl_DStringInit(&scripts);
    while (1) {
	/*
	 * Find everything up to the next % character and append it to the
	 * result string.
	 */

	for (string = before; (*string != 0) && (*string != '%'); string++) {
	    /* Empty loop body. */
	}
	if (string != before) {
	    Tcl_DStringAppend(&scripts, before, string - before);
	    before = string;
	}
	if (*before == 0) {
	    break;
	}
	/*
	 * There's a percent sequence here.  Process it.
	 */

	switch (before[1]) {
	case 'r':
	    Tcl_DStringAppend(&scripts, r, strlen(r));	/* vgcanvasHandle */
	    break;
	case 't':
	    dgsprintxy(&scripts, npts, ppos);
	    break;
	default:
	    Tcl_DStringAppend(&scripts, before + 1, 1);
	    break;
	}
	before += 2;
    }
    if (Tcl_GlobalEval(interp, Tcl_DStringValue(&scripts)) != TCL_OK)
	fprintf(stderr, "%s while in binding: %s\n\n",
		Tcl_GetStringResult(interp), Tcl_DStringValue(&scripts));
    Tcl_DStringFree(&scripts);
}

void triangle_callback(void *vgparg, point pqr[])
{
    char vbuf[20];
    vgpane_t *vgp;

    vgp = vgparg;

/*	    TBL_ENTRY((tblHeader_pt)vgpaneTable, (ubyte_pt)vgp));*/

    if (vgp->triangle_cmd) {
	sprintf(vbuf, "vgpane%lu",
		(unsigned long) (((ubyte_pt) vgp - (vgpaneTable->bodyPtr))
				 / (vgpaneTable->entrySize)));
	expandPercentsEval(vgp->interp, vgp->triangle_cmd, vbuf, 3, pqr);
    }
}

static char *buildBindings(char *s1, char *s2)
/*
 * previous binding in s1 binding to be added in s2 result in s3
 *
 * if s2 begins with + then append (separated by \n) else s2 replaces if
 * resultant string is null then bindings are deleted
 */
{
    char *s3;
    int l;

    if (s2[0] == '+') {
	if (s1) {
	    l = strlen(s2) - 1;
	    if (l) {
		s3 = malloc(strlen(s1) + l + 2);
		strcpy(s3, s1);
		strcat(s3, "\n");
		strcat(s3, s2 + 1);
		free(s1);
	    } else {
		s3 = s1;
	    }
	} else {
	    l = strlen(s2) - 1;
	    if (l) {
		s3 = malloc(l + 2);
		strcpy(s3, s2 + 1);
	    } else {
		s3 = (char *) NULL;
	    }
	}
    } else {
	if (s1)
	    free(s1);
	l = strlen(s2);
	if (l) {
	    s3 = malloc(l + 2);
	    strcpy(s3, s2);
	} else {
	    s3 = (char *) NULL;
	}
    }
    return s3;
}



/* convert x and y string args to point */
static int scanpoint(Tcl_Interp * interp, char *argv[], point * p)
{
    if (sscanf(argv[0], "%lg", &(p->x)) != 1) {
	Tcl_AppendResult(interp, "invalid x coordinate: \"", argv[0],
			 "\"", (char *) NULL);
	return TCL_ERROR;
    }
    if (sscanf(argv[1], "%lg", &(p->y)) != 1) {
	Tcl_AppendResult(interp, "invalid y coordinate: \"", argv[1],
			 "\"", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static point center(point vertex[], int n)
{
    int i;
    point c;

    c.x = c.y = 0;
    for (i = 0; i < n; i++) {
	c.x += vertex[i].x;
	c.y += vertex[i].y;
    }
    c.x /= n;
    c.y /= n;
    return c;
}

static double distance(point p, point q)
{
    double dx, dy;

    dx = p.x - q.x;
    dy = p.y - q.y;
    return sqrt(dx * dx + dy * dy);
}

static point rotate(point c, point p, double alpha)
{
    point q;
    double beta, r, sina, cosa;

    r = distance(c, p);
    beta = atan2(p.x - c.x, p.y - c.y);
    sincos(beta + alpha, &sina, &cosa);
    q.x = c.x + r * sina;
    q.y = c.y - r * cosa;	/* adjust for tk y-down */
    return q;
}

static point scale(point c, point p, double gain)
{
    point q;

    q.x = c.x + gain * (p.x - c.x);
    q.y = c.y + gain * (p.y - c.y);
    return q;
}

static int remove_poly(vgpane_t * vgp, int polyid)
{
    int i, j;

    for (i = 0; i < vgp->Npoly; i++) {
	if (vgp->poly[i].id == polyid) {
	    free(vgp->poly[i].boundary.ps);
	    for (j = i++; i < vgp->Npoly; i++, j++) {
		vgp->poly[j] = vgp->poly[i];
	    }
	    vgp->Npoly -= 1;
	    vc_stale(vgp);
	    return TRUE;
	}
    }
    return FALSE;
}

static int
insert_poly(Tcl_Interp * interp, vgpane_t * vgp, int polyid, char *vargv[],
	    int vargc)
{
    poly *np;
    int i, result;

    np = allocpoly(vgp, polyid, vargc);
    for (i = 0; i < vargc; i += 2) {
	result =
	    scanpoint(interp, &vargv[i],
		      &(np->boundary.ps[np->boundary.pn]));
	if (result != TCL_OK)
	    return result;
	np->boundary.pn++;
    }
    make_CW(&(np->boundary));
    vc_stale(vgp);
    return TCL_OK;
}

static void
make_barriers(vgpane_t * vgp, int pp, int qp, Pedge_t ** barriers,
	      int *n_barriers)
{
    int i, j, k, n, b;
    Pedge_t *bar;

    n = 0;
    for (i = 0; i < vgp->Npoly; i++) {
	if (vgp->poly[i].id == pp)
	    continue;
	if (vgp->poly[i].id == qp)
	    continue;
	n = n + vgp->poly[i].boundary.pn;
    }
    bar = malloc(n * sizeof(Pedge_t));
    b = 0;
    for (i = 0; i < vgp->Npoly; i++) {
	if (vgp->poly[i].id == pp)
	    continue;
	if (vgp->poly[i].id == qp)
	    continue;
	for (j = 0; j < vgp->poly[i].boundary.pn; j++) {
	    k = j + 1;
	    if (k >= vgp->poly[i].boundary.pn)
		k = 0;
	    bar[b].a = vgp->poly[i].boundary.ps[j];
	    bar[b].b = vgp->poly[i].boundary.ps[k];
	    b++;
	}
    }
    assert(b == n);
    *barriers = bar;
    *n_barriers = n;
}

/* append the x and y coordinates of a point to the Tcl result */
static void appendpoint(Tcl_Interp * interp, point p)
{
    char buf[30];

    sprintf(buf, "%g", p.x);
    Tcl_AppendElement(interp, buf);
    sprintf(buf, "%g", p.y);
    Tcl_AppendElement(interp, buf);
}

/* process vgpane methods */
static int
vgpanecmd(ClientData clientData, Tcl_Interp * interp, int argc,
	  char *argv[])
{
    int vargc, length, i, j, n, result;
    char c, *s, **vargv, vbuf[30];
    vgpane_t *vgp, **vgpp;
    point p, q, *ps;
    poly *tpp;
    double alpha, gain;
    Pvector_t slopes[2];
    Ppolyline_t line, spline;
    int pp, qp;			/* polygon indices for p, q */
    Pedge_t *barriers;
    int n_barriers;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
			 " ", argv[0], " method ?arg arg ...?\"",
			 (char *) NULL);
	return TCL_ERROR;
    }
    if (!(vgpp = (vgpane_t **) tclhandleXlate(vgpaneTable, argv[0]))) {
	Tcl_AppendResult(interp, "Invalid handle: \"", argv[0],
			 "\"", (char *) NULL);
	return TCL_ERROR;
    }
    vgp = *vgpp;

    c = argv[1][0];
    length = strlen(argv[1]);

    if ((c == 'c') && (strncmp(argv[1], "coords", length) == 0)) {
	if ((argc < 3)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " ", argv[1], " id ?x1 y1 x2 y2...?\"",
			     (char *) NULL);
	    return TCL_ERROR;
	}
	if (sscanf(argv[2], "%d", &polyid) != 1) {
	    Tcl_AppendResult(interp, "not an integer: ", argv[2],
			     (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    /* find poly and return its coordinates */
	    for (i = 0; i < vgp->Npoly; i++) {
		if (vgp->poly[i].id == polyid) {
		    n = vgp->poly[i].boundary.pn;
		    for (j = 0; j < n; j++) {
			appendpoint(interp, vgp->poly[i].boundary.ps[j]);
		    }
		    return TCL_OK;
		}
	    }
	    Tcl_AppendResult(interp, " no such polygon: ", argv[2],
			     (char *) NULL);
	    return TCL_ERROR;
	}
	/* accept either inline or delimited list */
	if ((argc == 4)) {
	    result =
		Tcl_SplitList(interp, argv[3], &vargc,
			      (CONST84 char ***) &vargv);
	    if (result != TCL_OK) {
		return result;
	    }
	} else {
	    vargc = argc - 3;
	    vargv = &argv[3];
	}
	if (!vargc || vargc % 2) {
	    Tcl_AppendResult(interp,
			     "There must be a multiple of two terms in the list.",
			     (char *) NULL);
	    return TCL_ERROR;
	}

	/* remove old poly, add modified polygon to the end with 
	   the same id as the original */

	if (!(remove_poly(vgp, polyid))) {
	    Tcl_AppendResult(interp, " no such polygon: ", argv[2],
			     (char *) NULL);
	    return TCL_ERROR;
	}

	return (insert_poly(interp, vgp, polyid, vargv, vargc));

    } else if ((c == 'd') && (strncmp(argv[1], "debug", length) == 0)) {
	/* debug only */
	printf("debug output goes here\n");
	return TCL_OK;

    } else if ((c == 'd') && (strncmp(argv[1], "delete", length) == 0)) {
	/* delete a vgpane and all memory associated with it */
	if (vgp->vc)
	    Pobsclose(vgp->vc);
	free(vgp->poly);	/* ### */
	Tcl_DeleteCommand(interp, argv[0]);
	free((char *) tclhandleFree(vgpaneTable, argv[0]));
	return TCL_OK;

    } else if ((c == 'f') && (strncmp(argv[1], "find", length) == 0)) {
	/* find the polygon that the point is inside and return it
	   id, or null */
	if ((argc < 3)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " ", argv[1], " x y\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    result =
		Tcl_SplitList(interp, argv[2], &vargc,
			      (CONST84 char ***) &vargv);
	    if (result != TCL_OK) {
		return result;
	    }
	} else {
	    vargc = argc - 2;
	    vargv = &argv[2];
	}
	result = scanpoint(interp, &vargv[0], &p);
	if (result != TCL_OK)
	    return result;

	/* determine the polygons (if any) that contain the point */
	for (i = 0; i < vgp->Npoly; i++) {
	    if (in_poly(vgp->poly[i].boundary, p)) {
		sprintf(vbuf, "%d", vgp->poly[i].id);
		Tcl_AppendElement(interp, vbuf);
	    }
	}
	return TCL_OK;

    } else if ((c == 'i') && (strncmp(argv[1], "insert", length) == 0)) {
	/* add poly to end poly list, and it coordinates to the end of 
	   the point list */
	if ((argc < 3)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " ", argv[1], " x1 y1 x2 y2 ...\"",
			     (char *) NULL);
	    return TCL_ERROR;
	}
	/* accept either inline or delimited list */
	if ((argc == 3)) {
	    result =
		Tcl_SplitList(interp, argv[2], &vargc,
			      (CONST84 char ***) &vargv);
	    if (result != TCL_OK) {
		return result;
	    }
	} else {
	    vargc = argc - 2;
	    vargv = &argv[2];
	}

	if (!vargc || vargc % 2) {
	    Tcl_AppendResult(interp,
			     "There must be a multiple of two terms in the list.",
			     (char *) NULL);
	    return TCL_ERROR;
	}

	polyid++;

	result = insert_poly(interp, vgp, polyid, vargv, vargc);
	if (result != TCL_OK)
	    return result;

	sprintf(vbuf, "%d", polyid);
	Tcl_AppendResult(interp, vbuf, (char *) NULL);
	return TCL_OK;

    } else if ((c == 'l') && (strncmp(argv[1], "list", length) == 0)) {
	/* return list of polygon ids */
	for (i = 0; i < vgp->Npoly; i++) {
	    sprintf(vbuf, "%d", vgp->poly[i].id);
	    Tcl_AppendElement(interp, vbuf);
	}
	return TCL_OK;

    } else if ((c == 'p') && (strncmp(argv[1], "path", length) == 0)) {
	/* return a list of points corresponding to the shortest path
	   that does not cross the remaining "visible" polygons. */
	if ((argc < 3)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " ", argv[1], " x1 y1 x2 y2\"",
			     (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    result =
		Tcl_SplitList(interp, argv[2], &vargc,
			      (CONST84 char ***) &vargv);
	    if (result != TCL_OK) {
		return result;
	    }
	} else {
	    vargc = argc - 2;
	    vargv = &argv[2];
	}
	if ((vargc < 4)) {
	    Tcl_AppendResult(interp,
			     "invalid points: should be: \"x1 y1 x2 y2\"",
			     (char *) NULL);
	    return TCL_ERROR;
	}
	result = scanpoint(interp, &vargv[0], &p);
	if (result != TCL_OK)
	    return result;
	result = scanpoint(interp, &vargv[2], &q);
	if (result != TCL_OK)
	    return result;

	/* only recompute the visibility graph if we have to */
	if ((vc_refresh(vgp))) {
	    Pobspath(vgp->vc, p, POLYID_UNKNOWN, q, POLYID_UNKNOWN, &line);

	    for (i = 0; i < line.pn; i++) {
		appendpoint(interp, line.ps[i]);
	    }
	}

	return TCL_OK;

    } else if ((c == 'b') && (strncmp(argv[1], "bind", length) == 0)) {
	if ((argc < 2) || (argc > 4)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
			     argv[0], " bind triangle ?command?\"",
			     (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 2) {
	    Tcl_AppendElement(interp, "triangle");
	    return TCL_OK;
	}
	length = strlen(argv[2]);
	if (strncmp(argv[2], "triangle", length) == 0) {
	    s = vgp->triangle_cmd;
	    if (argc == 4)
		vgp->triangle_cmd = s = buildBindings(s, argv[3]);
	} else {
	    Tcl_AppendResult(interp, "unknown event \"", argv[2],
			     "\": must be one of:\n\ttriangle.",
			     (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3)
	    Tcl_AppendResult(interp, s, (char *) NULL);
	return TCL_OK;

    } else if ((c == 'b') && (strncmp(argv[1], "bpath", length) == 0)) {
	/* return a list of points corresponding to the shortest path
	   that does not cross the remaining "visible" polygons. */
	if ((argc < 3)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " ", argv[1], " x1 y1 x2 y2\"",
			     (char *) NULL);
	    return TCL_ERROR;
	}
	if (argc == 3) {
	    result =
		Tcl_SplitList(interp, argv[2], &vargc,
			      (CONST84 char ***) &vargv);
	    if (result != TCL_OK) {
		return result;
	    }
	} else {
	    vargc = argc - 2;
	    vargv = &argv[2];
	}
	if ((vargc < 4)) {
	    Tcl_AppendResult(interp,
			     "invalid points: should be: \"x1 y1 x2 y2\"",
			     (char *) NULL);
	    return TCL_ERROR;
	}

	result = scanpoint(interp, &vargv[0], &p);
	if (result != TCL_OK)
	    return result;
	result = scanpoint(interp, &vargv[2], &q);
	if (result != TCL_OK)
	    return result;

	/* determine the polygons (if any) that contain the endpoints */
	pp = qp = POLYID_NONE;
	for (i = 0; i < vgp->Npoly; i++) {
	    tpp = &(vgp->poly[i]);
	    if ((pp == POLYID_NONE) && in_poly(tpp->boundary, p))
		pp = i;
	    if ((qp == POLYID_NONE) && in_poly(tpp->boundary, q))
		qp = i;
	}

	if (vc_refresh(vgp)) {
	    /*Pobspath(vgp->vc, p, pp, q, qp, &line); */
	    Pobspath(vgp->vc, p, POLYID_UNKNOWN, q, POLYID_UNKNOWN, &line);
	    make_barriers(vgp, pp, qp, &barriers, &n_barriers);
	    slopes[0].x = slopes[0].y = 0.0;
	    slopes[1].x = slopes[1].y = 0.0;
	    Proutespline(barriers, n_barriers, line, slopes, &spline);

	    for (i = 0; i < spline.pn; i++) {
		appendpoint(interp, spline.ps[i]);
	    }
	}
	return TCL_OK;

    } else if ((c == 'b') && (strncmp(argv[1], "bbox", length) == 0)) {
	if ((argc < 3)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " ", argv[1], " id\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (sscanf(argv[2], "%d", &polyid) != 1) {
	    Tcl_AppendResult(interp, "not an integer: ", argv[2],
			     (char *) NULL);
	    return TCL_ERROR;
	}
	for (i = 0; i < vgp->Npoly; i++) {
	    if (vgp->poly[i].id == polyid) {
		Ppoly_t pp = vgp->poly[i].boundary;
		point LL, UR;
		LL = UR = pp.ps[0];
		for (j = 1; j < pp.pn; j++) {
		    p = pp.ps[j];
		    if (p.x > UR.x)
			UR.x = p.x;
		    if (p.y > UR.y)
			UR.y = p.y;
		    if (p.x < LL.x)
			LL.x = p.x;
		    if (p.y < LL.y)
			LL.y = p.y;
		}
		appendpoint(interp, LL);
		appendpoint(interp, UR);
		return TCL_OK;
	    }
	}
	Tcl_AppendResult(interp, " no such polygon: ", argv[2],
			 (char *) NULL);
	return TCL_ERROR;

    } else if ((c == 'c') && (strncmp(argv[1], "center", length) == 0)) {
	if ((argc < 3)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " ", argv[1], " id\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (sscanf(argv[2], "%d", &polyid) != 1) {
	    Tcl_AppendResult(interp, "not an integer: ", argv[2],
			     (char *) NULL);
	    return TCL_ERROR;
	}
	for (i = 0; i < vgp->Npoly; i++) {
	    if (vgp->poly[i].id == polyid) {
		appendpoint(interp, center(vgp->poly[i].boundary.ps,
					   vgp->poly[i].boundary.pn));
		return TCL_OK;
	    }
	}
	Tcl_AppendResult(interp, " no such polygon: ", argv[2],
			 (char *) NULL);
	return TCL_ERROR;

    } else if ((c == 't')
	       && (strncmp(argv[1], "triangulate", length) == 0)) {
	if ((argc < 2)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " id ", (char *) NULL);
	    return TCL_ERROR;
	}

	if (sscanf(argv[2], "%d", &polyid) != 1) {
	    Tcl_AppendResult(interp, "not an integer: ", argv[2],
			     (char *) NULL);
	    return TCL_ERROR;
	}

	for (i = 0; i < vgp->Npoly; i++) {
	    if (vgp->poly[i].id == polyid) {
		Ptriangulate(&(vgp->poly[i].boundary), triangle_callback,
			     vgp);
		return TCL_OK;
	    }
	}
	Tcl_AppendResult(interp, " no such polygon: ", argv[2],
			 (char *) NULL);
	return TCL_ERROR;
    } else if ((c == 'r') && (strncmp(argv[1], "rotate", length) == 0)) {
	if ((argc < 4)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " ", argv[1], " id alpha\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (sscanf(argv[2], "%d", &polyid) != 1) {
	    Tcl_AppendResult(interp, "not an integer: ", argv[2],
			     (char *) NULL);
	    return TCL_ERROR;
	}
	if (sscanf(argv[3], "%lg", &alpha) != 1) {
	    Tcl_AppendResult(interp, "not an angle in radians: ", argv[3],
			     (char *) NULL);
	    return TCL_ERROR;
	}
	for (i = 0; i < vgp->Npoly; i++) {
	    if (vgp->poly[i].id == polyid) {
		n = vgp->poly[i].boundary.pn;
		ps = vgp->poly[i].boundary.ps;
		p = center(ps, n);
		for (j = 0; j < n; j++) {
		    appendpoint(interp, rotate(p, ps[j], alpha));
		}
		return TCL_OK;
	    }
	}
	Tcl_AppendResult(interp, " no such polygon: ", argv[2],
			 (char *) NULL);
	return TCL_ERROR;

    } else if ((c == 's') && (strncmp(argv[1], "scale", length) == 0)) {
	if ((argc < 4)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " ", argv[1], " id gain\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (sscanf(argv[2], "%d", &polyid) != 1) {
	    Tcl_AppendResult(interp, "not an integer: ", argv[2],
			     (char *) NULL);
	    return TCL_ERROR;
	}
	if (sscanf(argv[3], "%lg", &gain) != 1) {
	    Tcl_AppendResult(interp, "not a number: ", argv[3],
			     (char *) NULL);
	    return TCL_ERROR;
	}
	for (i = 0; i < vgp->Npoly; i++) {
	    if (vgp->poly[i].id == polyid) {
		n = vgp->poly[i].boundary.pn;
		ps = vgp->poly[i].boundary.ps;
		for (j = 0; j < n; j++) {
		    appendpoint(interp, scale(p, ps[j], gain));
		}
		return TCL_OK;
	    }
	}
	Tcl_AppendResult(interp, " no such polygon: ", argv[2],
			 (char *) NULL);
	return TCL_ERROR;

    } else if ((c == 'r') && (strncmp(argv[1], "remove", length) == 0)) {
	if ((argc < 3)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " ", argv[1], " id\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (sscanf(argv[2], "%d", &polyid) != 1) {
	    Tcl_AppendResult(interp, "not an integer: ", argv[2],
			     (char *) NULL);
	    return TCL_ERROR;
	}

	if (remove_poly(vgp, polyid))
	    return TCL_OK;

	Tcl_AppendResult(interp, " no such polygon: ", argv[2],
			 (char *) NULL);
	return TCL_ERROR;
    }

    Tcl_AppendResult(interp, "bad method \"", argv[1],
		     "\" must be one of:",
		     "\n\tbbox, bind, bpath, center, coords, delete, find,",
		     "\n\tinsert, list, path, remove, rotate, scale, triangulate.",
		     (char *) NULL);
    return TCL_ERROR;
}

static int
vgpane(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
{
    char vbuf[30];
    vgpane_t *vgp;

    vgp = (vgpane_t *) malloc(sizeof(vgpane_t));
    *(vgpane_t **) tclhandleAlloc(vgpaneTable, vbuf, NULL) = vgp;

    vgp->vc = (vconfig_t *) 0;
    vgp->Npoly = 0;
    vgp->N_poly_alloc = 250;
    vgp->poly = malloc(vgp->N_poly_alloc * sizeof(poly));
    vgp->interp = interp;
    vgp->triangle_cmd = (char *) NULL;

    Tcl_CreateCommand(interp, vbuf, vgpanecmd,
		      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_AppendResult(interp, vbuf, (char *) NULL);
    return TCL_OK;
}

int Tclpathplan_Init(Tcl_Interp * interp)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
#else
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
#endif
    if (Tcl_PkgProvide(interp, "Tclpathplan", VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_CreateCommand(interp, "vgpane", vgpane,
		      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    vgpaneTable = tclhandleInit("vgpane", sizeof(vgpane_t), 10);

    return TCL_OK;
}

int Tclpathplan_SafeInit(Tcl_Interp * interp)
{
    return Tclpathplan_Init(interp);
}
