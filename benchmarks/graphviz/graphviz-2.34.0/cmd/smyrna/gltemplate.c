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

#include <stdlib.h>
#include <math.h>
#include "gui.h"
/* #include "beacon.h" */
#include "viewport.h"
/* #include "topview.h" */
#include "gltemplate.h"
#include "glutils.h"
#include "glexpose.h"
#include "glmotion.h"

#include "glcompset.h"
#include "viewportcamera.h"
#include "gui/menucallbacks.h"
#include "arcball.h"
#include "appmouse.h"
static float begin_x = 0.0;
static float begin_y = 0.0;
static float dx = 0.0;
static float dy = 0.0;

/*mouse mode mapping funvtion from gtk to glcomp*/
static glMouseButtonType getGlCompMouseType(int n)
{
    switch (n) {
    case 1:
	return glMouseLeftButton;
    case 2:
	return glMouseMiddleButton;
    case 3:
	return glMouseRightButton;

    default:
	return glMouseLeftButton;
    }
}


/*
	test single opengl parameter, all visual , it doesnt return a value
	params:gtk gl config class , attribute name and id,if boolean expected send is_boolean true
	return value:none
*/
void print_gl_config_attrib(GdkGLConfig * glconfig,
			    const gchar * attrib_str,
			    int attrib, gboolean is_boolean)
{
    int value;

    g_print("%s = ", attrib_str);
    if (gdk_gl_config_get_attrib(glconfig, attrib, &value)) {
	if (is_boolean)
	    g_print("%s\n", value == TRUE ? "TRUE" : "FALSE");
	else
	    g_print("%d\n", value);
    } else
	g_print("*** Cannot get %s attribute value\n", attrib_str);
}


/*
	test opengl parameters, configuration.Run this function to see machine's open gl capabilities
	params:gtk gl config class ,gtk takes care of all these tests
	return value:none
*/
static void examine_gl_config_attrib(GdkGLConfig * glconfig)
{
    g_print("\nOpenGL visual configurations :\n\n");
    g_print("gdk_gl_config_is_rgba (glconfig) = %s\n",
	    gdk_gl_config_is_rgba(glconfig) ? "TRUE" : "FALSE");
    g_print("gdk_gl_config_is_double_buffered (glconfig) = %s\n",
	    gdk_gl_config_is_double_buffered(glconfig) ? "TRUE" : "FALSE");
    g_print("gdk_gl_config_is_stereo (glconfig) = %s\n",
	    gdk_gl_config_is_stereo(glconfig) ? "TRUE" : "FALSE");
    g_print("gdk_gl_config_has_alpha (glconfig) = %s\n",
	    gdk_gl_config_has_alpha(glconfig) ? "TRUE" : "FALSE");
    g_print("gdk_gl_config_has_depth_buffer (glconfig) = %s\n",
	    gdk_gl_config_has_depth_buffer(glconfig) ? "TRUE" : "FALSE");
    g_print("gdk_gl_config_has_stencil_buffer (glconfig) = %s\n",
	    gdk_gl_config_has_stencil_buffer(glconfig) ? "TRUE" : "FALSE");
    g_print("gdk_gl_config_has_accum_buffer (glconfig) = %s\n",
	    gdk_gl_config_has_accum_buffer(glconfig) ? "TRUE" : "FALSE");
    g_print("\n");
    print_gl_config_attrib(glconfig, "GDK_GL_USE_GL", GDK_GL_USE_GL, TRUE);
    print_gl_config_attrib(glconfig, "GDK_GL_BUFFER_SIZE",
			   GDK_GL_BUFFER_SIZE, FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_LEVEL", GDK_GL_LEVEL, FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_RGBA", GDK_GL_RGBA, TRUE);
    print_gl_config_attrib(glconfig, "GDK_GL_DOUBLEBUFFER",
			   GDK_GL_DOUBLEBUFFER, TRUE);
    print_gl_config_attrib(glconfig, "GDK_GL_STEREO", GDK_GL_STEREO, TRUE);
    print_gl_config_attrib(glconfig, "GDK_GL_AUX_BUFFERS",
			   GDK_GL_AUX_BUFFERS, FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_RED_SIZE", GDK_GL_RED_SIZE,
			   FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_GREEN_SIZE",
			   GDK_GL_GREEN_SIZE, FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_BLUE_SIZE", GDK_GL_BLUE_SIZE,
			   FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_ALPHA_SIZE",
			   GDK_GL_ALPHA_SIZE, FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_DEPTH_SIZE",
			   GDK_GL_DEPTH_SIZE, FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_STENCIL_SIZE",
			   GDK_GL_STENCIL_SIZE, FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_ACCUM_RED_SIZE",
			   GDK_GL_ACCUM_RED_SIZE, FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_ACCUM_GREEN_SIZE",
			   GDK_GL_ACCUM_GREEN_SIZE, FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_ACCUM_BLUE_SIZE",
			   GDK_GL_ACCUM_BLUE_SIZE, FALSE);
    print_gl_config_attrib(glconfig, "GDK_GL_ACCUM_ALPHA_SIZE",
			   GDK_GL_ACCUM_ALPHA_SIZE, FALSE);
    g_print("\n");
}

