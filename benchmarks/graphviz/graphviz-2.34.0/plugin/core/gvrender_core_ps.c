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

#include <stdlib.h>
#include <string.h>

#include "gvplugin_render.h"
#include "gvplugin_device.h"
#include "gvio.h"
#include "agxbuf.h"
#include "utils.h"
#include "ps.h"

/* for CHAR_LATIN1  */
#include "const.h"

/*
 *     J$: added `pdfmark' URL embedding.  PostScript rendered from
 *         dot files with URL attributes will get active PDF links
 *         from Adobe's Distiller.
 */
#define PDFMAX  14400           /*  Maximum size of PDF page  */

typedef enum { FORMAT_PS, FORMAT_PS2, FORMAT_EPS } format_type;

static int isLatin1;
static char setupLatin1;

static void psgen_begin_job(GVJ_t * job)
{
    gvputs(job, "%!PS-Adobe-3.0");
    if (job->render.id == FORMAT_EPS)
	gvputs(job, " EPSF-3.0\n");
    else
	gvputs(job, "\n");
    gvprintf(job, "%%%%Creator: %s version %s (%s)\n",
	    job->common->info[0], job->common->info[1], job->common->info[2]);
}

static void psgen_end_job(GVJ_t * job)
{
    gvputs(job, "%%Trailer\n");
    if (job->render.id != FORMAT_EPS)
        gvprintf(job, "%%%%Pages: %d\n", job->common->viewNum);
    if (job->common->show_boxes == NULL)
        if (job->render.id != FORMAT_EPS)
	    gvprintf(job, "%%%%BoundingBox: %d %d %d %d\n",
	        job->boundingBox.LL.x, job->boundingBox.LL.y,
	        job->boundingBox.UR.x, job->boundingBox.UR.y);
    gvputs(job, "end\nrestore\n");
    gvputs(job, "%%EOF\n");
}

static void psgen_begin_graph(GVJ_t * job)
{
    obj_state_t *obj = job->obj;

    setupLatin1 = FALSE;

    if (job->common->viewNum == 0) {
        gvprintf(job, "%%%%Title: %s\n", agnameof(obj->u.g));
    	if (job->render.id != FORMAT_EPS)
            gvputs(job, "%%Pages: (atend)\n");
	else
	    gvputs(job, "%%Pages: 1\n");
        if (job->common->show_boxes == NULL) {
    	    if (job->render.id != FORMAT_EPS)
                gvputs(job, "%%BoundingBox: (atend)\n");
	    else
	        gvprintf(job, "%%%%BoundingBox: %d %d %d %d\n",
	            job->pageBoundingBox.LL.x, job->pageBoundingBox.LL.y,
	            job->pageBoundingBox.UR.x, job->pageBoundingBox.UR.y);
	}
        gvputs(job, "%%EndComments\nsave\n");
        /* include shape library */
        cat_libfile(job, job->common->lib, ps_txt);
	/* include epsf */
        epsf_define(job);
        if (job->common->show_boxes) {
            const char* args[2];
            args[0] = job->common->show_boxes[0];
            args[1] = NULL;
            cat_libfile(job, NULL, args);
        }
    }
    isLatin1 = (GD_charset(obj->u.g) == CHAR_LATIN1);
    /* We always setup Latin1. The charset info is always output,
     * and installing it is cheap. With it installed, we can then
     * rely on ps_string to convert UTF-8 characters whose encoding
     * is in the range of Latin-1 into the Latin-1 equivalent and
     * get the expected PostScript output.
     */
    if (!setupLatin1) {
	gvputs(job, "setupLatin1\n");	/* as defined in ps header */
	setupLatin1 = TRUE;
    }
    /*  Set base URL for relative links (for Distiller >= 3.0)  */
    if (obj->url)
	gvprintf(job, "[ {Catalog} << /URI << /Base (%s) >> >>\n"
		"/PUT pdfmark\n", obj->url);
}

static void psgen_begin_layer(GVJ_t * job, char *layername, int layerNum, int numLayers)
{
    gvprintf(job, "%d %d setlayer\n", layerNum, numLayers);
}

