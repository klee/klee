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

/*
	this code is used to set up a opngl window and set
	some basic features (panning zooming and rotating)
	Viewport.h provides a higher level control such as drawing primitives
*/
#ifndef GL_TEMPLATE_H
#define GL_TEMPLATE_H

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkcursor.h>

#ifdef __cplusplus
extern "C" {
#endif

#if 0
    void examine_gl_config_attrib(GdkGLConfig * glconfig);
    void print_gl_config_attrib(GdkGLConfig * glconfig,
				const gchar * attrib_str, int attrib,
				gboolean is_boolean);
void realize (GtkWidget *widget,gpointer   data);
gboolean configure_event (GtkWidget *widget,GdkEventConfigure *event, gpointer data);
gboolean button_press_event (GtkWidget *widget,GdkEventButton *event,gpointer data);
gboolean button_release_event (GtkWidget *widget,GdkEventButton *event,gpointer data);
gboolean motion_notify_event (GtkWidget *widget,GdkEventMotion *event,gpointer data);
gboolean key_press_event (GtkWidget *widget, GdkEventKey *event,gpointer data);
gboolean button_press_event_popup_menu (GtkWidget *widget,GdkEventButton *event,gpointer data);
    void switch_Mouse(GtkMenuItem * menuitem, int mouse_mode);
#endif

    gboolean expose_event(GtkWidget * widget, GdkEventExpose * event,
			  gpointer data);
    extern GdkGLConfig *configure_gl(void);
    void create_window(GdkGLConfig * glconfig, GtkWidget * vbox);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