/*
	initialize the gl , run only once!!
	params:gtk opgn gl canvas and optional data pointer
	return value:none
*/
static void realize(GtkWidget * widget, gpointer data)
{

    GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

    /*smyrna does not use any ligthting affects but can be turned on for more effects in the future */
    GLfloat ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat diffuse[] = { 0.5, 0.5, 0.5, 1.0 };
    GLfloat position[] = { 0.0, 3.0, 3.0, 0.0 };

    GLfloat lmodel_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat local_view[] = { 0.0 };

    /* static char *smyrna_font; */


	/*** OpenGL BEGIN ***/
    if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
	return;

    glClearColor(view->bgColor.R, view->bgColor.G, view->bgColor.B, view->bgColor.A);	//background color
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT);


    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, local_view);

    glFrontFace(GL_CW);
// glEnable (GL_LIGHTING);
// glEnable (GL_LIGHT0);
//  glEnable (GL_AUTO_NORMAL);
//  glEnable (GL_NORMALIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glDepthFunc(GL_LESS);
//  glEnable(GL_LINE_SMOOTH);
    gdk_gl_drawable_gl_end(gldrawable);


  /*** OpenGL END ***/
    return;
}

/*
	set up GL for window size changes, run this when necesary (when window size or monitor resolution is changed)
	params:gtk opgn gl canvas , GdkEventConfigure object to retrieve window dimensions and custom data
	return value:true or false, fails (false) if cannot init gl
*/
static gboolean configure_event(GtkWidget * widget,
				GdkEventConfigure * event, gpointer data)
{
    /* static int doonce=0; */
    int vPort[4];
    float aspect;
    GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);
    view->w = widget->allocation.width;
    view->h = widget->allocation.height;
    if (view->widgets)
	glcompsetUpdateBorder(view->widgets, view->w, view->h);


	/*** OpenGL BEGIN ***/
    if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
	return FALSE;
    glViewport(0, 0, view->w, view->h);
    /* get current viewport */
    glGetIntegerv(GL_VIEWPORT, vPort);
    /* setup various opengl things that we need */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (widget->allocation.width > 1)
	init_arcBall(view->arcball, (GLfloat) view->w, (GLfloat) view->h);

    if (view->w > view->h) {
	aspect = (float) view->w / (float) view->h;
	glOrtho(-aspect * GL_VIEWPORT_FACTOR, aspect * GL_VIEWPORT_FACTOR,
		GL_VIEWPORT_FACTOR * -1, GL_VIEWPORT_FACTOR, -1500, 1500);
    } else {
	aspect = (float) view->h / (float) view->w;
	glOrtho(GL_VIEWPORT_FACTOR * -1, GL_VIEWPORT_FACTOR,
		-aspect * GL_VIEWPORT_FACTOR, aspect * GL_VIEWPORT_FACTOR,
		-1500, 1500);
    }

    glMatrixMode(GL_MODELVIEW);
    gdk_gl_drawable_gl_end(gldrawable);
	/*** OpenGL END ***/


    return TRUE;
}