static void psgen_begin_page(GVJ_t * job)
{
    box pbr = job->pageBoundingBox;

    gvprintf(job, "%%%%Page: %d %d\n",
	    job->common->viewNum + 1, job->common->viewNum + 1);
    if (job->common->show_boxes == NULL)
        gvprintf(job, "%%%%PageBoundingBox: %d %d %d %d\n",
	    pbr.LL.x, pbr.LL.y, pbr.UR.x, pbr.UR.y);
    gvprintf(job, "%%%%PageOrientation: %s\n",
	    (job->rotation ? "Landscape" : "Portrait"));
    if (job->render.id == FORMAT_PS2)
        gvprintf(job, "<< /PageSize [%d %d] >> setpagedevice\n",
            pbr.UR.x, pbr.UR.y);
    gvprintf(job, "%d %d %d beginpage\n",
	    job->pagesArrayElem.x, job->pagesArrayElem.y, job->numPages);
    if (job->common->show_boxes == NULL)
        gvprintf(job, "gsave\n%d %d %d %d boxprim clip newpath\n",
	    pbr.LL.x, pbr.LL.y, pbr.UR.x-pbr.LL.x, pbr.UR.y-pbr.LL.y);
    gvprintf(job, "%g %g set_scale %d rotate %g %g translate\n",
	    job->scale.x, job->scale.y,
	    job->rotation,
	    job->translation.x, job->translation.y);

    /*  Define the size of the PS canvas  */
    if (job->render.id == FORMAT_PS2) {
	if (pbr.UR.x >= PDFMAX || pbr.UR.y >= PDFMAX)
	    job->common->errorfn("canvas size (%d,%d) exceeds PDF limit (%d)\n"
		  "\t(suggest setting a bounding box size, see dot(1))\n",
		  pbr.UR.x, pbr.UR.y, PDFMAX);
	gvprintf(job, "[ /CropBox [%d %d %d %d] /PAGES pdfmark\n",
		pbr.LL.x, pbr.LL.y, pbr.UR.x, pbr.UR.y);
    }
}

static void psgen_end_page(GVJ_t * job)
{
    if (job->common->show_boxes) {
	gvputs(job, "0 0 0 edgecolor\n");
	cat_libfile(job, NULL, job->common->show_boxes + 1);
    }
    /* the showpage is really a no-op, but at least one PS processor
     * out there needs to see this literal token.  endpage does the real work.
     */
    gvputs(job, "endpage\nshowpage\ngrestore\n");
    gvputs(job, "%%PageTrailer\n");
    gvprintf(job, "%%%%EndPage: %d\n", job->common->viewNum);
}

static void psgen_begin_cluster(GVJ_t * job)
{
    obj_state_t *obj = job->obj;

    gvprintf(job, "%% %s\n", agnameof(obj->u.g));

    gvputs(job, "gsave\n");
}

static void psgen_end_cluster(GVJ_t * job)
{
    gvputs(job, "grestore\n");
}

static void psgen_begin_node(GVJ_t * job)
{
    gvputs(job, "gsave\n");
}

static void psgen_end_node(GVJ_t * job)
{
    gvputs(job, "grestore\n");
}

static void
psgen_begin_edge(GVJ_t * job)
{
    gvputs(job, "gsave\n");
}

static void psgen_end_edge(GVJ_t * job)
{
    gvputs(job, "grestore\n");
}

static void psgen_begin_anchor(GVJ_t *job, char *url, char *tooltip, char *target, char *id)
{
    obj_state_t *obj = job->obj;

    if (url && obj->url_map_p) {
        gvputs(job, "[ /Rect [ ");
	gvprintpointflist(job, obj->url_map_p, 2);
        gvputs(job, " ]\n");
        gvprintf(job, "  /Border [ 0 0 0 ]\n"
		"  /Action << /Subtype /URI /URI %s >>\n"
		"  /Subtype /Link\n"
		"/ANN pdfmark\n",
		ps_string(url, isLatin1));
    }
}

static void
ps_set_pen_style(GVJ_t *job)
{
    double penwidth = job->obj->penwidth;
    char *p, *line, **s = job->obj->rawstyle;

    gvprintdouble(job, penwidth);
    gvputs(job," setlinewidth\n");

    while (s && (p = line = *s++)) {
	if (strcmp(line, "setlinewidth") == 0)
	    continue;
	while (*p)
	    p++;
	p++;
	while (*p) {
            gvprintf(job,"%s ", p);
	    while (*p)
		p++;
	    p++;
	}
	if (strcmp(line, "invis") == 0)
	    job->obj->penwidth = 0;
	gvprintf(job, "%s\n", line);
    }
}

