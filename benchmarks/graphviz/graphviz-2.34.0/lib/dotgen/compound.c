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


/* Module for clipping splines to cluster boxes.
 */

#include	"dot.h"

/* pf2s:
 * Convert a pointf to its string representation.
 */
static char *pf2s(pointf p, char *buf)
{
    sprintf(buf, "(%.5g,%.5g)", p.x, p.y);
    return buf;
}

/* Return point where line segment [pp,cp] intersects
 * the box bp. Assume cp is outside the box, and pp is
 * on or in the box. 
 */
static pointf boxIntersectf(pointf pp, pointf cp, boxf * bp)
{
    pointf ipp;
    double ppx = pp.x;
    double ppy = pp.y;
    double cpx = cp.x;
    double cpy = cp.y;
    pointf ll;
    pointf ur;

    ll = bp->LL;
    ur = bp->UR;
    if (cp.x < ll.x) {
	ipp.x = ll.x;
	ipp.y = pp.y + (int) ((ipp.x - ppx) * (ppy - cpy) / (ppx - cpx));
	if (ipp.y >= ll.y && ipp.y <= ur.y)
	    return ipp;
    }
    if (cp.x > ur.x) {
	ipp.x = ur.x;
	ipp.y = pp.y + (int) ((ipp.x - ppx) * (ppy - cpy) / (ppx - cpx));
	if (ipp.y >= ll.y && ipp.y <= ur.y)
	    return ipp;
    }
    if (cp.y < ll.y) {
	ipp.y = ll.y;
	ipp.x = pp.x + (int) ((ipp.y - ppy) * (ppx - cpx) / (ppy - cpy));
	if (ipp.x >= ll.x && ipp.x <= ur.x)
	    return ipp;
    }
    if (cp.y > ur.y) {
	ipp.y = ur.y;
	ipp.x = pp.x + (int) ((ipp.y - ppy) * (ppx - cpx) / (ppy - cpy));
	if (ipp.x >= ll.x && ipp.x <= ur.x)
	    return ipp;
    }

    /* failure */
    {
	char ppbuf[100], cpbuf[100], llbuf[100], urbuf[100];

	agerr(AGERR,
		"segment [%s,%s] does not intersect box ll=%s,ur=%s\n",
		pf2s(pp, ppbuf), pf2s(cp, cpbuf),
		pf2s(ll, llbuf), pf2s(ur, urbuf));
	assert(0);
    }
    return ipp;
}

/* inBoxf:
 * Returns true if p is on or in box bb
 */
static int inBoxf(pointf p, boxf * bb)
{
    return INSIDE(p, *bb);
}

/* getCluster:
 * Returns subgraph of g with given name.
 * Returns NULL if no name is given, or subgraph of
 * that name does not exist.
 */
#ifdef WITH_CGRAPH
static graph_t *getCluster(graph_t * g, char *cluster_name, Dt_t* map)
#else
static graph_t *getCluster(graph_t * g, char *cluster_name)
#endif
{
    Agraph_t* sg;

    if (!cluster_name || (*cluster_name == '\0'))
	return NULL;
#ifdef WITH_CGRAPH
    sg = findCluster (map, cluster_name);
#else
    sg = agfindsubg(g, cluster_name);
#endif
    if (sg == NULL) {
	agerr(AGWARN, "cluster named %s not found\n", cluster_name);
    }
    return sg;
}

/* The following functions are derived from pp. 411-415 (pp. 791-795)
 * of Graphics Gems. In the code there, they use a SGN function to
 * count crossings. This doesn't seem to handle certain special cases,
 * as when the last point is on the line. It certainly didn't work
 * for us when we used int values; see bug 145. We needed to use CMP instead.
 *
 * Possibly unnecessary with double values, but harmless.
 */

/* countVertCross:
 * Return the number of times the Bezier control polygon crosses
 * the vertical line x = xcoord.
 */
static int countVertCross(pointf * pts, double xcoord)
{
    int i;
    int sign, old_sign;
    int num_crossings = 0;

    sign = CMP(pts[0].x, xcoord);
    if (sign == 0)
	num_crossings++;
    for (i = 1; i <= 3; i++) {
	old_sign = sign;
	sign = CMP(pts[i].x, xcoord);
	if ((sign != old_sign) && (old_sign != 0))
	    num_crossings++;
    }
    return num_crossings;
}

