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

#ifndef TOPVIEWSETTINGS_H
#define TOPVIEWSETTINGS_H

#include "smyrnadefs.h"

#ifdef __cplusplus
extern "C" {
#endif

    _BB void on_settingsOKBtn_clicked(GtkWidget * widget,
				      gpointer user_data);
    _BB void on_settingsCancelBtn_clicked(GtkWidget * widget,
					  gpointer user_data);
    _BB void on_settingsApplyBtn_clicked(GtkWidget * widget,
					 gpointer user_data);
    _BB void color_change_request(GtkWidget * widget, gpointer user_data);
    _BB void size_change_request(GtkWidget * widget, gpointer user_data);
    _BB void on_dlgSettings_close (GtkWidget * widget, gpointer user_data);
    extern int load_settings_from_graph(Agraph_t * g);
    extern int update_graph_from_settings(Agraph_t * g);
    extern int show_settings_form(void);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
