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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gtk/gtk.h>

#include "gvplugin_device.h"

#include "callbacks.h"
#include "interface.h"
#include "support.h"

void
on_new1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWindow *window1;
    GVJ_t *job;

    window1 = GTK_WINDOW(menuitem);
    job = g_object_get_data(G_OBJECT(window1), "job");

    (job->callbacks->read)(job, NULL, "dot");

    // should there be specific menus for (un)directed graphs etc?
    //  - I think the directed flag only affects layout and rendering
    //      so I plan to make it look like a graph attribute.
    //      Similarly "strict".
}

static void
ui_open_graph(GtkWindow *window1, gchar *filename)
{
    GVJ_t *job;
    GtkWidget *dialog;

    job = g_object_get_data(G_OBJECT(window1), "job");
    dialog = gtk_file_chooser_dialog_new(
		"Open graph", window1, GTK_FILE_CHOOSER_ACTION_OPEN,
		"Cancel", GTK_RESPONSE_CANCEL,
		"Open", GTK_RESPONSE_ACCEPT,
		NULL);
    if (filename)
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), filename);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_widget_destroy(dialog);
    if (filename) {
    	(job->callbacks->read)(job, filename, "dot");
//	if (!file) // we'll probably want to create a error dialog function
//	    fprintf(stderr, "Could not open file: %s\n", filename);
//	else
	    g_object_set_data_full(G_OBJECT(window1),
		    "activefilename", filename, (GDestroyNotify)g_free);
    }
}

void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWindow *window1;
    gchar *filename;

    window1 = GTK_WINDOW(menuitem);
    filename = g_object_get_data(G_OBJECT(window1), "activefilename");
    ui_open_graph(window1, filename);
}

static void
ui_save_graph(GtkWindow *window1, gchar *filename)
{
    GVJ_t *job;
    GtkWidget *dialog;

    job = (GVJ_t *)g_object_get_data(G_OBJECT(window1), "job");

    dialog = gtk_file_chooser_dialog_new(
		"Save graph as", window1, GTK_FILE_CHOOSER_ACTION_SAVE,
		"Cancel", GTK_RESPONSE_CANCEL,
		"Save", GTK_RESPONSE_ACCEPT,
		NULL);
    filename = g_object_get_data(G_OBJECT(window1), "activefilename");
    if (filename)
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), filename);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_widget_destroy(dialog);
    if (filename) {
	(job->callbacks->render)(job, "dot", filename);
	g_object_set_data_full(G_OBJECT(window1),
		"activefilename", filename, (GDestroyNotify)g_free);
    }
}

void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWindow *window1;
    gchar *filename;

    window1 = GTK_WINDOW(menuitem);
    filename = (gchar *)g_object_get_data(G_OBJECT(window1), "activefilename");
    ui_save_graph(window1, filename);
}


void
on_save_as1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWindow *window1;
       
    window1 = GTK_WINDOW(menuitem);
    ui_save_graph(window1, NULL);
}


void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtk_widget_destroy(GTK_WIDGET(gtk_widget_get_toplevel(GTK_WIDGET(menuitem))));
    gtk_main_quit();
}


void
on_cut1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    // I am thinking that we will annotate a node as to whether it is selected,
    // then retrieve a list of selected nodes for these operations
}


void
on_copy1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_paste1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_delete1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    static gchar *authors[] = {
		"John Ellson", 
		"Emden Gansner",
		"Stephen North",
		"special thanks to Michael Lawrence",
		NULL };
    GtkWindow *window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(menuitem)));
    gtk_show_about_dialog(window,
		"name", "DotEdit",
		"version", "0.1",
		"copyright", "(C) 2005 AT&T Corp.",
		"license", "Common Public License, Version 1.0.",
		"website", "http://www.graphviz.org",
		"comments", "Visualize and edit graphs of nodes and edges",
		"authors", authors,
		NULL);
}

static void
load_store_with_attrs(GtkListStore *model, GVJ_t *job)
{
#if 0
        gint attrs_len = job->selected_obj_attributes.argc, i;
        gchar **attrs = job->selected_obj_attributes.argv;
        GtkTreeIter iter;
        gvattr_t type;
#endif

        gtk_list_store_clear(model);

#if 0
        for (i = 0; i < attrs_len; i+=3) {
                gtk_list_store_append(model, &iter);
                gtk_list_store_set(model, &iter, 0, attrs[i], 1, g_strdup(attrs[i+1]), -1);
                type = (gvattr_t)attrs[i+2];
        }
#endif
}


