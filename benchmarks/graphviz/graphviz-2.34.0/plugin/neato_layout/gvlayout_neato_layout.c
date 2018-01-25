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
 *  neato layout plugin
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "gvplugin_layout.h"

/* FIXME - globals.h is needed for Nop */
#include "globals.h"


#ifdef WIN32 //*dependencies
#ifdef WITH_CGRAPH
    #pragma comment( lib, "cgraph.lib" )
#else
    #pragma comment( lib, "graph.lib" )
#endif
    #pragma comment( lib, "gvc.lib" )
    #pragma comment( lib, "pathplan.lib" )
    #pragma comment( lib, "neatogen.lib" )
    #pragma comment( lib, "circogen.lib" )
    #pragma comment( lib, "twopigen.lib" )
    #pragma comment( lib, "fdpgen.lib" )
    #pragma comment( lib, "sparse.lib" )
    #pragma comment( lib, "cdt.lib" )
    #pragma comment( lib, "gts.lib" )
    #pragma comment( lib, "glib-2.0.lib" )
    #pragma comment( lib, "vpsc.lib" )
    #pragma comment( lib, "patchwork.lib" )
    #pragma comment( lib, "gvortho.lib" )
#endif


typedef enum { LAYOUT_NEATO,
		LAYOUT_FDP,
		LAYOUT_SFDP,
		LAYOUT_TWOPI,
		LAYOUT_CIRCO,
		LAYOUT_PATCHWORK,
		LAYOUT_CLUSTER,
		LAYOUT_NOP1,
		LAYOUT_NOP2,
	} layout_type;

extern void neato_layout(graph_t * g);
extern void fdp_layout(graph_t * g);
extern void sfdp_layout(graph_t * g);
extern void twopi_layout(graph_t * g);
extern void circo_layout(graph_t * g);
extern void patchwork_layout(graph_t * g);
extern void osage_layout(graph_t * g);

extern void neato_cleanup(graph_t * g);
extern void fdp_cleanup(graph_t * g);
extern void sfdp_cleanup(graph_t * g);
extern void twopi_cleanup(graph_t * g);
extern void circo_cleanup(graph_t * g);
extern void patchwork_cleanup(graph_t * g);
extern void osage_cleanup(graph_t * g);

static void nop1_layout(graph_t * g)
{
    Nop = 1;
    neato_layout(g);
    Nop = 0;
}

static void nop2_layout(graph_t * g)
{
    Nop = 2;
    neato_layout(g);
    Nop = 0;
}

gvlayout_engine_t neatogen_engine = {
    neato_layout,
    neato_cleanup,
};

gvlayout_engine_t fdpgen_engine = {
    fdp_layout,
    fdp_cleanup,
};

#ifdef SFDP
gvlayout_engine_t sfdpgen_engine = {
    sfdp_layout,
    sfdp_cleanup,
};
#endif

gvlayout_engine_t twopigen_engine = {
    twopi_layout,
    twopi_cleanup,
};

gvlayout_engine_t circogen_engine = {
    circo_layout,
    circo_cleanup,
};

gvlayout_engine_t nop1gen_engine = {
    nop1_layout,
    neato_cleanup,
};

gvlayout_engine_t nop2gen_engine = {
    nop2_layout,
    neato_cleanup,
};

gvlayout_engine_t patchwork_engine = {
    patchwork_layout,
    patchwork_cleanup,
};

gvlayout_engine_t osage_engine = {
    osage_layout,
    osage_cleanup,
};

gvlayout_features_t neatogen_features = {
        0,
};

gvplugin_installed_t gvlayout_neato_types[] = {
    {LAYOUT_NEATO, "neato", 0, &neatogen_engine, &neatogen_features},
    {LAYOUT_FDP, "fdp", 0, &fdpgen_engine, &neatogen_features},
#ifdef SFDP
    {LAYOUT_SFDP, "sfdp", 0, &sfdpgen_engine, &neatogen_features},
#endif
    {LAYOUT_TWOPI, "twopi", 0, &twopigen_engine, &neatogen_features},
    {LAYOUT_CIRCO, "circo", 0, &circogen_engine, &neatogen_features},
    {LAYOUT_PATCHWORK, "patchwork", 0, &patchwork_engine, &neatogen_features},
    {LAYOUT_CLUSTER, "osage", 0, &osage_engine, &neatogen_features},
    {LAYOUT_NOP1, "nop", 0, &nop1gen_engine, &neatogen_features},
    {LAYOUT_NOP1, "nop1", 0, &nop1gen_engine, &neatogen_features},
    {LAYOUT_NOP1, "nop2", 0, &nop2gen_engine, &neatogen_features},
    {0, NULL, 0, NULL, NULL}
};
