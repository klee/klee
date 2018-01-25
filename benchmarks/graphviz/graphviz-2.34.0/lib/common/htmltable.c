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


/* Implementation of HTML-like tables.
 * 
 * The (now purged) CodeGen graphics model, especially with integral coodinates, is
 * not adequate to handle this as we would like. In particular, it is
 * difficult to handle notions of adjacency and correct rounding to pixels.
 * For example, if 2 adjacent boxes bb1.UR.x == bb2.LL.x, the rectangles
 * may be drawn overlapping. However, if we use bb1.UR.x+1 == bb2.LL.x
 * there may or may not be a gap between them, even in the same device
 * depending on their positions. When CELLSPACING > 1, this isn't as much
 * of a problem.
 *
 * We allow negative spacing as a hack to allow overlapping cell boundaries.
 * For the reasons discussed above, this is difficult to get correct.
 * This is an important enough case we should extend the table model to
 * support it correctly. This could be done by allowing a table attribute,
 * e.g., CELLGRID=n, which sets CELLBORDER=0 and has the border drawing
 * handled correctly by the table.
 */

#include <assert.h>
#include "render.h"
#include "htmltable.h"
#include "agxbuf.h"
#include "pointset.h"
#include "intset.h"

#define DEFAULT_BORDER    1
#define DEFAULT_CELLPADDING  2
#define DEFAULT_CELLSPACING  2

typedef struct {
    pointf pos;
    htmlfont_t finfo;
    void *obj;
    graph_t *g;
    char *imgscale;
    char *objid;
    boolean objid_set;
} htmlenv_t;

typedef struct {
    char *url;
    char *tooltip;
    char *target;
    char *id;
    boolean explicit_tooltip;
    point LL;
    point UR;
} htmlmap_data_t;

#ifdef DEBUG
static void printCell(htmlcell_t * cp, int ind);
#endif

/* pushFontInfo:
 * Replace current font attributes in env with ones from fp,
 * storing old attributes in savp. We only deal with attributes
 * set in env. The attributes are restored via popFontInfo.
 */
static void
pushFontInfo(htmlenv_t * env, htmlfont_t * fp, htmlfont_t * savp)
{
    if (env->finfo.name) {
	if (fp->name) {
	    savp->name = env->finfo.name;
	    env->finfo.name = fp->name;
	} else
	    savp->name = NULL;
    }
    if (env->finfo.color) {
	if (fp->color) {
	    savp->color = env->finfo.color;
	    env->finfo.color = fp->color;
	} else
	    savp->color = NULL;
    }
    if (env->finfo.size >= 0) {
	if (fp->size >= 0) {
	    savp->size = env->finfo.size;
	    env->finfo.size = fp->size;
	} else
	    savp->size = -1.0;
    }
}

/* popFontInfo:
 * Restore saved font attributes.
 * Copy only set values.
 */
static void popFontInfo(htmlenv_t * env, htmlfont_t * savp)
{
    if (savp->name)
	env->finfo.name = savp->name;
    if (savp->color)
	env->finfo.color = savp->color;
    if (savp->size >= 0.0)
	env->finfo.size = savp->size;
}

static void
emit_htextparas(GVJ_t * job, int nparas, htextpara_t * paras, pointf p,
		double halfwidth_x, htmlfont_t finfo, boxf b, int simple)
{
    int i, j;
    double center_x, left_x, right_x, fsize_;
    char *fname_, *fcolor_;
    textpara_t tl;
    pointf p_ = { 0.0, 0.0 };
    textpara_t *ti;

    center_x = p.x;
    left_x = center_x - halfwidth_x;
    right_x = center_x + halfwidth_x;

    /* Initial p is in center of text block; set initial baseline
     * to top of text block.
     */
    p_.y = p.y + (b.UR.y - b.LL.y) / 2.0;

    gvrender_begin_label(job, LABEL_HTML);
    for (i = 0; i < nparas; i++) {
	/* set p.x to leftmost point where the line of text begins */
	switch (paras[i].just) {
	case 'l':
	    p.x = left_x;
	    break;
	case 'r':
	    p.x = right_x - paras[i].size;
	    break;
	default:
	case 'n':
	    p.x = center_x - paras[i].size / 2.0;
	    break;
	}
	p_.y -= paras[i].lfsize;	/* move to current base line */

	ti = paras[i].items;
	for (j = 0; j < paras[i].nitems; j++) {
	    if (ti->font && (ti->font->size > 0))
		fsize_ = ti->font->size;
	    else
		fsize_ = finfo.size;
	    if (ti->font && ti->font->name)
		fname_ = ti->font->name;
	    else
		fname_ = finfo.name;
	    if (ti->font && ti->font->color)
		fcolor_ = ti->font->color;
	    else
		fcolor_ = finfo.color;

	    gvrender_set_pencolor(job, fcolor_);

	    tl.str = ti->str;
	    tl.fontname = fname_;
	    tl.fontsize = fsize_;
	    tl.font = ti->font;
	    tl.yoffset_layout = ti->yoffset_layout;
	    if (simple)
		tl.yoffset_centerline = ti->yoffset_centerline;
	    else
		tl.yoffset_centerline = 1;
	    tl.postscript_alias = ti->postscript_alias;
	    tl.layout = ti->layout;
	    tl.width = ti->size;
	    tl.height = paras[i].lfsize;
	    tl.just = 'l';

	    p_.x = p.x;
	    gvrender_textpara(job, p_, &tl);
	    p.x += ti->size;
	    ti++;
	}
    }

    gvrender_end_label(job);
}

static void emit_html_txt(GVJ_t * job, htmltxt_t * tp, htmlenv_t * env)
{
    double halfwidth_x;
    pointf p;

    /* make sure that there is something to do */
    if (tp->nparas < 1)
	return;

    halfwidth_x = ((double) (tp->box.UR.x - tp->box.LL.x)) / 2.0;
    p.x = env->pos.x + ((double) (tp->box.UR.x + tp->box.LL.x)) / 2.0;
    p.y = env->pos.y + ((double) (tp->box.UR.y + tp->box.LL.y)) / 2.0;

    emit_htextparas(job, tp->nparas, tp->paras, p, halfwidth_x, env->finfo,
		    tp->box, tp->simple);
}

static void doSide(GVJ_t * job, pointf p, double wd, double ht)
{
    boxf BF;

    BF.LL = p;
    BF.UR.x = p.x + wd;
    BF.UR.y = p.y + ht;
    gvrender_box(job, BF, 1);
}

/* mkPts:
 * Convert boxf into four corner points
 * If border is > 1, inset the points by half the border.
 * It is assume AF is pointf[4], so the data is store there
 * and AF is returned.
 */
static pointf *mkPts(pointf * AF, boxf b, int border)
{
    AF[0] = b.LL;
    AF[2] = b.UR;
    if (border > 1) {
	double delta = ((double) border) / 2.0;
	AF[0].x += delta;
	AF[0].y += delta;
	AF[2].x -= delta;
	AF[2].y -= delta;
    }
    AF[1].x = AF[2].x;
    AF[1].y = AF[0].y;
    AF[3].x = AF[0].x;
    AF[3].y = AF[2].y;

    return AF;
}

/* doBorder:
 * Draw a rectangular border for the box b.
 * Handles dashed and dotted styles, rounded corners.
 * Also handles thick lines.
 * Assume dp->border > 0
 */
static void doBorder(GVJ_t * job, htmldata_t * dp, boxf b)
{
    pointf AF[4];
    char *sptr[2];
    char *color = (dp->pencolor ? dp->pencolor : DEFAULT_COLOR);

    gvrender_set_pencolor(job, color);
    if ((dp->style & (DASHED | DOTTED))) {
	sptr[0] = sptr[1] = NULL;
	if (dp->style & DASHED)
	    sptr[0] = "dashed";
	else if (dp->style & DOTTED)
	    sptr[0] = "dotted";
	gvrender_set_style(job, sptr);
    } else
	gvrender_set_style(job, job->gvc->defaultlinestyle);
    gvrender_set_penwidth(job, dp->border);

    if (dp->style & ROUNDED)
	round_corners(job, mkPts(AF, b, dp->border), 4, ROUNDED, 0);
    else {
	if (dp->border > 1) {
	    double delta = ((double) dp->border) / 2.0;
	    b.LL.x += delta;
	    b.LL.y += delta;
	    b.UR.x -= delta;
	    b.UR.y -= delta;
	}
	gvrender_box(job, b, 0);
    }
}

/* setFill:
 * Set up fill values from given color; make pen transparent.
 * Return type of fill required.
 */
