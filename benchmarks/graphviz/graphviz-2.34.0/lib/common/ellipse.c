/* $id: shapes.c,v 1.82 2007/12/24 04:50:36 ellson Exp $ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2012 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

/* This code is derived from the Java implementation by Luc Maisonobe */
/* Copyright (c) 2003-2004, Luc Maisonobe
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that
 * the following conditions are met:
 *
 *    Redistributions of source code must retain the
 *    above copyright notice, this list of conditions and
 *    the following disclaimer.
 *    Redistributions in binary form must reproduce the
 *    above copyright notice, this list of conditions and
 *    the following disclaimer in the documentation
 *    and/or other materials provided with the
 *    distribution.
 *    Neither the names of spaceroots.org, spaceroots.com
 *    nor the names of their contributors may be used to
 *    endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if STANDALONE
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX(a,b)        ((a)>(b)?(a):(b))
#define MIN(a,b)        ((a)<(b)?(a):(b))

#define NEW(t) ((t*)calloc(1,sizeof(t)))
#define N_NEW(n,t) ((t*)calloc(n,sizeof(t)))

#define PI            3.14159265358979323846

#define TRUE 1
#define FALSE 0
typedef unsigned char boolean;

typedef struct pointf_s {
    double x, y;
} pointf;
typedef struct Ppoly_t {
    pointf *ps;
    int pn;
} Ppoly_t;

typedef Ppoly_t Ppolyline_t;
#else
#include "render.h"
#include "pathplan.h"
#endif

#define TWOPI (2*M_PI)

typedef struct {
    double cx, cy;		/* center */
    double a, b;		/* semi-major and -minor axes */

  /* Orientation of the major axis with respect to the x axis. */
    double theta, cosTheta, sinTheta;

  /* Start and end angles of the arc. */
    double eta1, eta2;

  /* Position of the start and end points. */
    double x1, y1, x2, y2;

  /* Position of the foci. */
    double xF1, yF1,  xF2, yF2;

  /* x of the leftmost point of the arc. */
    double xLeft;

  /* y of the highest point of the arc. */
    double yUp;

  /* Horizontal width and vertical height of the arc. */
    double width, height;

    double f, e2, g, g2;
} ellipse_t;

static void computeFoci(ellipse_t * ep)
{
    double d = sqrt(ep->a * ep->a - ep->b * ep->b);
    double dx = d * ep->cosTheta;
    double dy = d * ep->sinTheta;

    ep->xF1 = ep->cx - dx;
    ep->yF1 = ep->cy - dy;
    ep->xF2 = ep->cx + dx;
    ep->yF2 = ep->cy + dy;
}

  /* Compute the locations of the endpoints. */
static void computeEndPoints(ellipse_t * ep)
{
    double aCosEta1 = ep->a * cos(ep->eta1);
    double bSinEta1 = ep->b * sin(ep->eta1);
    double aCosEta2 = ep->a * cos(ep->eta2);
    double bSinEta2 = ep->b * sin(ep->eta2);

    // start point
    ep->x1 = ep->cx + aCosEta1 * ep->cosTheta - bSinEta1 * ep->sinTheta;
    ep->y1 = ep->cy + aCosEta1 * ep->sinTheta + bSinEta1 * ep->cosTheta;

    // end point
    ep->x2 = ep->cx + aCosEta2 * ep->cosTheta - bSinEta2 * ep->sinTheta;
    ep->y2 = ep->cy + aCosEta2 * ep->sinTheta + bSinEta2 * ep->cosTheta;
}

  /* Compute the bounding box. */
