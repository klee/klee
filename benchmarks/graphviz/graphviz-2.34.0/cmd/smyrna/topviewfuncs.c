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

#include "topviewfuncs.h"
#include "cgraph.h"
#include "smyrna_utils.h"
#include "colorprocs.h"
#include "draw.h"
#include "frmobjectui.h"
#include "xdot.h"
#include "glutils.h"
#include "selectionfuncs.h"
#include "memory.h"
#include <ctype.h>

static xdot *parseXdotwithattrs(void *e)
{
	
    int cnt=0;
    xdot* xDot=NULL;
    xDot=parseXDotFOn (agget(e,"_draw_" ), OpFns,sizeof(sdot_op), xDot);
    xDot=parseXDotFOn (agget(e,"_ldraw_" ), OpFns,sizeof(sdot_op), xDot);
    xDot=parseXDotFOn (agget(e,"_hdraw_" ), OpFns,sizeof(sdot_op), xDot);
    xDot=parseXDotFOn (agget(e,"_tdraw_" ), OpFns,sizeof(sdot_op), xDot);
    xDot=parseXDotFOn (agget(e,"_hldraw_" ), OpFns,sizeof(sdot_op), xDot);
    xDot=parseXDotFOn (agget(e,"_tldraw_" ), OpFns,sizeof(sdot_op), xDot);
    if(xDot)
    {
	for (cnt=0;cnt < xDot->cnt ; cnt++)
	{
	    ((sdot_op*)(xDot->ops))[cnt].obj=e;
        }
    }
    return xDot;

}

static void set_boundaries(Agraph_t * g, topview * t)
{
    Agnode_t *v;
    Agsym_t* pos_attr = GN_pos(g);
    glCompPoint pos;
    float left, right, top, bottom;
    int id=0;

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	pos=getPointFromStr(agxget(v, pos_attr));
	if(id==0)
	{
	    left=pos.x;
	    right=pos.x;
	    top=pos.y;
	    bottom=pos.y;
	}
	if (left > pos.x)
	    left = pos.x;
	if (right < pos.x)
	    right = pos.x;
	if (bottom > pos.y)
	    bottom = pos.y;
	if (top < pos.y)
	    top = pos.y;
	id++;
    }
    view->bdxLeft = left;
    view->bdyTop = top;
    view->bdxRight = right;
    view->bdyBottom = bottom;

    view->bdzTop = 0;
    view->bdzBottom = 0;
}

#if UNUSED
static float init_node_size(Agraph_t * g, topview * t)
{
    float vsize;
    int percent;
    float sz;
    percent = atoi(agget(g, "nodesize"));
    if (percent == 0)
	percent = 0.000001;
    vsize =
	0.05 * sqrt((view->bdxRight - view->bdxLeft) *
		    (view->bdyTop - view->bdyBottom));
    sz =	vsize * 2  * percent / 100.0 /
	sqrt(t->Nodecount);
/*    if (t->init_node_size < 1)
	t->init_node_size=1;*/
//    t->init_zoom = view->zoom;
    t->init_zoom = -20;
    return  sz;

}
#endif


static void draw_xdot(xdot* x,float base_z)
{
	int i;
	sdot_op *op;
	if (!x)
		return;

	view->Topview->global_z=base_z;

	op=(sdot_op*)x->ops;
	for (i=0; i < x->cnt; i++,op++)
	{
		if(op->op.drawfunc)
			op->op.drawfunc(&op->op,0);
	}


}



