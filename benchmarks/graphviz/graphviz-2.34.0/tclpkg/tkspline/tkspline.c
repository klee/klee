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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <tk.h>
#include <tkInt.h>

#ifndef CONST84
#define CONST84
#endif

/*
 * Prototypes for procedures defined in this file:
 */

static int SplineCurve _ANSI_ARGS_((Tk_Canvas canvas,
				    double *pointPtr, int numPoints,
				    int numSteps, XPoint xPoints[],
				    double dblPoints[]));

static void SplineCurvePostscript _ANSI_ARGS_((Tcl_Interp * interp,
					       Tk_Canvas canvas,
					       double *pointPtr,
					       int numPoints,
					       int numSteps));

#if TARGET_OS_MAC
/*
 * "export" is a MetroWerks specific pragma.  It flags the linker that  
 * any symbols that are defined when this pragma is on will be exported 
 * to shared libraries that link with this library.
 */
#   pragma export on
int Tkspline_Init(Tcl_Interp * interp);
int Tkspline_SafeInit(Tcl_Interp * interp);
#   pragma export reset

#   define VERSION "0.4"
#endif

int Tkspline_Init _ANSI_ARGS_((Tcl_Interp * interp));

int Tkspline_SafeInit _ANSI_ARGS_((Tcl_Interp * interp));

/* 
 * structure that is hooked in to describe the new smoothing method 
 */

static Tk_SmoothMethod splineSmoothMethod = {
    "spline",			/* used as in:  -smooth spline  */
    SplineCurve,		/* canvas curve generator */
    SplineCurvePostscript,	/* postscript curve generator */
};


/*
 *--------------------------------------------------------------
 *
 * SplineCurve --
 *
 *        Given a set of spline control points draw the Bezier splines. 
 *        There must be 3N+1 points for bezier splines, otherwise smoothing
 *        defaults to using the builtin algorithm.
 *        Produces output points in either of two    forms.
 *
 * Results:
 *        Either or both of the xPoints or dblPoints arrays are filled
 *        in.  The return value is the number of points placed in the
 *        arrays.
 *
 * Side effects:
 *        None.
 *
 *--------------------------------------------------------------
 */


static int
SplineCurve(canvas, pointPtr, numPoints, numSteps, xPoints, dblPoints)
Tk_Canvas canvas;		/* Canvas in which curve is to be
				 * drawn. */
double *pointPtr;		/* Array of input coordinates:  x0,
				 * y0, x1, y1, etc.. */
int numPoints;			/* Number of points at pointPtr. */
int numSteps;			/* Number of steps to use for each
				 * spline segments (determines
				 * smoothness of curve). */
XPoint xPoints[];		/* Array of XPoints to fill in (e.g.
				 * for display.  NULL means don't
				 * fill in any XPoints. */
double dblPoints[];		/* Array of points to fill in as
				 * doubles, in the form x0, y0,
				 * x1, y1, ....  NULL means don't
				 * fill in anything in this form. 
				 * Caller must make sure that this
				 * array has enough space. */
{
    int outputPoints, i;

    /* if the number of points is invalid, use the old function */
    if ((numPoints < 4) || (numPoints % 3 != 1)) {
	return TkMakeBezierCurve(canvas, pointPtr, numPoints, numSteps,
				 xPoints, dblPoints);
    }
    /* if pointPtr == NULL, just return a maximum for the number of
     * points to be calculated */
    if (!pointPtr) {
	return (1 + (numPoints / 3) * numSteps);
    }
    outputPoints = 0;
    if (xPoints != NULL) {
	Tk_CanvasDrawableCoords(canvas, pointPtr[0], pointPtr[1],
				&xPoints->x, &xPoints->y);
	xPoints += 1;
    }
    if (dblPoints != NULL) {
	dblPoints[0] = pointPtr[0];
	dblPoints[1] = pointPtr[1];
	dblPoints += 2;
    }
    outputPoints += 1;

    for (i = 2; i < numPoints; i += 3, pointPtr += 6) {
	if (xPoints != NULL) {
	    TkBezierScreenPoints(canvas, pointPtr, numSteps, xPoints);
	    xPoints += numSteps;
	}
	if (dblPoints != NULL) {
	    TkBezierPoints(pointPtr, numSteps, dblPoints);
	    dblPoints += 2 * numSteps;
	}
	outputPoints += numSteps;
    }
    return outputPoints;
}

/*
 *--------------------------------------------------------------
 *
 * SplineCurvePostscript --
 *
 *    This procedure generates Postscript commands that create
 *    a path corresponding to a given Bezier curve.
 *
 * Results:
 *    None.  Postscript commands to generate the path are appended
 *    to interp->result.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */

static void
SplineCurvePostscript(interp, canvas, pointPtr, numPoints, numSteps)
Tcl_Interp *interp;		/* Interpreter in whose result the
				 * Postscript is to be stored.  */
Tk_Canvas canvas;		/* Canvas widget for which the
				 * Postscript is being generated. */
double *pointPtr;		/* Array of input coordinates: x0,
				 * y0, x1, y1, etc.. */
int numPoints;			/* Number of points at pointPtr. */
int numSteps;			/* Not Used */
{
    int i;
    char buffer[200];

    /* if the number of points is invalid, use the old function */
    if ((numPoints < 4) || (numPoints % 3 != 1)) {
#if (TK_MAJOR_VERSION < 8) || ((TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION < 3))
	TkMakeBezierPostscript(interp, canvas, pointPtr, numPoints,
			       numSteps);
#else
	TkMakeBezierPostscript(interp, canvas, pointPtr, numPoints);
#endif
	return;
    }

    sprintf(buffer, "%.15g %.15g moveto\n",
	    pointPtr[0], Tk_CanvasPsY(canvas, pointPtr[1]));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    /*
     * Cycle through all the remaining points in the curve, generating
     * a curve section for each vertex in the linear path.
     */

    for (i = numPoints - 2, pointPtr += 2; i > 0; i -= 3, pointPtr += 6) {
	sprintf(buffer, "%.15g %.15g %.15g %.15g %.15g %.15g curveto\n",
		pointPtr[0], Tk_CanvasPsY(canvas, pointPtr[1]),
		pointPtr[2], Tk_CanvasPsY(canvas, pointPtr[3]),
		pointPtr[4], Tk_CanvasPsY(canvas, pointPtr[5]));
	Tcl_AppendResult(interp, buffer, (char *) NULL);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tkspline_Init -- Function to call on loading the Tkspline DLL
 *
 * Results: Returns TCL_OK, or TCL_ERROR if unable to load.
 *
 * Side effects: Package Tkspline added to package list
 *         lines and polygons have a new "spline" smooth method.
 *
 *--------------------------------------------------------------
 */

int Tkspline_Init(interp)
Tcl_Interp *interp;
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tk_InitStubs(interp, TK_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
#else
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_PkgRequire(interp, "Tk", TK_VERSION, 0) != TCL_OK) {
	return TCL_ERROR;
    }
#endif

    Tk_CreateSmoothMethod(interp, &splineSmoothMethod);

    return Tcl_PkgProvide(interp, "Tkspline", VERSION);
}

int Tkspline_SafeInit(Tcl_Interp * interp)
{
    return Tkspline_Init(interp);
}