static int
setFill(GVJ_t * job, char *color, int angle, int style, char *clrs[2])
{
    int filled;
    float frac;
    if (findStopColor(color, clrs, &frac)) {
	gvrender_set_fillcolor(job, clrs[0]);
	if (clrs[1])
	    gvrender_set_gradient_vals(job, clrs[1], angle, frac);
	else
	    gvrender_set_gradient_vals(job, DEFAULT_COLOR, angle, frac);
	if (style & RADIAL)
	    filled = RGRADIENT;
	else
	    filled = GRADIENT;
    } else {
	gvrender_set_fillcolor(job, color);
	filled = FILL;
    }
    gvrender_set_pencolor(job, "transparent");
    return filled;
}

/* initAnchor:
 * Save current map values
 * Initialize fields in job->obj pertaining to anchors.
 * In particular, this also sets the output rectangle.
 * If there is something to do, close current anchor if
 * necessary, start the anchor and returns 1.
 * Otherwise, it returns 0.
 *
 * FIX: Should we provide a tooltip if none is set, as is done
 * for nodes, edges, etc. ?
 */
static int
initAnchor(GVJ_t * job, htmlenv_t * env, htmldata_t * data, boxf b,
	   htmlmap_data_t * save, int closePrev)
{
    obj_state_t *obj = job->obj;
    int changed;
    char *id;
    static int anchorId;
    int internalId = 0;
    agxbuf xb;
    char intbuf[30];		/* hold 64-bit decimal integer */
    unsigned char buf[SMALLBUF];

    save->url = obj->url;
    save->tooltip = obj->tooltip;
    save->target = obj->target;
    save->id = obj->id;
    save->explicit_tooltip = obj->explicit_tooltip;
    id = data->id;
    if (!id || !*id) {		/* no external id, so use the internal one */
	agxbinit(&xb, SMALLBUF, buf);
	if (!env->objid) {
	    env->objid = strdup(getObjId(job, obj->u.n, &xb));
	    env->objid_set = 1;
	}
	agxbput(&xb, env->objid);
	sprintf(intbuf, "_%d", anchorId++);
	agxbput(&xb, intbuf);
	id = agxbuse(&xb);
	internalId = 1;
    }
    changed =
	initMapData(job, NULL, data->href, data->title, data->target, id,
		    obj->u.g);
    if (internalId)
	agxbfree(&xb);

    if (changed) {
	if (closePrev && (save->url || save->explicit_tooltip))
	    gvrender_end_anchor(job);
	if (obj->url || obj->explicit_tooltip) {
	    emit_map_rect(job, b);
	    gvrender_begin_anchor(job,
				  obj->url, obj->tooltip, obj->target,
				  obj->id);
	}
    }
    return changed;
}

#define RESET(fld) \
  if(obj->fld != save->fld) {free(obj->fld); obj->fld = save->fld;}

/* endAnchor:
 * Pop context pushed by initAnchor.
 * This is done by ending current anchor, restoring old values and
 * freeing new, and reopening previous anchor if necessary.
 *
 * NB: We don't save or restore geometric map info. This is because
 * this preservation of map context is only necessary for SVG-like
 * systems where graphical items are wrapped in an anchor, and we map
 * top-down. For ordinary map anchors, this is all done bottom-up, so
 * the geometric map info at the higher level hasn't been emitted yet.
 */
static void endAnchor(GVJ_t * job, htmlmap_data_t * save, int openPrev)
{
    obj_state_t *obj = job->obj;

    if (obj->url || obj->explicit_tooltip)
	gvrender_end_anchor(job);
    RESET(url);
    RESET(tooltip);
    RESET(target);
    RESET(id);
    obj->explicit_tooltip = save->explicit_tooltip;
    if (openPrev && (obj->url || obj->explicit_tooltip))
	gvrender_begin_anchor(job,
			      obj->url, obj->tooltip, obj->target,
			      obj->id);
}

/* forward declaration */
static void emit_html_cell(GVJ_t * job, htmlcell_t * cp, htmlenv_t * env);

/* emit_html_rules:
 * place vertical and horizontal lines between adjacent cells and
 * extend the lines to intersect the rounded table boundary 
 */
static void
emit_html_rules(GVJ_t * job, htmlcell_t * cp, htmlenv_t * env, char *color, htmlcell_t* nextc)
{
    pointf rule_pt;
    double rule_length;
    unsigned char base;
    boxf pts = cp->data.box;
    pointf pos = env->pos;

    if (!color)
	color = DEFAULT_COLOR;
    gvrender_set_fillcolor(job, color);
    gvrender_set_pencolor(job, color);

    pts = cp->data.box;
    pts.LL.x += pos.x;
    pts.UR.x += pos.x;
    pts.LL.y += pos.y;
    pts.UR.y += pos.y;

    //Determine vertical line coordinate and length
    if ((cp->ruled & HTML_VRULE) && (cp->col + cp->cspan < cp->parent->cc)) {
	if (cp->row == 0) {	// first row
	    // extend to center of table border and add half cell spacing
	    base = cp->parent->data.border + cp->parent->data.space / 2;
	    rule_pt.y = pts.LL.y - cp->parent->data.space / 2;
	} else if (cp->row + cp->rspan == cp->parent->rc) {	// bottom row
	    // extend to center of table border and add half cell spacing
	    base = cp->parent->data.border + cp->parent->data.space / 2;
	    rule_pt.y = pts.LL.y - cp->parent->data.space / 2 - base;
	} else {
	    base = 0;
	    rule_pt.y = pts.LL.y - cp->parent->data.space / 2;
	}
	rule_pt.x = pts.UR.x + cp->parent->data.space / 2;
	rule_length = base + pts.UR.y - pts.LL.y + cp->parent->data.space;
	doSide(job, rule_pt, 0, rule_length);
    }
    //Determine the horizontal coordinate and length
    if ((cp->ruled & HTML_HRULE) && (cp->row + cp->rspan < cp->parent->rc)) {
	if (cp->col == 0) {	// first column 
	    // extend to center of table border and add half cell spacing
	    base = cp->parent->data.border + cp->parent->data.space / 2;
	    rule_pt.x = pts.LL.x - base - cp->parent->data.space / 2;
	    if (cp->col + cp->cspan == cp->parent->cc)	// also last column
		base *= 2;
	    /* incomplete row of cells; extend line to end */
	    else if (nextc && (nextc->row != cp->row)) {
		base += (cp->parent->data.box.UR.x + pos.x) - (pts.UR.x + cp->parent->data.space / 2);
	    }
	} else if (cp->col + cp->cspan == cp->parent->cc) {	// last column
	    // extend to center of table border and add half cell spacing
	    base = cp->parent->data.border + cp->parent->data.space / 2;
	    rule_pt.x = pts.LL.x - cp->parent->data.space / 2;
	} else {
	    base = 0;
	    rule_pt.x = pts.LL.x - cp->parent->data.space / 2;
	    /* incomplete row of cells; extend line to end */
	    if (nextc && (nextc->row != cp->row)) {
		base += (cp->parent->data.box.UR.x + pos.x) - (pts.UR.x + cp->parent->data.space / 2);
	    }
	}
	rule_pt.y = pts.LL.y - cp->parent->data.space / 2;
	rule_length = base + pts.UR.x - pts.LL.x + cp->parent->data.space;
	doSide(job, rule_pt, rule_length, 0);
    }
}

static void emit_html_tbl(GVJ_t * job, htmltbl_t * tbl, htmlenv_t * env)
{
    boxf pts = tbl->data.box;
    pointf pos = env->pos;
    htmlcell_t **cells = tbl->u.n.cells;
    htmlcell_t *cp;
    static htmlfont_t savef;
    htmlmap_data_t saved;
    int anchor;			/* if true, we need to undo anchor settings. */
    int doAnchor = (tbl->data.href || tbl->data.target);
    pointf AF[4];

    if (tbl->font)
	pushFontInfo(env, tbl->font, &savef);

    pts.LL.x += pos.x;
    pts.UR.x += pos.x;
    pts.LL.y += pos.y;
    pts.UR.y += pos.y;

    if (doAnchor && !(job->flags & EMIT_CLUSTERS_LAST))
	anchor = initAnchor(job, env, &tbl->data, pts, &saved, 1);
    else
	anchor = 0;

    if (!(tbl->data.style & INVISIBLE)) {

	/* Fill first */
	if (tbl->data.bgcolor) {
	    char *clrs[2];
	    int filled =
		setFill(job, tbl->data.bgcolor, tbl->data.gradientangle,
			tbl->data.style, clrs);
	    if (tbl->data.style & ROUNDED) {
		round_corners(job, mkPts(AF, pts, tbl->data.border), 4,
			      ROUNDED, filled);
	    } else
		gvrender_box(job, pts, filled);
	    free(clrs[0]);
	}

	while (*cells) {
	    emit_html_cell(job, *cells, env);
	    cells++;
	}

	/* Draw table rules and border.
	 * Draw after cells so we can draw over any fill.
	 * At present, we set the penwidth to 1 for rules until we provide the calculations to take
	 * into account wider rules.
	 */
	cells = tbl->u.n.cells;
	gvrender_set_penwidth(job, 1.0);
	while ((cp = *cells++)) {
	    if (cp->ruled)
		emit_html_rules(job, cp, env, tbl->data.pencolor, *cells);
	}

	if (tbl->data.border)
	    doBorder(job, &tbl->data, pts);

    }

    if (anchor)
	endAnchor(job, &saved, 1);

    if (doAnchor && (job->flags & EMIT_CLUSTERS_LAST)) {
	if (initAnchor(job, env, &tbl->data, pts, &saved, 0))
	    endAnchor(job, &saved, 0);
    }

    if (tbl->font)
	popFontInfo(env, &savef);
}

