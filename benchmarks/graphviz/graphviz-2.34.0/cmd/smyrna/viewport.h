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

//view data structure
#ifndef VIEWPORT_H
#define VIEWPORT_H
#define bool int
#include "smyrnadefs.h"
#include <gtk/gtk.h>
#include "xdot.h"
#include "cgraph.h"

#ifdef __cplusplus
extern "C" {
#endif

    void init_viewport(ViewInfo * view);
    void set_viewport_settings_from_template(ViewInfo * view, Agraph_t *);
#if 0
    void clear_viewport(ViewInfo * view);
    colorschemaset *create_color_theme(int themeid);
    md5_byte_t *get_md5_key(Agraph_t * graph);
    void fill_key(md5_byte_t * b, md5_byte_t * data);
    void movenode(void *n, float dx, float dy);
    void please_dont_wait(void);
    void please_wait(void);
#endif
    void switch_graph(int);
    void refreshViewport(int doClear);
    int add_graph_to_viewport_from_file(char *fileName);
    int add_graph_to_viewport(Agraph_t * graph, char *);
    int close_graph(ViewInfo * view, int graphid);
    int save_graph(void);
    int save_graph_with_file_name(Agraph_t * graph, char *fileName);
    int save_as_graph(void);

    int do_graph_layout(Agraph_t * graph, int Engine, int keeppos);

    void glexpose(void);
    void move_nodes(Agraph_t * g);
    extern void getcolorfromschema(colorschemaset * sc, float l,
				   float maxl, glCompColor * c);
    void updateRecord (Agraph_t* g);


    /* helper functions */
    extern int setGdkColor(GdkColor * c, char *color);
    extern char *get_attribute_value(char *attr, ViewInfo * view,
				     Agraph_t * g);

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
