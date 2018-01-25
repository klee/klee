/* $Id: gvrender_visio_vdx.cpp,v 1.6 2011/01/25 16:30:51 ellson Exp $ $Revision: 1.6 $ */
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "gvplugin_render.h"
#include "gvplugin_device.h"

#include "VisioRender.h"

typedef enum { FORMAT_VDX } format_type;

static void vdxgen_begin_job(GVJ_t * job)
{
   	job->context = new Visio::Render();
}

static void vdxgen_end_job(GVJ_t* job)
{
	delete (Visio::Render*)job->context;
}

static void vdxgen_begin_graph(GVJ_t * job)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->BeginGraph(job);
}

static void vdxgen_end_graph(GVJ_t * job)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->EndGraph(job);
}

static void vdxgen_begin_page(GVJ_t * job)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->BeginPage(job);
	
}

static void vdxgen_end_page(GVJ_t * job)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->EndPage(job);	
}

static void vdxgen_begin_node(GVJ_t * job)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->BeginNode(job);	
}

static void vdxgen_end_node(GVJ_t * job)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->EndNode(job);
}

static void
vdxgen_begin_edge(GVJ_t * job)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->BeginEdge(job);	
}

static void vdxgen_end_edge(GVJ_t * job)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->EndEdge(job);
}

static void vdxgen_begin_anchor(GVJ_t *job, char *url, char *tooltip, char *target, char *id)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->AddAnchor(job, url, tooltip, target, id);
}

static void vdxgen_textpara(GVJ_t * job, pointf p, textpara_t * para)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->AddText(job, p, para);	
}

static void vdxgen_ellipse(GVJ_t * job, pointf * A, int filled)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->AddEllipse(job, A, filled);
}

static void vdxgen_bezier(GVJ_t * job, pointf * A, int n, int arrow_at_start, int arrow_at_end, int filled)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->AddBezier(job, A, n, arrow_at_start, arrow_at_end, filled);
}

static void vdxgen_polygon(GVJ_t * job, pointf * A, int n, int filled)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->AddPolygon(job, A, n, filled);
}

static void vdxgen_polyline(GVJ_t * job, pointf * A, int n)
{
	Visio::Render* context = (Visio::Render*)job->context;
	if (context)
		context->AddPolyline(job, A, n);
}

gvrender_engine_t vdxgen_engine = {
    vdxgen_begin_job,
    vdxgen_end_job,
    vdxgen_begin_graph,
    vdxgen_end_graph,
    0,				/* vdxgen_begin_layer */
    0,				/* vdxgen_end_layer */
    vdxgen_begin_page,
    vdxgen_end_page,
    0,				/* vdxgen_begin_cluster */
    0,				/* vdxgen_end_cluster */
    0,				/* vdxgen_begin_nodes */
    0,				/* vdxgen_end_nodes */
    0,				/* vdxgen_begin_edges */
    0,				/* vdxgen_end_edges */
    vdxgen_begin_node,
    vdxgen_end_node,
    vdxgen_begin_edge,
    vdxgen_end_edge,
	vdxgen_begin_anchor,
    0,				/* vdxgen_end_anchor */
    0,				/* vdxgen_begin_label */
	0,				/* vdxgen_end_label */
    vdxgen_textpara,
    0,				/* vdxgen_resolve_color */
    vdxgen_ellipse,
    vdxgen_polygon,
    vdxgen_bezier,
    vdxgen_polyline,
    0,				/* vdxgen_comment */
    0,				/* vdxgen_library_shape */
};

gvrender_features_t render_features_vdx = {
    GVRENDER_DOES_ARROWS
		| GVRENDER_DOES_MAPS
		| GVRENDER_DOES_TARGETS
		| GVRENDER_DOES_TOOLTIPS
		| GVRENDER_NO_WHITE_BG,
	4.,          /* default pad - graph units */
    NULL,		/* knowncolors */
    0,			/* sizeof knowncolors */
    RGBA_BYTE,			/* color_type */
};

gvdevice_features_t device_features_vdx = {
    GVDEVICE_DOES_TRUECOLOR,	/* flags */
    {36.,36.},			/* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {72.,72.},			/* default dpi */
};

extern "C"
{
gvplugin_installed_t gvrender_vdx_types[] = {
    {0, "visio", 1, &vdxgen_engine, &render_features_vdx},
    {0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_vdx_types[] = {
    {FORMAT_VDX, "vdx:visio", 1, NULL, &device_features_vdx},
    {0, NULL, 0, NULL, NULL}
};
}