/* emit_html_img:
 * The image will be centered in the given box.
 * Scaling is determined by either the image's scale attribute,
 * or the imagescale attribute of the graph object being drawn.
 */
static void emit_html_img(GVJ_t * job, htmlimg_t * cp, htmlenv_t * env)
{
    pointf A[4];
    boxf bb = cp->box;
    char *scale;

    bb.LL.x += env->pos.x;
    bb.LL.y += env->pos.y;
    bb.UR.x += env->pos.x;
    bb.UR.y += env->pos.y;

    A[0] = bb.UR;
    A[2] = bb.LL;
    A[1].x = A[2].x;
    A[1].y = A[0].y;
    A[3].x = A[0].x;
    A[3].y = A[2].y;

    if (cp->scale)
	scale = cp->scale;
    else
	scale = env->imgscale;
    gvrender_usershape(job, cp->src, A, 4, TRUE, scale);
}

static void emit_html_cell(GVJ_t * job, htmlcell_t * cp, htmlenv_t * env)
{
    htmlmap_data_t saved;
    boxf pts = cp->data.box;
    pointf pos = env->pos;
    int inAnchor, doAnchor = (cp->data.href || cp->data.target);
    pointf AF[4];

    pts.LL.x += pos.x;
    pts.UR.x += pos.x;
    pts.LL.y += pos.y;
    pts.UR.y += pos.y;

    if (doAnchor && !(job->flags & EMIT_CLUSTERS_LAST))
	inAnchor = initAnchor(job, env, &cp->data, pts, &saved, 1);
    else
	inAnchor = 0;

    if (!(cp->data.style & INVISIBLE)) {
	if (cp->data.bgcolor) {
	    char *clrs[2];
	    int filled =
		setFill(job, cp->data.bgcolor, cp->data.gradientangle,
			cp->data.style, clrs);
	    if (cp->data.style & ROUNDED) {
		round_corners(job, mkPts(AF, pts, cp->data.border), 4,
			      ROUNDED, filled);
	    } else
		gvrender_box(job, pts, filled);
	    free(clrs[0]);
	}

	if (cp->data.border)
	    doBorder(job, &cp->data, pts);

	if (cp->child.kind == HTML_TBL)
	    emit_html_tbl(job, cp->child.u.tbl, env);
	else if (cp->child.kind == HTML_IMAGE)
	    emit_html_img(job, cp->child.u.img, env);
	else
	    emit_html_txt(job, cp->child.u.txt, env);
    }

    if (inAnchor)
	endAnchor(job, &saved, 1);

    if (doAnchor && (job->flags & EMIT_CLUSTERS_LAST)) {
	if (initAnchor(job, env, &cp->data, pts, &saved, 0))
	    endAnchor(job, &saved, 0);
    }
}

/* allocObj:
 * Push new obj on stack to be used in common by all 
 * html elements with anchors.
 * This inherits the type, emit_state, and object of the
 * parent, as well as the url, explicit, target and tooltip.
 */
static void allocObj(GVJ_t * job)
{
    obj_state_t *obj;
    obj_state_t *parent;

    obj = push_obj_state(job);
    parent = obj->parent;
    obj->type = parent->type;
    obj->emit_state = parent->emit_state;
    switch (obj->type) {
    case NODE_OBJTYPE:
	obj->u.n = parent->u.n;
	break;
    case ROOTGRAPH_OBJTYPE:
	obj->u.g = parent->u.g;
	break;
    case CLUSTER_OBJTYPE:
	obj->u.sg = parent->u.sg;
	break;
    case EDGE_OBJTYPE:
	obj->u.e = parent->u.e;
	break;
    }
    obj->url = parent->url;
    obj->tooltip = parent->tooltip;
    obj->target = parent->target;
    obj->explicit_tooltip = parent->explicit_tooltip;
}

static void freeObj(GVJ_t * job)
{
    obj_state_t *obj = job->obj;

    obj->url = NULL;
    obj->tooltip = NULL;
    obj->target = NULL;
    obj->id = NULL;
    pop_obj_state(job);
}

static double
heightOfLbl (htmllabel_t * lp)
{
    double sz;

    switch (lp->kind) {
    case HTML_TBL:
	sz  = lp->u.tbl->data.box.UR.y - lp->u.tbl->data.box.LL.y;
	break;
    case HTML_IMAGE:
	sz  = lp->u.img->box.UR.y - lp->u.img->box.LL.y;
	break;
    case HTML_TEXT:
	sz  = lp->u.txt->box.UR.y - lp->u.txt->box.LL.y;
	break;
    }
    return sz;
}

/* emit_html_label:
 */
void emit_html_label(GVJ_t * job, htmllabel_t * lp, textlabel_t * tp)
{
    htmlenv_t env;
    pointf p;

    allocObj(job);

    p = tp->pos;
    switch (tp->valign) {
	case 't':
    	    p.y = tp->pos.y + (tp->space.y - heightOfLbl(lp))/ 2.0 - 1;
	    break;
	case 'b':
    	    p.y = tp->pos.y - (tp->space.y - heightOfLbl(lp))/ 2.0 - 1;
	    break;
	default:	
    	    /* no-op */
	    break;
    }
    env.pos = p;
    env.finfo.color = tp->fontcolor;
    env.finfo.name = tp->fontname;
    env.finfo.size = tp->fontsize;
    env.imgscale = agget(job->obj->u.n, "imagescale");
    env.objid = job->obj->id;
    env.objid_set = 0;
    if ((env.imgscale == NULL) || (env.imgscale[0] == '\0'))
	env.imgscale = "false";
    if (lp->kind == HTML_TBL) {
	htmltbl_t *tbl = lp->u.tbl;

	/* set basic graphics context */
	/* Need to override line style set by node. */
	gvrender_set_style(job, job->gvc->defaultlinestyle);
	if (tbl->data.pencolor)
	    gvrender_set_pencolor(job, tbl->data.pencolor);
	else
	    gvrender_set_pencolor(job, DEFAULT_COLOR);
	emit_html_tbl(job, tbl, &env);
    } else {
	emit_html_txt(job, lp->u.txt, &env);
    }
    if (env.objid_set)
	free(env.objid);
    freeObj(job);
}

void free_html_font(htmlfont_t * fp)
{
    fp->cnt--;
    if (fp->cnt == 0) {
	if (fp->name)
	    free(fp->name);
	if (fp->color)
	    free(fp->color);
	free(fp);
    }
}

void free_html_data(htmldata_t * dp)
{
    free(dp->href);
    free(dp->port);
    free(dp->target);
    free(dp->id);
    free(dp->title);
    free(dp->bgcolor);
    free(dp->pencolor);
}

void free_html_text(htmltxt_t * t)
{
    htextpara_t *tl;
    textpara_t *ti;
    int i, j;

    if (!t)
	return;

    tl = t->paras;
    for (i = 0; i < t->nparas; i++) {
	ti = tl->items;
	for (j = 0; j < tl->nitems; j++) {
	    if (ti->str)
		free(ti->str);
	    if (ti->font)
		free_html_font(ti->font);
	    if (ti->layout && ti->free_layout)
		ti->free_layout(ti->layout);
	    ti++;
	}
	tl++;
    }
    if (t->paras)
	free(t->paras);
    free(t);
}

void free_html_img(htmlimg_t * ip)
{
    free(ip->src);
    free(ip);
}

static void free_html_cell(htmlcell_t * cp)
{
    free_html_label(&cp->child, 0);
    free_html_data(&cp->data);
    free(cp);
}