static glCompPoint getEdgeHead(Agedge_t * edge)
{   
    return getPointFromStr(agget(aghead(edge),"pos"));
}
static glCompPoint getEdgeTail(Agedge_t *  edge)
{
    return getPointFromStr(agget(agtail(edge),"pos"));
}
static GLfloat getEdgeLength(Agedge_t *  edge)
{
    GLfloat rv=0;	
    glCompPoint A,B;
    A=getEdgeTail(edge);
    B=getEdgeHead(edge);
    rv=(A.x-B.x)*(A.x-B.x) + (A.y-B.y)*(A.y-B.y) +(A.z-B.z)*(A.z-B.z);
    rv=sqrt(rv);
    return rv;
}
static void glCompColorxlate(glCompColor* c,char* str)
{
        gvcolor_t cl;
	colorxlate(str, &cl, RGBA_DOUBLE);
	c->R=cl.u.RGBA[0];
	c->G=cl.u.RGBA[1];
	c->B=cl.u.RGBA[2];
	c->A=cl.u.RGBA[3];
}

/* If the "visible" attribute is not set or "", return true
 * else evaluate as boolean
 */
static int visible(Agsym_t* attr, void* obj)
{
    char* s;

    if (attr) {
	s = agxget (obj, attr);
	if (*s) return mapbool(s);
	else return 1;
    }
    else return 1;
}

static int object_color(void* obj,glCompColor* c)
{
    gvcolor_t cl;
    Agraph_t* g=view->g[view->activeGraph];
    Agraph_t* objg=agraphof(obj);
    int return_value = 1;
    int objType;
    float Alpha = 1;
    char* bf;
    Agsym_t* vis;

    objType=AGTYPE(obj);

    if(objType==AGEDGE) {
	Alpha=getAttrFloat(g,objg,"defaultedgealpha",1);
	vis = GE_visible (objg);
    }
    else if(objType==AGNODE) {
	Alpha=getAttrFloat(g,objg,"defaultnodealpha",1);
	vis = GN_visible (objg);
    }
    if (!visible(vis,obj))
	return 0;

    setColorScheme (agget (obj, "colorscheme"));
    /*get objects's color attribute */
    bf = getAttrStr(g,obj,"color",NULL);
    if(bf && (*bf)) {
	colorxlate(bf, &cl, RGBA_DOUBLE);
	c->R = cl.u.RGBA[0];
	c->G = cl.u.RGBA[1];
	c->B = cl.u.RGBA[2];
	c->A = cl.u.RGBA[3]*Alpha;
    }
    else
    {
	if(objType==AGEDGE)
	    getcolorfromschema(view->colschms, getEdgeLength(obj), view->Topview->maxedgelen,c);
	else
	{
	    colorxlate(agget(g, "defaultnodecolor"),&cl, RGBA_DOUBLE);
		c->R = cl.u.RGBA[0];
	    c->G = cl.u.RGBA[1];
	    c->B = cl.u.RGBA[2];
	    c->A = cl.u.RGBA[3];
	}
	c->A = c->A*Alpha;

    }
    return return_value;
}


/*
	draws multi edges , single edges
	this function assumes     glBegin(GL_LINES) has been called 
*/
static void draw_edge(glCompPoint* posT,glCompPoint* posH, GLfloat length,int deg)
{
    double alpha, R, ITERANGLE;
    double X1, Y1, X2, Y2;

    if (deg) {
	R = length / 20.0;
	if ((deg / 2) * 2 != deg)	/*odd */
	    ITERANGLE = (deg) * 15.00 * -1;
	else
	    ITERANGLE = (deg) * 15.00;
	ITERANGLE = DEG2RAD * ITERANGLE;

	alpha = atan((posH->y - posT->y) / (posH->x - posT->x));
	if (posT->x > posH->x)
	    ITERANGLE = 180 * DEG2RAD - ITERANGLE;
	X1 = R * cos(alpha - ITERANGLE) + posT->x;
	Y1 = R * sin(alpha - ITERANGLE) + posT->y;
	X2 = R * cos(alpha - (180 * DEG2RAD - ITERANGLE)) + posH->x;
	Y2 = R * sin(alpha - (180 * DEG2RAD - ITERANGLE)) + posH->y;
	glVertex3f(posT->x, posT->y, posT->z);
	glVertex3f(X1, Y1, posT->z);
	glVertex3f(X1, Y1, posT->z);
	glVertex3f(X2, Y2, posH->z);
	glVertex3f(X2, Y2, posH->z);
	glVertex3f(posH->x, posH->y, posH->z);
    } else {
	glVertex3f(posT->x, posT->y, posT->z);
	glVertex3f(posH->x, posH->y, posH->z);

    }

}

