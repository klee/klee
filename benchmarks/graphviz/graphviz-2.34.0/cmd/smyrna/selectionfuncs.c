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

#include "selectionfuncs.h"
#include "topviewfuncs.h"
#include "smyrna_utils.h"

static void select_node(Agraph_t* g,Agnode_t*  obj,int reverse)
{
    Agsym_t* sel_attr = GN_selected(g);

    if(!sel_attr)
	sel_attr = GN_selected(g) = agattr(g, AGNODE,"selected","0");
    if(!reverse)
    {
        agxset(obj,sel_attr,"1");
	ND_selected(obj) = 1;
    }
    else
    {
	if(ND_selected(obj)==1)
	{
            agxset(obj,sel_attr,"0");
	    ND_selected(obj) = 0;
	    ND_printLabel(obj) = 0;
	}
	else
	{
            agxset(obj,sel_attr,"1");
	    ND_selected(obj) = 1;
	}

    }


}
static void select_edge(Agraph_t* g,Agedge_t*  obj,int reverse)
{
    Agsym_t* sel_attr = GE_selected(g); 

    if (!sel_attr)
	sel_attr = GE_selected(g) = agattr(g, AGEDGE,"selected","0");
    if (!reverse)
    {
	agxset(obj,sel_attr,"1");
	ED_selected(obj) = 1;
    }
    else
    {
	if (ED_selected(obj) == 1)
	{
	    agxset(obj,sel_attr,"0");
	    ED_selected(obj) = 0;
	    ED_printLabel(obj) = 0;
	}
	else
	{
	    agxset(obj,sel_attr,"1");
	    ED_selected(obj) = 1;
	}
    }


}


static void pick_objects_in_rect(Agraph_t* g,GLfloat x1,GLfloat y1,GLfloat x2,GLfloat y2)
{
    Agnode_t *v;
    Agedge_t *e;
    glCompPoint posT;
    glCompPoint posH;
    glCompPoint posN;
     
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	if(view->Topview->sel.selectNodes==1)
	{
	    posN = ND_A(v);
	    if(!ND_visible(v))
		continue;
	    if(is_point_in_rectangle(posN.x,posN.y,x1,y1,x2-x1,y2-y1) )
		select_node(g,v,0);
	}
	if(view->Topview->sel.selectEdges==1) {
	    for (e = agfstout(g, v); e; e = agnxtout(g, e)) 
	    {
		posT = ED_posTail(e);
		posH = ED_posHead(e);
    		if(is_point_in_rectangle(posT.x,posT.y,x1,y1,x2-x1,y2-y1))
		    if(is_point_in_rectangle(posH.x,posH.y,x1,y1,x2-x1,y2-y1))
			select_edge(g,e,0);
	    }
	}
    }
}



static void* pick_object(Agraph_t* g,glCompPoint p)
{
    Agnode_t *v;
    Agedge_t *e;
    glCompPoint posT;
    glCompPoint posH;
    glCompPoint posN;
    int defaultNodeShape;
    float dist=999999999;
    GLfloat nd; /*node distance to point*/
    GLfloat ed; /*edge distance to point*/
    GLfloat nodeSize=0;
    void* rv=(void*)0;

    defaultNodeShape=getAttrBool(g,g,"defaultnodeshape",0);

    if(defaultNodeShape==0)
        nodeSize=GetOGLDistance(view->nodeScale*view->Topview->fitin_zoom/view->zoom);

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	if(!ND_visible(v))
	    continue;
	posN = ND_A(v);
	if(defaultNodeShape==1)
	{
	    nodeSize = ND_size(v);
	}

	nd=distBetweenPts(posN,p,nodeSize);
	if( nd < dist )
	{
	    rv=v;dist=nd;
	}

	for (e = agfstout(g, v); e; e = agnxtout(g, e)) 
	{
	    posT = ED_posTail(e);
	    posH = ED_posHead(e);
	    ed=point_to_lineseg_dist(p, posT,posH);
	    if( ed < dist ) {rv=e;dist=ed;}
	}
    }
    return rv;
}

