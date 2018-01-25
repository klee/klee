/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#include <math.h>
#include <stdlib.h>
#include "types.h"
#include "globals.h"
#include "general.h"
#include "SparseMatrix.h"
#include "edge_bundling.h"
#include "ink.h"

double ink_count;

static point_t addPoint (point_t a, point_t b)
{
  a.x += b.x;
  a.y += b.y;
  return a;
}

static point_t subPoint (point_t a, point_t b)
{
  a.x -= b.x;
  a.y -= b.y;
  return a;
}

static point_t scalePoint (point_t a, double d)
{
  a.x *= d;
  a.y *= d;
  return a;
}

static double dotPoint(point_t a, point_t b){
  return a.x*b.x + a.y*b.y;
}


point_t Origin;

/* sumLengths:
 */
static double sumLengths_avoid_bad_angle(point_t* points, int npoints, point_t end, point_t meeting, real angle_param) 
{
  /* avoid sharp turns, we want cos_theta to be as close to -1 as possible */
  int i;
  double len0, len, sum = 0;
  double diff_x, diff_y, diff_x0, diff_y0;
  double cos_theta, cos_max = -10;

  diff_x0 = end.x-meeting.x;
  diff_y0 = end.y-meeting.y;
  len0 = sum = sqrt(diff_x0*diff_x0+diff_y0*diff_y0);

  // distance form each of 'points' till 'meeting'
  for (i=0; i<npoints; i++) {
    diff_x = points[i].x-meeting.x;
    diff_y = points[i].y-meeting.y;
    len = sqrt(diff_x*diff_x+diff_y*diff_y);
    sum += len;
    cos_theta = (diff_x0*diff_x + diff_y0*diff_y)/MAX((len*len0),0.00001);
    cos_max = MAX(cos_max, cos_theta);
  }

  // distance of single line from 'meeting' to 'end'
  return sum*(cos_max + angle_param);/* straight line gives angle_param - 1, turning angle of 180 degree gives angle_param + 1 */
}
static double sumLengths(point_t* points, int npoints, point_t end, point_t meeting) 
{
  int i;
  double sum = 0;
  double diff_x, diff_y;

    // distance form each of 'points' till 'meeting'
  for (i=0; i<npoints; i++) {
        diff_x = points[i].x-meeting.x;
        diff_y = points[i].y-meeting.y;
        sum += sqrt(diff_x*diff_x+diff_y*diff_y);
  }
    // distance of single line from 'meeting' to 'end'
  diff_x = end.x-meeting.x;
  diff_y = end.y-meeting.y;
  sum += sqrt(diff_x*diff_x+diff_y*diff_y);
  return sum;
}

/* bestInk:
 */
static double bestInk(point_t* points, int npoints, point_t begin, point_t end, double prec, point_t *meet, real angle_param)
{
  point_t first, second, third, fourth, diff, meeting;
  double value1, value2, value3, value4;

  first = begin;
  fourth = end;

  do {
    diff = subPoint(fourth,first);
    second = addPoint(first,scalePoint(diff,1.0/3.0));
    third = addPoint(first,scalePoint(diff,2.0/3.0));

    if (angle_param < 1){
      value1 = sumLengths(points, npoints, end, first);
      value2 = sumLengths(points, npoints, end, second);
      value3 = sumLengths(points, npoints, end, third);
      value4 = sumLengths(points, npoints, end, fourth);
    } else {
      value1 = sumLengths_avoid_bad_angle(points, npoints, end, first, angle_param);
      value2 = sumLengths_avoid_bad_angle(points, npoints, end, second, angle_param);
      value3 = sumLengths_avoid_bad_angle(points, npoints, end, third, angle_param);
      value4 = sumLengths_avoid_bad_angle(points, npoints, end, fourth, angle_param);
    }

    if (value1<value2) {
      if (value1<value3) {
        if (value1<value4) {
          // first is smallest
          fourth = second;
        }
        else {
          // fourth is smallest
          first = third;
        }
      }
      else {
        if (value3<value4) {
          // third is smallest
          first = second;
        }
        else {
          // fourth is smallest
          first = third;
        }
      }
    }
    else {
      if (value2<value3) {
        if (value2<value4) {
          // second is smallest
          fourth = third;
        }
        else {
          // fourth is smallest
          first = third;
        }
      }
      else {
        if (value3<value4) {
          // third is smallest
          first = second;
        }
        else {
          // fourth is smallest
          first = third;
        }
      }
    }
  } while (fabs(value1-value4)/(MIN(value1,value4)+1e-10)>prec && dotPoint(diff, diff) > 1.e-20);

  meeting = scalePoint(addPoint(first,fourth),0.5);
  *meet = meeting;

  return sumLengths(points, npoints, end, meeting);

}

