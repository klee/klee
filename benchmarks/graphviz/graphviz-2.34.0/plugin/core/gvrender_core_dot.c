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

#ifdef WIN32
#include <io.h>
#include "compat.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "macros.h"
#include "const.h"

#include "gvplugin_render.h"
#include "gvplugin_device.h"
#include "agxbuf.h"
#include "utils.h"
#include "gvio.h"

#define GNEW(t)          (t*)malloc(sizeof(t))

/* #define NEW_XDOT */

typedef enum {
	FORMAT_DOT,
	FORMAT_CANON,
	FORMAT_PLAIN,
	FORMAT_PLAIN_EXT,
	FORMAT_XDOT,
	FORMAT_XDOT12,
	FORMAT_XDOT14,
} format_type;

#ifdef WIN32 /*dependencies*/
#ifdef WITH_CGRAPH
    #pragma comment( lib, "cgraph.lib" )
#else
    #pragma comment( lib, "graph.lib" )
#endif
    #pragma comment( lib, "gvc.lib" )
//    #pragma comment( lib, "ingraphs.lib" )
#endif

#define XDOTVERSION "1.5"

#define NUMXBUFS (EMIT_HLABEL+1)
/* There are as many xbufs as there are values of emit_state_t.
 * However, only the first NUMXBUFS are distinct. Nodes, clusters, and
 * edges are drawn atomically, so they share the DRAW and LABEL buffers
 */
static agxbuf xbuf[NUMXBUFS];
static agxbuf* xbufs[] = {
    xbuf+EMIT_GDRAW, xbuf+EMIT_CDRAW, xbuf+EMIT_TDRAW, xbuf+EMIT_HDRAW, 
    xbuf+EMIT_GLABEL, xbuf+EMIT_CLABEL, xbuf+EMIT_TLABEL, xbuf+EMIT_HLABEL, 
    xbuf+EMIT_CDRAW, xbuf+EMIT_CDRAW, xbuf+EMIT_CLABEL, xbuf+EMIT_CLABEL, 
};
static double penwidth [] = {
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
};

typedef struct {
    attrsym_t *g_draw;
    attrsym_t *g_l_draw;
    attrsym_t *n_draw;
    attrsym_t *n_l_draw;
    attrsym_t *e_draw;
    attrsym_t *h_draw;
    attrsym_t *t_draw;
    attrsym_t *e_l_draw;
    attrsym_t *hl_draw;
    attrsym_t *tl_draw;
    unsigned char buf[NUMXBUFS][BUFSIZ];
    unsigned short version;
    char* version_s;
} xdot_state_t;
static xdot_state_t* xd;

static void xdot_str_xbuf (agxbuf* xb, char* pfx, char* s)
{
    char buf[BUFSIZ];

    sprintf (buf, "%s%d -", pfx, (int)strlen(s));
    agxbput(xb, buf);
    agxbput(xb, s);
    agxbputc(xb, ' ');
}

static void xdot_str (GVJ_t *job, char* pfx, char* s)
{   
    emit_state_t emit_state = job->obj->emit_state;
    xdot_str_xbuf (xbufs[emit_state], pfx, s);
}

/* xdot_trim_zeros
 * Trailing zeros are removed and decimal point, if possible.
 * Add trailing space if addSpace is non-zero.
 */
static void xdot_trim_zeros (char* buf, int addSpace)
{
    char* dotp;
    char* p;

    if ((dotp = strchr (buf,'.'))) {
	p = dotp+1;
	while (*p) p++;  // find end of string
	p--;
	while (*p == '0') *p-- = '\0';
        if (*p == '.')        // If all decimals were zeros, remove ".".
            *p = '\0';
	else
	    p++;
    }
    else if (addSpace)
	p = buf + strlen(buf);

    if (addSpace) { /* p points to null byte */
	*p++ = ' ';
	*p = '\0';
    }
}

/* xdot_fmt_num:
 * Convert double to string with space at end.
 * Trailing zeros are removed and decimal point, if possible.
 */
static void xdot_fmt_num (char* buf, double v)
{
    sprintf(buf, "%.02f", v);
    xdot_trim_zeros (buf, 1);
}

