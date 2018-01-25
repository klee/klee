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

#ifndef VIEWPORTCAMERA_H
#define VIEWPORTCAMERA_H

#include "smyrnadefs.h"
#include "glcompset.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
    void set_camera_x_y(viewport_camera * c);
    int delete_camera_from_viewport(ViewInfo * view, viewport_camera * c);
    viewport_camera *add_camera_to_viewport(ViewInfo * view);
    int activate_viewport_camera(ViewInfo * view, int cam_index);
    _BB void dlgcameracancelbutton_clicked_cb(GtkWidget * widget,
					      gpointer user_data);
    _BB void dlgcameraokbutton_clicked_cb(GtkWidget * widget,
					  gpointer user_data);
    int refresh_viewport_camera(ViewInfo * view);
    int save_camera_settings(viewport_camera * c);
    int show_camera_settings(viewport_camera * c);
#endif
    extern void attach_camera_widget(ViewInfo * view);
    extern void menu_click_add_camera(void *p);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