static double project_to_line(point_t pt, point_t left, point_t right, real angle){
  /* pt
     ^  ^          
     .   \ \ 
     .    \   \        
   d .    a\    \
     .      \     \
     .       \       \
     .   c    \  alpha \             b
     .<------left:0 ----------------------------> right:1. Find the projection of pt on the left--right line. If the turning angle is small, 
     |                  |
     |<-------f---------
     we should get a negative number. Let a := left->pt, b := left->right, then we are calculating:
     c = |a| cos(a,b)/|b| b
     d = a - c
     f = -ctan(alpha)*|d|/|b| b
     and the project of alpha degree on the left->right line is
     c-f = |a| cos(a,b)/|b| b - -ctan(alpha)*|d|/|b| b
     = (|a| a.b/(|a||b|) + ctan(alpha)|a-c|)/|b| b
     = (a.b/|b| + ctan(alpha)|a-c|)/|b| b
     the dimentionless projection is:
     a.b/|b|^2 + ctan(alpha)|a-c|/|b|
     = a.b/|b|^2 + ctan(alpha)|d|/|b|
  */


  point_t b, a;
  real bnorm, dnorm;
  real alpha, ccord;

  if (angle <=0 || angle >= M_PI) return 2;/* return outside of the interval which should be handled as a sign of infeasible turning angle */
  alpha = angle;

  assert(alpha > 0 && alpha < M_PI);
  b = subPoint(right, left);
  a = subPoint(pt, left);
  bnorm = MAX(1.e-10, dotPoint(b, b));
  ccord = dotPoint(b, a)/bnorm;
  dnorm = dotPoint(a,a)/bnorm - ccord*ccord;
  if (alpha == M_PI/2){
    return ccord;
  } else {
    //    assert(dnorm >= MIN(-1.e-5, -1.e-5*bnorm));
    return ccord + sqrt(MAX(0, dnorm))/tan(alpha);
  }
}









/* ink:
 * Compute minimal ink used the input edges are bundled.
 * Assumes tails all occur on one side and heads on the other.
 */