static void xdot_point(agxbuf *xbuf, pointf p)
{
    char buf[BUFSIZ];
    xdot_fmt_num (buf, p.x);
    agxbput(xbuf, buf);
    xdot_fmt_num (buf, yDir(p.y));
    agxbput(xbuf, buf);
}

static void xdot_num(agxbuf *xbuf, double v)
{
    char buf[BUFSIZ];
    xdot_fmt_num (buf, v);
    agxbput(xbuf, buf);
}

static void xdot_points(GVJ_t *job, char c, pointf * A, int n)
{
    emit_state_t emit_state = job->obj->emit_state;
    char buf[BUFSIZ];
    int i, rc;

    rc = agxbputc(xbufs[emit_state], c);
    sprintf(buf, " %d ", n);
    agxbput(xbufs[emit_state], buf);
    for (i = 0; i < n; i++)
        xdot_point(xbufs[emit_state], A[i]);
}

static char*
color2str (unsigned char rgba[4])
{
    static char buf [10];

    if (rgba[3] == 0xFF)
	sprintf (buf, "#%02x%02x%02x", rgba[0], rgba[1],  rgba[2]);
    else
	sprintf (buf, "#%02x%02x%02x%02x", rgba[0], rgba[1],  rgba[2], rgba[3]);
    return buf;
}

static void xdot_pencolor (GVJ_t *job)
{
    xdot_str (job, "c ", color2str (job->obj->pencolor.u.rgba));
}

static void xdot_fillcolor (GVJ_t *job)
{
    xdot_str (job, "C ", color2str (job->obj->fillcolor.u.rgba));
}

static void xdot_style (GVJ_t *job)
{
    unsigned char buf0[BUFSIZ];
    char buf [128]; /* enough to hold a double */
    agxbuf xbuf;
    char* p, **s;
    int more;

    agxbinit(&xbuf, BUFSIZ, buf0);

    /* First, check if penwidth state is correct */
    if (job->obj->penwidth != penwidth[job->obj->emit_state]) {
	penwidth[job->obj->emit_state] = job->obj->penwidth;
	agxbput (&xbuf, "setlinewidth(");
	sprintf (buf, "%.3f", job->obj->penwidth);
	xdot_trim_zeros (buf, 0);
	agxbput(&xbuf, buf);
	agxbputc (&xbuf, ')');
        xdot_str (job, "S ", agxbuse(&xbuf));
    }

    /* now process raw style, if any */
    s = job->obj->rawstyle;
    if (!s)
	return;

    while ((p = *s++)) {
	if (streq(p, "filled") || streq(p, "bold") || streq(p, "setlinewidth")) continue;
        agxbput(&xbuf, p);
        while (*p)
            p++;
        p++;
        if (*p) {  /* arguments */
            agxbputc(&xbuf, '(');
            more = 0;
            while (*p) {
                if (more)
                    agxbputc(&xbuf, ',');
                agxbput(&xbuf, p);
                while (*p) p++;
                p++;
                more++;
            }
            agxbputc(&xbuf, ')');
        }
        xdot_str (job, "S ", agxbuse(&xbuf));
    }

    agxbfree(&xbuf);

}

static void xdot_end_node(GVJ_t* job)
{
    Agnode_t* n = job->obj->u.n; 
    if (agxblen(xbufs[EMIT_NDRAW]))
#ifndef WITH_CGRAPH
	agxset(n, xd->n_draw->index, agxbuse(xbufs[EMIT_NDRAW]));
#else /* WITH_CGRAPH */
	agxset(n, xd->n_draw, agxbuse(xbufs[EMIT_NDRAW]));
#endif /* WITH_CGRAPH */
    if (agxblen(xbufs[EMIT_NLABEL]))
#ifndef WITH_CGRAPH
	agxset(n, xd->n_l_draw->index, agxbuse(xbufs[EMIT_NLABEL]));
#else /* WITH_CGRAPH */
	agxset(n, xd->n_l_draw, agxbuse(xbufs[EMIT_NLABEL]));
#endif /* WITH_CGRAPH */
    penwidth[EMIT_NDRAW] = 1;
    penwidth[EMIT_NLABEL] = 1;
}