/*
	expose function.real drawing takes place here, 
	params:gtk opgn gl canvas , GdkEventExpose object and custom data
	return value:true or false, fails (false) if cannot init gl
*/
gboolean expose_event(GtkWidget * widget, GdkEventExpose * event,
		      gpointer data)
{
    GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

	/*** OpenGL BEGIN ***/
    if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
	return FALSE;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glexpose_main(view);	//draw all stuff
    /* Swap buffers */
    if (gdk_gl_drawable_is_double_buffered(gldrawable))
	gdk_gl_drawable_swap_buffers(gldrawable);
    else
	glFlush();
    gdk_gl_drawable_gl_end(gldrawable);
  /*** OpenGL END ***/
    if (view->initFile) {
	view->initFile = 0;
	if (view->activeGraph == 0)
	    close_graph(view, 0);
	add_graph_to_viewport_from_file(view->initFileName);
    }
    return TRUE;
}

/*
	when a mouse button is clicked this function is called
	params:gtk opgn gl canvas , GdkEventButton object and custom data
	return value:true or false, fails (false) if cannot init gl
*/
static gboolean button_press_event(GtkWidget * widget,
				   GdkEventButton * event, gpointer data)
{
    Agraph_t* g;

    if (view->g == 0) return FALSE;
    g=view->g[view->activeGraph];

    begin_x = (float) event->x;
    begin_y = (float) event->y;
    view->widgets->common.functions.mousedown((glCompObj*)view->widgets,(GLfloat) event->x,(GLfloat) event->y,getGlCompMouseType(event->button));
    if (event->button == 1)	//left click
	appmouse_left_click_down(view,(int) event->x,(int) event->y);

    if (event->button == 3)	//right click
	appmouse_right_click_down(view,(int) event->x,(int) event->y);
    if (event->button == 2)	//middle click
	appmouse_middle_click_down(view,(int) event->x,(int) event->y);

    expose_event(view->drawing_area, NULL, NULL);

    return FALSE;
}

/*
	when a mouse button is released(always after click) this function is called
	params:gtk opgn gl canvas , GdkEventButton object and custom data
	return value:true or false, fails (false) if cannot init gl
*/
static gboolean button_release_event(GtkWidget * widget,
				     GdkEventButton * event, gpointer data)
{
    if (view->widgets == 0) return FALSE;
    view->FontSizeConst = GetOGLDistance(14);
    view->arcball->isDragging = 0;
    view->widgets->common.functions.mouseup((glCompObj*)view->widgets,(GLfloat) event->x,(GLfloat) event->y,getGlCompMouseType(event->button));


    if (event->button == 1)	//left click release
	appmouse_left_click_up(view,(int) event->x,(int) event->y);
    if (event->button == 3)	//right click
	appmouse_right_click_up(view,(int) event->x,(int) event->y);
    if (event->button == 2)	//right click
	appmouse_middle_click_up(view,(int) event->x,(int) event->y);

    expose_event(view->drawing_area, NULL, NULL);
    dx = 0.0;
    dy = 0.0;

    return FALSE;
}
static gboolean key_press_event(GtkWidget * widget, GdkEventKey * event, gpointer data)
{
    appmouse_key_press(view,event->keyval);
    return FALSE;




}
static gboolean key_release_event(GtkWidget * widget, GdkEventKey * event, gpointer data)
{
    appmouse_key_release(view,event->keyval);
    return FALSE;

}