double ink(pedge* edges, int numEdges, int *pick, double *ink0, point_t *meet1, point_t *meet2, real angle_param, real angle)
{
  int i;
  point_t begin, end, mid, diff;
  pedge e;
  real *x;
  point_t* sources = N_NEW(numEdges, point_t);
  point_t* targets = N_NEW(numEdges, point_t);
  double inkUsed;
  //double eps = 1.0e-2;
  double eps = 1.0e-2;
  double cend = 0, cbegin = 0;
  double wgt = 0;

  //  fprintf(stderr,"in ink code ========\n");
  ink_count += numEdges;

  *ink0 = 0;

  /* canonicalize so that edges 1,2,3 and 3,2,1 gives the same optimal ink */
  if (pick) vector_sort_int(numEdges, pick, TRUE);

  begin = end = Origin;
  for (i = 0; i < numEdges; i++) {
    if (pick) {
      e = edges[pick[i]];
    } else {
      e = edges[i];
    }
    x = e->x;
    sources[i].x = x[0];
    sources[i].y = x[1];
    targets[i].x = x[e->dim*e->npoints - e->dim];
    targets[i].y = x[e->dim*e->npoints - e->dim + 1];
    (*ink0) += sqrt((sources[i].x - targets[i].x)*(sources[i].x - targets[i].x) + (sources[i].y - targets[i].y)*(sources[i].y - targets[i].y));
    begin = addPoint (begin, scalePoint(sources[i], e->wgt));
    end = addPoint (end, scalePoint(targets[i], e->wgt));
    wgt += e->wgt;
    //fprintf(stderr,"source={%f,%f}, target = {%f,%f}\n",sources[i].x, sources[i].y,
    //targets[i].x, targets[i].y);
  }

  begin = scalePoint (begin, 1.0/wgt);
  end = scalePoint (end, 1.0/wgt);


  if (numEdges == 1){
    *meet1 = begin;
    *meet2 = end;
    //fprintf(stderr,"ink used = %f\n",*ink0);
      free (sources);
      free (targets);
      return *ink0;
  }

  /* shift the begin and end point to avoid sharp turns */
  for (i = 0; i < numEdges; i++) {
    if (pick) {
      e = edges[pick[i]];
    } else {
      e = edges[i];
    }
    x = e->x;
    sources[i].x = x[0];
    sources[i].y = x[1];
    targets[i].x = x[e->dim*e->npoints - e->dim];
    targets[i].y = x[e->dim*e->npoints - e->dim + 1];
    /* begin(1) ----------- mid(0) */
    if (i == 0){
      cbegin = project_to_line(sources[i], begin, end, angle);
      cend =   project_to_line(targets[i], end, begin, angle);
    } else {
      cbegin = MAX(cbegin, project_to_line(sources[i], begin, end, angle));
      cend = MAX(cend, project_to_line(targets[i], end, begin, angle));
    }
  }

  if (angle > 0 && angle < M_PI){
    if (cbegin + cend > 1 || cbegin > 1 || cend > 1){
      /* no point can be found that satisfies the angular constraints, so we give up and set ink to a large value */
      if (Verbose && 0) fprintf(stderr,"no point satisfying any angle constraints can be found. cbeg=%f cend=%f\n",cbegin,cend);
      inkUsed = 1000*(*ink0);
      *meet1 = *meet2 = mid;
      free (sources);
      free (targets);
      return inkUsed;
    }
    /* make sure the turning angle is no more than alpha degree */
    cbegin = MAX(0, cbegin);/* make sure the new adjusted point is with in [begin,end] internal */
    diff = subPoint(end, begin);
    begin = addPoint(begin, scalePoint(diff, cbegin));
    
    cend = MAX(0, cend);/* make sure the new adjusted point is with in [end,begin] internal */
    end = subPoint(end, scalePoint(diff, cend));
  }
  mid = scalePoint (addPoint(begin,end),0.5);

  inkUsed = (bestInk (sources, numEdges, begin, mid, eps, meet1, angle_param)
	     + bestInk (targets, numEdges, end, mid, eps, meet2, angle_param));
  //fprintf(stderr,"beg={%f,%f}, meet1={%f,%f}, meet2={%f,%f}, mid={%f,%f}, end={%f,%f}\n",begin.x, begin.y, meet1->x, meet1->y, meet2->x, meet2->y,
  //mid.x, mid.y, end.x, end.y);

  //fprintf(stderr,"ink used = %f\n",inkUsed);
  //  fprintf(stderr,"{cb,ce}={%f, %f} end={%f,%f}, meet={%f,%f}, mid={%f,%f}\n",cbegin, cend, end.x, end.y, meet2->x, meet2->y, mid.x, mid.y);
  free (sources);
  free (targets);
  return inkUsed;
}


double ink1(pedge e){


  real *x, xx, yy;

  real ink0 = 0;

  x = e->x;
  xx = x[0] - x[e->dim*e->npoints - e->dim];
  yy = x[1] - x[e->dim*e->npoints - e->dim + 1];
  ink0 += sqrt(xx*xx + yy*yy);
  return ink0;
}