static void ps_set_color(GVJ_t *job, gvcolor_t *color)
{
    char *objtype;

    if (color) {
	switch (job->obj->type) {
	    case ROOTGRAPH_OBJTYPE:
	    case CLUSTER_OBJTYPE:
		objtype = "graph";
		break;
	    case NODE_OBJTYPE:
		objtype = "node";
		break;
	    case EDGE_OBJTYPE:
		objtype = "edge";
		break;
	    default:
		objtype = "sethsb";
		break;
	}
	gvprintf(job, "%.5g %.5g %.5g %scolor\n",
	    color->u.HSVA[0], color->u.HSVA[1], color->u.HSVA[2], objtype);
    }
}

static void psgen_textpara(GVJ_t * job, pointf p, textpara_t * para)
{
    char *str;

    if (job->obj->pencolor.u.HSVA[3] < .5)
	return;  /* skip transparent text */

    ps_set_color(job, &(job->obj->pencolor));
    gvprintdouble(job, para->fontsize);
    gvprintf(job, " /%s set_font\n", para->fontname);
    str = ps_string(para->str,isLatin1);
    switch (para->just) {
    case 'r':
        p.x -= para->width;
        break;
    case 'l':
        p.x -= 0.0;
        break;
    case 'n':
    default:
        p.x -= para->width / 2.0;
        break;
    }
    p.y += para->yoffset_centerline;
    gvprintpointf(job, p);
    gvputs(job, " moveto ");
    gvprintdouble(job, para->width);
    gvprintf(job, " %s alignedtext\n", str);
}

static void psgen_ellipse(GVJ_t * job, pointf * A, int filled)
{
    /* A[] contains 2 points: the center and corner. */
    pointf AA[2];

    AA[0] = A[0];
    AA[1].x = A[1].x - A[0].x;
    AA[1].y = A[1].y - A[0].y;

    if (filled && job->obj->fillcolor.u.HSVA[3] > .5) {
	ps_set_color(job, &(job->obj->fillcolor));
	gvprintpointflist(job, AA, 2);
	gvputs(job, " ellipse_path fill\n");
    }
    if (job->obj->pencolor.u.HSVA[3] > .5) {
        ps_set_pen_style(job);
        ps_set_color(job, &(job->obj->pencolor));
	gvprintpointflist(job, AA, 2);
	gvputs(job, " ellipse_path stroke\n");
    }
}

static void
psgen_bezier(GVJ_t * job, pointf * A, int n, int arrow_at_start,
	     int arrow_at_end, int filled)
{
    int j;

    if (filled && job->obj->fillcolor.u.HSVA[3] > .5) {
	ps_set_color(job, &(job->obj->fillcolor));
	gvputs(job, "newpath ");
	gvprintpointf(job, A[0]);
	gvputs(job, " moveto\n");
	for (j = 1; j < n; j += 3) {
	    gvprintpointflist(job, &A[j], 3);
	    gvputs(job, " curveto\n");
	}
	gvputs(job, "closepath fill\n");
    }
    if (job->obj->pencolor.u.HSVA[3] > .5) {
        ps_set_pen_style(job);
        ps_set_color(job, &(job->obj->pencolor));
	gvputs(job, "newpath ");
	gvprintpointf(job, A[0]);
	gvputs(job, " moveto\n");
	for (j = 1; j < n; j += 3) {
	    gvprintpointflist(job, &A[j], 3);
	    gvputs(job, " curveto\n");
	}
        gvputs(job, "stroke\n");
    }
}

static void psgen_polygon(GVJ_t * job, pointf * A, int n, int filled)
{
    int j;

    if (filled && job->obj->fillcolor.u.HSVA[3] > .5) {
	ps_set_color(job, &(job->obj->fillcolor));
	gvputs(job, "newpath ");
	gvprintpointf(job, A[0]);
	gvputs(job, " moveto\n");
	for (j = 1; j < n; j++) {
	    gvprintpointf(job, A[j]);
	    gvputs(job, " lineto\n");
        }
	gvputs(job, "closepath fill\n");
    }
    if (job->obj->pencolor.u.HSVA[3] > .5) {
        ps_set_pen_style(job);
        ps_set_color(job, &(job->obj->pencolor));
	gvputs(job, "newpath ");
	gvprintpointf(job, A[0]);
	gvputs(job, " moveto\n");
        for (j = 1; j < n; j++) {
	    gvprintpointf(job, A[j]);
	    gvputs(job, " lineto\n");
	}
        gvputs(job, "closepath stroke\n");
    }
}