/* free_html_tbl:
 * If tbl->n_rows is negative, table is in initial state from
 * HTML parse, with data stored in u.p. Once run through processTbl,
 * data is stored in u.n and tbl->n_rows is > 0.
 */
static void free_html_tbl(htmltbl_t * tbl)
{
    htmlcell_t **cells;

    if (tbl->rc == -1) {
	dtclose(tbl->u.p.rows);
    } else {
	cells = tbl->u.n.cells;

	free(tbl->heights);
	free(tbl->widths);
	while (*cells) {
	    free_html_cell(*cells);
	    cells++;
	}
	free(tbl->u.n.cells);
    }
    if (tbl->font)
	free_html_font(tbl->font);
    free_html_data(&tbl->data);
    free(tbl);
}

void free_html_label(htmllabel_t * lp, int root)
{
    if (lp->kind == HTML_TBL)
	free_html_tbl(lp->u.tbl);
    else if (lp->kind == HTML_IMAGE)
	free_html_img(lp->u.img);
    else
	free_html_text(lp->u.txt);
    if (root)
	free(lp);
}

static htmldata_t *portToTbl(htmltbl_t *, char *);	/* forward declaration */

static htmldata_t *portToCell(htmlcell_t * cp, char *id)
{
    htmldata_t *rv;

    if (cp->data.port && (strcasecmp(cp->data.port, id) == 0))
	rv = &cp->data;
    else if (cp->child.kind == HTML_TBL)
	rv = portToTbl(cp->child.u.tbl, id);
    else
	rv = NULL;

    return rv;
}

/* portToTbl:
 * See if tp or any of its child cells has the given port id.
 * If true, return corresponding box.
 */
static htmldata_t *portToTbl(htmltbl_t * tp, char *id)
{
    htmldata_t *rv;
    htmlcell_t **cells;
    htmlcell_t *cp;

    if (tp->data.port && (strcasecmp(tp->data.port, id) == 0))
	rv = &tp->data;
    else {
	rv = NULL;
	cells = tp->u.n.cells;
	while ((cp = *cells++)) {
	    if ((rv = portToCell(cp, id)))
		break;
	}
    }

    return rv;
}

/* html_port:
 * See if edge port corresponds to part of the html node.
 * Assume pname != "".
 * If successful, return pointer to port's box.
 * Else return NULL.
 */
boxf *html_port(node_t * n, char *pname, int *sides)
{
    htmldata_t *tp;
    htmllabel_t *lbl = ND_label(n)->u.html;
    boxf *rv = NULL;

    if (lbl->kind == HTML_TEXT)
	return NULL;

    tp = portToTbl(lbl->u.tbl, pname);
    if (tp) {
	rv = &tp->box;
	*sides = tp->sides;
    }
    return rv;

}

/* html_path:
 * Return a box in a table containing the given endpoint.
 * If the top flow is text (no internal structure), return 
 * the box of the flow
 * Else return the box of the subtable containing the point.
 * Because of spacing, the point might not be in any subtable.
 * In that case, return the top flow's box.
 * Note that box[0] must contain the edge point. Additional boxes
 * move out to the boundary.
 *
 * At present, unimplemented, since the label may be inside a
 * non-box node and we need to figure out what this means.
 */
int html_path(node_t * n, port * p, int side, boxf * rv, int *k)
{
#ifdef UNIMPL
    point p;
    tbl_t *info;
    tbl_t *t;
    boxf b;
    int i;

    info = (tbl_t *) ND_shape_info(n);
    assert(info->tbls);
    info = info->tbls[0];	/* top-level flow */
    assert(IS_FLOW(info));

    b = info->box;
    if (info->tbl) {
	info = info->tbl;
	if (pt == 1)
	    p = ED_tail_port(e).p;
	else
	    p = ED_head_port(e).p;
	p = flip_pt(p, GD_rankdir(n->graph));	/* move p to node's coordinate system */
	for (i = 0; (t = info->tbls[i]) != 0; i++)
	    if (INSIDE(p, t->box)) {
		b = t->box;
		break;
	    }
    }

    /* move box into layout coordinate system */
    if (GD_flip(n->graph))
	b = flip_trans_box(b, ND_coord_i(n));
    else
	b = move_box(b, ND_coord_i(n));

    *k = 1;
    *rv = b;
    if (pt == 1)
	return BOTTOM;
    else
	return TOP;
#endif
    return 0;
}

static int size_html_txt(graph_t * g, htmltxt_t * ftxt, htmlenv_t * env)
{
    double xsize = 0.0;		/* width of text block */
    double ysize = 0.0;		/* height of text block */
    double fsize;
    double lsize;		/* height of current line */
    double mxfsize = 0.0;	/* max. font size for the current line */
    double curbline = 0.0;	/* dist. of current base line from top */
    pointf sz;
    int i, j;
    double width;
    char *fname;
    textpara_t lp;
    htmlfont_t lhf;
    double maxoffset, mxysize;
    int simple = 1;              /* one item per param, same font size/face, no flags */
    double prev_fsize = -1;
    char* prev_fname = NULL;

    lp.font = &lhf;

    for (i = 0; i < ftxt->nparas; i++) {
	if (ftxt->paras[i].nitems > 1) {
	    simple = 0;
	    break;
	}
	if (ftxt->paras[i].items[0].font) {
	    if (ftxt->paras[i].items[0].font->flags) {
		simple = 0;
		break;
	    }
	    if (ftxt->paras[i].items[0].font->size > 0)
		fsize = ftxt->paras[i].items[0].font->size;
	    else
		fsize = env->finfo.size;
	    if (ftxt->paras[i].items[0].font->name)
		fname = ftxt->paras[i].items[0].font->name;
	    else
		fname = env->finfo.name;
	}
	else {
	    fsize = env->finfo.size;
	    fname = env->finfo.name;
	}
	if (prev_fsize == -1)
	    prev_fsize = fsize;
	else if (fsize != prev_fsize) {
	    simple = 0;
	    break;
	}
	if (prev_fname == NULL)
	    prev_fname = fname;
	else if (strcmp(fname,prev_fname)) {
	    simple = 0;
	    break;
	}
    }
    ftxt->simple = simple;

    for (i = 0; i < ftxt->nparas; i++) {
	width = 0;
	mxysize = maxoffset = mxfsize = 0;
	for (j = 0; j < ftxt->paras[i].nitems; j++) {
	    lp.str =
		strdup_and_subst_obj(ftxt->paras[i].items[j].str,
				     env->obj);
	    if (ftxt->paras[i].items[j].font) {
		if (ftxt->paras[i].items[j].font->flags)
		    lp.font->flags = ftxt->paras[i].items[j].font->flags;
		else if (env->finfo.flags > 0)
		    lp.font->flags = env->finfo.flags;
		else
		    lp.font->flags = 0;
		if (ftxt->paras[i].items[j].font->size > 0)
		    fsize = ftxt->paras[i].items[j].font->size;
		else
		    fsize = env->finfo.size;
		if (ftxt->paras[i].items[j].font->name)
		    fname = ftxt->paras[i].items[j].font->name;
		else
		    fname = env->finfo.name;
	    } else {
		fsize = env->finfo.size;
		fname = env->finfo.name;
		lp.font->flags = 0;
	    }
	    sz = textsize(g, &lp, fname, fsize);
	    free(ftxt->paras[i].items[j].str);
	    ftxt->paras[i].items[j].str = lp.str;
	    ftxt->paras[i].items[j].size = sz.x;
	    ftxt->paras[i].items[j].yoffset_layout = lp.yoffset_layout;
	    ftxt->paras[i].items[j].yoffset_centerline = lp.yoffset_centerline;
	    ftxt->paras[i].items[j].postscript_alias = lp.postscript_alias;
	    ftxt->paras[i].items[j].layout = lp.layout;
	    ftxt->paras[i].items[j].free_layout = lp.free_layout;
	    width += sz.x;
	    mxfsize = MAX(fsize, mxfsize);
	    mxysize = MAX(sz.y, mxysize);
	    maxoffset = MAX(lp.yoffset_centerline, maxoffset);
	}
	/* lsize = mxfsize * LINESPACING; */
	ftxt->paras[i].size = width;
	/* ysize - curbline is the distance from the previous
	 * baseline to the bottom of the previous line.
	 * Then, in the current line, we set the baseline to
	 * be 5/6 of the max. font size. Thus, lfsize gives the
	 * distance from the previous baseline to the new one.
	 */
	/* ftxt->paras[i].lfsize = 5*mxfsize/6 + ysize - curbline; */
	if (simple) {
	    lsize = mxysize;
	    if (i == 0)
		ftxt->paras[i].lfsize = mxfsize;
	    else
		ftxt->paras[i].lfsize = mxysize;
	}
	else {
	    lsize = mxfsize;
	    if (i == 0)
		ftxt->paras[i].lfsize = mxfsize - maxoffset;
	    else
		ftxt->paras[i].lfsize = mxfsize + ysize - curbline - maxoffset;
	}
	curbline += ftxt->paras[i].lfsize;
	xsize = MAX(width, xsize);
	ysize += lsize;
    }
    ftxt->box.UR.x = xsize;
    if (ftxt->nparas == 1)
	ftxt->box.UR.y = mxysize;
    else
	ftxt->box.UR.y = ysize;
    return 0;
}

