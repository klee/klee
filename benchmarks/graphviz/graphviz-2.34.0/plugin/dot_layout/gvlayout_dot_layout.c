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
#include "config.h"
#endif

#include "gvplugin_layout.h"

typedef enum { LAYOUT_DOT, } layout_type;

#ifdef WIN32 /*dependencies*/
    #pragma comment( lib, "gvc.lib" )
    #pragma comment( lib, "ingraphs.lib" )
    #pragma comment( lib, "cdt.lib" )
    #pragma comment( lib, "gvortho.lib" )

#endif


#ifdef WITH_CGRAPH
    #pragma comment( lib, "cgraph.lib" )
    #pragma comment( lib, "dotgen.lib" )
    extern void dot_layout(graph_t * g);
    extern void dot_cleanup(graph_t * g);

    gvlayout_engine_t dotgen_engine = {
    dot_layout,
    dot_cleanup,
    };


#else
    #pragma comment( lib, "graph.lib" )
    #pragma comment( lib, "dotgen.lib" )
    extern void dot_layout(graph_t * g);
    extern void dot_cleanup(graph_t * g);

    gvlayout_engine_t dotgen_engine = {
    dot_layout,
    dot_cleanup,
    };

#endif





gvlayout_features_t dotgen_features = {
    LAYOUT_USES_RANKDIR,
};

gvplugin_installed_t gvlayout_dot_layout[] = {
    {LAYOUT_DOT, "dot", 0, &dotgen_engine, &dotgen_features},
    {0, NULL, 0, NULL, NULL}
};
