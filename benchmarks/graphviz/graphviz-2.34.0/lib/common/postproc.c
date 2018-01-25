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


#include "render.h"
#include "xlabels.h"

static int Rankdir;
static boolean Flip;
static pointf Offset;

static void place_flip_graph_label(graph_t * g);

#define M1 \
"/pathbox {\n\
    /Y exch %.5g sub def\n\
    /X exch %.5g sub def\n\
    /y exch %.5g sub def\n\
    /x exch %.5g sub def\n\
    newpath x y moveto\n\
    X y lineto\n\
    X Y lineto\n\
    x Y lineto\n\
    closepath stroke\n \
} def\n\
/dbgstart { gsave %.5g %.5g translate } def\n\
/arrowlength 10 def\n\
/arrowwidth arrowlength 2 div def\n\
/arrowhead {\n\
    gsave\n\
    rotate\n\
    currentpoint\n\
    newpath\n\
    moveto\n\
    arrowlength arrowwidth 2 div rlineto\n\
    0 arrowwidth neg rlineto\n\
    closepath fill\n\
    grestore\n\
} bind def\n\
/makearrow {\n\
    currentpoint exch pop sub exch currentpoint pop sub atan\n\
    arrowhead\n\
} bind def\n\
/point {\
    newpath\
    2 0 360 arc fill\
} def\
/makevec {\n\
    /Y exch def\n\
    /X exch def\n\
    /y exch def\n\
    /x exch def\n\
    newpath x y moveto\n\
    X Y lineto stroke\n\
    X Y moveto\n\
    x y makearrow\n\
} def\n"

#define M2 \
"/pathbox {\n\
    /X exch neg %.5g sub def\n\
    /Y exch %.5g sub def\n\
    /x exch neg %.5g sub def\n\
    /y exch %.5g sub def\n\
    newpath x y moveto\n\
    X y lineto\n\
    X Y lineto\n\
    x Y lineto\n\
    closepath stroke\n\
} def\n"

static pointf map_point(pointf p)
{
    p = ccwrotatepf(p, Rankdir * 90);
    p.x -= Offset.x;
    p.y -= Offset.y;
    return p;
}

static void map_edge(edge_t * e)
{
    int j, k;
    bezier bz;

    if (ED_spl(e) == NULL) {
	if ((Concentrate == FALSE) && (ED_edge_type(e) != IGNORED))
	    agerr(AGERR, "lost %s %s edge\n", agnameof(agtail(e)),
		  agnameof(aghead(e)));
	return;
    }
    for (j = 0; j < ED_spl(e)->size; j++) {
	bz = ED_spl(e)->list[j];
	for (k = 0; k < bz.size; k++)
	    bz.list[k] = map_point(bz.list[k]);
	if (bz.sflag)
	    ED_spl(e)->list[j].sp = map_point(ED_spl(e)->list[j].sp);
	if (bz.eflag)
	    ED_spl(e)->list[j].ep = map_point(ED_spl(e)->list[j].ep);
    }
    if (ED_label(e))
	ED_label(e)->pos = map_point(ED_label(e)->pos);
    if (ED_xlabel(e))
	ED_xlabel(e)->pos = map_point(ED_xlabel(e)->pos);
    /* vladimir */
    if (ED_head_label(e))
	ED_head_label(e)->pos = map_point(ED_head_label(e)->pos);
    if (ED_tail_label(e))
	ED_tail_label(e)->pos = map_point(ED_tail_label(e)->pos);
}

void translate_bb(graph_t * g, int rankdir)
{
    int c;
    boxf bb, new_bb;

    bb = GD_bb(g);
    if (rankdir == RANKDIR_LR || rankdir == RANKDIR_BT) {
	new_bb.LL = map_point(pointfof(bb.LL.x, bb.UR.y));
	new_bb.UR = map_point(pointfof(bb.UR.x, bb.LL.y));
    } else {
	new_bb.LL = map_point(pointfof(bb.LL.x, bb.LL.y));
	new_bb.UR = map_point(pointfof(bb.UR.x, bb.UR.y));
    }
    GD_bb(g) = new_bb;
    if (GD_label(g)) {
	GD_label(g)->pos = map_point(GD_label(g)->pos);
    }
    for (c = 1; c <= GD_n_cluster(g); c++)
	translate_bb(GD_clust(g)[c], rankdir);
}

