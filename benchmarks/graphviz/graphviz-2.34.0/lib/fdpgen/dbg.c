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


/* dbg.c:
 * Written by Emden R. Gansner
 *
 * Simple debugging infrastructure
 */
#ifdef DEBUG

#define FDP_PRIVATE

#include <dbg.h>
#include <neatoprocs.h>
#include <fdp.h>
#include <math.h>

static int indent = -1;

void incInd()
{
    indent++;
}

void decInd()
{
    if (indent >= 0)
	indent--;
}

void prIndent(void)
{
    int i;
    for (i = 0; i < indent; i++)
	fputs("  ", stderr);
}

void prEdge(edge_t *e,char *s)
{
	fprintf(stderr,"%s --", agnameof(agtail(e)));
	fprintf(stderr,"%s%s", agnameof(aghead(e)),s);
}

static void dumpBB(graph_t * g)
{
    boxf bb;
    boxf b;

    bb = BB(g);
    b = GD_bb(g);
    prIndent();
    fprintf(stderr, "  LL (%f,%f)  UR (%f,%f)\n", bb.LL.x, bb.LL.y,
	    bb.UR.x, bb.UR.y);
    prIndent();
    fprintf(stderr, "  LL (%f,%f)  UR (%f,%f)\n", b.LL.x, b.LL.y,
	    b.UR.x, b.UR.y);
}

static void dumpSG(graph_t * g)
{
    graph_t *subg;
    int i;

    if (GD_n_cluster(g) == 0)
	return;
    prIndent();
    fprintf(stderr, "  {\n");
    for (i = 1; i <= GD_n_cluster(g); i++) {
	subg = (GD_clust(g))[i];
	prIndent();
	fprintf(stderr, "  subgraph %s : %d nodes\n", agnameof(subg),
		agnnodes(subg));
	dumpBB(subg);
	incInd ();
	dumpSG(subg);
	decInd ();
    }
    prIndent();
    fprintf(stderr, "  }\n");
}

/* dumpE:
 */
void dumpE(graph_t * g, int derived)
{
    Agnode_t *n;
    Agedge_t *e;
    Agedge_t **ep;
    Agedge_t *el;
    int i;
    int deg;

    prIndent();
    fprintf(stderr, "Graph %s : %d nodes %d edges\n", agnameof(g), agnnodes(g),
	    agnedges(g));
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	deg = 0;
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    deg++;
	    prIndent();
		prEdge(e,"\n");
	    if (derived) {
		for (i = 0, ep = (Agedge_t **) ED_to_virt(e);
		     i < ED_count(e); i++, ep++) {
		    el = *ep;
		    prIndent();
			prEdge(el,"\n");
		}
	    }
	}
	if (deg == 0) {		/* no out edges */
	    if (!agfstin(g, n))	/* no in edges */
		fprintf(stderr, " %s\n", agnameof(n));
	}
    }
    if (!derived) {
	bport_t *pp;
	if ((pp = PORTS(g))) {
	    int sz = NPORTS(g);
	    fprintf(stderr, "   %d ports\n", sz);
	    while (pp->e) {
		fprintf(stderr, "   %s : ", agnameof(pp->n));
		prEdge(pp->e,"\n");
		pp++;
	    }
	}
    }
}

/* dump:
 */
void dump(graph_t * g, int level, int doBB)
{
    node_t *n;
    boxf bb;
    double w, h;
    pointf pos;

    if (Verbose < level)
	return;
    prIndent();
    fprintf(stderr, "Graph %s : %d nodes\n", agnameof(g), agnnodes(g));
    dumpBB(g);
    if (Verbose > level) {
	incInd();
	dumpSG(g);
	decInd();
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    pos.x = ND_pos(n)[0];
	    pos.y = ND_pos(n)[1];
	    prIndent();
	    w = ND_width(n);
	    h = ND_height(n);
	    if (doBB) {
		bb.LL.x = pos.x - w / 2.0;
		bb.LL.y = pos.y - h / 2.0;
		bb.UR.x = bb.LL.x + w;
		bb.UR.y = bb.LL.y + h;
		fprintf(stderr, "%s: (%f,%f) ((%f,%f) , (%f,%f))\n",
			agnameof(n), pos.x, pos.y, bb.LL.x, bb.LL.y, bb.UR.x,
			bb.UR.y);
	    } else {
		fprintf(stderr, "%s: (%f,%f) (%f,%f) \n",
			agnameof(n), pos.x, pos.y, w, h);
	    }
	}
    }
}

void dumpG(graph_t * g, char *fname, int expMode)
{
    FILE *fp;

    fp = fopen(fname, "w");
    if (!fp) {
	fprintf(stderr, "Couldn not open %s \n", fname);
	exit(1);
    }
    outputGraph(g, fp, expMode);
    fclose(fp);
}