static gboolean
scroll_event(GtkWidget * widget, GdkEventScroll * event, gpointer data)
{
    gdouble seconds;

    seconds = g_timer_elapsed(view->timer2, NULL);
    if (seconds > 0.005) {
	g_timer_stop(view->timer2);
	if (event->direction == 0)
	    view->mouse.dragX = -30;
	if (event->direction == 1)
	    view->mouse.dragX = +30;
	glmotion_zoom(view);
	glexpose();
	g_timer_start(view->timer2);

    }
    return TRUE;
}

/*
	when  mouse is moved over glcanvas this function is called
	params:gtk opgn gl canvas , GdkEventMotion object and custom data
	return value:always TRUE !!!
*/
static gboolean motion_notify_event(GtkWidget * widget,
				    GdkEventMotion * event, gpointer data)
{
    float x = (float) event->x;
    float y = (float) event->y;

    gboolean redraw = FALSE;
    if (view->widgets)
	view->widgets->common.functions.mouseover((glCompObj*)view->widgets, (GLfloat) x,(GLfloat) y);

    dx = x - begin_x;
    dy = y - begin_y;
    view->mouse.dragX = dx;
    view->mouse.dragY = dy;
    appmouse_move(view,(int)event->x,(int)event->y);

    if((view->mouse.t==glMouseLeftButton) && (view->mouse.down)  )
    {
	appmouse_left_drag(view,(int)event->x,(int)event->y);
	redraw = TRUE;

    }
    if((view->mouse.t==glMouseRightButton) && (view->mouse.down))
    {
	appmouse_right_drag(view,(int)event->x,(int)event->y);
	redraw = TRUE;
    }
    if((view->mouse.t==glMouseMiddleButton) && (view->mouse.down))
    {
	appmouse_middle_drag(view,(int)event->x,(int)event->y);
	redraw = TRUE;
    }
    if(view->Topview->sel.selPoly.cnt > 0)
	redraw=TRUE;




    begin_x = x;
    begin_y = y;


    if (redraw)
	gdk_window_invalidate_rect(widget->window, &widget->allocation,FALSE);
    return TRUE;
}

/*
	when a key is pressed this function is called
	params:gtk opgn gl canvas , GdkEventKey(to retrieve which key is pressed) object and custom data
	return value:true or false, fails (false) if listed keys (in switch) are not pressed
*/
#ifdef UNUSED
static gboolean key_press_event(GtkWidget * widget, GdkEventKey * event,
				gpointer data)
{
    switch (event->keyval) {
    case GDK_Escape:
	gtk_main_quit();
	break;
    default:
	return FALSE;
    }
    return TRUE;
}
#endif


/*
	call back for mouse right click, this function activates the gtk right click pop up menu
	params:widget to shop popup , event handler to check click type and custom data
	return value:true or false, fails (false) if listed keys (in switch) are not pressed
*/
#ifdef UNUSED
static gboolean button_press_event_popup_menu(GtkWidget * widget,
					      GdkEventButton * event,
					      gpointer data)
{
    if (event->button == 3) {
	/* Popup menu. */
	gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL,
		       event->button, event->time);
	return TRUE;
    }
    return FALSE;
}
#endif


/*
	configures various opengl settings
	params:none
	return value:GdkGLConfig object
*/
GdkGLConfig *configure_gl(void)
{
    GdkGLConfig *glconfig;
    /* Try double-buffered visual */
    glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB |
					 GDK_GL_MODE_DEPTH |
					 GDK_GL_MODE_DOUBLE);
    if (glconfig == NULL) {
	g_print("\n*** Cannot find the double-buffered visual.\n");
	g_print("\n*** Trying single-buffered visual.\n");

	/* Try single-buffered visual */
	glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB |
					     GDK_GL_MODE_DEPTH);
	if (glconfig == NULL) {
	    g_print("*** No appropriate OpenGL-capable visual found.\n");
	    exit(1);
	}
    }

    return glconfig;
}