/* translate_drawing:
 * Translate and/or rotate nodes, spline points, and bbox info if
 * necessary. Also, if Rankdir (!= RANKDIR_BT), reset ND_lw, ND_rw, 
 * and ND_ht to correct value.
 */
static void translate_drawing(graph_t * g)
{
    node_t *v;
    edge_t *e;
    int shift = (Offset.x || Offset.y);

    if (!shift && !Rankdir)
	return;
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if (Rankdir)
	    gv_nodesize(v, FALSE);
	ND_coord(v) = map_point(ND_coord(v));
	if (ND_xlabel(v))
	    ND_xlabel(v)->pos = map_point(ND_xlabel(v)->pos);
	if (State == GVSPLINES)
	    for (e = agfstout(g, v); e; e = agnxtout(g, e))
		map_edge(e);
    }
    translate_bb(g, GD_rankdir(g));
}

/* place_root_label:
 * Set position of root graph label.
 * Note that at this point, after translate_drawing, a
 * flipped drawing has been transposed, so we don't have
 * to worry about switching x and y.
 */
static void place_root_label(graph_t * g, pointf d)
{
    pointf p;

    if (GD_label_pos(g) & LABEL_AT_RIGHT) {
	p.x = GD_bb(g).UR.x - d.x / 2;
    } else if (GD_label_pos(g) & LABEL_AT_LEFT) {
	p.x = GD_bb(g).LL.x + d.x / 2;
    } else {
	p.x = (GD_bb(g).LL.x + GD_bb(g).UR.x) / 2;
    }

    if (GD_label_pos(g) & LABEL_AT_TOP) {
	p.y = GD_bb(g).UR.y - d.y / 2;
    } else {
	p.y = GD_bb(g).LL.y + d.y / 2;
    }

    GD_label(g)->pos = p;
    GD_label(g)->set = TRUE;
}

/* centerPt:
 * Calculate the center point of the xlabel. The returned positions for
 * xlabels always correspond to the lower left corner. 
 */
static pointf
centerPt (xlabel_t* xlp) {
  pointf p;

  p = xlp->pos;
  p.x += (xlp->sz.x)/2.0;
  p.y += (xlp->sz.y)/2.0;

  return p;
}

static int
printData (object_t* objs, int n_objs, xlabel_t* lbls, int n_lbls,
	   label_params_t* params) {
  int i;
  xlabel_t* xp;
  fprintf (stderr, "%d objs %d xlabels force=%d bb=(%.02f,%.02f) (%.02f,%.02f)\n",
	   n_objs, n_lbls, params->force, params->bb.LL.x, params->bb.LL.y,
	   params->bb.UR.x, params->bb.UR.y);
  if (Verbose < 2) return 0;
  fprintf(stderr, "objects\n");
  for (i = 0; i < n_objs; i++) {
    xp = objs->lbl;
    fprintf (stderr, " [%d] (%.02f,%.02f) (%.02f,%.02f) %p \"%s\"\n",
	    i, objs->pos.x,objs->pos.y,objs->sz.x,objs->sz.y, objs->lbl, 
	    (xp?((textlabel_t*)(xp->lbl))->text:""));
    objs++;
  }
  fprintf(stderr, "xlabels\n");
  for (i = 0; i < n_lbls; i++) {
    fprintf (stderr, " [%d] %p set %d (%.02f,%.02f) (%.02f,%.02f) %s\n",
	     i, lbls,  lbls->set, lbls->pos.x,lbls->pos.y, lbls->sz.x,lbls->sz.y, ((textlabel_t*)lbls->lbl)->text);  
    lbls++;
  }
  return 0;
}

static pointf
edgeTailpoint (Agedge_t* e)
{
    splines *spl;
    bezier *bez;

    if ((spl = getsplinepoints(e)) == NULL) {
	pointf p;
	p.x = p.y = 0;
	return p;
    }
    bez = &spl->list[0];
    if (bez->sflag) {
	return bez->sp;
    } else {
	return bez->list[0];
    }
}