/* #define BOX */

/* static char* pos_name      = "pos"; */
/* static char* lp_name       = "lp"; */

double Scale = 0.0;
double ArrowScale = 1.0;

#define         ARROW_LENGTH    10
#define         ARROW_WIDTH      5
/* #define DEGREES(rad)   ((rad)/M_PI * 180.0) */

static char *plog = "%%!PS-Adobe-2.0\n\n\
/Times-Roman findfont 14 scalefont setfont\n\
/lLabel {\n\
\tmoveto\n\
\tgsave\n\
\tshow\n\
\tgrestore\n\
} def\n\
/inch {\n\
\t72 mul\n\
} def\n\
/doBox {\n\
\tnewpath\n\
\tmoveto\n\
\t/ht exch def\n\
\t/wd exch def\n\
\t0 ht rlineto\n\
\twd 0 rlineto\n\
\t0 0 ht sub rlineto\n\
\tclosepath\n\
\tgsave\n\
\t\t.9 setgray\n\
\t\tfill\n\
\tgrestore\n\
\tstroke\n\
} def\n\
/drawCircle {\n\
\t/r exch def\n\
\t/y exch def\n\
\t/x exch def\n\
\tnewpath\n\
\tx y r 0 360 arc\n\
\tstroke\n\
} def\n\
/fillCircle {\n\
\t/r exch def\n\
\t/y exch def\n\
\t/x exch def\n\
\tnewpath\n\
\tx y r 0 360 arc\n\
\tfill\n\
} def\n";

static char *elog = "showpage\n";

/*
static char* arrow = "/doArrow {\n\
\t/arrowwidth exch def\n\
\t/arrowlength exch def\n\
\tgsave\n\
\t\t3 1 roll\n\
\t\ttranslate\n\
\t\t\trotate\n\
\t\t\tnewpath\n\
\t\t\tarrowlength arrowwidth 2 div moveto\n\
\t\t\t0 0 lineto\n\
\t\t\tarrowlength arrowwidth -2 div lineto\n\
\t\tclosepath fill\n\
\t\tstroke\n\
\tgrestore\n\
} def\n";
*/

static double PSWidth = 550.0;
static double PSHeight = 756.0;

static void pswrite(Agraph_t * g, FILE * fp, int expMode)
{
    Agnode_t *n;
    Agnode_t *h;
    Agedge_t *e;
    double minx, miny, maxx, maxy;
    double scale, width, height;
    int do_arrow;
    int angle;
    char *p;
    double theta;
    double arrow_w, arrow_l;
    int portColor;

    fprintf(fp, plog);

/*
    if (agisdirected (g) && DoArrow) {
      do_arrow = 1;
      fprintf(fp,arrow);
    }
    else 
*/
    do_arrow = 0;

    n = agfstnode(g);
    minx = ND_pos(n)[0];
    miny = ND_pos(n)[1];
    maxx = ND_pos(n)[0];
    maxy = ND_pos(n)[1];
    n = agnxtnode(g, n);
    for (; n; n = agnxtnode(g, n)) {
	if (ND_pos(n)[0] < minx)
	    minx = ND_pos(n)[0];
	if (ND_pos(n)[1] < miny)
	    miny = ND_pos(n)[1];
	if (ND_pos(n)[0] > maxx)
	    maxx = ND_pos(n)[0];
	if (ND_pos(n)[1] > maxy)
	    maxy = ND_pos(n)[1];
    }

    /* Convert to points
     */
    minx *= POINTS_PER_INCH;
    miny *= POINTS_PER_INCH;
    maxx *= POINTS_PER_INCH;
    maxy *= POINTS_PER_INCH;

    /* Check for rotation
     */
    if ((p = agget(g, "rotate")) && (*p != '\0')
	&& ((angle = atoi(p)) != 0)) {
	fprintf(fp, "306 396 translate\n");
	fprintf(fp, "%d rotate\n", angle);
	fprintf(fp, "-306 -396 translate\n");
    }

    /* If user gives scale factor, use it.
     * Else if figure too large for standard PS page, scale it to fit.
     */
    if (Scale > 0.0)
	scale = Scale;
    else {
	width = maxx - minx + 20;
	height = maxy - miny + 20;
	if (width > PSWidth) {
	    if (height > PSHeight) {
		scale =
		    (PSWidth / width <
		     PSHeight / height ? PSWidth / width : PSHeight /
		     height);
	    } else
		scale = PSWidth / width;
	} else if (height > PSHeight) {
	    scale = PSHeight / height;
	} else
	    scale = 1.0;
    }

    fprintf(fp, "%f %f translate\n",
	    (PSWidth - scale * (minx + maxx)) / 2.0,
	    (PSHeight - scale * (miny + maxy)) / 2.0);
    fprintf(fp, "%f %f scale\n", scale, scale);

/*
    if (Verbose)
      fprintf (stderr, "Region (%f,%f) (%f,%f), scale %f\n", 
        minx, miny, maxx, maxy, scale);
*/

    if (do_arrow) {
	arrow_w = ArrowScale * ARROW_WIDTH / scale;
	arrow_l = ArrowScale * ARROW_LENGTH / scale;
    }

    fprintf(fp, "0.0 setlinewidth\n");
#ifdef SHOW_GRID
    if (UseGrid) {
	int i;
	fprintf(fp, "%f %f 5 fillCircle\n", 0.0, 0.0);
	for (i = 0; i < maxx; i += CellW) {
	    fprintf(fp, "%f 0.0 moveto %f %f lineto stroke\n",
		    (float) i, (float) i, maxy);
	}
	for (i = 0; i < maxy; i += CellH) {
	    fprintf(fp, "0.0 %f moveto %f %f lineto stroke\n",
		    (float) i, maxx, (float) i);
	}
    }
#endif
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (IS_PORT(n)) {
	    double r;
	    r = sqrt(ND_pos(n)[0] * ND_pos(n)[0] +
		     ND_pos(n)[1] * ND_pos(n)[1]);
	    fprintf(fp, "0 0 %f inch drawCircle\n", r);
	    break;
	}
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    h = aghead(e);
	    fprintf(fp, "%f inch %f inch moveto %f inch %f inch lineto\n",
		    ND_pos(n)[0], ND_pos(n)[1], ND_pos(h)[0],
		    ND_pos(h)[1]);
	    fprintf(fp, "stroke\n");
	    if (do_arrow) {
		theta =
		    atan2(ND_pos(n)[1] - ND_pos(h)[1],
			  ND_pos(n)[0] - ND_pos(h)[0]);
		fprintf(fp, "%f %f %.2f %.2f %.2f doArrow\n",
			ND_pos(h)[0], ND_pos(h)[1], DEGREES(theta),
			arrow_l, arrow_w);
	    }

	}
    }