static void computeBounds(ellipse_t * ep)
{
    double bOnA = ep->b / ep->a;
    double etaXMin, etaXMax, etaYMin, etaYMax;

    if (abs(ep->sinTheta) < 0.1) {
	double tanTheta = ep->sinTheta / ep->cosTheta;
	if (ep->cosTheta < 0) {
	    etaXMin = -atan(tanTheta * bOnA);
	    etaXMax = etaXMin + M_PI;
	    etaYMin = 0.5 * M_PI - atan(tanTheta / bOnA);
	    etaYMax = etaYMin + M_PI;
	} else {
	    etaXMax = -atan(tanTheta * bOnA);
	    etaXMin = etaXMax - M_PI;
	    etaYMax = 0.5 * M_PI - atan(tanTheta / bOnA);
	    etaYMin = etaYMax - M_PI;
	}
    } else {
	double invTanTheta = ep->cosTheta / ep->sinTheta;
	if (ep->sinTheta < 0) {
	    etaXMax = 0.5 * M_PI + atan(invTanTheta / bOnA);
	    etaXMin = etaXMax - M_PI;
	    etaYMin = atan(invTanTheta * bOnA);
	    etaYMax = etaYMin + M_PI;
	} else {
	    etaXMin = 0.5 * M_PI + atan(invTanTheta / bOnA);
	    etaXMax = etaXMin + M_PI;
	    etaYMax = atan(invTanTheta * bOnA);
	    etaYMin = etaYMax - M_PI;
	}
    }

    etaXMin -= (TWOPI * floor((etaXMin - ep->eta1) / TWOPI));
    etaYMin -= (TWOPI * floor((etaYMin - ep->eta1) / TWOPI));
    etaXMax -= (TWOPI * floor((etaXMax - ep->eta1) / TWOPI));
    etaYMax -= (TWOPI * floor((etaYMax - ep->eta1) / TWOPI));

    ep->xLeft = (etaXMin <= ep->eta2)
	? (ep->cx + ep->a * cos(etaXMin) * ep->cosTheta -
	   ep->b * sin(etaXMin) * ep->sinTheta)
	: MIN(ep->x1, ep->x2);
    ep->yUp = (etaYMin <= ep->eta2)
	? (ep->cy + ep->a * cos(etaYMin) * ep->sinTheta +
	   ep->b * sin(etaYMin) * ep->cosTheta)
	: MIN(ep->y1, ep->y2);
    ep->width = ((etaXMax <= ep->eta2)
		 ? (ep->cx + ep->a * cos(etaXMax) * ep->cosTheta -
		    ep->b * sin(etaXMax) * ep->sinTheta)
		 : MAX(ep->x1, ep->x2)) - ep->xLeft;
    ep->height = ((etaYMax <= ep->eta2)
		  ? (ep->cy + ep->a * cos(etaYMax) * ep->sinTheta +
		     ep->b * sin(etaYMax) * ep->cosTheta)
		  : MAX(ep->y1, ep->y2)) - ep->yUp;

}

static void
initEllipse(ellipse_t * ep, double cx, double cy, double a, double b,
	    double theta, double lambda1, double lambda2)
{
    ep->cx = cx;
    ep->cy = cy;
    ep->a = a;
    ep->b = b;
    ep->theta = theta;

    ep->eta1 = atan2(sin(lambda1) / b, cos(lambda1) / a);
    ep->eta2 = atan2(sin(lambda2) / b, cos(lambda2) / a);
    ep->cosTheta = cos(theta);
    ep->sinTheta = sin(theta);

    // make sure we have eta1 <= eta2 <= eta1 + 2*PI
    ep->eta2 -= TWOPI * floor((ep->eta2 - ep->eta1) / TWOPI);

    // the preceding correction fails if we have exactly eta2 - eta1 = 2*PI
    // it reduces the interval to zero length
    if ((lambda2 - lambda1 > M_PI) && (ep->eta2 - ep->eta1 < M_PI)) {
	ep->eta2 += TWOPI;
    }

    computeFoci(ep);
    computeEndPoints(ep);
    computeBounds(ep);

    /* Flatness parameters */
    ep->f = (ep->a - ep->b) / ep->a;
    ep->e2 = ep->f * (2.0 - ep->f);
    ep->g = 1.0 - ep->f;
    ep->g2 = ep->g * ep->g;
}

typedef double erray_t[2][4][4];

  // coefficients for error estimation
  // while using quadratic Bezier curves for approximation
  // 0 < b/a < 1/4
static erray_t coeffs2Low = {
    {
	{3.92478, -13.5822, -0.233377, 0.0128206},
	{-1.08814, 0.859987, 0.000362265, 0.000229036},
	{-0.942512, 0.390456, 0.0080909, 0.00723895},
	{-0.736228, 0.20998, 0.0129867, 0.0103456}
    }, 
    {
	{-0.395018, 6.82464, 0.0995293, 0.0122198},
	{-0.545608, 0.0774863, 0.0267327, 0.0132482},
	{0.0534754, -0.0884167, 0.012595, 0.0343396},
	{0.209052, -0.0599987, -0.00723897, 0.00789976}
    }
};

  // coefficients for error estimation
  // while using quadratic Bezier curves for approximation
  // 1/4 <= b/a <= 1