static void xdot_end_edge(GVJ_t* job)
{
    Agedge_t* e = job->obj->u.e; 

    if (agxblen(xbufs[EMIT_EDRAW]))
#ifndef WITH_CGRAPH
	agxset(e, xd->e_draw->index, agxbuse(xbufs[EMIT_EDRAW]));
#else /* WITH_CGRAPH */
	agxset(e, xd->e_draw, agxbuse(xbufs[EMIT_EDRAW]));
#endif /* WITH_CGRAPH */
    if (agxblen(xbufs[EMIT_TDRAW]))
#ifndef WITH_CGRAPH
	agxset(e, xd->t_draw->index, agxbuse(xbufs[EMIT_TDRAW]));
#else /* WITH_CGRAPH */
	agxset(e, xd->t_draw, agxbuse(xbufs[EMIT_TDRAW]));
#endif /* WITH_CGRAPH */
    if (agxblen(xbufs[EMIT_HDRAW]))
#ifndef WITH_CGRAPH
	agxset(e, xd->h_draw->index, agxbuse(xbufs[EMIT_HDRAW]));
#else /* WITH_CGRAPH */
	agxset(e, xd->h_draw, agxbuse(xbufs[EMIT_HDRAW]));
#endif /* WITH_CGRAPH */
    if (agxblen(xbufs[EMIT_ELABEL]))
#ifndef WITH_CGRAPH
	agxset(e, xd->e_l_draw->index,agxbuse(xbufs[EMIT_ELABEL]));
#else /* WITH_CGRAPH */
	agxset(e, xd->e_l_draw,agxbuse(xbufs[EMIT_ELABEL]));
#endif /* WITH_CGRAPH */
    if (agxblen(xbufs[EMIT_TLABEL]))
#ifndef WITH_CGRAPH
	agxset(e, xd->tl_draw->index, agxbuse(xbufs[EMIT_TLABEL]));
#else /* WITH_CGRAPH */
	agxset(e, xd->tl_draw, agxbuse(xbufs[EMIT_TLABEL]));
#endif /* WITH_CGRAPH */
    if (agxblen(xbufs[EMIT_HLABEL]))
#ifndef WITH_CGRAPH
	agxset(e, xd->hl_draw->index, agxbuse(xbufs[EMIT_HLABEL]));
#else /* WITH_CGRAPH */
	agxset(e, xd->hl_draw, agxbuse(xbufs[EMIT_HLABEL]));
#endif /* WITH_CGRAPH */
    penwidth[EMIT_EDRAW] = 1;
    penwidth[EMIT_ELABEL] = 1;
    penwidth[EMIT_TDRAW] = 1;
    penwidth[EMIT_HDRAW] = 1;
    penwidth[EMIT_TLABEL] = 1;
    penwidth[EMIT_HLABEL] = 1;
}

#ifdef NEW_XDOT
/* xdot_begin_anchor:
 * The encoding of which fields are present assumes that one of the fields is present,
 * so there is never a 0 after the H.
 */
static void xdot_begin_anchor(GVJ_t * job, char *href, char *tooltip, char *target, char *id)
{
    emit_state_t emit_state = job->obj->emit_state;
    char buf[3];  /* very small integer */
    unsigned int flags = 0;

    agxbput(xbufs[emit_state], "H ");
    if (href)
	flags |= 1;
    if (tooltip)
	flags |= 2;
    if (target)
	flags |= 4;
    sprintf (buf, "%d ", flags);
    agxbput(xbufs[emit_state], buf);
    if (href)
	xdot_str (job, "", href);
    if (tooltip)
	xdot_str (job, "", tooltip);
    if (target)
	xdot_str (job, "", target);
}

static void xdot_end_anchor(GVJ_t * job)
{
    emit_state_t emit_state = job->obj->emit_state;

    agxbput(xbufs[emit_state], "H 0 ");
}
#endif