gboolean
on_drawingarea1_expose_event           (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    GVJ_t *job;
    cairo_t *cr;

    job = (GVJ_t *)g_object_get_data(G_OBJECT(widget),"job");
    cr = gdk_cairo_create(widget->window);

    (job->callbacks->motion)(job, job->pointer);

    job->context = (void *)cr;
    job->external_context = TRUE;
    job->width = widget->allocation.width;
    job->height = widget->allocation.height;
    if (job->has_been_rendered) {
    	(job->callbacks->refresh)(job);
    }
    else {
	(job->callbacks->refresh)(job);
	
// FIXME - copy image to keyhole
//      the keyhole image is a fixed size and doesn;t need to be recomputed 
//      each time.   save a pixmap, then each time, show pixmap and overlay
//      with scaled view rectangle.

    }
    cairo_destroy(cr);

    load_store_with_attrs(GTK_LIST_STORE(g_object_get_data(G_OBJECT(widget), "attr_store")), job);

    return FALSE;
}


gboolean
on_drawingarea1_motion_notify_event    (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    GVJ_t *job;

    job = (GVJ_t *)g_object_get_data(G_OBJECT(widget),"job");
    job->pointer.x = event->x;
    job->pointer.y = event->y;
    gtk_widget_queue_draw(widget);

#if 0
    if (job->active_tooltip && job->active_tooltip[0])
	fprintf(stderr,"tooltip = \"%s\"\n", job->active_tooltip);
#endif

    return FALSE;
}


gboolean
on_drawingarea2_motion_notify_event    (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{

  return FALSE;
}



gboolean
on_drawingarea2_expose_event           (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    GVJ_t *job;
    cairo_t *cr;
    double tmp;

    job = (GVJ_t *)g_object_get_data(G_OBJECT(widget),"job");
    cr = gdk_cairo_create(widget->window);

    (job->callbacks->motion)(job, job->pointer);

    job->context = (void *)cr;
    job->external_context = TRUE;
    job->width = widget->allocation.width;
    job->height = widget->allocation.height;

    tmp = job->zoom;
    job->zoom = MIN(job->width * POINTS_PER_INCH / (job->bb.UR.x * job->dpi.x),
                    job->height * POINTS_PER_INCH / (job->bb.UR.y * job->dpi.y));
    (job->callbacks->refresh)(job);
    job->zoom = tmp;

    cairo_destroy(cr);

    return FALSE;
}

gboolean
on_window1_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    gtk_main_quit();
    return FALSE;
}


gboolean
on_drawingarea1_configure_event        (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    GVJ_t *job;
    double zoom_to_fit;

/*FIXME - should allow for margins */
/*      - similar zoom_to_fit code exists in: */
/*      plugin/gtk/callbacks.c */
/*      plugin/xlib/gvdevice_xlib.c */
/*      lib/gvc/gvevent.c */

    job = (GVJ_t *)g_object_get_data(G_OBJECT(widget),"job");
    if (! job->has_been_rendered) {
	zoom_to_fit = MIN((double) event->width / (double) job->width,
			  (double) event->height / (double) job->height);
        if (zoom_to_fit < 1.0) /* don't make bigger */
	    job->zoom *= zoom_to_fit;
    }
    else if (job->fit_mode) {
	zoom_to_fit = MIN((double) event->width / (double) job->width,
			  (double) event->height / (double) job->height);
	job->zoom *= zoom_to_fit;
    }
    if (event->width > job->width || event->height > job->height)
	job->has_grown = TRUE;
    job->width = event->width;
    job->height = event->height;
    job->needs_refresh = TRUE;

    return FALSE;
}


gboolean
on_drawingarea1_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    GVJ_t *job;
    pointf pointer;

    job = (GVJ_t *)g_object_get_data(G_OBJECT(widget),"job");
    pointer.x = event->x;
    pointer.y = event->y;
    (job->callbacks->button_press)(job, event->button, pointer);
    
    load_store_with_attrs(GTK_LIST_STORE(g_object_get_data(G_OBJECT(widget), "attr_store")), job);
    return FALSE;
}


gboolean
on_drawingarea1_button_release_event   (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    GVJ_t *job;
    pointf pointer;

    job = (GVJ_t *)g_object_get_data(G_OBJECT(widget),"job");
    pointer.x = event->x;
    pointer.y = event->y;
    (job->callbacks->button_release)(job, event->button, pointer);

    return FALSE;
}


gboolean
on_drawingarea1_scroll_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GVJ_t *job;
    pointf pointer;

    job = (GVJ_t *)g_object_get_data(G_OBJECT(widget),"job");
    pointer.x = ((GdkEventScroll *)event)->x;
    pointer.y = ((GdkEventScroll *)event)->y;
    switch (((GdkEventScroll *)event)->direction) {
	case GDK_SCROLL_UP:
	    (job->callbacks->button_press)(job, 4, pointer);
	    break;
	case GDK_SCROLL_DOWN:
	    (job->callbacks->button_press)(job, 5, pointer);
	    break;
	case GDK_SCROLL_LEFT:
	case GDK_SCROLL_RIGHT:
	    break;
    }
    gtk_widget_queue_draw(widget);

    return FALSE;
}

gboolean
on_button1_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{


fprintf(stderr, "will delete selected object\n");

  return FALSE;
}