static erray_t coeffs2High = {
    {
	{0.0863805, -11.5595, -2.68765, 0.181224},
	{0.242856, -1.81073, 1.56876, 1.68544},
	{0.233337, -0.455621, 0.222856, 0.403469},
	{0.0612978, -0.104879, 0.0446799, 0.00867312}
    }, 
    {
	{0.028973, 6.68407, 0.171472, 0.0211706},
	{0.0307674, -0.0517815, 0.0216803, -0.0749348},
	{-0.0471179, 0.1288, -0.0781702, 2.0},
	{-0.0309683, 0.0531557, -0.0227191, 0.0434511}
    }
};

  // safety factor to convert the "best" error approximation
  // into a "max bound" error
static double safety2[] = {
    0.02, 2.83, 0.125, 0.01
};

  // coefficients for error estimation
  // while using cubic Bezier curves for approximation
  // 0 < b/a < 1/4
static erray_t coeffs3Low = {
    {
	{3.85268, -21.229, -0.330434, 0.0127842},
	{-1.61486, 0.706564, 0.225945, 0.263682},
	{-0.910164, 0.388383, 0.00551445, 0.00671814},
	{-0.630184, 0.192402, 0.0098871, 0.0102527}
    }, 
    {
	{-0.162211, 9.94329, 0.13723, 0.0124084},
	{-0.253135, 0.00187735, 0.0230286, 0.01264},
	{-0.0695069, -0.0437594, 0.0120636, 0.0163087},
	{-0.0328856, -0.00926032, -0.00173573, 0.00527385}
    }
};

  // coefficients for error estimation
  // while using cubic Bezier curves for approximation
  // 1/4 <= b/a <= 1
static erray_t coeffs3High = {
    {
	{0.0899116, -19.2349, -4.11711, 0.183362},
	{0.138148, -1.45804, 1.32044, 1.38474},
	{0.230903, -0.450262, 0.219963, 0.414038},
	{0.0590565, -0.101062, 0.0430592, 0.0204699}
    }, 
    {
	{0.0164649, 9.89394, 0.0919496, 0.00760802},
	{0.0191603, -0.0322058, 0.0134667, -0.0825018},
	{0.0156192, -0.017535, 0.00326508, -0.228157},
	{-0.0236752, 0.0405821, -0.0173086, 0.176187}
    }
};

  // safety factor to convert the "best" error approximation
  // into a "max bound" error
static double safety3[] = {
    0.001, 4.98, 0.207, 0.0067
};

/* Compute the value of a rational function.
 * This method handles rational functions where the numerator is
 * quadratic and the denominator is linear
 */
#define RationalFunction(x,c) ((x * (x * c[0] + c[1]) + c[2]) / (x + c[3]))

/* Estimate the approximation error for a sub-arc of the instance.
 * degree specifies degree of the Bezier curve to use (1, 2 or 3)
 * tA and tB give the start and end angle of the subarc
 * Returns upper bound of the approximation error between the Bezier
 * curve and the real ellipse
 */