/* forward declarion for recursive usage */
static int size_html_tbl(graph_t * g, htmltbl_t * tbl, htmlcell_t * parent,
			 htmlenv_t * env);

/* size_html_img:
 */
static int size_html_img(htmlimg_t * img, htmlenv_t * env)
{
    box b;
    int rv;

    b.LL.x = b.LL.y = 0;
    b.UR = gvusershape_size(env->g, img->src);
    if ((b.UR.x == -1) && (b.UR.y == -1)) {
	rv = 1;
	b.UR.x = b.UR.y = 0;
	agerr(AGERR, "No or improper image file=\"%s\"\n", img->src);
    } else {
	rv = 0;
	GD_has_images(env->g) = TRUE;
    }

    B2BF(b, img->box);
    return rv;
}

/* size_html_cell:
 */
static int
size_html_cell(graph_t * g, htmlcell_t * cp, htmltbl_t * parent,
	       htmlenv_t * env)
{
    int rv;
    pointf sz, child_sz;
    int margin;

    cp->parent = parent;
    if (!(cp->data.flags & PAD_SET)) {
	if (parent->data.flags & PAD_SET)
	    cp->data.pad = parent->data.pad;
	else
	    cp->data.pad = DEFAULT_CELLPADDING;
    }
    if (!(cp->data.flags & BORDER_SET)) {
	if (parent->cb >= 0)
	    cp->data.border = parent->cb;
	else if (parent->data.flags & BORDER_SET)
	    cp->data.border = parent->data.border;
	else
	    cp->data.border = DEFAULT_BORDER;
    }

    if (cp->child.kind == HTML_TBL) {
	rv = size_html_tbl(g, cp->child.u.tbl, cp, env);
	child_sz = cp->child.u.tbl->data.box.UR;
    } else if (cp->child.kind == HTML_IMAGE) {
	rv = size_html_img(cp->child.u.img, env);
	child_sz = cp->child.u.img->box.UR;
    } else {
	rv = size_html_txt(g, cp->child.u.txt, env);
	child_sz = cp->child.u.txt->box.UR;
    }

    margin = 2 * (cp->data.pad + cp->data.border);
    sz.x = child_sz.x + margin;
    sz.y = child_sz.y + margin;

    if (cp->data.flags & FIXED_FLAG) {
	if (cp->data.width && cp->data.height) {
	    if ((cp->data.width < sz.x) || (cp->data.height < sz.y)) {
		agerr(AGWARN, "cell size too small for content\n");
		rv = 1;
	    }
	    sz.x = sz.y = 0;

	} else {
	    agerr(AGWARN,
		  "fixed cell size with unspecified width or height\n");
	    rv = 1;
	}
    }
    cp->data.box.UR.x = MAX(sz.x, cp->data.width);
    cp->data.box.UR.y = MAX(sz.y, cp->data.height);
    return rv;
}

static int findCol(PointSet * ps, int row, int col, htmlcell_t * cellp)
{
    int notFound = 1;
    int lastc;
    int i, j, c;
    int end = cellp->cspan - 1;

    while (notFound) {
	lastc = col + end;
	for (c = lastc; c >= col; c--) {
	    if (isInPS(ps, c, row))
		break;
	}
	if (c >= col)		/* conflict : try column after */
	    col = c + 1;
	else
	    notFound = 0;
    }
    for (j = col; j < col + cellp->cspan; j++) {
	for (i = row; i < row + cellp->rspan; i++) {
	    addPS(ps, j, i);
	}
    }
    return col;
}

/* processTbl:
 * Convert parser representation of cells into final form.
 * Find column and row positions of cells.
 * Recursively size cells.
 * Return 1 if problem sizing a cell.
 */
static int processTbl(graph_t * g, htmltbl_t * tbl, htmlenv_t * env)
{
    pitem *rp;
    pitem *cp;
    Dt_t *cdict;
    int r, c, cnt;
    htmlcell_t *cellp;
    htmlcell_t **cells;
    Dt_t *rows = tbl->u.p.rows;
    int rv = 0;
    int n_rows = 0;
    int n_cols = 0;
    PointSet *ps = newPS();
    Dt_t *is = openIntSet();

    rp = (pitem *) dtflatten(rows);
    cnt = 0;
    r = 0;
    while (rp) {
	cdict = rp->u.rp;
	cp = (pitem *) dtflatten(cdict);
	while (cp) {
	    cellp = cp->u.cp;
	    cnt++;
	    cp = (pitem *) dtlink(cdict, (Dtlink_t *) cp);
	}
	if (rp->ruled) {
	    addIntSet(is, r + 1);
	}
	rp = (pitem *) dtlink(rows, (Dtlink_t *) rp);
	r++;
    }

    cells = tbl->u.n.cells = N_NEW(cnt + 1, htmlcell_t *);
    rp = (pitem *) dtflatten(rows);
    r = 0;
    while (rp) {
	cdict = rp->u.rp;
	cp = (pitem *) dtflatten(cdict);
	c = 0;
	while (cp) {
	    cellp = cp->u.cp;
	    *cells++ = cellp;
	    rv |= size_html_cell(g, cellp, tbl, env);
	    c = findCol(ps, r, c, cellp);
	    cellp->row = r;
	    cellp->col = c;
	    c += cellp->cspan;
	    n_cols = MAX(c, n_cols);
	    n_rows = MAX(r + cellp->rspan, n_rows);
	    if (inIntSet(is, r + cellp->rspan))
		cellp->ruled |= HTML_HRULE;
	    cp = (pitem *) dtlink(cdict, (Dtlink_t *) cp);
	}
	rp = (pitem *) dtlink(rows, (Dtlink_t *) rp);
	r++;
    }
    tbl->rc = n_rows;
    tbl->cc = n_cols;
    dtclose(rows);
    dtclose(is);
    freePS(ps);
    return rv;
}

/* Split size x over n pieces with spacing s.
 * We substract s*(n-1) from x, divide by n and 
 * take the ceiling.
 */
#define SPLIT(x,n,s) (((x) - ((s)-1)*((n)-1)) / (n))

/* sizeLinearArray:
 * Determine sizes of rows and columns. The size of a column is the
 * maximum width of any cell in it. Similarly for rows.
 * A cell spanning columns contributes proportionately to each column
 * it is in.
 */
void sizeLinearArray(htmltbl_t * tbl)
{
    htmlcell_t *cp;
    htmlcell_t **cells;
    int wd, ht, i, x, y;

    tbl->heights = N_NEW(tbl->rc + 1, int);
    tbl->widths = N_NEW(tbl->cc + 1, int);

    for (cells = tbl->u.n.cells; *cells; cells++) {
	cp = *cells;
	if (cp->rspan == 1)
	    ht = cp->data.box.UR.y;
	else {
	    ht = SPLIT(cp->data.box.UR.y, cp->rspan, tbl->data.space);
	    ht = MAX(ht, 1);
	}
	if (cp->cspan == 1)
	    wd = cp->data.box.UR.x;
	else {
	    wd = SPLIT(cp->data.box.UR.x, cp->cspan, tbl->data.space);
	    wd = MAX(wd, 1);
	}
	for (i = cp->row; i < cp->row + cp->rspan; i++) {
	    y = tbl->heights[i];
	    tbl->heights[i] = MAX(y, ht);
	}
	for (i = cp->col; i < cp->col + cp->cspan; i++) {
	    x = tbl->widths[i];
	    tbl->widths[i] = MAX(x, wd);
	}
    }
}

static char *nnames[] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
};

/* nToName:
 * Convert int to its decimal string representation.
 */
char *nToName(int c)
{
    static char name[100];

    if (c < sizeof(nnames) / sizeof(char *))
	return nnames[c];

    sprintf(name, "%d", c);
    return name;
}

/* closeGraphs:
 * Clean up graphs made for setting column and row widths.
 */
