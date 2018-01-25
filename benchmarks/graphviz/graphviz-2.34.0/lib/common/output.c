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

#include "render.h"
#include "agxbuf.h"
#include <stdarg.h>
#include <string.h>

#define YDIR(y) (Y_invert ? (Y_off - (y)) : (y))
#define YFDIR(y) (Y_invert ? (YF_off - (y)) : (y))

static double Y_off;        /* ymin + ymax */
static double YF_off;       /* Y_off in inches */

double yDir (double y)
{
    return YDIR(y);
}

#ifdef WITH_CGRAPH
static int (*putstr) (void *chan, const char *str);

static void agputs (const char* s, FILE* fp)
{
    putstr ((void*)fp, s);
}
static void agputc (int c, FILE* fp)
{
    static char buf[2] = {'\0','\0'};
    buf[0] = c;
    putstr ((void*)fp, buf);
}

#endif

static void printstring(FILE * f, char *prefix, char *s)
{
    if (prefix) agputs(prefix, f);
    agputs(s, f);
}

static void printint(FILE * f, char *prefix, int i)
{
    char buf[BUFSIZ];
    
    if (prefix) agputs(prefix, f);
    sprintf(buf, "%d", i);
    agputs(buf, f);
}

static void printdouble(FILE * f, char *prefix, double v)
{
    char buf[BUFSIZ];
    
    if (prefix) agputs(prefix, f);
    sprintf(buf, "%.5g", v);
    agputs(buf, f);
}

static void printpoint(FILE * f, pointf p)
{
    printdouble(f, " ", PS2INCH(p.x));
    printdouble(f, " ", PS2INCH(YDIR(p.y)));
}

/* setYInvert:
 * Set parameters used to flip coordinate system (y=0 at top).
 * Values do not need to be unset, since if Y_invert is set, it's
 * set for * all graphs during current run, so each will 
 * reinitialize the values for its bbox.
 */
static void setYInvert(graph_t * g)
{
    if (Y_invert) {
	Y_off = GD_bb(g).UR.y + GD_bb(g).LL.y;
	YF_off = PS2INCH(Y_off);
    }
}

/* canon:
 * Canonicalize a string which may not have been allocated using agstrdup.
 */
static char* canon (graph_t *g, char* s)
{
#ifndef WITH_CGRAPH
    char* ns = agstrdup (s);
    char* cs = agcanonStr (ns);
    agstrfree (ns);
#else
    char* ns = agstrdup (g, s);
    char* cs = agcanonStr (ns);
    agstrfree (g, ns);
#endif
    return cs;
}

static void writenodeandport(FILE * f, node_t * node, char *port)
{
    char *name;
    if (IS_CLUST_NODE(node))
	name = canon (agraphof(node), strchr(agnameof(node), ':') + 1);
    else
	name = agcanonStr (agnameof(node));
    printstring(f, " ", name); /* slimey i know */
    if (port && *port)
	printstring(f, ":", agcanonStr(port));
}

/* _write_plain:
 */