static double
estimateError(ellipse_t * ep, int degree, double etaA, double etaB)
{
    double c0, c1, eta = 0.5 * (etaA + etaB);

    if (degree < 2) {

	// start point
	double aCosEtaA = ep->a * cos(etaA);
	double bSinEtaA = ep->b * sin(etaA);
	double xA =
	    ep->cx + aCosEtaA * ep->cosTheta - bSinEtaA * ep->sinTheta;
	double yA =
	    ep->cy + aCosEtaA * ep->sinTheta + bSinEtaA * ep->cosTheta;

	// end point
	double aCosEtaB = ep->a * cos(etaB);
	double bSinEtaB = ep->b * sin(etaB);
	double xB =
	    ep->cx + aCosEtaB * ep->cosTheta - bSinEtaB * ep->sinTheta;
	double yB =
	    ep->cy + aCosEtaB * ep->sinTheta + bSinEtaB * ep->cosTheta;

	// maximal error point
	double aCosEta = ep->a * cos(eta);
	double bSinEta = ep->b * sin(eta);
	double x =
	    ep->cx + aCosEta * ep->cosTheta - bSinEta * ep->sinTheta;
	double y =
	    ep->cy + aCosEta * ep->sinTheta + bSinEta * ep->cosTheta;

	double dx = xB - xA;
	double dy = yB - yA;

	return abs(x * dy - y * dx + xB * yA - xA * yB)
	    / sqrt(dx * dx + dy * dy);

    } else {

	double x = ep->b / ep->a;
	double dEta = etaB - etaA;
	double cos2 = cos(2 * eta);
	double cos4 = cos(4 * eta);
	double cos6 = cos(6 * eta);

	// select the right coefficient's set according to degree and b/a
	double (*coeffs)[4][4];
	double *safety;
	if (degree == 2) {
	    coeffs = (x < 0.25) ? coeffs2Low : coeffs2High;
	    safety = safety2;
	} else {
	    coeffs = (x < 0.25) ? coeffs3Low : coeffs3High;
	    safety = safety3;
	}

	c0 = RationalFunction(x, coeffs[0][0])
	    + cos2 * RationalFunction(x, coeffs[0][1])
	    + cos4 * RationalFunction(x, coeffs[0][2])
	    + cos6 * RationalFunction(x, coeffs[0][3]);

	c1 = RationalFunction(x, coeffs[1][0])
	    + cos2 * RationalFunction(x, coeffs[1][1])
	    + cos4 * RationalFunction(x, coeffs[1][2])
	    + cos6 * RationalFunction(x, coeffs[1][3]);

	return RationalFunction(x, safety) * ep->a * exp(c0 + c1 * dEta);
    }
}

/* Non-reentrant code to append points to a Bezier path
 * Assume initial call to moveTo to initialize, followed by
 * calls to curveTo and lineTo, and finished with endPath.
 */
static int bufsize;

static void moveTo(Ppolyline_t * path, double x, double y)
{
    bufsize = 100;
    path->ps = N_NEW(bufsize, pointf);
    path->ps[0].x = x;
    path->ps[0].y = y;
    path->pn = 1;
}

static void
curveTo(Ppolyline_t * path, double x1, double y1,
	double x2, double y2, double x3, double y3)
{
    if (path->pn + 3 >= bufsize) {
	bufsize *= 2;
	path->ps = realloc(path->ps, bufsize * sizeof(pointf));
    }
    path->ps[path->pn].x = x1;
    path->ps[path->pn++].y = y1;
    path->ps[path->pn].x = x2;
    path->ps[path->pn++].y = y2;
    path->ps[path->pn].x = x3;
    path->ps[path->pn++].y = y3;
}

static void lineTo(Ppolyline_t * path, double x, double y)
{
    pointf curp = path->ps[path->pn - 1];
    curveTo(path, curp.x, curp.y, x, y, x, y);
}

static void endPath(Ppolyline_t * path, boolean close)
{
    if (close) {
	pointf p0 = path->ps[0];
	lineTo(path, p0.x, p0.y);
    }

    path->ps = realloc(path->ps, path->pn * sizeof(pointf));
    bufsize = 0;
}

/* genEllipticPath:
 * Approximate an elliptical arc via Beziers of given degree
 * threshold indicates quality of approximation
 * if isSlice is true, the path begins and ends with line segments
 * to the center of the ellipse.
 * Returned path must be freed by the caller.
 */