static void closeGraphs(graph_t * rowg, graph_t * colg)
{
    node_t *n;
    for (n = GD_nlist(colg); n; n = ND_next(n)) {
	free_list(ND_in(n));
	free_list(ND_out(n));
    }

    agclose(rowg);
    agclose(colg);
}

static void checkChain(graph_t * g)
{
    node_t *t;
    node_t *h;
    edge_t *e;
    t = GD_nlist(g);
    for (h = ND_next(t); h; h = ND_next(h)) {
	if (!agfindedge(g, t, h)) {
#ifdef WITH_CGRAPH
	    e = agedge(g, t, h, NULL, 1);
	    agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);
#else
	    e = agedge(g, t, h);
#endif
	    ED_minlen(e) = 0;
	    elist_append(e, ND_out(t));
	    elist_append(e, ND_in(h));
	}
	t = h;
    }
}

/* makeGraphs:
 * Generate dags modeling the row and column constraints.
 * If the table has cc columns, we create the graph
 *  0 -> 1 -> 2 -> ... -> cc
 * and if a cell starts in column c with span cspan, with
 * width w, we add the edge c -> c+cspan [minlen = w].
 *
 * We might simplify the graph by removing multiedges,
 * using the max minlen, but will affect the balancing?
 */
void makeGraphs(htmltbl_t * tbl, graph_t * rowg, graph_t * colg)
{
    htmlcell_t *cp;
    htmlcell_t **cells;
    node_t *t;
    node_t *lastn;
    node_t *h;
    edge_t *e;
    int i;
    int *minc;
    int *minr;

    lastn = NULL;
    for (i = 0; i <= tbl->cc; i++) {
#ifdef WITH_CGRAPH
	t = agnode(colg, nToName(i), 1);
	agbindrec(t, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);
#else
	t = agnode(colg, nToName(i));
#endif
	alloc_elist(tbl->rc, ND_in(t));
	alloc_elist(tbl->rc, ND_out(t));
	if (lastn) {
	    ND_next(lastn) = t;
	    lastn = t;
	} else {
	    lastn = GD_nlist(colg) = t;
	}
    }
    lastn = NULL;
    for (i = 0; i <= tbl->rc; i++) {
#ifdef WITH_CGRAPH
	t = agnode(rowg, nToName(i), 1);
	agbindrec(t, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);
#else
	t = agnode(rowg, nToName(i));
#endif
	alloc_elist(tbl->cc, ND_in(t));
	alloc_elist(tbl->cc, ND_out(t));
	if (lastn) {
	    ND_next(lastn) = t;
	    lastn = t;
	} else {
	    lastn = GD_nlist(rowg) = t;
	}
    }
    minr = N_NEW(tbl->rc, int);
    minc = N_NEW(tbl->cc, int);
    for (cells = tbl->u.n.cells; *cells; cells++) {
	int x, y, c, r;
	cp = *cells;
	x = (cp->data.box.UR.x + (cp->cspan - 1)) / cp->cspan;
	for (c = 0; c < cp->cspan; c++)
	    minc[cp->col + c] = MAX(minc[cp->col + c], x);
	y = (cp->data.box.UR.y + (cp->rspan - 1)) / cp->rspan;
	for (r = 0; r < cp->rspan; r++)
	    minr[cp->row + r] = MAX(minr[cp->row + r], y);
    }
    for (cells = tbl->u.n.cells; *cells; cells++) {
	int x, y, c, r;
	cp = *cells;
	t = agfindnode(colg, nToName(cp->col));
	h = agfindnode(colg, nToName(cp->col + cp->cspan));
#ifdef WITH_CGRAPH
	e = agedge(colg, t, h, NULL, 1);
	agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);
#else
	e = agedge(colg, t, h);
#endif
	x = 0;
	for (c = 0; c < cp->cspan; c++)
	    x += minc[cp->col + c];
	ED_minlen(e) = x;
	/* ED_minlen(e) = cp->data.box.UR.x; */
#if (DEBUG==2)
	fprintf(stderr, "col edge %s ", agnameof(t));
	fprintf(stderr, "-> %s %d\n", agnameof(h), ED_minlen(e));
#endif
	elist_append(e, ND_out(t));
	elist_append(e, ND_in(h));

	t = agfindnode(rowg, nToName(cp->row));
	h = agfindnode(rowg, nToName(cp->row + cp->rspan));
#ifdef WITH_CGRAPH
	e = agedge(rowg, t, h, NULL, 1);
	agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);
#else
	e = agedge(rowg, t, h);
#endif
	y = 0;
	for (r = 0; r < cp->rspan; r++)
	    y += minr[cp->row + r];
	ED_minlen(e) = y;
	/* ED_minlen(e) = cp->data.box.UR.y; */
#if (DEBUG==2)
	fprintf(stderr, "row edge %s -> %s %d\n", agnameof(t), agnameof(h),
		ED_minlen(e));
#endif
	elist_append(e, ND_out(t));
	elist_append(e, ND_in(h));
    }

    /* Make sure that 0 <= 1 <= 2 ...k. This implies graph connected. */
    checkChain(colg);
    checkChain(rowg);

    free(minc);
    free(minr);
}

/* setSizes:
 * Use rankings to determine cell dimensions. The rank values
 * give the coordinate, so to get the width/height, we have
 * to subtract the previous value.
 */
void setSizes(htmltbl_t * tbl, graph_t * rowg, graph_t * colg)
{
    int i;
    node_t *n;
    int prev;

    prev = 0;
    n = GD_nlist(rowg);
    for (i = 0, n = ND_next(n); n; i++, n = ND_next(n)) {
	tbl->heights[i] = ND_rank(n) - prev;
	prev = ND_rank(n);
    }
    prev = 0;
    n = GD_nlist(colg);
    for (i = 0, n = ND_next(n); n; i++, n = ND_next(n)) {
	tbl->widths[i] = ND_rank(n) - prev;
	prev = ND_rank(n);
    }

}

/* sizeArray:
 * Set column and row sizes. Optimize for minimum width and
 * height. Where there is slack, try to distribute evenly.
 * We do this by encoding cells as edges with min length is
 * a dag on a chain. We then run network simplex, using
 * LR_balance.
 */
void sizeArray(htmltbl_t * tbl)
{
    graph_t *rowg;
    graph_t *colg;

    /* Do the 1D cases by hand */
    if ((tbl->rc == 1) || (tbl->cc == 1)) {
	sizeLinearArray(tbl);
	return;
    }

    tbl->heights = N_NEW(tbl->rc + 1, int);
    tbl->widths = N_NEW(tbl->cc + 1, int);

#ifdef WITH_CGRAPH
    rowg = agopen("rowg", Agdirected, NIL(Agdisc_t *));
    colg = agopen("colg", Agdirected, NIL(Agdisc_t *));
    /* Only need GD_nlist */
    agbindrec(rowg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);	// graph custom data
    agbindrec(colg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);	// graph custom data
#else
    rowg = agopen("rowg", AGDIGRAPH);
    colg = agopen("colg", AGDIGRAPH);
#endif
    makeGraphs(tbl, rowg, colg);
    rank(rowg, 2, INT_MAX);
    rank(colg, 2, INT_MAX);
    setSizes(tbl, rowg, colg);
    closeGraphs(rowg, colg);
}

static void pos_html_tbl(htmltbl_t *, boxf, int);	/* forward declaration */

/* pos_html_img:
 * Place image in cell
 * storing allowed space handed by parent cell.
 * How this space is used is handled in emit_html_img.
 */
static void pos_html_img(htmlimg_t * cp, boxf pos)
{
    cp->box = pos;
}

/* pos_html_txt:
 * Set default alignment.
 */
static void pos_html_txt(htmltxt_t * ftxt, char c)
{
    int i;

    for (i = 0; i < ftxt->nparas; i++) {
	if (ftxt->paras[i].just == UNSET_ALIGN)	/* unset */
	    ftxt->paras[i].just = c;
    }
}

/* pos_html_cell:
 */
