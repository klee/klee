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

#include "glmotion.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkcursor.h>
#include "draw.h"
/* #include "topview.h" */
#include "glutils.h"
#include "hotkeymap.h"

gboolean redraw = FALSE;

#if 0
void glmotion_main(ViewInfo * v, GdkEventMotion * event,
		   GtkWidget * widget)
{

    int a=get_mode(v);
    redraw = FALSE;
    /*panning */
    if (a == MM_PAN)
	glmotion_pan(v);
    /*rotating, only in 3d v */
    if (a == MM_ROTATE)
	glmotion_rotate(v);

    /*zooming */
    if (a == MM_ZOOM)
	glmotion_zoom(v);
    /*selection rect */
    if ((a == MM_RECTANGULAR_SELECT) || (a == 5)) 
    {
	GetFixedOGLPos((int) event->x, (int) event->y, v->GLDepth,
		       &(v->mouse.GLfinalPos.x), &(v->mouse.GLfinalPos.y), &(v->mouse.GLfinalPos.z));
	redraw = TRUE;
    }
    if (a == MM_MOVE) 
    {
	GetFixedOGLPos((int) event->x, (int) event->y, v->GLDepth,
		       &(v->mouse.GLfinalPos.x), &(v->mouse.GLfinalPos.y), &(v->mouse.GLfinalPos.z));
	redraw = TRUE;
    }
    if (redraw)
	gdk_window_invalidate_rect(widget->window, &widget->allocation,
				   FALSE);

}
#endif
/*real zoom in out is done here, all other functions send this one what they desire, it is not guranteed,*/
static void graph_zoom(float real_zoom)
{
    float old_zoom;

    if (view->active_camera == -1)
		old_zoom = view->zoom;
    else
		old_zoom = view->cameras[view->active_camera]->r;

    if (real_zoom < view->Topview->fitin_zoom * MAX_ZOOM)
		real_zoom = (float) view->Topview->fitin_zoom * MAX_ZOOM;
    if (real_zoom > view->Topview->fitin_zoom * MIN_ZOOM)
		real_zoom = (float) view->Topview->fitin_zoom * MIN_ZOOM;
    if (view->active_camera == -1)
		view->zoom = real_zoom;
    else
		view->cameras[view->active_camera]->r = real_zoom * -1;
    /*adjust pan values */
    view->panx = old_zoom * view->panx / real_zoom;
    view->pany = old_zoom * view->pany / real_zoom;

    /*set label to new zoom value */
#ifdef UNUSED
    xx = ((float) 100.0 - (float) 1.0) * (v->zoom -
					  (float) MIN_ZOOM) /
	((float) MAX_ZOOM - (float) MIN_ZOOM) + (float) 1.0;
    sprintf(buf, "%i", (int) xx);
#endif
/*	if (v->Topview->customptr)
		glCompLabelSetText((glCompLabel *) v->Topview->customptr, buf);*/
}

void glmotion_zoom_inc(int zoomin)
{
    float inc_value;
    inc_value =
	(float) (view->Topview->fitin_zoom * MAX_ZOOM * -1 -
		 view->Topview->fitin_zoom * MIN_ZOOM * -1) / ZOOM_STEPS;
    if (zoomin)			/*zooming in , zoom value should be decreased */
	graph_zoom(view->zoom - view->zoom * 0.25);
    else
	graph_zoom(view->zoom + view->zoom * 0.25);	/*zoom out */
    glexpose();

}

void glmotion_zoom(ViewInfo * v)
{
    float real_zoom;
    if (view->active_camera == -1) {
	real_zoom =
	    view->zoom + view->mouse.dragX / 10 * (view->zoom * -1 / 20);
    } else {
	real_zoom =
	    (view->cameras[view->active_camera]->r +
	     view->mouse.dragX / 10 * (view->cameras[view->active_camera]->r /
				    20)) * -1;
    }
    graph_zoom(real_zoom);

}

void glmotion_pan(ViewInfo * v)
{
    float gldx, gldy;
    if (v->active_camera == -1) {
	gldx = GetOGLDistance((int) v->mouse.dragX) / v->zoom * -1;
	gldy = GetOGLDistance((int) v->mouse.dragY) / v->zoom * -1;
	v->panx = v->panx - gldx;
	v->pany = v->pany + gldy;
/*	v->panx = v->panx - v->mouse.dragX / v->zoom * -1;;
	v->pany = v->pany + v->mouse.dragY/ v->zoom * -1;*/
    } else {
	gldx =
	    GetOGLDistance((int) v->mouse.dragX) /
	    v->cameras[v->active_camera]->r;
	gldy =
	    GetOGLDistance((int) v->mouse.dragY) /
	    v->cameras[v->active_camera]->r;
	v->cameras[v->active_camera]->x -= gldx;
	v->cameras[v->active_camera]->y -= gldy;
	v->cameras[v->active_camera]->targetx -= gldx;
	v->cameras[v->active_camera]->targety += gldy;
    }

    redraw = TRUE;
}
#if 0
void glmotion_adjust_pan(ViewInfo * v, float panx, float pany)
{
    float gldx, gldy;
    if (v->active_camera == -1) {
	gldx = GetOGLDistance((int) panx) / v->zoom * -1;
	gldy = GetOGLDistance((int) pany) / v->zoom * -1;
	v->panx = v->panx - gldx;
	v->pany = v->pany + gldy;
    } else {
	gldx =
	    GetOGLDistance((int) panx) / v->cameras[v->active_camera]->r;
	gldy =
	    GetOGLDistance((int) pany) / v->cameras[v->active_camera]->r;
	v->cameras[v->active_camera]->x -= gldx;
	v->cameras[v->active_camera]->y -= gldy;
	v->cameras[v->active_camera]->targetx -= gldx;
	v->cameras[v->active_camera]->targety += gldy;
    }

    redraw = TRUE;

}
#endif

#ifdef UNUSED
static float mod_angle(float angle)
{
//      if (angle > 360)

}
#endif
#if 0
void glmotion_rotate(ViewInfo * v)
{
/*	if(v->mouse.rotate_axis==MOUSE_ROTATE_XY)
	{
		v->arcball

		v->cameras[v->active_camera]->angley-=v->mouse.dy/7;
		v->cameras[v->active_camera]->anglex-=v->mouse.dragX/7;
	}
	if(v->mouse.rotate_axis==MOUSE_ROTATE_Y)
	{
		v->cameras[v->active_camera]->anglex-=v->mouse.dragX/7;
	}
	if(v->mouse.rotate_axis==MOUSE_ROTATE_X)
	{
		v->cameras[v->active_camera]->angley-=v->mouse.dy/7;
	}
	if(v->mouse.rotate_axis==MOUSE_ROTATE_Z)
	{
		v->cameras[v->active_camera]->anglez-=v->mouse.dragX/7;
		v->cameras[v->active_camera]->anglez-=v->mouse.dy/7;
	}*/
}
#endif