static void xdot_end_cluster(GVJ_t * job)
{
    Agraph_t* cluster_g = job->obj->u.sg;

#ifndef WITH_CGRAPH
    agxset(cluster_g, xd->g_draw->index, agxbuse(xbufs[EMIT_CDRAW]));
#else /* WITH_CGRAPH */
    agxset(cluster_g, xd->g_draw, agxbuse(xbufs[EMIT_CDRAW]));
#endif /* WITH_CGRAPH */
    if (GD_label(cluster_g))
#ifndef WITH_CGRAPH
	agxset(cluster_g, xd->g_l_draw->index, agxbuse(xbufs[EMIT_CLABEL]));
#else /* WITH_CGRAPH */
	agxset(cluster_g, xd->g_l_draw, agxbuse(xbufs[EMIT_CLABEL]));
#endif /* WITH_CGRAPH */
    penwidth[EMIT_CDRAW] = 1;
    penwidth[EMIT_CLABEL] = 1;
}

static unsigned short
versionStr2Version (char* str)
{
    char c, buf[BUFSIZ];
    int n = 0;
    char* s = str;
    unsigned short us;

    while ((c = *s++)) {
	if (isdigit(c)) {
	    if (n < BUFSIZ-1) buf[n++] = c;
	    else {
		agerr(AGWARN, "xdot version \"%s\" too long", str);
		break;
	    }
	}
    }
    buf[n] = '\0';
    
    us = atoi(buf);
    return us;
}

/* 
 * John M. suggests:
 * You might want to add four more:
 *
 * _ohdraw_ (optional head-end arrow for edges)
 * _ohldraw_ (optional head-end label for edges)
 * _otdraw_ (optional tail-end arrow for edges)
 * _otldraw_ (optional tail-end label for edges)
 * 
 * that would be generated when an additional option is supplied to 
 * dot, etc. and 
 * these would be the arrow/label positions to use if a user want to flip the 
 * direction of an edge (as sometimes is there want).
 * 
 * N.B. John M. asks:
 *   By the way, I don't know if you ever plan to add other letters for 
 * the xdot spec, but could you reserve "a" and also "A" (for  attribute), 
 * "n" and also "N" (for numeric), "w" (for sWitch),  "s" (for string) 
 * and "t" (for tooltip) and "x" (for position). We use  those letters in 
 * our drawing spec (and also "<" and ">"), so if you  start generating 
 * output with them, it could break what we have. 
 */
static void
xdot_begin_graph (graph_t *g, int s_arrows, int e_arrows, format_type id)
{
    int i, us;
    char* s;

    xd = GNEW(xdot_state_t);

    if (id == FORMAT_XDOT14) {
	xd->version = 14;
	xd->version_s = "1.4";
    }
    else if (id == FORMAT_XDOT12) {
	xd->version = 12;
	xd->version_s = "1.2";
    }
    else if ((s = agget(g, "xdotversion")) && s[0] && ((us = versionStr2Version(s)) > 10)) {
	xd->version = us;
	xd->version_s = s;
    }
    else {
	xd->version = versionStr2Version(XDOTVERSION);
	xd->version_s = XDOTVERSION;
    }

    if (GD_n_cluster(g))
#ifndef WITH_CGRAPH
	xd->g_draw = safe_dcl(g, g, "_draw_", "", agraphattr);
#else
	xd->g_draw = safe_dcl(g, AGRAPH, "_draw_", "");
#endif
    else
	xd->g_draw = NULL;
    if (GD_has_labels(g) & GRAPH_LABEL)
#ifndef WITH_CGRAPH
	xd->g_l_draw = safe_dcl(g, g, "_ldraw_", "", agraphattr);
#else
	xd->g_l_draw = safe_dcl(g, AGRAPH, "_ldraw_", "");
#endif
    else
	xd->g_l_draw = NULL;

#ifndef WITH_CGRAPH
    xd->n_draw = safe_dcl(g, g->proto->n, "_draw_", "", agnodeattr);
    xd->n_l_draw = safe_dcl(g, g->proto->n, "_ldraw_", "", agnodeattr);

    xd->e_draw = safe_dcl(g, g->proto->e, "_draw_", "", agedgeattr);
#else
    xd->n_draw = safe_dcl(g, AGNODE, "_draw_", "");
    xd->n_l_draw = safe_dcl(g, AGNODE, "_ldraw_", "");

    xd->e_draw = safe_dcl(g, AGEDGE, "_draw_", "");
#endif
    if (e_arrows)
#ifndef WITH_CGRAPH
	xd->h_draw = safe_dcl(g, g->proto->e, "_hdraw_", "", agedgeattr);
#else
	xd->h_draw = safe_dcl(g, AGEDGE, "_hdraw_", "");
#endif
    else
	xd->h_draw = NULL;
    if (s_arrows)
#ifndef WITH_CGRAPH
	xd->t_draw = safe_dcl(g, g->proto->e, "_tdraw_", "", agedgeattr);
#else
	xd->t_draw = safe_dcl(g, AGEDGE, "_tdraw_", "");
#endif
    else
	xd->t_draw = NULL;
    if (GD_has_labels(g) & (EDGE_LABEL|EDGE_XLABEL))
#ifndef WITH_CGRAPH
	xd->e_l_draw = safe_dcl(g, g->proto->e, "_ldraw_", "", agedgeattr);
#else
	xd->e_l_draw = safe_dcl(g, AGEDGE, "_ldraw_", "");
#endif
    else
	xd->e_l_draw = NULL;
    if (GD_has_labels(g) & HEAD_LABEL)
#ifndef WITH_CGRAPH
	xd->hl_draw = safe_dcl(g, g->proto->e, "_hldraw_", "", agedgeattr);
#else
	xd->hl_draw = safe_dcl(g, AGEDGE, "_hldraw_", "");
#endif
    else
	xd->hl_draw = NULL;
    if (GD_has_labels(g) & TAIL_LABEL)
#ifndef WITH_CGRAPH
	xd->tl_draw = safe_dcl(g, g->proto->e, "_tldraw_", "", agedgeattr);
#else
	xd->tl_draw = safe_dcl(g, AGEDGE, "_tldraw_", "");
#endif
    else
	xd->tl_draw = NULL;

    for (i = 0; i < NUMXBUFS; i++)
	agxbinit(xbuf+i, BUFSIZ, xd->buf[i]);
}