static void pos_html_cell(htmlcell_t * cp, boxf pos, int sides)
{
    double delx, dely;
    pointf oldsz;
    boxf cbox;

    if (!cp->data.pencolor && cp->parent->data.pencolor)
	cp->data.pencolor = strdup(cp->parent->data.pencolor);

    /* If fixed, align cell */
    if (cp->data.flags & FIXED_FLAG) {
	oldsz = cp->data.box.UR;
	delx = (pos.UR.x - pos.LL.x) - oldsz.x;
	if (delx > 0) {
	    switch (cp->data.flags & HALIGN_MASK) {
	    case HALIGN_LEFT:
		pos.UR.x = pos.LL.x + oldsz.x;
		break;
	    case HALIGN_RIGHT:
		pos.UR.x += delx;
		pos.LL.x += delx;
		break;
	    default:
		pos.LL.x += delx / 2;
		pos.UR.x -= delx / 2;
		break;
	    }
	}
	dely = (pos.UR.y - pos.LL.y) - oldsz.y;
	if (dely > 0) {
	    switch (cp->data.flags & VALIGN_MASK) {
	    case VALIGN_BOTTOM:
		pos.UR.y = pos.LL.y + oldsz.y;
		break;
	    case VALIGN_TOP:
		pos.UR.y += dely;
		pos.LL.y += dely;
		break;
	    default:
		pos.LL.y += dely / 2;
		pos.UR.y -= dely / 2;
		break;
	    }
	}
    }
    cp->data.box = pos;
    cp->data.sides = sides;

    /* set up child's position */
    cbox.LL.x = pos.LL.x + cp->data.border + cp->data.pad;
    cbox.LL.y = pos.LL.y + cp->data.border + cp->data.pad;
    cbox.UR.x = pos.UR.x - cp->data.border - cp->data.pad;
    cbox.UR.y = pos.UR.y - cp->data.border - cp->data.pad;

    if (cp->child.kind == HTML_TBL) {
	pos_html_tbl(cp->child.u.tbl, cbox, sides);
    } else if (cp->child.kind == HTML_IMAGE) {
	/* Note that alignment trumps scaling */
	oldsz = cp->child.u.img->box.UR;
	delx = (cbox.UR.x - cbox.LL.x) - oldsz.x;
	if (delx > 0) {
	    switch (cp->data.flags & HALIGN_MASK) {
	    case HALIGN_LEFT:
		cbox.UR.x -= delx;
		break;
	    case HALIGN_RIGHT:
		cbox.LL.x += delx;
		break;
	    }
	}

	dely = (cbox.UR.y - cbox.LL.y) - oldsz.y;
	if (dely > 0) {
	    switch (cp->data.flags & VALIGN_MASK) {
	    case VALIGN_BOTTOM:
		cbox.UR.y -= dely;
		break;
	    case VALIGN_TOP:
		cbox.LL.y += dely;
		break;
	    }
	}
	pos_html_img(cp->child.u.img, cbox);
    } else {
	char dfltalign;
	int af;

	oldsz = cp->child.u.txt->box.UR;
	delx = (cbox.UR.x - cbox.LL.x) - oldsz.x;
	/* If the cell is larger than the text block and alignment is 
	 * done at textblock level, the text box is shrunk accordingly. 
	 */
	if ((delx > 0)
	    && ((af = (cp->data.flags & HALIGN_MASK)) != HALIGN_TEXT)) {
	    switch (af) {
	    case HALIGN_LEFT:
		cbox.UR.x -= delx;
		break;
	    case HALIGN_RIGHT:
		cbox.LL.x += delx;
		break;
	    default:
		cbox.LL.x += delx / 2;
		cbox.UR.x -= delx / 2;
		break;
	    }
	}

	dely = (cbox.UR.y - cbox.LL.y) - oldsz.y;
	if (dely > 0) {
	    switch (cp->data.flags & VALIGN_MASK) {
	    case VALIGN_BOTTOM:
		cbox.UR.y -= dely;
		break;
	    case VALIGN_TOP:
		cbox.LL.y += dely;
		break;
	    default:
		cbox.LL.y += dely / 2;
		cbox.UR.y -= dely / 2;
		break;
	    }
	}
	cp->child.u.txt->box = cbox;

	/* Set default text alignment
	 */
	switch (cp->data.flags & BALIGN_MASK) {
	case BALIGN_LEFT:
	    dfltalign = 'l';
	    break;
	case BALIGN_RIGHT:
	    dfltalign = 'r';
	    break;
	default:
	    dfltalign = 'n';
	    break;
	}
	pos_html_txt(cp->child.u.txt, dfltalign);
    }
}

/* pos_html_tbl:
 * Position table given its box, then calculate
 * the position of each cell. In addition, set the sides
 * attribute indicating which external sides of the node
 * are accessible to the table.
 */
static void pos_html_tbl(htmltbl_t * tbl, boxf pos, int sides)
{
    int x, y, delx, dely, oldsz;
    int i, extra, plus;
    htmlcell_t **cells = tbl->u.n.cells;
    htmlcell_t *cp;
    boxf cbox;

    if (tbl->u.n.parent && tbl->u.n.parent->data.pencolor
	&& !tbl->data.pencolor)
	tbl->data.pencolor = strdup(tbl->u.n.parent->data.pencolor);

    oldsz = tbl->data.box.UR.x;
    delx = (pos.UR.x - pos.LL.x) - oldsz;
    assert(delx >= 0);
    oldsz = tbl->data.box.UR.y;
    dely = (pos.UR.y - pos.LL.y) - oldsz;
    assert(dely >= 0);

    /* If fixed, align box */
    if (tbl->data.flags & FIXED_FLAG) {
	if (delx > 0) {
	    switch (tbl->data.flags & HALIGN_MASK) {
	    case HALIGN_LEFT:
		pos.UR.x = pos.LL.x + oldsz;
		break;
	    case HALIGN_RIGHT:
		pos.UR.x += delx;
		pos.LL.x += delx;
		break;
	    default:
		pos.LL.x += delx / 2;
		pos.UR.x -= delx / 2;
		break;
	    }
	    delx = 0;
	}
	if (dely > 0) {
	    switch (tbl->data.flags & VALIGN_MASK) {
	    case VALIGN_BOTTOM:
		pos.UR.y = pos.LL.y + oldsz;
		break;
	    case VALIGN_TOP:
		pos.UR.y += dely;
		pos.LL.y += dely;
		break;
	    default:
		pos.LL.y += dely / 2;
		pos.UR.y -= dely / 2;
		break;
	    }
	    dely = 0;
	}
    }

    /* change sizes to start positions and distribute extra space */
    x = pos.LL.x + tbl->data.border + tbl->data.space;
    extra = delx / (tbl->cc);
    plus = ROUND(delx - extra * (tbl->cc));
    for (i = 0; i <= tbl->cc; i++) {
	delx = tbl->widths[i] + extra + (i < plus ? 1 : 0);
	tbl->widths[i] = x;
	x += delx + tbl->data.space;
    }
    y = pos.UR.y - tbl->data.border - tbl->data.space;
    extra = dely / (tbl->rc);
    plus = ROUND(dely - extra * (tbl->rc));
    for (i = 0; i <= tbl->rc; i++) {
	dely = tbl->heights[i] + extra + (i < plus ? 1 : 0);
	tbl->heights[i] = y;
	y -= dely + tbl->data.space;
    }

    while ((cp = *cells++)) {
	int mask = 0;
	if (sides) {
	    if (cp->col == 0)
		mask |= LEFT;
	    if (cp->row == 0)
		mask |= TOP;
	    if (cp->col + cp->cspan == tbl->cc)
		mask |= RIGHT;
	    if (cp->row + cp->rspan == tbl->rc)
		mask |= BOTTOM;
	}
	cbox.LL.x = tbl->widths[cp->col];
	cbox.UR.x = tbl->widths[cp->col + cp->cspan] - tbl->data.space;
	cbox.UR.y = tbl->heights[cp->row];
	cbox.LL.y = tbl->heights[cp->row + cp->rspan] + tbl->data.space;
	pos_html_cell(cp, cbox, sides & mask);
    }

    tbl->data.sides = sides;
    tbl->data.box = pos;
}

/* size_html_tbl:
 * Determine the size of a table by first determining the
 * size of each cell.
 */
static int
size_html_tbl(graph_t * g, htmltbl_t * tbl, htmlcell_t * parent,
	      htmlenv_t * env)
{
    int i, wd, ht;
    int rv = 0;
    static htmlfont_t savef;

    if (tbl->font)
	pushFontInfo(env, tbl->font, &savef);
    tbl->u.n.parent = parent;
    rv = processTbl(g, tbl, env);

    /* Set up border and spacing */
    if (!(tbl->data.flags & SPACE_SET)) {
	tbl->data.space = DEFAULT_CELLSPACING;
    }
    if (!(tbl->data.flags & BORDER_SET)) {
	tbl->data.border = DEFAULT_BORDER;
    }

    sizeArray(tbl);

    wd = (tbl->cc + 1) * tbl->data.space + 2 * tbl->data.border;
    ht = (tbl->rc + 1) * tbl->data.space + 2 * tbl->data.border;
    for (i = 0; i < tbl->cc; i++)
	wd += tbl->widths[i];
    for (i = 0; i < tbl->rc; i++)
	ht += tbl->heights[i];

    if (tbl->data.flags & FIXED_FLAG) {
	if (tbl->data.width && tbl->data.height) {
	    if ((tbl->data.width < wd) || (tbl->data.height < ht)) {
		agerr(AGWARN, "table size too small for content\n");
		rv = 1;
	    }
	    wd = ht = 0;
	} else {
	    agerr(AGWARN,
		  "fixed table size with unspecified width or height\n");
	    rv = 1;
	}
    }
    tbl->data.box.UR.x = MAX(wd, tbl->data.width);
    tbl->data.box.UR.y = MAX(ht, tbl->data.height);

    if (tbl->font)
	popFontInfo(env, &savef);
    return rv;
}