/*
	run this function only once to create and customize gtk based opengl canvas
	all signal, socket connections are done here for opengl interractions
	params:gl config object, gtk container widget for opengl canvas
	return value:none
*/
void create_window(GdkGLConfig * glconfig, GtkWidget * vbox)
{
    gint major, minor;

    /*
     * Query OpenGL extension version.
     */

    gdk_gl_query_version(&major, &minor);

    /* Try double-buffered visual */

    if (IS_TEST_MODE_ON)	//printf some gl values, to test if your system has opengl stuff
	examine_gl_config_attrib(glconfig);
    /* Drawing area for drawing OpenGL scene. */
    view->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(view->drawing_area, 300, 300);
    /* Set OpenGL-capability to the widget. */
    gtk_widget_set_gl_capability(view->drawing_area,
				 glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);

    gtk_widget_add_events(view->drawing_area,
//  GDK_BUTTON_MOTION_MASK      = 1 << 4,
			  GDK_BUTTON_MOTION_MASK |
			  GDK_POINTER_MOTION_MASK |
			  GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_SCROLL | GDK_VISIBILITY_NOTIFY_MASK |  GDK_KEY_PRESS_MASK	| GDK_KEY_RELEASE_MASK	);

    g_signal_connect_after(G_OBJECT(view->drawing_area), "realize",
			   G_CALLBACK(realize), NULL);
    g_signal_connect(G_OBJECT(view->drawing_area), "configure_event",
		     G_CALLBACK(configure_event), NULL);
    g_signal_connect(G_OBJECT(view->drawing_area), "expose_event",
		     G_CALLBACK(expose_event), NULL);

    g_signal_connect(G_OBJECT(view->drawing_area), "button_press_event",
		     G_CALLBACK(button_press_event), NULL);
/*    g_signal_connect(G_OBJECT(view->drawing_area), "2button_press_event",
		     G_CALLBACK(button_press_event), NULL);*/

    g_signal_connect(G_OBJECT(view->drawing_area), "button_release_event",G_CALLBACK(button_release_event), NULL);
    g_signal_connect(G_OBJECT(view->drawing_area), "key_release_event", G_CALLBACK(key_release_event), NULL);
    g_signal_connect(G_OBJECT(view->drawing_area), "key_press_event", G_CALLBACK(key_press_event), NULL);
    g_signal_connect(G_OBJECT(view->drawing_area), "scroll_event",
		     G_CALLBACK(scroll_event), NULL);

    g_signal_connect(G_OBJECT(view->drawing_area), "motion_notify_event",
		     G_CALLBACK(motion_notify_event), NULL);


//gtk_accel_group_connect (GTK_ACCEL_GROUP (mainw->accel_group), GDK_Page_Up, GDK_CONTROL_MASK, 0, g_cclosure_new (G_CALLBACK (prevclip_callback),NULL,NULL));


    gtk_box_pack_start(GTK_BOX(vbox), view->drawing_area, TRUE, TRUE, 0);

    gtk_widget_show(view->drawing_area);



    gtk_widget_add_events(glade_xml_get_widget(xml, "frmMain"),
			  GDK_BUTTON_MOTION_MASK |
			  GDK_POINTER_MOTION_MASK |
			  GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_SCROLL | GDK_VISIBILITY_NOTIFY_MASK |  GDK_KEY_PRESS_MASK	| GDK_KEY_RELEASE_MASK	);


    g_signal_connect(G_OBJECT(glade_xml_get_widget(xml, "frmMain")), "key_release_event", G_CALLBACK(key_release_event), NULL);
    g_signal_connect(G_OBJECT(glade_xml_get_widget(xml, "frmMain")), "key_press_event", G_CALLBACK(key_press_event), NULL);


    /* Popup menu. */

#ifdef UNUSED
    menu = create_popup_menu(view->drawing_area);

    /* Signal handler */
    g_signal_connect_swapped(G_OBJECT(view->drawing_area),
			     "button_press_event",
			     G_CALLBACK(button_press_event_popup_menu),
			     menu);
#endif

}