static pointf
edgeHeadpoint (Agedge_t* e)
{
    splines *spl;
    bezier *bez;

    if ((spl = getsplinepoints(e)) == NULL) {
	pointf p;
	p.x = p.y = 0;
	return p;
    }
    bez = &spl->list[spl->size - 1];
    if (bez->eflag) {
	return bez->ep;
    } else {
	return bez->list[bez->size - 1];
    }
}

/* adjustBB:
 */
static boxf
adjustBB (object_t* objp, boxf bb)
{
    pointf ur;

    /* Adjust bounding box */
    bb.LL.x = MIN(bb.LL.x, objp->pos.x);
    bb.LL.y = MIN(bb.LL.y, objp->pos.y);
    ur.x = objp->pos.x + objp->sz.x;
    ur.y = objp->pos.y + objp->sz.y;
    bb.UR.x = MAX(bb.UR.x, ur.x);
    bb.UR.y = MAX(bb.UR.y, ur.y);

    return bb;
}

/* addXLabel:
 * Set up xlabel_t object and connect with related object.
 * If initObj is set, initialize the object.
 */
static void
addXLabel (textlabel_t* lp, object_t* objp, xlabel_t* xlp, int initObj, pointf pos)
{
    if (initObj) {
	objp->sz.x = 0;
	objp->sz.y = 0;
	objp->pos = pos;
    }

    if (Flip) {
	xlp->sz.x = lp->dimen.y;
	xlp->sz.y = lp->dimen.x;
    }
    else {
	xlp->sz = lp->dimen;
    }
    xlp->lbl = lp;
    xlp->set = 0;
    objp->lbl = xlp;
}

/* addLabelObj:
 * Set up obstacle object based on set external label.
 * This includes dot edge labels.
 * Use label information to determine size and position of object.
 * Then adjust given bounding box bb to include label and return new bb.
 */
static boxf
addLabelObj (textlabel_t* lp, object_t* objp, boxf bb)
{
    if (Flip) {
	objp->sz.x = lp->dimen.y; 
	objp->sz.y = lp->dimen.x;
    }
    else {
	objp->sz.x = lp->dimen.x; 
	objp->sz.y = lp->dimen.y;
    }
    objp->pos = lp->pos;
    objp->pos.x -= (objp->sz.x) / 2.0;
    objp->pos.y -= (objp->sz.y) / 2.0;

    return adjustBB(objp, bb);
}

/* addNodeOjb:
 * Set up obstacle object based on a node.
 * Use node information to determine size and position of object.
 * Then adjust given bounding box bb to include label and return new bb.
 */
static boxf
addNodeObj (node_t* np, object_t* objp, boxf bb)
{
    if (Flip) {
	objp->sz.x = INCH2PS(ND_height(np));
	objp->sz.y = INCH2PS(ND_width(np));
    }
    else {
	objp->sz.x = INCH2PS(ND_width(np));
	objp->sz.y = INCH2PS(ND_height(np));
    }
    objp->pos = ND_coord(np);
    objp->pos.x -= (objp->sz.x) / 2.0;
    objp->pos.y -= (objp->sz.y) / 2.0;

    return adjustBB(objp, bb);
}

typedef struct {
    boxf bb;
    object_t* objp;
} cinfo_t;

static cinfo_t
addClusterObj (Agraph_t* g, cinfo_t info)
{
    int c;

    for (c = 1; c <= GD_n_cluster(g); c++)
	info = addClusterObj (GD_clust(g)[c], info);
    if ((g != agroot(g)) && (GD_label(g)) && GD_label(g)->set) {
	object_t* objp = info.objp;
	info.bb = addLabelObj (GD_label(g), objp, info.bb);
	info.objp++;
    }

    return info;
}

static int
countClusterLabels (Agraph_t* g)
{
    int c, i = 0;
    if ((g != agroot(g)) && (GD_label(g)) && GD_label(g)->set)
	i++;
    for (c = 1; c <= GD_n_cluster(g); c++)
	i += countClusterLabels (GD_clust(g)[c]);
    return i;
}