static char* labelOf (Agraph_t* g, Agnode_t* v)
{
    char* lbl;
    char* s;

    Agsym_t* data_attr = GN_labelattribute(g);
    if (data_attr)
	s = agxget (v, data_attr);
    else
	s = agxget (g, GG_labelattribute(g));
    if ((*s == '\0') || !strcmp (s, "name"))
	lbl = agnameof (v);
    else {
	lbl = agget (v, s);
	if (!lbl) lbl = "";
    }
    return lbl;
}

static void renderSelectedNodes(Agraph_t * g)
{
    Agnode_t *v;
    xdot * x;
    glCompPoint pos;
    Agsym_t* l_color_attr = GG_nodelabelcolor(g);
    glCompColor c;
    int defaultNodeShape;
    GLfloat nodeSize;

    glCompColorxlate(&c,agxget(g,l_color_attr));

    defaultNodeShape=getAttrBool(g,g,"defaultnodeshape",0);
    if(defaultNodeShape==0)
	glBegin(GL_POINTS);

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	if(!ND_selected(v));
	    continue;
	x=parseXdotwithattrs(v);
	draw_xdot(x,-1);
	if(x)
	    freeXDot (x);
    }

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	if(!ND_selected(v))
	    continue;
	glColor4f(view->selectedNodeColor.R, view->selectedNodeColor.G,view->selectedNodeColor.B, view->selectedNodeColor.A);
	pos = ND_A(v);
	nodeSize = ND_size(v);

	if (defaultNodeShape == 0) 
	    glVertex3f(pos.x,pos.y,pos.z+0.001);
	else if (defaultNodeShape == 1) 
	    drawCircle(pos.x,pos.y,nodeSize,pos.z+0.001);
    }
    if(defaultNodeShape==0)
	glEnd();
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	if(!ND_selected(v))
	    continue;
	if (ND_printLabel(v)==1)
	{
	    pos = ND_A(v);
	    glColor4f(c.R, c.G,c.B, c.A);
            glprintfglut(view->glutfont,pos.x,pos.y,pos.z+0.002,labelOf(g,v));
	}
    }
}



static void renderNodes(Agraph_t * g)
{
    Agnode_t *v;
    glCompPoint pos;
    Agsym_t* pos_attr = GN_pos(g);
    Agsym_t* size_attr = GN_size(g);
    Agsym_t* selected_attr = GN_selected(g);
    int defaultNodeShape;
    GLfloat nodeSize;
    glCompColor c;
    xdot * x;
    int ind;

    defaultNodeShape=getAttrInt(g,g,"defaultnodeshape",0);

    x=parseXdotwithattrs(g);
    if (x) {
	draw_xdot(x,-0.2);
	freeXDot (x);
    }
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	    if(!object_color(v,&c))
		continue;
	    x=parseXdotwithattrs(v);
	    draw_xdot(x,-0.1);

	    if(x)
		freeXDot (x);
    }

    if(defaultNodeShape==0)
	glBegin(GL_POINTS);

    ind=0;

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	ND_TVref(v) = ind;
	if(!object_color(v,&c))
	{
	    ND_visible(v) = 0;
	    continue;
	}
	else
	    ND_visible(v) = 1;

	if(l_int(v, selected_attr,0))
	{
	    ND_selected(v) = 1;
	}
	glColor4f(c.R,c.G,c.B,c.A);	    
	pos=getPointFromStr(agxget(v, pos_attr));
	nodeSize=(GLfloat)l_float(v, size_attr,0);

	ND_A(v) = pos;

        if (nodeSize > 0)
	    nodeSize=nodeSize*view->nodeScale;
	else
	    nodeSize=view->nodeScale;
	if(defaultNodeShape==0)
	    nodeSize=1;
	ND_size(v) = nodeSize;
	if (defaultNodeShape == 0) 
	    glVertex3f(pos.x,pos.y,pos.z);
	else if (defaultNodeShape == 1) 
	    drawCircle(pos.x,pos.y,nodeSize,pos.z);
	ind++;
    }
    if(defaultNodeShape==0)
	glEnd();
}