void pick_object_xyz(Agraph_t* g,topview* t,GLfloat x,GLfloat y,GLfloat z) 
{
    glCompPoint p;
    void* a;
    p.x=x;p.y=y;p.z=z;
    a=pick_object(g,p);
    if (!a)
	return;
    if(agobjkind(a)==AGNODE)
    {
	select_node(g,a,1);	
	ND_printLabel((Agnode_t*)a) = 1;

	cacheSelectedNodes(g,t);

    }
    if(agobjkind(a)==AGEDGE)
    {
	select_edge(g,a,1);	
	cacheSelectedEdges(g,t);
	ED_printLabel((Agedge_t*)a) = 1;
    }
}
void pick_objects_rect(Agraph_t* g) 
{

    GLfloat x1;
    GLfloat y1;
    GLfloat x2;
    GLfloat y2;
    if(view->mouse.GLfinalPos.x > view->mouse.GLinitPos.x)
    {
        x1=view->mouse.GLinitPos.x;
	x2=view->mouse.GLfinalPos.x;
    }
    else
    {
        x2=view->mouse.GLinitPos.x;
	x1=view->mouse.GLfinalPos.x;

    }
    if(view->mouse.GLfinalPos.y > view->mouse.GLinitPos.y)
    {
        y1=view->mouse.GLinitPos.y;
	y2=view->mouse.GLfinalPos.y;
    }
    else
    {
        y2=view->mouse.GLinitPos.y;
	y1=view->mouse.GLfinalPos.y;
    }
    pick_objects_in_rect(g,x1,y1,x2,y2);
    cacheSelectedNodes(g,view->Topview);
    cacheSelectedEdges(g,view->Topview);
}



void deselect_all(Agraph_t* g)
{
    Agnode_t *v;
    Agedge_t *e;
    Agsym_t* nsel_attr = GN_selected(g);
    Agsym_t* esel_attr = GE_selected(g);
    if(!nsel_attr)
	nsel_attr = GN_selected(g) = agattr(g, AGNODE,"selected","0");
    if(!esel_attr)
	esel_attr = GE_selected(g) = agattr(g, AGEDGE,"selected","0");
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	agxset(v,nsel_attr,"0");
	ND_selected(v) = 0;
	ND_printLabel(v) = 0;

	for (e = agfstout(g, v); e; e = agnxtout(g, e)) 
	{
	    agxset(e,esel_attr,"0");
	    ED_selected(e) = 0;
	    ED_printLabel(e) = 0;
	}
    }
    cacheSelectedNodes(g,view->Topview);
    cacheSelectedEdges(g,view->Topview);
}

void clear_selpoly(glCompPoly* sp)
{
    sp->pts=realloc(sp->pts,0);
    sp->cnt=0;
}
static int close_poly(glCompPoly* selPoly,glCompPoint pt)
{
    /* int i=0; */
    float EPS=GetOGLDistance(3);
    if (selPoly->cnt < 2)
	return 0;
    if(
	( (selPoly->pts[0].x-pt.x) < EPS) &&
	( (selPoly->pts[0].y-pt.y) < EPS))
	return 1;
    return 0;
}


static void select_polygon (Agraph_t* g,glCompPoly* selPoly)
{
    Agnode_t *v;
    glCompPoint posN;
     
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	posN = ND_A(v);
	if(point_in_polygon(selPoly,posN))
	    select_node(g,v,0);
    }
    cacheSelectedNodes(g,view->Topview);
}


void add_selpoly(Agraph_t* g,glCompPoly* selPoly,glCompPoint pt)
{
    if(!close_poly(selPoly,pt))
    {
	selPoly->cnt ++;
	selPoly->pts=realloc(selPoly->pts,sizeof(glCompPoint)*selPoly->cnt);
	selPoly->pts[selPoly->cnt-1].x=pt.x;
	selPoly->pts[selPoly->cnt-1].y=pt.y;
	selPoly->pts[selPoly->cnt-1].z=0;
    }
    else
    {
	select_polygon (g,selPoly);
	clear_selpoly(selPoly);	
    }
}