static void dot_begin_graph(GVJ_t *job)
{
    int e_arrows;            /* graph has edges with end arrows */
    int s_arrows;            /* graph has edges with start arrows */
    graph_t *g = job->obj->u.g;

    switch (job->render.id) {
	case FORMAT_DOT:
	    attach_attrs(g);
	    break;
	case FORMAT_CANON:
	    if (HAS_CLUST_EDGE(g))
		undoClusterEdges(g);
	    break;
	case FORMAT_PLAIN:
	case FORMAT_PLAIN_EXT:
	    break;
	case FORMAT_XDOT:
	case FORMAT_XDOT12:
	case FORMAT_XDOT14:
	    attach_attrs_and_arrows(g, &s_arrows, &e_arrows);
	    xdot_begin_graph(g, s_arrows, e_arrows, job->render.id);
	    break;
    }
}

static void xdot_end_graph(graph_t* g)
{
    int i;

    if (agxblen(xbufs[EMIT_GDRAW])) {
	if (!xd->g_draw)
#ifndef WITH_CGRAPH
	    xd->g_draw = safe_dcl(g, g, "_draw_", "", agraphattr);
	agxset(g, xd->g_draw->index, agxbuse(xbufs[EMIT_GDRAW]));
#else /* WITH_CGRAPH */
	    xd->g_draw = safe_dcl(g, AGRAPH, "_draw_", "");
	agxset(g, xd->g_draw, agxbuse(xbufs[EMIT_GDRAW]));
#endif /* WITH_CGRAPH */
    }
    if (GD_label(g))
#ifndef WITH_CGRAPH
	agxset(g, xd->g_l_draw->index, agxbuse(xbufs[EMIT_GLABEL]));
#else /* WITH_CGRAPH */
	agxset(g, xd->g_l_draw, agxbuse(xbufs[EMIT_GLABEL]));
#endif /* WITH_CGRAPH */
    agsafeset (g, "xdotversion", xd->version_s, "");

    for (i = 0; i < NUMXBUFS; i++)
	agxbfree(xbuf+i);
    free (xd);
    penwidth[EMIT_GDRAW] = 1;
    penwidth[EMIT_GLABEL] = 1;
}

