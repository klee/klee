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

#include "render.h"
#include "tree_map.h"

static void squarify(int n, real *area, rectangle *recs, int nadded, real maxarea, real minarea, real totalarea,
		     real asp, rectangle fillrec){
  /* add a list of area in fillrec using squarified treemap alg.
     n: number of items to add
     area: area of these items, Sum to 1 (?).
     nadded: number of items already added
     maxarea: maxarea of already added items
     minarea: min areas of already added items
     asp: current worst aspect ratio of the already added items so far
     fillrec: the rectangle to be filled in.
   */
  real w = MIN(fillrec.size[0], fillrec.size[1]);
  int i;

  if (n <= 0) return;

  if (Verbose) {
    fprintf(stderr, "trying to add to rect {%f +/- %f, %f +/- %f}\n",fillrec.x[0], fillrec.size[0], fillrec.x[1], fillrec.size[1]);
    fprintf(stderr, "total added so far = %d\n", nadded);
  }

  if (nadded == 0){
    nadded = 1;
    maxarea = minarea = area[0];
    asp = MAX(area[0]/(w*w), (w*w)/area[0]);
    totalarea = area[0];
    squarify(n, area, recs, nadded, maxarea, minarea, totalarea, asp, fillrec);
  } else {
    real newmaxarea, newminarea, s, h, maxw, minw, newasp, hh, ww, xx, yy;
    if (nadded < n){
      newmaxarea = MAX(maxarea, area[nadded]);
      newminarea = MIN(minarea, area[nadded]);
      s = totalarea + area[nadded];
      h = s/w;
      maxw = newmaxarea/h;
      minw = newminarea/h;
      newasp = MAX(h/minw, maxw/h);/* same as MAX{s^2/(w^2*newminarea), (w^2*newmaxarea)/(s^2)}*/
    }
    if (nadded < n && newasp <= asp){/* aspectio improved, keep adding */
      squarify(n, area, recs, ++nadded, newmaxarea, newminarea, s, newasp, fillrec);
    } else {
      /* aspectio worsen if add another area, fixed the already added recs */
      if (Verbose) fprintf(stderr,"adding %d items, total area = %f, w = %f, area/w=%f\n",nadded, totalarea, w, totalarea/w);
      if (w == fillrec.size[0]){/* tall rec. fix the items along x direction, left to right, at top*/
	hh = totalarea/w;
	xx = fillrec.x[0] - fillrec.size[0]/2;
	for (i = 0; i < nadded; i++){
	  recs[i].size[1] = hh;
	  ww = area[i]/hh;
	  recs[i].size[0] = ww;
	  recs[i].x[1] = fillrec.x[1] + 0.5*(fillrec.size[1]) - hh/2;
	  recs[i].x[0] = xx + ww/2;
	  xx += ww;
	}
	fillrec.x[1] -= hh/2;/* the new empty space is below the filled space */
	fillrec.size[1] -= hh;
      } else {/* short rec. fix along y top to bot, at left*/
	ww = totalarea/w;
	yy = fillrec.x[1] + fillrec.size[1]/2;
	for (i = 0; i < nadded; i++){
	  recs[i].size[0] = ww;
	  hh = area[i]/ww;
	  recs[i].size[1] = hh;
	  recs[i].x[0] = fillrec.x[0] - 0.5*(fillrec.size[0]) + ww/2;
	  recs[i].x[1] = yy - hh/2;
	  yy -= hh;
	}
	fillrec.x[0] += ww/2;/* the new empty space is right of the filled space */
	fillrec.size[0] -= ww;
      }
      squarify(n - nadded, area + nadded, recs + nadded, 0, 0., 0., 0., 1., fillrec);
    }

  }
}

/* tree_map:
 * Perform a squarified treemap layout on a single level.
 *  n - number of rectangles
 *  area - area of rectangles
 *  fillred - rectangle to be filled
 *  return array of rectangles 
 */
rectangle* tree_map(int n, real *area, rectangle fillrec){
  /* fill a rectangle rec with n items, each item i has area[i] area. */
  rectangle *recs;
  int i;
  real total = 0, minarea = 1., maxarea = 0., asp = 1, totalarea = 0;
  int nadded = 0;

  for (i = 0; i < n; i++) total += area[i]; 
    /* make sure there is enough area */
  if (total > fillrec.size[0] * fillrec.size[1] + 0.001)
    return NULL;
  
  recs = N_NEW(n,rectangle);
  squarify(n, area, recs, nadded, maxarea, minarea, totalarea, asp, fillrec);
  return recs;
}

/* rectangle_new:
 * Create and initialize a new rectangle structure
 */
rectangle rectangle_new(real x, real y, real width, real height){
  rectangle r;
  r.x[0] = x;
  r.x[1] = y;
  r.size[0] = width;
  r.size[1] = height;
  return r;
}