/* addXLabels:
 * Position xlabels and any unpositioned edge labels using
 * a map placement algorithm to avoid overlap.
 *
 * TODO: interaction with spline=ortho
 */
static void addXLabels(Agraph_t * gp)
{
    Agnode_t *np;
    Agedge_t *ep;
    int cnt, i, n_objs, n_lbls;
    int n_nlbls = 0;		/* # of unset node xlabels */
    int n_elbls = 0;		/* # of unset edge labels or xlabels */
    int n_set_lbls = 0;		/* # of set xlabels and edge labels */
    int n_clbls = 0;		/* # of set cluster labels */
    boxf bb;
    pointf ur;
    textlabel_t* lp;
    label_params_t params;
    object_t* objs;
    xlabel_t* lbls;
    object_t* objp;
    xlabel_t* xlp;
    Agsym_t* force;
    int et = EDGE_TYPE(gp);

    if (!(GD_has_labels(gp) & NODE_XLABEL) &&
	!(GD_has_labels(gp) & EDGE_XLABEL) &&
	!(GD_has_labels(gp) & TAIL_LABEL) &&
	!(GD_has_labels(gp) & HEAD_LABEL) &&
	(!(GD_has_labels(gp) & EDGE_LABEL) || EdgeLabelsDone))
	return;

    for (np = agfstnode(gp); np; np = agnxtnode(gp, np)) {
	if (ND_xlabel(np)) {
	    if (ND_xlabel(np)->set)
		n_set_lbls++;
	    else
		n_nlbls++;
	}
	for (ep = agfstout(gp, np); ep; ep = agnxtout(gp, ep)) {
	    if (ED_xlabel(ep)) {
		if (ED_xlabel(ep)->set)
		    n_set_lbls++;
		else if (et != ET_NONE)
		    n_elbls++;
	    }
	    if (ED_head_label(ep)) {
		if (ED_head_label(ep)->set)
		    n_set_lbls++;
		else if (et != ET_NONE)
		    n_elbls++;
	    }
	    if (ED_tail_label(ep)) {
		if (ED_tail_label(ep)->set)
		    n_set_lbls++;
		else if (et != ET_NONE)
		    n_elbls++;
	    }
	    if (ED_label(ep)) {
		if (ED_label(ep)->set)
		    n_set_lbls++;
		else if (et != ET_NONE)
		    n_elbls++;
	    }
	}
    }
    if (GD_has_labels(gp) & GRAPH_LABEL)
	n_clbls = countClusterLabels (gp);

    /* A label for each unpositioned external label */
    n_lbls = n_nlbls + n_elbls;
    if (n_lbls == 0) return;

    /* An object for each node, each positioned external label, any cluster label, 
     * and all unset edge labels and xlabels.
     */
    n_objs = agnnodes(gp) + n_set_lbls + n_clbls + n_elbls;
    objp = objs = N_NEW(n_objs, object_t);
    xlp = lbls = N_NEW(n_lbls, xlabel_t);
    bb.LL = pointfof(INT_MAX, INT_MAX);
    bb.UR = pointfof(-INT_MAX, -INT_MAX);

    for (np = agfstnode(gp); np; np = agnxtnode(gp, np)) {

	bb = addNodeObj (np, objp, bb);
	if ((lp = ND_xlabel(np))) {
	    if (lp->set) {
		objp++;
		bb = addLabelObj (lp, objp, bb);
	    }
	    else {
		addXLabel (lp, objp, xlp, 0, ur); 
		xlp++;
	    }
	}
	objp++;
	for (ep = agfstout(gp, np); ep; ep = agnxtout(gp, ep)) {
	    if ((lp = ED_label(ep))) {
		if (lp->set) {
		    bb = addLabelObj (lp, objp, bb);
		}
		else if (et != ET_NONE) {
		    addXLabel (lp, objp, xlp, 1, edgeMidpoint(gp, ep)); 
		    xlp++;
		}
	        objp++;
	    }
	    if ((lp = ED_tail_label(ep))) {
		if (lp->set) {
		    bb = addLabelObj (lp, objp, bb);
		}
		else if (et != ET_NONE) {
		    addXLabel (lp, objp, xlp, 1, edgeTailpoint(ep)); 
		    xlp++;
		}
		objp++;
	    }
	    if ((lp = ED_head_label(ep))) {
		if (lp->set) {
		    bb = addLabelObj (lp, objp, bb);
		}
		else if (et != ET_NONE) {
		    addXLabel (lp, objp, xlp, 1, edgeHeadpoint(ep)); 
		    xlp++;
		}
		objp++;
	    }
	    if ((lp = ED_xlabel(ep))) {
		if (lp->set) {
		    bb = addLabelObj (lp, objp, bb);
		}
		else if (et != ET_NONE) {
		    addXLabel (lp, objp, xlp, 1, edgeMidpoint(gp, ep)); 
		    xlp++;
		}
		objp++;
	    }
	}
    }
    if (n_clbls) {
	cinfo_t info;
	info.bb = bb;
	info.objp = objp;
	info = addClusterObj (gp, info);
	bb = info.bb;
    }

    force = agfindgraphattr(gp, "forcelabels");

    params.force = late_bool(gp, force, TRUE);
    params.bb = bb;
    placeLabels(objs, n_objs, lbls, n_lbls, &params);
    if (Verbose)
	printData(objs, n_objs, lbls, n_lbls, &params);

    xlp = lbls;
    cnt = 0;
    for (i = 0; i < n_lbls; i++) {
	if (xlp->set) {
	    cnt++;
	    lp = (textlabel_t *) (xlp->lbl);
	    lp->set = 1;
	    lp->pos = centerPt(xlp);
	    updateBB (gp, lp);
	}
	xlp++;
    }
    if (Verbose)
	fprintf (stderr, "%d out of %d labels positioned.\n", cnt, n_lbls);
    else if (cnt != n_lbls)
	agerr(AGWARN, "%d out of %d exterior labels positioned.\n", cnt, n_lbls);
    free(objs);
    free(lbls);
}