static char *nameOf(void *obj, agxbuf * xb)
{
    Agedge_t *ep;
    switch (agobjkind(obj)) {
#ifndef WITH_CGRAPH
    case AGGRAPH:
#else
    case AGRAPH:
#endif
	agxbput(xb, agnameof(((Agraph_t *) obj)));
	break;
    case AGNODE:
	agxbput(xb, agnameof(((Agnode_t *) obj)));
	break;
    case AGEDGE:
	ep = (Agedge_t *) obj;
	agxbput(xb, agnameof(agtail(ep)));
	agxbput(xb, agnameof(aghead(ep)));
	if (agisdirected(agraphof(aghead(ep))))
	    agxbput(xb, "->");
	else
	    agxbput(xb, "--");
	break;
    }
    return agxbuse(xb);
}

#ifdef DEBUG
void indent(int i)
{
    while (i--)
	fprintf(stderr, "  ");
}

void printBox(boxf b)
{
    fprintf(stderr, "(%f,%f)(%f,%f)", b.LL.x, b.LL.y, b.UR.x, b.UR.y);
}

void printImage(htmlimg_t * ip, int ind)
{
    indent(ind);
    fprintf(stderr, "img: %s\n", ip->src);
}

void printTxt(htmltxt_t * txt, int ind)
{
    int i, j;

    indent(ind);
    fprintf(stderr, "txt paras = %d \n", txt->nparas);
    for (i = 0; i < txt->nparas; i++) {
	indent(ind + 1);
	fprintf(stderr, "[%d] %d items\n", i, txt->paras[i].nitems);
	for (j = 0; j < txt->paras[i].nitems; j++) {
	    indent(ind + 2);
	    fprintf(stderr, "[%d] (%f) \"%s\" ",
		    j, txt->paras[i].items[j].size,
		    txt->paras[i].items[j].str);
	    if (txt->paras[i].items[j].font)
		fprintf(stderr, "font %s color %s size %f\n",
			txt->paras[i].items[j].font->name,
			txt->paras[i].items[j].font->color,
			txt->paras[i].items[j].font->size);
	    else
		fprintf(stderr, "\n");
	}
    }
}

void printData(htmldata_t * dp)
{
    unsigned char flags = dp->flags;
    char c;

    fprintf(stderr, "s%d(%d) ", dp->space, (flags & SPACE_SET ? 1 : 0));
    fprintf(stderr, "b%d(%d) ", dp->border, (flags & BORDER_SET ? 1 : 0));
    fprintf(stderr, "p%d(%d) ", dp->pad, (flags & PAD_SET ? 1 : 0));
    switch (flags & HALIGN_MASK) {
    case HALIGN_RIGHT:
	c = 'r';
	break;
    case HALIGN_LEFT:
	c = 'l';
	break;
    default:
	c = 'n';
	break;
    }
    fprintf(stderr, "%c", c);
    switch (flags & VALIGN_MASK) {
    case VALIGN_TOP:
	c = 't';
	break;
    case VALIGN_BOTTOM:
	c = 'b';
	break;
    default:
	c = 'c';
	break;
    }
    fprintf(stderr, "%c ", c);
    printBox(dp->box);
}

void printTbl(htmltbl_t * tbl, int ind)
{
    htmlcell_t **cells = tbl->u.n.cells;
    indent(ind);
    fprintf(stderr, "tbl (%p) %d %d ", tbl, tbl->cc, tbl->rc);
    printData(&tbl->data);
    fputs("\n", stderr);
    while (*cells)
	printCell(*cells++, ind + 1);
}

static void printCell(htmlcell_t * cp, int ind)
{
    indent(ind);
    fprintf(stderr, "cell %d %d %d %d ", cp->cspan, cp->rspan, cp->col,
	    cp->row);
    printData(&cp->data);
    fputs("\n", stderr);
    switch (cp->child.kind) {
    case HTML_TBL:
	printTbl(cp->child.u.tbl, ind + 1);
	break;
    case HTML_TEXT:
	printTxt(cp->child.u.txt, ind + 1);
	break;
    case HTML_IMAGE:
	printImage(cp->child.u.img, ind + 1);
	break;
    default:
	break;
    }
}

void printLbl(htmllabel_t * lbl)
{
    if (lbl->kind == HTML_TBL)
	printTbl(lbl->u.tbl, 0);
    else
	printTxt(lbl->u.txt, 0);
}
#endif				/* DEBUG */

static char *getPenColor(void *obj)
{
    char *str;

    if (((str = agget(obj, "pencolor")) != 0) && str[0])
	return str;
    else if (((str = agget(obj, "color")) != 0) && str[0])
	return str;
    else
	return NULL;
}

/* make_html_label:
 * Return non-zero if problem parsing HTML. In this case, use object name.
 */
int make_html_label(void *obj, textlabel_t * lp)
{
    int rv;
    double wd2, ht2;
    boxf box;
    graph_t *g;
    htmllabel_t *lbl;
    htmlenv_t env;
    char *s;

    env.obj = obj;
    switch (agobjkind(obj)) {
#ifdef WITH_CGRAPH
    case AGRAPH:
#else
    case AGGRAPH:
#endif
	env.g = ((Agraph_t *) obj)->root;
	break;
    case AGNODE:
	env.g = agraphof(((Agnode_t *) obj));
	break;
    case AGEDGE:
	env.g = agraphof(aghead(((Agedge_t *) obj)));
	break;
    }
    g = env.g->root;

    env.finfo.size = lp->fontsize;
    env.finfo.name = lp->fontname;
    env.finfo.color = lp->fontcolor;
    env.finfo.flags = 0;
    lbl = parseHTML(lp->text, &rv, GD_charset(env.g));
    if (!lbl) {
	/* Parse of label failed; revert to simple text label */
	agxbuf xb;
	unsigned char buf[SMALLBUF];
	agxbinit(&xb, SMALLBUF, buf);
	lp->html = FALSE;
	lp->text = strdup(nameOf(obj, &xb));
	switch (lp->charset) {
	case CHAR_LATIN1:
	    s = latin1ToUTF8(lp->text);
	    break;
	default:		/* UTF8 */
	    s = htmlEntityUTF8(lp->text, env.g);
	    break;
	}
	free(lp->text);
	lp->text = s;
	make_simple_label(g, lp);
	agxbfree(&xb);
	return rv;
    }

    if (lbl->kind == HTML_TBL) {
	if (!lbl->u.tbl->data.pencolor && getPenColor(obj))
	    lbl->u.tbl->data.pencolor = strdup(getPenColor(obj));
	rv |= size_html_tbl(g, lbl->u.tbl, NULL, &env);
	wd2 = (lbl->u.tbl->data.box.UR.x + 1) / 2;
	ht2 = (lbl->u.tbl->data.box.UR.y + 1) / 2;
	box = boxfof(-wd2, -ht2, wd2, ht2);
	pos_html_tbl(lbl->u.tbl, box, BOTTOM | RIGHT | TOP | LEFT);
	lp->dimen.x = box.UR.x - box.LL.x;
	lp->dimen.y = box.UR.y - box.LL.y;
    } else {
	rv |= size_html_txt(g, lbl->u.txt, &env);
	wd2 = lbl->u.txt->box.UR.x  / 2;
	ht2 = lbl->u.txt->box.UR.y  / 2;
	box = boxfof(-wd2, -ht2, wd2, ht2);
	lbl->u.txt->box = box;
	lp->dimen.x = box.UR.x - box.LL.x;
	lp->dimen.y = box.UR.y - box.LL.y;
    }

    lp->u.html = lbl;

    /* If the label is a table, replace label text because this may
     * be used for the title and alt fields in image maps.
     */
    if (lbl->kind == HTML_TBL) {
	free(lp->text);
	lp->text = strdup("<TABLE>");
    }

    return rv;
}