static void renderSelectedEdges(Agraph_t * g)
{

    Agedge_t *e;
    Agnode_t *v;
    xdot * x;
    glCompPoint posT;	/*Tail position*/
    glCompPoint posH;	/*Head position*/
    glCompColor c;
    /*xdots tend to be drawn as background shapes,that is why they are being rendered before edges*/

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	for (e = agfstout(g, v); e; e = agnxtout(g, e)) 
	{
	    if(!ED_selected(e))
		continue;
	    if(!object_color(e,&c))
		continue;

	    x=parseXdotwithattrs(e);
	    draw_xdot(x,0);
	    if(x)
		freeXDot (x);
	}
    }

    glBegin(GL_LINES);
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	for (e = agfstout(g, v); e; e = agnxtout(g, e)) 
	{
	    if(!ED_selected(e))
		continue;

	    if(!object_color(e,&c))
		continue;
	    glColor4f(1,0,0,1);	    
	    posT = ED_posTail(e);
	    posH = ED_posHead(e);
	    posT.z +=0.01;
	    posH.z +=0.01;
	    draw_edge(&posT,&posH,getEdgeLength(e),0);
	}
    }
    glEnd();
}

/* skipWS:
 * Skip whitespace
 */
static char* skipWS (char* p)
{
    while (isspace(*p)) p++;
    return p;
}

/* skipNWS:
 * Skip non-whitespace
 */
static char* skipNWS (char* p)
{
    while (*p && !isspace(*p)) p++;
    return p;
}

/* readPoint:
 * Parse x,y[,z] and store in pt.
 * If z is not specified, set to 0.
 * Return pointer to next character after reading the point.
 * Return NULL on error.
 */
static char* readPoint (char* p, xdot_point* pt)
{
    char* endp;

    pt->z = 0;
    pt->x = strtod (p, &endp);
    if (p == endp) {
	return 0;
    }
    else
	p = endp;
    if (*p == ',') p++;
    else return 0;

    pt->y = strtod (p, &endp);
    if (p == endp) {
	return 0;
    }
    else
	p = endp;
    if ((*p == ' ') || (*p == '\0')) return p;
    else if (*p == ',') p++;
    else return 0;

    pt->z = strtod (p, &endp);
    if (p == endp) {
	return 0;
    }
    else
	return endp;
}

/* countPoints:
 * count number of points in pos attribute; store in cntp;
 * check for e and s points; store if found and increment number of
 * points by 3 for each.
 * return start of point list (skip over e and s points).
 * return NULL on failure
 */
static char* countPoints (char* pos, int* have_sp, xdot_point* sp, int* have_ep, xdot_point* ep, int* cntp)
{
    int cnt = 0;
    char* p;

    pos = skipWS (pos);
    if (*pos == 's') {
	if ((pos = readPoint (pos+2, sp))) {
	    *have_sp = 1;
	    cnt += 3;
	}
	else
	    return 0;
    }
    else
	*have_sp = 0;

    pos = skipWS (pos);
    if (*pos == 'e') {
	if ((pos = readPoint (pos+2, ep))) {
	    *have_ep = 1;
	    cnt += 3;
	}
	else
	    return 0;
    }
    else
	*have_ep = 0;

    p = pos = skipWS (pos);

    while (*p) {
	cnt++;
	p = skipNWS (p);
	p = skipWS (p);
    }
    *cntp = cnt;

    return pos;
}

/* storePoints:
 * read comma-separated list of points
 * and store them in ps
 * Assumes enough storage is available.
 * return -1 on error
 */