/* countHorzCross:
 * Return the number of times the Bezier control polygon crosses
 * the horizontal line y = ycoord.
 */
static int countHorzCross(pointf * pts, double ycoord)
{
    int i;
    int sign, old_sign;
    int num_crossings = 0;

    sign = CMP(pts[0].y, ycoord);
    if (sign == 0)
	num_crossings++;
    for (i = 1; i <= 3; i++) {
	old_sign = sign;
	sign = CMP(pts[i].y, ycoord);
	if ((sign != old_sign) && (old_sign != 0))
	    num_crossings++;
    }
    return num_crossings;
}

/* findVertical:
 * Given 4 Bezier control points pts, corresponding to the portion
 * of an initial spline with path parameter in the range
 * 0.0 <= tmin <= t <= tmax <= 1.0, return t where the spline 
 * first crosses a vertical line segment
 * [(xcoord,ymin),(xcoord,ymax)]. Return -1 if not found.
 * This is done by binary subdivision. 
 */
static double
findVertical(pointf * pts, double tmin, double tmax,
	     double xcoord, double ymin, double ymax)
{
    pointf Left[4];
    pointf Right[4];
    double t;
    int no_cross = countVertCross(pts, xcoord);

    if (no_cross == 0)
	return -1.0;

    /* if 1 crossing and on the line x == xcoord (within 1 point) */
    if ((no_cross == 1) && (ROUND(pts[3].x) == ROUND(xcoord))) {
	if ((ymin <= pts[3].y) && (pts[3].y <= ymax)) {
	    return tmax;
	} else
	    return -1.0;
    }

    /* split the Bezier into halves, trying the first half first. */
    Bezier(pts, 3, 0.5, Left, Right);
    t = findVertical(Left, tmin, (tmin + tmax) / 2.0, xcoord, ymin, ymax);
    if (t >= 0.0)
	return t;
    return findVertical(Right, (tmin + tmax) / 2.0, tmax, xcoord, ymin,
			ymax);

}

/* findHorizontal:
 * Given 4 Bezier control points pts, corresponding to the portion
 * of an initial spline with path parameter in the range
 * 0.0 <= tmin <= t <= tmax <= 1.0, return t where the spline 
 * first crosses a horizontal line segment
 * [(xmin,ycoord),(xmax,ycoord)]. Return -1 if not found.
 * This is done by binary subdivision. 
 */
static double
findHorizontal(pointf * pts, double tmin, double tmax,
	       double ycoord, double xmin, double xmax)
{
    pointf Left[4];
    pointf Right[4];
    double t;
    int no_cross = countHorzCross(pts, ycoord);

    if (no_cross == 0)
	return -1.0;

    /* if 1 crossing and on the line y == ycoord (within 1 point) */
    if ((no_cross == 1) && (ROUND(pts[3].y) == ROUND(ycoord))) {
	if ((xmin <= pts[3].x) && (pts[3].x <= xmax)) {
	    return tmax;
	} else
	    return -1.0;
    }

    /* split the Bezier into halves, trying the first half first. */
    Bezier(pts, 3, 0.5, Left, Right);
    t = findHorizontal(Left, tmin, (tmin + tmax) / 2.0, ycoord, xmin,
		       xmax);
    if (t >= 0.0)
	return t;
    return findHorizontal(Right, (tmin + tmax) / 2.0, tmax, ycoord, xmin,
			  xmax);
}

/* splineIntersectf:
 * Given four spline control points and a box,
 * find the shortest portion of the spline from
 * pts[0] to the intersection with the box, if any.
 * If an intersection is found, the four points are stored in pts[0..3]
 * with pts[3] being on the box, and 1 is returned. Otherwise, pts
 * is left unchanged and 0 is returned.
 */
static int splineIntersectf(pointf * pts, boxf * bb)
{
    double tmin = 2.0;
    double t;
    pointf origpts[4];
    int i;

    for (i = 0; i < 4; i++) {
	origpts[i] = pts[i];
    }

    t = findVertical(pts, 0.0, 1.0, bb->LL.x, bb->LL.y, bb->UR.y);
    if ((t >= 0) && (t < tmin)) {
	Bezier(origpts, 3, t, pts, NULL);
	tmin = t;
    }
    t = findVertical(pts, 0.0, MIN(1.0, tmin), bb->UR.x, bb->LL.y,
		     bb->UR.y);
    if ((t >= 0) && (t < tmin)) {
	Bezier(origpts, 3, t, pts, NULL);
	tmin = t;
    }
    t = findHorizontal(pts, 0.0, MIN(1.0, tmin), bb->LL.y, bb->LL.x,
		       bb->UR.x);
    if ((t >= 0) && (t < tmin)) {
	Bezier(origpts, 3, t, pts, NULL);
	tmin = t;
    }
    t = findHorizontal(pts, 0.0, MIN(1.0, tmin), bb->UR.y, bb->LL.x,
		       bb->UR.x);
    if ((t >= 0) && (t < tmin)) {
	Bezier(origpts, 3, t, pts, NULL);
	tmin = t;
    }

    if (tmin < 2.0) {
	return 1;
    } else
	return 0;
}