void write_plain(GVJ_t * job, graph_t * g, FILE * f, boolean extend)
{
    int i, j, splinePoints;
    char *tport, *hport;
    node_t *n;
    edge_t *e;
    bezier bz;
    pointf pt;
    char *lbl;
    char* fillcolor;

#ifdef WITH_CGRAPH
    putstr = g->clos->disc.io->putstr;
#endif
//    setup_graph(job, g);
    setYInvert(g);
    pt = GD_bb(g).UR;
    printdouble(f, "graph ", job->zoom);
    printdouble(f, " ", PS2INCH(pt.x));
    printdouble(f, " ", PS2INCH(pt.y));
    agputc('\n', f);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (IS_CLUST_NODE(n))
	    continue;
	printstring(f, "node ", agcanonStr(agnameof(n)));
	printpoint(f, ND_coord(n));
	if (ND_label(n)->html)   /* if html, get original text */
#ifndef WITH_CGRAPH
	    lbl = agcanonStr (agxget(n, N_label->index));
#else
	    lbl = agcanonStr (agxget(n, N_label));
#endif
	else
	    lbl = canon(agraphof(n),ND_label(n)->text);
        printdouble(f, " ", ND_width(n));
        printdouble(f, " ", ND_height(n));
        printstring(f, " ", lbl);
	printstring(f, " ", late_nnstring(n, N_style, "solid"));
	printstring(f, " ", ND_shape(n)->name);
	printstring(f, " ", late_nnstring(n, N_color, DEFAULT_COLOR));
	fillcolor = late_nnstring(n, N_fillcolor, "");
        if (fillcolor[0] == '\0')
	    fillcolor = late_nnstring(n, N_color, DEFAULT_FILL);
	printstring(f, " ", fillcolor);
	agputc('\n', f);
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {

#ifndef WITH_CGRAPH
/* FIXME - there must be a proper way to get port info - these are 
 * supposed to be private to libgraph - from libgraph.h */
#define TAILX 1
#define HEADX 2

	    if (extend && e->attr) {
		tport = e->attr[TAILX];
		hport = e->attr[HEADX];
	    }
#else /* WITH_CGRAPH */
	    if (extend) {		//assuming these two attrs have already been created by cgraph
		if (!(tport = agget(e,"tailport")))
		    tport = "";
		if (!(hport = agget(e,"headport")))
		    hport = "";
	    }
#endif /* WITH_CGRAPH */
	    else
		tport = hport = "";
	    if (ED_spl(e)) {
		splinePoints = 0;
		for (i = 0; i < ED_spl(e)->size; i++) {
		    bz = ED_spl(e)->list[i];
		    splinePoints += bz.size;
		}
		printstring(f, NULL, "edge");
		writenodeandport(f, agtail(e), tport);
		writenodeandport(f, aghead(e), hport);
		printint(f, " ", splinePoints);
		for (i = 0; i < ED_spl(e)->size; i++) {
		    bz = ED_spl(e)->list[i];
		    for (j = 0; j < bz.size; j++)
			printpoint(f, bz.list[j]);
		}
	    }
	    if (ED_label(e)) {
		printstring(f, " ", canon(agraphof(agtail(e)),ED_label(e)->text));
		printpoint(f, ED_label(e)->pos);
	    }
	    printstring(f, " ", late_nnstring(e, E_style, "solid"));
	    printstring(f, " ", late_nnstring(e, E_color, DEFAULT_COLOR));
	    agputc('\n', f);
	}
    }
    agputs("stop\n", f);
}

static void set_record_rects(node_t * n, field_t * f, agxbuf * xb)
{
    int i;
    char buf[BUFSIZ];

    if (f->n_flds == 0) {
	sprintf(buf, "%.5g,%.5g,%.5g,%.5g ",
		f->b.LL.x + ND_coord(n).x,
		YDIR(f->b.LL.y + ND_coord(n).y),
		f->b.UR.x + ND_coord(n).x,
		YDIR(f->b.UR.y + ND_coord(n).y));
	agxbput(xb, buf);
    }
    for (i = 0; i < f->n_flds; i++)
	set_record_rects(n, f->fld[i], xb);
}

static void rec_attach_bb(graph_t * g, Agsym_t* bbsym)
{
    int c;
    char buf[BUFSIZ];
    pointf pt;

    sprintf(buf, "%.5g,%.5g,%.5g,%.5g", GD_bb(g).LL.x, YDIR(GD_bb(g).LL.y),
	    GD_bb(g).UR.x, YDIR(GD_bb(g).UR.y));
#ifndef WITH_CGRAPH
    agxset(g, bbsym->index, buf);
#else
    agxset(g, bbsym, buf);
#endif
    if (GD_label(g) && GD_label(g)->text[0]) {
	pt = GD_label(g)->pos;
	sprintf(buf, "%.5g,%.5g", pt.x, YDIR(pt.y));
	agset(g, "lp", buf);
	pt = GD_label(g)->dimen;
	sprintf(buf, "%.2f", PS2INCH(pt.x));
	agset (g, "lwidth", buf);
	sprintf(buf, "%.2f", PS2INCH(pt.y));
	agset (g, "lheight", buf);
    }
    for (c = 1; c <= GD_n_cluster(g); c++)
	rec_attach_bb(GD_clust(g)[c], bbsym);
}