static int storePoints (char* pos, xdot_point* ps)
{
    
    while (*pos) {
	if ((pos = readPoint (pos, ps))) {
	    ps++;
	    pos = skipWS(pos);
	}
	else
	    return -1;
    }
    return 0;
}

/* makeXDotSpline:
 * Generate an xdot representation of an edge's pos attribute
 */
static xdot* makeXDotSpline (char* pos)
{
    xdot_point s, e;
    int v, have_s, have_e, cnt;
    int sz = sizeof(sdot_op);
    xdot* xd;
    xdot_op* op;
    xdot_point* pts;

    if (*pos == '\0') return NULL;

    pos = countPoints (pos, &have_s, &s, &have_e, &e, &cnt);
    if (pos == 0) return NULL;

    pts = N_NEW(cnt,xdot_point);
    if (have_s) {
	v = storePoints (pos, pts+3);
	pts[0] = pts[1] = s;
	pts[2] = pts[3];
    }
    else
	v = storePoints (pos, pts);
    if (v) {
	free (pts);
	return NULL;
    }

    if (have_e) {
	pts[cnt-1] = pts[cnt-2] = e;
	pts[cnt-3] = pts[cnt-4];
    }

    op = (xdot_op*)N_NEW(sz,char);
    op->kind = xd_unfilled_bezier;
    op->drawfunc = OpFns[xop_bezier];
    op->u.bezier.cnt = cnt; 
    op->u.bezier.pts = pts; 

    xd = NEW(xdot);
    xd->cnt = 1;
    xd->sz = sz;
    xd->ops = op;

    return xd;
}

typedef void (*edgefn) (Agraph_t *, Agedge_t*, glCompColor);

static void renderEdgesFn (Agraph_t * g, edgefn ef, int skipSelected)
{
    Agedge_t *e;
    Agnode_t *v;
    glCompColor c;

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	for (e = agfstout(g, v); e; e = agnxtout(g, e)) 
	{
	    if ((ND_visible(agtail(e))==0) || (ND_visible(aghead(e))==0))
		continue;

	    if(!object_color(e,&c)) {
		ED_visible(e) = 0;
		continue;
	    }
	    else
		ED_visible(e) = 1;
	    if (ED_selected(e) && skipSelected)
		continue;

	    ef (g, e, c);
	}
    }
}

static void edge_xdot (Agraph_t* g, Agedge_t* e, glCompColor c)
{
    xdot * x;
    x=parseXdotwithattrs(e);
    draw_xdot(x,0);
    if(x)
	freeXDot (x);
}

static void edge_seg (Agraph_t* g, Agedge_t* e, glCompColor c)
{
    Agsym_t* pos_attr = GN_pos(g);
    glCompPoint posT;	/*Tail position*/
    glCompPoint posH;	/*Head position*/

    glColor4f(c.R,c.G,c.B,c.A);	   
    posT=getPointFromStr(agxget(agtail(e), pos_attr));
    posH=getPointFromStr(agxget(aghead(e), pos_attr));
    draw_edge(&posT,&posH,getEdgeLength(e),0);
    ED_posTail(e) = posT;
    ED_posHead(e) = posH;
}

static void edge_spline (Agraph_t* g, Agedge_t* e, glCompColor c)
{
    Agsym_t* pos_attr_e = GE_pos(g);
    xdot * x;

    glColor4f(c.R,c.G,c.B,c.A);	   
    x = makeXDotSpline (agxget(e,pos_attr_e));
    if (x) {
	draw_xdot(x,0);
	freeXDot (x);
    }
}

static void renderEdges(Agraph_t * g)
{
    Agsym_t* pos_attr_e = GE_pos(g);
    int drawSegs = !(pos_attr_e && view->drawSplines);
    /*xdots tend to be drawn as background shapes,that is why they are being rendered before edges*/

    renderEdgesFn (g, edge_xdot, 0);

    if (drawSegs) {
	glBegin(GL_LINES);
	renderEdgesFn (g, edge_seg, 1);
	glEnd();
    }
    else
	renderEdgesFn (g, edge_spline, 1);
}