/* makeCompoundEdge:
 * If edge e has a cluster head and/or cluster tail,
 * clip spline to outside of cluster. 
 * Requirement: spline is composed of only one part, 
 * with n control points where n >= 4 and n (mod 3) = 1.
 * If edge has arrowheads, reposition them.
 */
#ifdef WITH_CGRAPH
static void makeCompoundEdge(graph_t * g, edge_t * e, Dt_t* clustMap)
#else
static void makeCompoundEdge(graph_t * g, edge_t * e)
#endif
{
    graph_t *lh;		/* cluster containing head */
    graph_t *lt;		/* cluster containing tail */
    bezier *bez;		/* original Bezier for e */
    bezier *nbez;		/* new Bezier  for e */
    int starti = 0, endi = 0;	/* index of first and last control point */
    node_t *head;
    node_t *tail;
    boxf *bb;
    int i, j;
    int size;
    pointf pts[4];
    pointf p;
    int fixed;

    /* find head and tail target clusters, if defined */
#ifdef WITH_CGRAPH
    lh = getCluster(g, agget(e, "lhead"), clustMap);
    lt = getCluster(g, agget(e, "ltail"), clustMap);
#else
    lh = getCluster(g, agget(e, "lhead"));
    lt = getCluster(g, agget(e, "ltail"));
#endif
    if (!lt && !lh)
	return;
    if (!ED_spl(e)) return;

    /* at present, we only handle single spline case */
    if (ED_spl(e)->size > 1) {
	agerr(AGWARN, "%s -> %s: spline size > 1 not supported\n",
	      agnameof(agtail(e)), agnameof(aghead(e)));
	return;
    }
    bez = ED_spl(e)->list;
    size = bez->size;

    head = aghead(e);
    tail = agtail(e);

    /* allocate new Bezier */
    nbez = GNEW(bezier);
    nbez->eflag = bez->eflag;
    nbez->sflag = bez->sflag;

    /* if Bezier has four points, almost collinear,
     * make line - unimplemented optimization?
     */

    /* If head cluster defined, find first Bezier
     * crossing head cluster, and truncate spline to
     * box edge.
     * Otherwise, leave end alone.
     */
    fixed = 0;
    if (lh) {
	bb = &(GD_bb(lh));
	if (!inBoxf(ND_coord(head), bb)) {
	    agerr(AGWARN, "%s -> %s: head not inside head cluster %s\n",
		  agnameof(agtail(e)), agnameof(aghead(e)), agget(e, "lhead"));
	} else {
	    /* If first control point is in bb, degenerate case. Spline
	     * reduces to four points between the arrow head and the point 
	     * where the segment between the first control point and arrow head 
	     * crosses box.
	     */
	    if (inBoxf(bez->list[0], bb)) {
		if (inBoxf(ND_coord(tail), bb)) {
		    agerr(AGWARN,
			  "%s -> %s: tail is inside head cluster %s\n",
			  agnameof(agtail(e)), agnameof(aghead(e)), agget(e, "lhead"));
		} else {
		    assert(bez->sflag);	/* must be arrowhead on tail */
		    p = boxIntersectf(bez->list[0], bez->sp, bb);
		    bez->list[3] = p;
		    bez->list[1] = mid_pointf(p, bez->sp);
		    bez->list[0] = mid_pointf(bez->list[1], bez->sp);
		    bez->list[2] = mid_pointf(bez->list[1], p);
		    if (bez->eflag)
			endi = arrowEndClip(e, bez->list,
					 starti, 0, nbez, bez->eflag);
		    endi += 3;
		    fixed = 1;
		}
	    } else {
		for (endi = 0; endi < size - 1; endi += 3) {
		    if (splineIntersectf(&(bez->list[endi]), bb))
			break;
		}
		if (endi == size - 1) {	/* no intersection */
		    assert(bez->eflag);
		    nbez->ep = boxIntersectf(bez->ep, bez->list[endi], bb);
		} else {
		    if (bez->eflag)
			endi =
			    arrowEndClip(e, bez->list,
					 starti, endi, nbez, bez->eflag);
		    endi += 3;
		}
		fixed = 1;
	    }
	}
    }
    if (fixed == 0) {		/* if no lh, or something went wrong, use original head */
	endi = size - 1;
	if (bez->eflag)
	    nbez->ep = bez->ep;
    }

    /* If tail cluster defined, find last Bezier
     * crossing tail cluster, and truncate spline to
     * box edge.
     * Otherwise, leave end alone.
     */
    fixed = 0;
    if (lt) {
	bb = &(GD_bb(lt));
	if (!inBoxf(ND_coord(tail), bb)) {
	    agerr(AGWARN, "%s -> %s: tail not inside tail cluster %s\n",
		agnameof(agtail(e)), agnameof(aghead(e)), agget(e, "ltail"));
	} else {
	    /* If last control point is in bb, degenerate case. Spline
	     * reduces to four points between arrow head, and the point
	     * where the segment between the last control point and the 
	     * arrow head crosses box.
	     */
	    if (inBoxf(bez->list[endi], bb)) {
		if (inBoxf(ND_coord(head), bb)) {
		    agerr(AGWARN,
			"%s -> %s: head is inside tail cluster %s\n",
		  	agnameof(agtail(e)), agnameof(aghead(e)), agget(e, "ltail"));
		} else {
		    assert(bez->eflag);	/* must be arrowhead on head */
		    p = boxIntersectf(bez->list[endi], nbez->ep, bb);
		    starti = endi - 3;
		    bez->list[starti] = p;
		    bez->list[starti + 2] = mid_pointf(p, nbez->ep);
		    bez->list[starti + 3] = mid_pointf(bez->list[starti + 2], nbez->ep);
		    bez->list[starti + 1] = mid_pointf(bez->list[starti + 2], p);
		    if (bez->sflag)
			starti = arrowStartClip(e, bez->list, starti,
				endi - 3, nbez, bez->sflag);
		    fixed = 1;
		}
	    } else {
		for (starti = endi; starti > 0; starti -= 3) {
		    for (i = 0; i < 4; i++)
			pts[i] = bez->list[starti - i];
		    if (splineIntersectf(pts, bb)) {
			for (i = 0; i < 4; i++)
			    bez->list[starti - i] = pts[i];
			break;
		    }
		}
		if (starti == 0) {
		    assert(bez->sflag);
		    nbez->sp =
			boxIntersectf(bez->sp, bez->list[starti], bb);
		} else {
		    starti -= 3;
		    if (bez->sflag)
			starti = arrowStartClip(e, bez->list, starti,
				endi - 3, nbez, bez->sflag);
		}
		fixed = 1;
	    }
	}
    }
    if (fixed == 0) {		/* if no lt, or something went wrong, use original tail */
	/* Note: starti == 0 */
	if (bez->sflag)
	    nbez->sp = bez->sp;
    }

    /* complete Bezier, free garbage and attach new Bezier to edge 
     */
    nbez->size = endi - starti + 1;
    nbez->list = N_GNEW(nbez->size, pointf);
    for (i = 0, j = starti; i < nbez->size; i++, j++)
	nbez->list[i] = bez->list[j];
    free(bez->list);
    free(bez);
    ED_spl(e)->list = nbez;
}
#if 0
static void dump(Dt_t* map)
{
  clust_t* p;  
  fprintf (stderr, "# in map: %d\n", dtsize(map));
  for (p=(clust_t*)dtfirst(map);p; p = (clust_t*)dtnext(map,p)) {
    fprintf (stderr, "  %s\n", p->name);
 }
}
#endif

/* dot_compoundEdges:
 */
void dot_compoundEdges(graph_t * g)
{
    edge_t *e;
    node_t *n;
#ifdef WITH_CGRAPH
    Dt_t* clustMap = mkClustMap (g);
#endif
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
#ifdef WITH_CGRAPH
	    makeCompoundEdge(g, e, clustMap);
#else
	    makeCompoundEdge(g, e);
#endif
	}
    }
#ifdef WITH_CGRAPH
    dtclose(clustMap);
#endif
}