void attach_attrs_and_arrows(graph_t* g, int* sp, int* ep)
{
    int e_arrows;		/* graph has edges with end arrows */
    int s_arrows;		/* graph has edges with start arrows */
    int i, j, sides;
    char buf[BUFSIZ];		/* Used only for small strings */
    unsigned char xbuffer[BUFSIZ];	/* Initial buffer for xb */
    agxbuf xb;
    node_t *n;
    edge_t *e;
    pointf ptf;
    int dim3 = (GD_odim(g) >= 3);
    Agsym_t* bbsym;

    gv_fixLocale (1);
    e_arrows = s_arrows = 0;
    setYInvert(g);
    agxbinit(&xb, BUFSIZ, xbuffer);
#ifndef WITH_CGRAPH
    safe_dcl(g, g->proto->n, "pos", "", agnodeattr);
    safe_dcl(g, g->proto->n, "rects", "", agnodeattr);
    N_width = safe_dcl(g, g->proto->n, "width", "", agnodeattr);
    N_height = safe_dcl(g, g->proto->n, "height", "", agnodeattr);
    safe_dcl(g, g->proto->e, "pos", "", agedgeattr);
    if (GD_has_labels(g) & NODE_XLABEL)
	safe_dcl(g, g->proto->n, "xlp", "", agnodeattr);
    if (GD_has_labels(g) & EDGE_LABEL)
	safe_dcl(g, g->proto->e, "lp", "", agedgeattr);
    if (GD_has_labels(g) & EDGE_XLABEL)
	safe_dcl(g, g->proto->e, "xlp", "", agedgeattr);
    if (GD_has_labels(g) & HEAD_LABEL)
	safe_dcl(g, g->proto->e, "head_lp", "", agedgeattr);
    if (GD_has_labels(g) & TAIL_LABEL)
	safe_dcl(g, g->proto->e, "tail_lp", "", agedgeattr);
    if (GD_label(g)) {
	safe_dcl(g, g, "lp", "", agraphattr);
	safe_dcl(g, g, "lwidth", "", agraphattr);
	safe_dcl(g, g, "lheight", "", agraphattr);
	if (GD_label(g)->text[0]) {
	    ptf = GD_label(g)->pos;
	    sprintf(buf, "%.5g,%.5g", ptf.x, YDIR(ptf.y));
	    agset(g, "lp", buf);
	    ptf = GD_label(g)->dimen;
	    sprintf(buf, "%.2f", PS2INCH(ptf.x));
	    agset(g, "lwidth", buf);
	    sprintf(buf, "%.2f", PS2INCH(ptf.y));
	    agset(g, "lheight", buf);
	}
    }
    bbsym = safe_dcl(g, g, "bb", "", agraphattr);
#else
    safe_dcl(g, AGNODE, "pos", "");
    safe_dcl(g, AGNODE, "rects", "");
    N_width = safe_dcl(g, AGNODE, "width", "");
    N_height = safe_dcl(g, AGNODE, "height", "");
    safe_dcl(g, AGEDGE, "pos", "");
    if (GD_has_labels(g) & NODE_XLABEL)
	safe_dcl(g, AGNODE, "xlp", "");
    if (GD_has_labels(g) & EDGE_LABEL)
	safe_dcl(g, AGEDGE, "lp", "");
    if (GD_has_labels(g) & EDGE_XLABEL)
	safe_dcl(g, AGEDGE, "xlp", "");
    if (GD_has_labels(g) & HEAD_LABEL)
	safe_dcl(g, AGEDGE, "head_lp", "");
    if (GD_has_labels(g) & TAIL_LABEL)
	safe_dcl(g, AGEDGE, "tail_lp", "");
    if (GD_label(g)) {
	safe_dcl(g, AGRAPH, "lp", "");
	safe_dcl(g, AGRAPH, "lwidth", "");
	safe_dcl(g, AGRAPH, "lheight", "");
	if (GD_label(g)->text[0]) {
	    ptf = GD_label(g)->pos;
	    sprintf(buf, "%.5g,%.5g", ptf.x, YDIR(ptf.y));
	    agset(g, "lp", buf);
	    ptf = GD_label(g)->dimen;
	    sprintf(buf, "%.2f", PS2INCH(ptf.x));
	    agset(g, "lwidth", buf);
	    sprintf(buf, "%.2f", PS2INCH(ptf.y));
	    agset(g, "lheight", buf);
	}
    }
    bbsym = safe_dcl(g, AGRAPH, "bb", "");
#endif
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (dim3) {
	    int k;

	    sprintf(buf, "%.5g,%.5g,%.5g", ND_coord(n).x, YDIR(ND_coord(n).y), POINTS_PER_INCH*(ND_pos(n)[2]));
	    agxbput (&xb, buf);
	    for (k = 3; k < GD_odim(g); k++) {
		sprintf(buf, ",%.5g", POINTS_PER_INCH*(ND_pos(n)[k]));
		agxbput (&xb, buf);
	    }
	    agset(n, "pos", agxbuse(&xb));
	} else {
	    sprintf(buf, "%.5g,%.5g", ND_coord(n).x, YDIR(ND_coord(n).y));
	    agset(n, "pos", buf);
	}
	sprintf(buf, "%.5g", PS2INCH(ND_ht(n)));
#ifndef WITH_CGRAPH
	agxset(n, N_height->index, buf);
	sprintf(buf, "%.5g", PS2INCH(ND_lw(n) + ND_rw(n)));
	agxset(n, N_width->index, buf);
#else
	agxset(n, N_height, buf);
	sprintf(buf, "%.5g", PS2INCH(ND_lw(n) + ND_rw(n)));
	agxset(n, N_width, buf);
#endif
	if (ND_xlabel(n) && ND_xlabel(n)->set) {
	    ptf = ND_xlabel(n)->pos;
	    sprintf(buf, "%.5g,%.5g", ptf.x, YDIR(ptf.y));
	    agset(n, "xlp", buf);
	}
	if (strcmp(ND_shape(n)->name, "record") == 0) {
	    set_record_rects(n, ND_shape_info(n), &xb);
	    agxbpop(&xb);	/* get rid of last space */
	    agset(n, "rects", agxbuse(&xb));
	} else {
	    polygon_t *poly;
	    int i;
	    if (N_vertices && isPolygon(n)) {
		poly = (polygon_t *) ND_shape_info(n);
		sides = poly->sides;
		if (sides < 3) {
		    char *p = agget(n, "samplepoints");
		    if (p)
			sides = atoi(p);
		    else
			sides = 8;
		    if (sides < 3)
			sides = 8;
		}
		for (i = 0; i < sides; i++) {
		    if (i > 0)
			agxbputc(&xb, ' ');
		    if (poly->sides >= 3)
			sprintf(buf, "%.5g %.5g",
				PS2INCH(poly->vertices[i].x),
				YFDIR(PS2INCH(poly->vertices[i].y)));
		    else
			sprintf(buf, "%.5g %.5g",
				ND_width(n) / 2.0 * cos(i / (double) sides * M_PI * 2.0),
				YFDIR(ND_height(n) / 2.0 * sin(i / (double) sides * M_PI * 2.0)));
		    agxbput(&xb, buf);
		}
#ifndef WITH_CGRAPH
		agxset(n, N_vertices->index, agxbuse(&xb));
#else /* WITH_CGRAPH */
		agxset(n, N_vertices, agxbuse(&xb));
#endif /* WITH_CGRAPH */
	    }
	}
	if (State >= GVSPLINES) {
	    for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
		if (ED_edge_type(e) == IGNORED)
		    continue;
		if (ED_spl(e) == NULL)
		    continue;	/* reported in postproc */
		for (i = 0; i < ED_spl(e)->size; i++) {
		    if (i > 0)
			agxbputc(&xb, ';');
		    if (ED_spl(e)->list[i].sflag) {
			s_arrows = 1;
			sprintf(buf, "s,%.5g,%.5g ",
				ED_spl(e)->list[i].sp.x,
				YDIR(ED_spl(e)->list[i].sp.y));
			agxbput(&xb, buf);
		    }
		    if (ED_spl(e)->list[i].eflag) {
			e_arrows = 1;
			sprintf(buf, "e,%.5g,%.5g ",
				ED_spl(e)->list[i].ep.x,
				YDIR(ED_spl(e)->list[i].ep.y));
			agxbput(&xb, buf);
		    }
		    for (j = 0; j < ED_spl(e)->list[i].size; j++) {
			if (j > 0)
			    agxbputc(&xb, ' ');
			ptf = ED_spl(e)->list[i].list[j];
			sprintf(buf, "%.5g,%.5g", ptf.x, YDIR(ptf.y));
			agxbput(&xb, buf);
		    }
		}
		agset(e, "pos", agxbuse(&xb));
		if (ED_label(e)) {
		    ptf = ED_label(e)->pos;
		    sprintf(buf, "%.5g,%.5g", ptf.x, YDIR(ptf.y));
		    agset(e, "lp", buf);
		}
		if (ED_xlabel(e) && ED_xlabel(e)->set) {
		    ptf = ED_xlabel(e)->pos;
		    sprintf(buf, "%.5g,%.5g", ptf.x, YDIR(ptf.y));
		    agset(e, "xlp", buf);
		}
		if (ED_head_label(e)) {
		    ptf = ED_head_label(e)->pos;
		    sprintf(buf, "%.5g,%.5g", ptf.x, YDIR(ptf.y));
		    agset(e, "head_lp", buf);
		}
		if (ED_tail_label(e)) {
		    ptf = ED_tail_label(e)->pos;
		    sprintf(buf, "%.5g,%.5g", ptf.x, YDIR(ptf.y));
		    agset(e, "tail_lp", buf);
		}
	    }
	}
    }
    rec_attach_bb(g, bbsym);
    agxbfree(&xb);

    if (HAS_CLUST_EDGE(g))
	undoClusterEdges(g);
    
    *sp = s_arrows;
    *ep = e_arrows;
    gv_fixLocale (0);
}

void attach_attrs(graph_t * g)
{
    int e, s;
    attach_attrs_and_arrows (g, &s, &e);
}