static void psgen_polyline(GVJ_t * job, pointf * A, int n)
{
    int j;

    if (job->obj->pencolor.u.HSVA[3] > .5) {
        ps_set_pen_style(job);
        ps_set_color(job, &(job->obj->pencolor));
	gvputs(job, "newpath ");
	gvprintpointf(job, A[0]);
	gvputs(job, " moveto\n");
        for (j = 1; j < n; j++) {
	    gvprintpointf(job, A[j]);
	    gvputs(job, " lineto\n");
	}
        gvputs(job, "stroke\n");
    }
}

static void psgen_comment(GVJ_t * job, char *str)
{
    gvputs(job, "% ");
    gvputs(job, str);
    gvputs(job, "\n");
}

static void psgen_library_shape(GVJ_t * job, char *name, pointf * A, int n, int filled)
{
    if (filled && job->obj->fillcolor.u.HSVA[3] > .5) {
	ps_set_color(job, &(job->obj->fillcolor));
	gvputs(job, "[ ");
        gvprintpointflist(job, A, n);
        gvputs(job, " ");
        gvprintpointf(job, A[0]);
	gvprintf(job, " ]  %d true %s\n", n, name);
    }
    if (job->obj->pencolor.u.HSVA[3] > .5) {
        ps_set_pen_style(job);
        ps_set_color(job, &(job->obj->pencolor));
        gvputs(job, "[ ");
        gvprintpointflist(job, A, n);
        gvputs(job, " ");
        gvprintpointf(job, A[0]);
        gvprintf(job, " ]  %d false %s\n", n, name);
    }
}

static gvrender_engine_t psgen_engine = {
    psgen_begin_job,
    psgen_end_job,
    psgen_begin_graph,
    0,				/* psgen_end_graph */
    psgen_begin_layer,
    0,				/* psgen_end_layer */
    psgen_begin_page,
    psgen_end_page,
    psgen_begin_cluster,
    psgen_end_cluster,
    0,				/* psgen_begin_nodes */
    0,				/* psgen_end_nodes */
    0,				/* psgen_begin_edges */
    0,				/* psgen_end_edges */
    psgen_begin_node,
    psgen_end_node,
    psgen_begin_edge,
    psgen_end_edge,
    psgen_begin_anchor,
    0,				/* psgen_end_anchor */
    0,				/* psgen_begin_label */
    0,				/* psgen_end_label */
    psgen_textpara,
    0,				/* psgen_resolve_color */
    psgen_ellipse,
    psgen_polygon,
    psgen_bezier,
    psgen_polyline,
    psgen_comment,
    psgen_library_shape,
};

static gvrender_features_t render_features_ps = {
    GVRENDER_DOES_TRANSFORM
	| GVRENDER_DOES_MAPS
	| GVRENDER_NO_WHITE_BG
	| GVRENDER_DOES_MAP_RECTANGLE,
    4.,                         /* default pad - graph units */
    NULL,			/* knowncolors */
    0,				/* sizeof knowncolors */
    HSVA_DOUBLE,		/* color_type */
};

static gvdevice_features_t device_features_ps = {
    GVDEVICE_DOES_PAGES
	| GVDEVICE_DOES_LAYERS,	/* flags */
    {36.,36.},			/* default margin - points */
    {612.,792.},                /* default page width, height - points */
    {72.,72.},			/* default dpi */
};

static gvdevice_features_t device_features_eps = {
    0,				/* flags */
    {36.,36.},			/* default margin - points */
    {612.,792.},                /* default page width, height - points */
    {72.,72.},			/* default dpi */
};

gvplugin_installed_t gvrender_ps_types[] = {
    {FORMAT_PS, "ps", 1, &psgen_engine, &render_features_ps},
    {0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_ps_types[] = {
    {FORMAT_PS, "ps:ps", 1, NULL, &device_features_ps},
    {FORMAT_PS2, "ps2:ps", 1, NULL, &device_features_ps},
    {FORMAT_EPS, "eps:ps", 1, NULL, &device_features_eps},
    {0, NULL, 0, NULL, NULL}
};