#ifdef BOX
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	float wd, ht;

	data = getData(n);
	wd = data->wd;
	ht = data->ht;
	fprintf(fp, "%f %f %f %f doBox\n", wd, ht,
		data->pos.x - (wd / 2), data->pos.y - (ht / 2));
    }
#else
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	fprintf(fp, "%% %s\n", agnameof(n));
	if (expMode) {
	    double wd, ht;
	    double r;
	    wd = ND_width(n);
	    ht = ND_height(n);
	    r = sqrt((wd * wd / 4) + ht * ht / 4);
	    fprintf(fp, "%f inch %f inch %f inch %f inch doBox\n", wd, ht,
		    ND_pos(n)[0] - (wd / 2), ND_pos(n)[1] - (ht / 2));
	    fprintf(fp, "%f inch %f inch %f inch drawCircle\n",
		    ND_pos(n)[0], ND_pos(n)[1], r);
	} else {
	    if (IS_PORT(n)) {
		if (!portColor) {
		    fprintf(fp, "0.667 1.000 1.000 sethsbcolor\n");
		    portColor = 1;
		}
	    } else {
		if (portColor) {
		    fprintf(fp, "0.0 0.000 0.000 sethsbcolor\n");
		    portColor = 0;
		}
	    }
	}
	fprintf(fp, "%f inch %f inch %f fillCircle\n", ND_pos(n)[0],
		ND_pos(n)[1], 3 / scale);
    }
#endif

    fprintf(fp, "0.667 1.000 1.000 sethsbcolor\n");
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    h = aghead(e);
	    fprintf(fp, "%f inch %f inch moveto %f inch %f inch lineto\n",
		    ND_pos(n)[0], ND_pos(n)[1], ND_pos(h)[0],
		    ND_pos(h)[1]);
	    fprintf(fp, "stroke\n");
	    if (do_arrow) {
		theta =
		    atan2(ND_pos(n)[1] - ND_pos(h)[1],
			  ND_pos(n)[0] - ND_pos(h)[0]);
		fprintf(fp, "%f %f %.2f %.2f %.2f doArrow\n",
			ND_pos(h)[0], ND_pos(h)[1], DEGREES(theta),
			arrow_l, arrow_w);
	    }

	}
    }

    fprintf(fp, elog);
}

void outputGraph(Agraph_t * g, FILE * fp, int expMode)
{
    pswrite(g, fp, expMode);
}

#endif				/* DEBUG */