static void renderNodeLabels(Agraph_t * g)
{
    Agnode_t *v;
    glCompPoint pos;
    Agsym_t* data_attr = GN_labelattribute(g);
    Agsym_t* l_color_attr = GG_nodelabelcolor(g);
    GLfloat nodeSize;
    glCompColor c;

    glCompColorxlate(&c,agxget(g,l_color_attr));

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	 if(ND_visible(v)==0)
	    continue;
	 if(ND_selected(v)==1)
	    continue;

	pos = ND_A(v);
	nodeSize = ND_size(v);
	glColor4f(c.R,c.G,c.B,c.A);
	if(!data_attr)
            glprintfglut(view->glutfont,pos.x,pos.y,pos.z,agnameof(v));
	else
	    glprintfglut(view->glutfont,pos.x,pos.y,pos.z,agxget(v,data_attr));
    }
}

static void renderEdgeLabels(Agraph_t * g)
{
    Agedge_t *e;
    Agnode_t *v;
    glCompPoint posT;
    glCompPoint posH;
    Agsym_t* data_attr = GE_labelattribute(g);
    Agsym_t* l_color_attr = GG_edgelabelcolor(g);
    glCompColor c;
    GLfloat x,y,z;

    glCompColorxlate(&c,agxget(g,l_color_attr));

    if(!data_attr || !l_color_attr)
	return;

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	for (e = agfstout(g, v); e; e = agnxtout(g, e)) 
	{

	    if (ND_visible(v)==0)
		continue;

	    posT = ED_posTail(e);
	    posH = ED_posHead(e);
	    glColor4f(c.R,c.G,c.B,c.A);
	    x=posH.x+(posT.x-posH.x)/2;
	    y=posH.y+(posT.y-posH.y)/2;
	    z=posH.z+(posT.z-posH.z)/2;
	    glprintfglut(view->glutfont,x,y,z,agxget(e,data_attr));

	}
    }
}





static void cacheNodes(Agraph_t * g,topview* t)
{
    if(t->cache.node_id!=-1)	/*clean existing cache*/
	glDeleteLists(t->cache.node_id,1);
    t->cache.node_id=glGenLists(1);
    glNewList(t->cache.node_id,GL_COMPILE);
    renderNodes(g);
    glEndList();




}
static void cacheEdges(Agraph_t * g,topview* t)
{
    if(t->cache.edge_id!=-1)	/*clean existing cache*/
	glDeleteLists(t->cache.edge_id,1);
    t->cache.edge_id=glGenLists(1);
    glNewList(t->cache.edge_id,GL_COMPILE);
    renderEdges(g);
    glEndList();


}
void cacheSelectedEdges(Agraph_t * g,topview* t)
{
    if(t->cache.seledge_id!=-1)	/*clean existing cache*/
	glDeleteLists(t->cache.seledge_id,1);
    t->cache.seledge_id=glGenLists(1);
    glNewList(t->cache.seledge_id,GL_COMPILE);
    renderSelectedEdges(g);
    glEndList();


}
void cacheSelectedNodes(Agraph_t * g,topview* t)
{
    if(t->cache.selnode_id!=-1)	/*clean existing cache*/
	glDeleteLists(t->cache.selnode_id,1);
    t->cache.selnode_id=glGenLists(1);
    glNewList(t->cache.selnode_id,GL_COMPILE);
    renderSelectedNodes(g);
    glEndList();
}
static void cacheNodeLabels(Agraph_t * g,topview* t)
{
    if(t->cache.nodelabel_id!=-1)	/*clean existing cache*/
	glDeleteLists(t->cache.nodelabel_id,1);
    t->cache.nodelabel_id=glGenLists(1);
    glNewList(t->cache.nodelabel_id,GL_COMPILE);
    renderNodeLabels(g);
    glEndList();
}
static void cacheEdgeLabels(Agraph_t * g,topview* t)
{
    if(t->cache.edgelabel_id!=-1)	/*clean existing cache*/
	glDeleteLists(t->cache.edgelabel_id,1);
    t->cache.edgelabel_id=glGenLists(1);
    glNewList(t->cache.edgelabel_id,GL_COMPILE);
    renderEdgeLabels(g);
    glEndList();
}