#ifdef WITH_CGRAPH
typedef int (*putstrfn) (void *chan, const char *str);
typedef int (*flushfn) (void *chan);
#endif
static void dot_end_graph(GVJ_t *job)
{
    graph_t *g = job->obj->u.g;
#ifdef WITH_CGRAPH
    Agiodisc_t* io_save;
    static Agiodisc_t io;

    if (io.afread == NULL) {
	io.afread = AgIoDisc.afread;
	io.putstr = (putstrfn)gvputs;
	io.flush = (flushfn)gvflush;
    }
#endif

#ifndef WITH_CGRAPH
    agsetiodisc(NULL, gvfwrite, gvferror);
#else
    io_save = g->clos->disc.io;
    g->clos->disc.io = &io;
#endif
    switch (job->render.id) {
	case FORMAT_PLAIN:
	    write_plain(job, g, (FILE*)job, FALSE);
	    break;
	case FORMAT_PLAIN_EXT:
	    write_plain(job, g, (FILE*)job, TRUE);
	    break;
	case FORMAT_DOT:
	case FORMAT_CANON:
	    if (!(job->flags & OUTPUT_NOT_REQUIRED))
		agwrite(g, (FILE*)job);
	    break;
	case FORMAT_XDOT:
	case FORMAT_XDOT12:
	case FORMAT_XDOT14:
	    xdot_end_graph(g);
	    if (!(job->flags & OUTPUT_NOT_REQUIRED))
		agwrite(g, (FILE*)job);
	    break;
    }
#ifndef WITH_CGRAPH
    agsetiodisc(NULL, NULL, NULL);
#else
    g->clos->disc.io = io_save;
#endif
}

static void xdot_textpara(GVJ_t * job, pointf p, textpara_t * para)
{
    emit_state_t emit_state = job->obj->emit_state;
    int flags;
    char buf[BUFSIZ];
    int j;
    
    agxbput(xbufs[emit_state], "F ");
    xdot_fmt_num (buf, para->fontsize);
    agxbput(xbufs[emit_state], buf);
    xdot_str (job, "", para->fontname);
    xdot_pencolor(job);

    switch (para->just) {
    case 'l':
        j = -1; 
        break;
    case 'r':
        j = 1;
        break;
    default:
    case 'n':
        j = 0;
        break;
    }
    if (para->font)
	flags = para->font->flags;
    else
	flags = 0;
    sprintf (buf, "t %d ", flags);
    agxbput(xbufs[emit_state], buf);

    p.y += para->yoffset_centerline;
    agxbput(xbufs[emit_state], "T ");
    xdot_point(xbufs[emit_state], p);
    sprintf(buf, "%d ", j);
    agxbput(xbufs[emit_state], buf);
    xdot_fmt_num (buf, para->width);
    agxbput(xbufs[emit_state], buf);
    xdot_str (job, "", para->str);
}

static void xdot_color_stop (agxbuf* xb, float v, gvcolor_t* clr)
{
    char buf[BUFSIZ];

    sprintf (buf, "%.03f", v);
    xdot_trim_zeros (buf, 1);
    xdot_str_xbuf (xb, buf, color2str (clr->u.rgba));
}

static void xdot_gradient_fillcolor (GVJ_t* job, int filled, pointf* A, int n)
{
    unsigned char buf0[BUFSIZ];
    agxbuf xbuf;
    obj_state_t* obj = job->obj;
    float angle = obj->gradient_angle * M_PI / 180;
    float r1,r2;
    pointf G[2],c1,c2;

    if (xd->version < 14) {
	xdot_fillcolor (job);
	return;
    }

    agxbinit(&xbuf, BUFSIZ, buf0);
    if (filled == GRADIENT) {
	get_gradient_points(A, G, n, angle, 2);
	agxbputc (&xbuf, '[');
	xdot_point (&xbuf, G[0]);
	xdot_point (&xbuf, G[1]);
    }
    else {
	get_gradient_points(A, G, n, 0, 3);
	  //r1 is inner radius, r2 is outer radius
	r1 = G[1].x;
	r2 = G[1].y;
	if (angle == 0) {
	    c1.x = G[0].x;
	    c1.y = G[0].y;
	}
	else {
	    c1.x = G[0].x +  (r2/4) * cos(angle);
	    c1.y = G[0].y -  (r2/4) * sin(angle);
	}
	c2.x = G[0].x;
	c2.y = G[0].y;
	r1 = r2/4;
	agxbputc(&xbuf, '(');
	xdot_point (&xbuf, c1);
	xdot_num (&xbuf, r1);
	xdot_point (&xbuf, c2);
	xdot_num (&xbuf, r2);
    }
    
    agxbput(&xbuf, "2 ");
    if (obj->gradient_frac > 0) {
	xdot_color_stop (&xbuf, obj->gradient_frac, &obj->fillcolor);
	xdot_color_stop (&xbuf, obj->gradient_frac, &obj->stopcolor);
    }
    else {
	xdot_color_stop (&xbuf, 0, &obj->fillcolor);
	xdot_color_stop (&xbuf, 1, &obj->stopcolor);
    }
    agxbpop(&xbuf);
    if (filled == GRADIENT)
	agxbputc(&xbuf, ']');
    else
	agxbputc(&xbuf, ')');
    xdot_str (job, "C ", agxbuse(&xbuf));
    agxbfree(&xbuf);
}

