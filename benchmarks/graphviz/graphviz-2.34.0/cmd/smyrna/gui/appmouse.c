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

#include "appmouse.h"
#include "topfisheyeview.h"
#include "arcball.h"
/* #include "topview.h" */
#include "glmotion.h"
/* #include "beacon.h" */
#include "hotkeymap.h"

#include "selectionfuncs.h"
#include "topviewfuncs.h"


static float prevX=0;
static float prevY=0;
static int lastAction;

static void apply_actions(ViewInfo* v,int x,int y)
{
    int a;
    gdouble seconds;
    a=get_mode(v);
    if((a==MM_PAN) && (view->guiMode==GUI_FULLSCREEN) &&((v->active_camera >= 0)))
	a=MM_ROTATE;	
    switch (a) {
    case MM_ROTATE :
	seconds = g_timer_elapsed(view->timer3, NULL);
	if (seconds > 0.1) {
	    g_timer_stop(view->timer3);
		view->arcball->MousePt.s.X = (GLfloat) x;
		view->arcball->MousePt.s.Y = (GLfloat) y;
		if (!view->arcball->isDragging) {
		    arcmouseClick(view);
		    view->arcball->isDragging = 1;
		}
		else 
		    arcmouseDrag(view);
	    g_timer_start(view->timer3);
	}
	return;
	break;
    case MM_PAN :
	glmotion_pan(v);
	break;
    case MM_MOVE :
        /* move_TVnodes(); */
	break;
    case MM_RECTANGULAR_SELECT :
	if (!view->mouse.down) {
//	    rectangle_select(v);
	    pick_objects_rect(view->g[view->activeGraph]) ;
	}
	break;
    case MM_POLYGON_SELECT :
	add_selpoly(view->g[view->activeGraph],&view->Topview->sel.selPoly,view->mouse.GLfinalPos);
	break;
    case MM_SINGLE_SELECT :
	pick_object_xyz(view->g[view->activeGraph],view->Topview,view->mouse.GLfinalPos.x,view->mouse.GLfinalPos.y,view->mouse.GLfinalPos.z);
	break;
    case MM_FISHEYE_PICK :
	if (view->activeGraph >= 0) {
	    if (view->Topview->fisheyeParams.active) 
		changetopfishfocus(view->Topview,&view->mouse.GLpos.x,&view->mouse.GLpos.y,  0, 1);
#if 0
	    else    //single right click
		pick_node_from_coords(view->mouse.GLpos.x, view->mouse.GLpos.y,view->mouse.GLpos.z);
#endif

	}
	break;
    }
    lastAction=a;
}

static int singleclick(ViewInfo* v)
{
       return(((int)v->mouse.initPos.x == (int)v->mouse.finalPos.x) && ((int)v->mouse.initPos.y == (int)v->mouse.finalPos.y));

}
static void appmouse_left_click(ViewInfo* v,int x,int y)
{

}
static void appmouse_right_click(ViewInfo* v,int x,int y)
{
}
static void appmouse_down(ViewInfo* v,int x,int y)
{
    view->mouse.dragX = 0;
    view->mouse.dragY = 0;
    v->mouse.down=1;
    v->mouse.initPos.x=x;
    v->mouse.initPos.y=y;
    v->mouse.pos.x=x;
    v->mouse.pos.y=y;
    
    to3D(x,y,&v->mouse.GLinitPos.x,&v->mouse.GLinitPos.y,&v->mouse.GLinitPos.z);
    to3D( x,y, &v->mouse.GLpos.x,&v->mouse.GLpos.y,&v->mouse.GLpos.z);

    prevX=0;
    prevY=0;

/*    view->Selection.X = view->mouse.GLpos.x;
    view->Selection.Y = view->mouse.GLpos.y;*/
}
static void appmouse_up(ViewInfo* v,int x,int y)
{

    /* int a; */
    v->mouse.down=0;
    v->mouse.finalPos.x=x;
    v->mouse.finalPos.y=y;
    /* a=get_mode(v); */
    to3D(x,y, &v->mouse.GLfinalPos.x,&v->mouse.GLfinalPos.y,&v->mouse.GLfinalPos.z);
    if(singleclick(v))
    {
	if (v->mouse.t==glMouseLeftButton)
	    appmouse_left_click(v,x,y);
	if (v->mouse.t==glMouseRightButton)
	    appmouse_right_click(v,x,y);
    }
    apply_actions(v,x,y);
    view->mouse.dragX = 0;
    view->mouse.dragY = 0;
    view->arcball->isDragging=0;


}
static void appmouse_drag(ViewInfo* v,int x,int y)
{
    v->mouse.pos.x=x;
    v->mouse.pos.y=y;
    to3D( x,y, &v->mouse.GLpos.x,&v->mouse.GLpos.y,&v->mouse.GLpos.z);
    prevX=v->mouse.GLpos.x;
    prevY=v->mouse.GLpos.y;
    apply_actions(v,x,y);
}

void appmouse_left_click_down(ViewInfo* v,int x,int y)
{
       v->mouse.t=glMouseLeftButton;
       appmouse_down(v,x,y);


}
void appmouse_left_click_up(ViewInfo* v,int x,int y)
{
    appmouse_up(v,x,y);
/*	if (v->mouse.mouse_mode == MM_MOVE)
	    move_TVnodes();*/
}
void appmouse_left_drag(ViewInfo* v,int x,int y)
{
    appmouse_drag(v,x,y);



}
void appmouse_right_click_down(ViewInfo* v,int x,int y)
{
    v->mouse.t=glMouseRightButton;
    appmouse_down(v,x,y);


}
void appmouse_right_click_up(ViewInfo* v,int x,int y)
{
    appmouse_up(v,x,y);
 
}
void appmouse_right_drag(ViewInfo* v,int x,int y)
{
        
    appmouse_drag(v,x,y);

}


void appmouse_middle_click_down(ViewInfo* v,int x,int y)
{
    v->mouse.t=glMouseMiddleButton;
    appmouse_down(v,x,y);


}
void appmouse_middle_click_up(ViewInfo* v,int x,int y)
{
    appmouse_up(v,x,y);
 
}
void appmouse_middle_drag(ViewInfo* v,int x,int y)
{
        
    appmouse_drag(v,x,y);

}
void appmouse_move(ViewInfo* v,int x,int y)
{
    to3D( x,y, &v->mouse.GLpos.x,&v->mouse.GLpos.y,&v->mouse.GLpos.z);
}
void appmouse_key_release(ViewInfo* v,int key)
{
    /* int action=get_key_action(v,key); */
    if(lastAction==MM_POLYGON_SELECT)
    {
	clear_selpoly(&view->Topview->sel.selPoly);	
	glexpose();
    }
    v->keymap.down=0;
    v->keymap.keyVal=0;
}
void appmouse_key_press(ViewInfo* v,int key)
{
    v->keymap.down=1;
    v->keymap.keyVal=key;
}