void updateSmGraph(Agraph_t * g,topview* t)
{
    Agnode_t *v;
    Agedge_t *e;
    float eLength=0;
    float totalELength=0;

    t->Nodecount=0;
    t->Edgecount=0;
    t->avgedgelength=0;
    t->maxedgelen=0;
    t->minedgelen=-1;

    t->picked_node_count = 0;
    t->picked_nodes = '\0';
    t->picked_edge_count = 0;
    t->picked_edges = '\0';
    t->global_z=0;
    t->sel.selPoly.cnt=0;
    t->sel.selPoly.pts=NULL;

    if(!t)
	return ;
    /*Node Loop*/
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	for (e = agfstout(g, v); e; e = agnxtout(g, e)) 
	{
	    t->Edgecount++;
	    eLength=getEdgeLength(e);
	    if((t->minedgelen == -1) || (t->minedgelen > eLength))
		t->minedgelen=eLength;
	    if(eLength > t->maxedgelen)
		t->maxedgelen=eLength;
	    totalELength += eLength;
	}
	t->Nodecount++;

    }
    aginit(g, AGNODE, "nodeRec", sizeof(nodeRec), 0);
    aginit(g, AGEDGE, "edgeRec", sizeof(edgeRec), 0);

    set_boundaries(g,t);
    t->avgedgelength = totalELength / t->Edgecount;
//    t->init_node_size=init_node_size(g, t);
    view->Topview=t;


    /*render nodes once to get set some attributes set,THIS IS A HACK, FIX IT*/
    renderNodes(g);
    cacheEdges(g,t);
    cacheSelectedEdges(g,t);
    cacheNodes(g,t);
    cacheSelectedNodes(g,t);
    cacheEdgeLabels(g,t);
    cacheNodeLabels(g,t);
}
void initSmGraph(Agraph_t * g,topview* rv)
{
    rv->maxnodedegree = 1;

        

    /*create attribute list*/
    rv->attributes=load_attr_list(view->g[view->activeGraph]);
    rv->filtered_attr_list=NULL;

    /*set topologilca fisheye to NULL */
    rv->fisheyeParams.h = '\0';

    if (view->dfltViewType == VT_TOPFISH)
	rv->fisheyeParams.active = 1;
    else
	rv->fisheyeParams.active = 0;
    rv->cache.node_id=-1;
    rv->cache.selnode_id=-1;
    rv->cache.edge_id=-1;
    rv->cache.seledge_id=-1;
    rv->sel.selectEdges=0;
    rv->sel.selectNodes=1;

    updateSmGraph(g,rv);
}

void renderSmGraph(Agraph_t * g,topview* t)
{
    /*
	we like to have blending affect where node and edge  overlap
	to achive this depth test should be turned off.
    */

    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    /* glDepthFunc (GL_ALWAYS); */
    glEnable(GL_DEPTH);
//    glDepthMask(0);

    if(view->drawedges)
    {
	glCallList(t->cache.edge_id);
        glCallList(t->cache.seledge_id);
        if(view->drawedgelabels)
	{
	    if(view->zoom*-1 <	t->fitin_zoom /(float)view->labelnumberofnodes*-1) 
		    glCallList(t->cache.edgelabel_id);

	}
    }
    if(view->drawnodes)
    {
	glPointSize(view->nodeScale*t->fitin_zoom/view->zoom);
	glCallList(t->cache.node_id);
        glCallList(t->cache.selnode_id);
        if(view->drawnodelabels)
	{
	    if(view->zoom*-1 <	t->fitin_zoom /(float)view->labelnumberofnodes*-1) 
		glCallList(t->cache.nodelabel_id);
	}
    }

}

void freeSmGraph(Agraph_t * g,topview* t)
{
    return ;
}