static void xdot_ellipse(GVJ_t * job, pointf * A, int filled)
{
    emit_state_t emit_state = job->obj->emit_state;

    char buf[BUFSIZ];

    xdot_style (job);
    xdot_pencolor (job);
    if (filled) {
	if ((filled == GRADIENT) || (filled == RGRADIENT)) {
	   xdot_gradient_fillcolor (job, filled, A, 2);
	}
        else 
	    xdot_fillcolor (job);
        agxbput(xbufs[emit_state], "E ");
    }
    else
        agxbput(xbufs[emit_state], "e ");
    xdot_point(xbufs[emit_state], A[0]);
    xdot_fmt_num (buf, A[1].x - A[0].x);
    agxbput(xbufs[emit_state], buf);
    xdot_fmt_num (buf, A[1].y - A[0].y);
    agxbput(xbufs[emit_state], buf);
}

static void xdot_bezier(GVJ_t * job, pointf * A, int n, int arrow_at_start, int arrow_at_end, int filled)
{
    xdot_style (job);
    xdot_pencolor (job);
    if (filled) {
	if ((filled == GRADIENT) || (filled == RGRADIENT)) {
	   xdot_gradient_fillcolor (job, filled, A, n);
	}
        else
	    xdot_fillcolor (job);
        xdot_points(job, 'b', A, n);   /* NB - 'B' & 'b' are reversed in comparison to the other items */
    }
    else
        xdot_points(job, 'B', A, n);
}

static void xdot_polygon(GVJ_t * job, pointf * A, int n, int filled)
{
    xdot_style (job);
    xdot_pencolor (job);
    if (filled) {
	if ((filled == GRADIENT) || (filled == RGRADIENT)) {
	   xdot_gradient_fillcolor (job, filled, A, n);
	}
        else
	    xdot_fillcolor (job);
        xdot_points(job, 'P', A, n);
    }
    else
        xdot_points(job, 'p', A, n);
}

static void xdot_polyline(GVJ_t * job, pointf * A, int n)
{
    xdot_style (job);
    xdot_pencolor (job);
    xdot_points(job, 'L', A, n);
}

void core_loadimage_xdot(GVJ_t * job, usershape_t *us, boxf b, boolean filled)
{
    emit_state_t emit_state = job->obj->emit_state;
    char buf[BUFSIZ];
    
    agxbput(xbufs[emit_state], "I ");
    xdot_point(xbufs[emit_state], b.LL);
    xdot_fmt_num (buf, b.UR.x - b.LL.x);
    agxbput(xbufs[emit_state], buf);
    xdot_fmt_num (buf, b.UR.y - b.LL.y);
    agxbput(xbufs[emit_state], buf);
    xdot_str (job, "", (char*)(us->name));
}