/* dotneato_postprocess:
 * Set graph and cluster label positions.
 * Add space for root graph label and translate graph accordingly.
 * Set final nodesize using ns.
 * Assumes the boxes of all clusters have been computed.
 * When done, the bounding box of g has LL at origin.
 */
void gv_postprocess(Agraph_t * g, int allowTranslation)
{
    double diff;
    pointf dimen = { 0., 0. };


    Rankdir = GD_rankdir(g);
    Flip = GD_flip(g);
    /* Handle cluster labels */
    if (Flip)
	place_flip_graph_label(g);
    else
	place_graph_label(g);

    /* Everything has been placed except the root graph label, if any.
     * The graph positions have not yet been rotated back if necessary.
     */
    addXLabels(g);

    /* Add space for graph label if necessary */
    if (GD_label(g) && !GD_label(g)->set) {
	dimen = GD_label(g)->dimen;
	PAD(dimen);
	if (Flip) {
	    if (GD_label_pos(g) & LABEL_AT_TOP) {
		GD_bb(g).UR.x += dimen.y;
	    } else {
		GD_bb(g).LL.x -= dimen.y;
	    }

	    if (dimen.x > (GD_bb(g).UR.y - GD_bb(g).LL.y)) {
		diff = dimen.x - (GD_bb(g).UR.y - GD_bb(g).LL.y);
		diff = diff / 2.;
		GD_bb(g).LL.y -= diff;
		GD_bb(g).UR.y += diff;
	    }
	} else {
	    if (GD_label_pos(g) & LABEL_AT_TOP) {
		if (Rankdir == RANKDIR_TB)
		    GD_bb(g).UR.y += dimen.y;
		else
		    GD_bb(g).LL.y -= dimen.y;
	    } else {
		if (Rankdir == RANKDIR_TB)
		    GD_bb(g).LL.y -= dimen.y;
		else
		    GD_bb(g).UR.y += dimen.y;
	    }

	    if (dimen.x > (GD_bb(g).UR.x - GD_bb(g).LL.x)) {
		diff = dimen.x - (GD_bb(g).UR.x - GD_bb(g).LL.x);
		diff = diff / 2.;
		GD_bb(g).LL.x -= diff;
		GD_bb(g).UR.x += diff;
	    }
	}
    }
    if (allowTranslation) {
	switch (Rankdir) {
	case RANKDIR_TB:
	    Offset = GD_bb(g).LL;
	    break;
	case RANKDIR_LR:
	    Offset = pointfof(-GD_bb(g).UR.y, GD_bb(g).LL.x);
	    break;
	case RANKDIR_BT:
	    Offset = pointfof(GD_bb(g).LL.x, -GD_bb(g).UR.y);
	    break;
	case RANKDIR_RL:
	    Offset = pointfof(GD_bb(g).LL.y, GD_bb(g).LL.x);
	    break;
	}
	translate_drawing(g);
    }
    if (GD_label(g) && !GD_label(g)->set)
	place_root_label(g, dimen);

    if (Show_boxes) {
	char buf[BUFSIZ];
	if (Flip)
	    sprintf(buf, M2, Offset.x, Offset.y, Offset.x, Offset.y);
	else
	    sprintf(buf, M1, Offset.y, Offset.x, Offset.y, Offset.x,
		    -Offset.x, -Offset.y);
	Show_boxes[0] = strdup(buf);
    }
}