static Ppolyline_t *genEllipticPath(ellipse_t * ep, int degree,
				    double threshold, boolean isSlice)
{
    double dEta;
    double etaB;
    double cosEtaB;
    double sinEtaB;
    double aCosEtaB;
    double bSinEtaB;
    double aSinEtaB;
    double bCosEtaB;
    double xB;
    double yB;
    double xBDot;
    double yBDot;
    double t;
    double alpha;
    Ppolyline_t *path = NEW(Ppolyline_t);

    // find the number of Bezier curves needed
    boolean found = FALSE;
    int i, n = 1;
    while ((!found) && (n < 1024)) {
	double dEta = (ep->eta2 - ep->eta1) / n;
	if (dEta <= 0.5 * M_PI) {
	    double etaB = ep->eta1;
	    found = TRUE;
	    for (i = 0; found && (i < n); ++i) {
		double etaA = etaB;
		etaB += dEta;
		found =
		    (estimateError(ep, degree, etaA, etaB) <= threshold);
	    }
	}
	n = n << 1;
    }

    dEta = (ep->eta2 - ep->eta1) / n;
    etaB = ep->eta1;

    cosEtaB = cos(etaB);
    sinEtaB = sin(etaB);
    aCosEtaB = ep->a * cosEtaB;
    bSinEtaB = ep->b * sinEtaB;
    aSinEtaB = ep->a * sinEtaB;
    bCosEtaB = ep->b * cosEtaB;
    xB = ep->cx + aCosEtaB * ep->cosTheta - bSinEtaB * ep->sinTheta;
    yB = ep->cy + aCosEtaB * ep->sinTheta + bSinEtaB * ep->cosTheta;
    xBDot = -aSinEtaB * ep->cosTheta - bCosEtaB * ep->sinTheta;
    yBDot = -aSinEtaB * ep->sinTheta + bCosEtaB * ep->cosTheta;

    if (isSlice) {
	moveTo(path, ep->cx, ep->cy);
	lineTo(path, xB, yB);
    } else {
	moveTo(path, xB, yB);
    }

    t = tan(0.5 * dEta);
    alpha = sin(dEta) * (sqrt(4 + 3 * t * t) - 1) / 3;

    for (i = 0; i < n; ++i) {

	double xA = xB;
	double yA = yB;
	double xADot = xBDot;
	double yADot = yBDot;

	etaB += dEta;
	cosEtaB = cos(etaB);
	sinEtaB = sin(etaB);
	aCosEtaB = ep->a * cosEtaB;
	bSinEtaB = ep->b * sinEtaB;
	aSinEtaB = ep->a * sinEtaB;
	bCosEtaB = ep->b * cosEtaB;
	xB = ep->cx + aCosEtaB * ep->cosTheta - bSinEtaB * ep->sinTheta;
	yB = ep->cy + aCosEtaB * ep->sinTheta + bSinEtaB * ep->cosTheta;
	xBDot = -aSinEtaB * ep->cosTheta - bCosEtaB * ep->sinTheta;
	yBDot = -aSinEtaB * ep->sinTheta + bCosEtaB * ep->cosTheta;

	if (degree == 1) {
	    lineTo(path, xB, yB);
#if DO_QUAD
	} else if (degree == 2) {
	    double k = (yBDot * (xB - xA) - xBDot * (yB - yA))
		/ (xADot * yBDot - yADot * xBDot);
	    quadTo(path, (xA + k * xADot), (yA + k * yADot), xB, yB);
#endif
	} else {
	    curveTo(path, (xA + alpha * xADot), (yA + alpha * yADot),
		    (xB - alpha * xBDot), (yB - alpha * yBDot), xB, yB);
	}

    }

    endPath(path, isSlice);

    return path;
}

/* ellipticWedge:
 * Return a cubic Bezier for an elliptical wedge, with center ctr, x and y
 * semi-axes xsemi and ysemi, start angle angle0 and end angle angle1.
 * This includes beginning and ending line segments to the ellipse center.
 * Calling function must free storage of returned path.
 */
Ppolyline_t *ellipticWedge(pointf ctr, double xsemi, double ysemi,
			   double angle0, double angle1)
{
    ellipse_t ell;
    Ppolyline_t *pp;

    initEllipse(&ell, ctr.x, ctr.y, xsemi, ysemi, 0, angle0, angle1);
    pp = genEllipticPath(&ell, 3, 0.00001, 1);
    return pp;
}

#ifdef STANDALONE
main()
{
    ellipse_t ell;
    Ppolyline_t *pp;
    int i;

    initEllipse(&ell, 200, 200, 100, 50, 0, M_PI / 4, 3 * M_PI / 2);
    pp = genEllipticPath(&ell, 3, 0.00001, 1);

    printf("newpath %.02lf %.02lf moveto\n", pp->ps[0].x, pp->ps[0].y);
    for (i = 1; i < pp->pn; i += 3) {
	printf("%.02lf %.02lf %.02lf %.02lf %.02lf %.02lf curveto\n",
	       pp->ps[i].x, pp->ps[i].y,
	       pp->ps[i + 1].x, pp->ps[i + 1].y,
	       pp->ps[i + 2].x, pp->ps[i + 2].y);
    }
    printf("stroke showpage\n");

}
#endif