gvrender_engine_t dot_engine = {
    0,				/* dot_begin_job */
    0,				/* dot_end_job */
    dot_begin_graph,
    dot_end_graph,
    0,				/* dot_begin_layer */
    0,				/* dot_end_layer */
    0,				/* dot_begin_page */
    0,				/* dot_end_page */
    0,				/* dot_begin_cluster */
    0,				/* dot_end_cluster */
    0,				/* dot_begin_nodes */
    0,				/* dot_end_nodes */
    0,				/* dot_begin_edges */
    0,				/* dot_end_edges */
    0,				/* dot_begin_node */
    0,				/* dot_end_node */
    0,				/* dot_begin_edge */
    0,				/* dot_end_edge */
    0,				/* dot_begin_anchor */
    0,				/* dot_end_anchor */
    0,				/* dot_begin_label */
    0,				/* dot_end_label */
    0,				/* dot_textpara */
    0,				/* dot_resolve_color */
    0,				/* dot_ellipse */
    0,				/* dot_polygon */
    0,				/* dot_bezier */
    0,				/* dot_polyline */
    0,				/* dot_comment */
    0,				/* dot_library_shape */
};

gvrender_engine_t xdot_engine = {
    0,				/* xdot_begin_job */
    0,				/* xdot_end_job */
    dot_begin_graph,
    dot_end_graph,
    0,				/* xdot_begin_layer */
    0,				/* xdot_end_layer */
    0,				/* xdot_begin_page */
    0,				/* xdot_end_page */
    0,				/* xdot_begin_cluster */
    xdot_end_cluster,
    0,				/* xdot_begin_nodes */
    0,				/* xdot_end_nodes */
    0,				/* xdot_begin_edges */
    0,				/* xdot_end_edges */
    0,				/* xdot_begin_node */
    xdot_end_node,
    0,				/* xdot_begin_edge */
    xdot_end_edge,
#ifdef NEW_XDOT
    xdot_begin_anchor,
    xdot_end_anchor,
#else
    0,                          /* xdot_begin_anchor */
    0,                          /* xdot_end_anchor */
#endif
    0,				/* xdot_begin_label */
    0,				/* xdot_end_label */
    xdot_textpara,
    0,				/* xdot_resolve_color */
    xdot_ellipse,
    xdot_polygon,
    xdot_bezier,
    xdot_polyline,
    0,				/* xdot_comment */
    0,				/* xdot_library_shape */
};

gvrender_features_t render_features_dot = {
    GVRENDER_DOES_TRANSFORM,	/* not really - uses raw graph coords */  /* flags */
    0.,                         /* default pad - graph units */
    NULL,			/* knowncolors */
    0,				/* sizeof knowncolors */
    COLOR_STRING,		/* color_type */
};

gvrender_features_t render_features_xdot = {
    GVRENDER_DOES_TRANSFORM 	/* not really - uses raw graph coords */  
	| GVRENDER_DOES_MAPS
	| GVRENDER_DOES_TARGETS
	| GVRENDER_DOES_TOOLTIPS, /* flags */
    0.,                         /* default pad - graph units */
    NULL,			/* knowncolors */
    0,				/* sizeof knowncolors */
    RGBA_BYTE,		/* color_type */
};

gvdevice_features_t device_features_canon = {
    LAYOUT_NOT_REQUIRED,	/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},                    /* default height, width - device units */
    {72.,72.},			/* default dpi */
};

gvdevice_features_t device_features_dot = {
    0,				/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},			/* default page width, height - points */
    {72.,72.},			/* default dpi */
};

gvplugin_installed_t gvrender_dot_types[] = {
    {FORMAT_DOT, "dot", 1, &dot_engine, &render_features_dot},
    {FORMAT_XDOT, "xdot", 1, &xdot_engine, &render_features_xdot},
    {0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_dot_types[] = {
    {FORMAT_DOT, "dot:dot", 1, NULL, &device_features_dot},
    {FORMAT_DOT, "gv:dot", 1, NULL, &device_features_dot},
    {FORMAT_CANON, "canon:dot", 1, NULL, &device_features_canon},
    {FORMAT_PLAIN, "plain:dot", 1, NULL, &device_features_dot},
    {FORMAT_PLAIN_EXT, "plain-ext:dot", 1, NULL, &device_features_dot},
    {FORMAT_XDOT, "xdot:xdot", 1, NULL, &device_features_dot},
    {FORMAT_XDOT12, "xdot1.2:xdot", 1, NULL, &device_features_dot},
    {FORMAT_XDOT14, "xdot1.4:xdot", 1, NULL, &device_features_dot},
    {0, NULL, 0, NULL, NULL}
};