void dotneato_postprocess(Agraph_t * g)
{
    gv_postprocess(g, 1);
}

/* place_flip_graph_label:
 * Put cluster labels recursively in the flip case.
 */
static void place_flip_graph_label(graph_t * g)
{
    int c;
    pointf p, d;

    if ((g != agroot(g)) && (GD_label(g)) && !GD_label(g)->set) {
	if (GD_label_pos(g) & LABEL_AT_TOP) {
	    d = GD_border(g)[RIGHT_IX];
	    p.x = GD_bb(g).UR.x - d.x / 2;
	} else {
	    d = GD_border(g)[LEFT_IX];
	    p.x = GD_bb(g).LL.x + d.x / 2;
	}

	if (GD_label_pos(g) & LABEL_AT_RIGHT) {
	    p.y = GD_bb(g).LL.y + d.y / 2;
	} else if (GD_label_pos(g) & LABEL_AT_LEFT) {
	    p.y = GD_bb(g).UR.y - d.y / 2;
	} else {
	    p.y = (GD_bb(g).LL.y + GD_bb(g).UR.y) / 2;
	}
	GD_label(g)->pos = p;
	GD_label(g)->set = TRUE;
    }

    for (c = 1; c <= GD_n_cluster(g); c++)
	place_flip_graph_label(GD_clust(g)[c]);
}

/* place_graph_label:
 * Put cluster labels recursively in the non-flip case.
 * The adjustments to the bounding boxes should no longer
 * be necessary, since we now guarantee the label fits in
 * the cluster.
 */
void place_graph_label(graph_t * g)
{
    int c;
    pointf p, d;

    if ((g != agroot(g)) && (GD_label(g)) && !GD_label(g)->set) {
	if (GD_label_pos(g) & LABEL_AT_TOP) {
	    d = GD_border(g)[TOP_IX];
	    p.y = GD_bb(g).UR.y - d.y / 2;
	} else {
	    d = GD_border(g)[BOTTOM_IX];
	    p.y = GD_bb(g).LL.y + d.y / 2;
	}

	if (GD_label_pos(g) & LABEL_AT_RIGHT) {
	    p.x = GD_bb(g).UR.x - d.x / 2;
	} else if (GD_label_pos(g) & LABEL_AT_LEFT) {
	    p.x = GD_bb(g).LL.x + d.x / 2;
	} else {
	    p.x = (GD_bb(g).LL.x + GD_bb(g).UR.x) / 2;
	}
	GD_label(g)->pos = p;
	GD_label(g)->set = TRUE;
    }

    for (c = 1; c <= GD_n_cluster(g); c++)
	place_graph_label(GD_clust(g)[c]);
}
