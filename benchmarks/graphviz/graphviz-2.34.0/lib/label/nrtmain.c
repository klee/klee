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

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <memory.h>
#include <gvc.h>
#include "xlabels.h"

#if 0
#define POINTS_PER_INCH 72
#define N_NEW(n,t)       (t*)calloc((n),sizeof(t))
#define MIN(a,b)        ((a)<(b)?(a):(b))
#define MAX(a,b)        ((a)>(b)?(a):(b))
#define INT_MAX         ((int)(~(unsigned)0 >> 1))
#define INCH2PS(a_inches)       ((a_inches)*(double)POINTS_PER_INCH)
#endif

static char *progname;
static int Verbose;
extern pointf edgeMidpoint(graph_t * g, edge_t * e);

static inline pointf pointfof(double x, double y)
{
    pointf r;

    r.x = x;
    r.y = y;
    return r;
}

typedef struct {
    GVC_t *gvc;
    char *infname;
    char *outfname;
    FILE *inf;
    FILE *outf;
    char *lay;
    char *fmt;
    int force;
} opts_t;

static pointf centerPt(xlabel_t * xlp)
{
    pointf p;

    p = xlp->pos;
    p.x += (xlp->sz.x) / 2.0;
    p.y += (xlp->sz.y) / 2.0;

    return p;
}

static int
printData(object_t * objs, int n_objs, xlabel_t * lbls, int n_lbls,
	  label_params_t * params)
{
    int i;
    fprintf(stderr,
	    "%d objs %d xlabels force=%d bb=(%.02f,%.02f) (%.02f,%.02f)\n",
	    n_objs, n_lbls, params->force, params->bb.LL.x,
	    params->bb.LL.y, params->bb.UR.x, params->bb.UR.y);
    if (Verbose < 2)
	return 0;
    fprintf(stderr, "objects\n");
    for (i = 0; i < n_objs; i++) {
      if(objs[i].lbl && objs[i].lbl->lbl)
	fprintf (stderr, " [%d] %p %p (%.02f, %.02f) (%.02f, %.02f) %s\n",
		 i, &objs[i], objs[i].lbl, objs[i].pos.x,objs[i].pos.y,
		 objs[i].sz.x,objs[i].sz.y,
		 ((textlabel_t*)objs[i].lbl->lbl)->text );  
      else
	fprintf (stderr, " [%d] %p %p (%.02f, %.02f) (%.02f, %.02f)\n",
		 i, &objs[i], objs[i].lbl, objs[i].pos.x,objs[i].pos.y,
		 objs[i].sz.x,objs[i].sz.y);  
    }
    fprintf(stderr, "xlabels\n");
    for (i = 0; i < n_lbls; i++) {
	fprintf(stderr, " [%d] %p (%.02f, %.02f) (%.02f, %.02f) %s\n",
		i, &lbls[i], lbls[i].pos.x, lbls[i].pos.y,
		lbls[i].sz.x, lbls[i].sz.y,
		((textlabel_t *)lbls[i].lbl)->text);
    }
    return 0;
}

int doxlabel(opts_t * opts)
{
    Agraph_t *gp;
    object_t *objs;
    xlabel_t *lbls;
    int i, n_objs, n_lbls;
    label_params_t params;
    Agnode_t *np;
    Agedge_t *ep;
    int n_nlbls = 0, n_elbls = 0;
    boxf bb;
    textlabel_t *lp;
    object_t *objp;
    xlabel_t *xlp;
    pointf ur;

    fprintf(stderr, "reading %s\n", opts->infname);
    if (!(gp = agread(opts->inf))) {
	fprintf(stderr, "%s: %s not a dot file\n", progname,
		opts->infname);
	exit(1);
    }
    fclose(opts->inf);

    fprintf(stderr, "laying out %s\n", opts->lay);
    if (gvLayout(opts->gvc, gp, opts->lay)) {
	fprintf(stderr, "%s: layout %s failed\n", progname, opts->lay);
	exit(1);
    }

    fprintf(stderr, "attach labels\n");
    /* In the real code, this should be optimized using GD_has_labels() */
    /* We could probably provide the number of node and edge xlabels */
    for (np = agfstnode(gp); np; np = agnxtnode(gp, np)) {
	if (ND_xlabel(np))
	    n_nlbls++;
	for (ep = agfstout(gp, np); ep; ep = agnxtout(gp, ep)) {
	    if (ED_xlabel(ep))
		n_elbls++;
	}
    }
    n_objs = agnnodes(gp) + n_elbls;
    n_lbls = n_nlbls + n_elbls;
    objp = objs = N_NEW(n_objs, object_t);
    xlp = lbls = N_NEW(n_lbls, xlabel_t);
    bb.LL = pointfof(INT_MAX, INT_MAX);
    bb.UR = pointfof(-INT_MAX, -INT_MAX);

    for (np = agfstnode(gp); np; np = agnxtnode(gp, np)) {
	/* Add an obstacle per node */
	objp->sz.x = INCH2PS(ND_width(np));
	objp->sz.y = INCH2PS(ND_height(np));
	objp->pos = ND_coord(np);
	objp->pos.x -= (objp->sz.x) / 2.0;
	objp->pos.y -= (objp->sz.y) / 2.0;

	/* Adjust bounding box */
	bb.LL.x = MIN(bb.LL.x, objp->pos.x);
	bb.LL.y = MIN(bb.LL.y, objp->pos.y);
	ur.x = objp->pos.x + objp->sz.x;
	ur.y = objp->pos.y + objp->sz.y;
	bb.UR.x = MAX(bb.UR.x, ur.x);
	bb.UR.y = MAX(bb.UR.y, ur.y);

	if (ND_xlabel(np)) {
	    xlp->sz = ND_xlabel(np)->dimen;
	    xlp->lbl = ND_xlabel(np);
	    xlp->set = 0;
	    objp->lbl = xlp;
	    xlp++;
	}
	objp++;
	for (ep = agfstout(gp, np); ep; ep = agnxtout(gp, ep)) {
	    if (ED_label(ep)) {
		textlabel_t *lp = ED_label(ep);
		lp->pos.x = lp->pos.y = 0.0;
	    }
	    if (!ED_xlabel(ep))
		continue;
	    objp->sz.x = 0;
	    objp->sz.y = 0;
	    objp->pos = edgeMidpoint(gp, ep);

	    xlp->sz = ED_xlabel(ep)->dimen;
	    xlp->lbl = ED_xlabel(ep);
	    xlp->set = 0;
	    objp->lbl = xlp;
	    xlp++;
	    objp++;
	}
    }

    params.force = opts->force;
    params.bb = bb;
    if (Verbose)
	printData(objs, n_objs, lbls, n_lbls, &params);
    placeLabels(objs, n_objs, lbls, n_lbls, &params);

    fprintf(stderr, "read label positions\n");
    xlp = lbls;
    for (i = 0; i < n_lbls; i++) {
	if (xlp->set) {
	    lp = (textlabel_t *) (xlp->lbl);
	    lp->set = 1;
	    lp->pos = centerPt(xlp);
	}
	xlp++;
    }
    free(objs);
    free(lbls);

    fprintf(stderr, "writing %s\n", opts->outfname);
    gvRender(opts->gvc, gp, opts->fmt, opts->outf);

    /* clean up */
    fprintf(stderr, "cleaning up\n");
    gvFreeLayout(opts->gvc, gp);
    agclose(gp);
    return 0;
}

int checkOpt(char *l, char *legal[], int n)
{
    int i;
    for (i = 0; i < n; i++) {
	if (strcmp(l, legal[i]) == 0)
	    return 1;
    }
    return 0;
}

void usage(char *pp)
{
    fprintf(stderr,
	    "Usage: %s [-fv] [-V$level] [-T$fmt] [-l$layout] [-o$outfile] dotfile\n",
	    pp);
    return;
}

static FILE *openFile(char *name, char *mode)
{
    FILE *fp;
    char *modestr;

    fp = fopen(name, mode);
    if (!fp) {
	if (*mode == 'r')
	    modestr = "reading";
	else
	    modestr = "writing";
	fprintf(stderr, "%s: could not open file %s for %s -- %s\n",
		progname, name, modestr, strerror(errno));
	exit(1);
    }
    return (fp);
}

static void init(int argc, char *argv[], opts_t * opts)
{
    int c, cnt;
    char **optList;
    opts->outf = stdout;
    opts->outfname = "stdout";

    progname = argv[0];
    opts->gvc = gvContext();
    opts->lay = "dot";
    opts->fmt = "ps";
    opts->force = 0;

    while ((c = getopt(argc, argv, "o:vFl:T:V:")) != EOF) {
	switch (c) {
	case 'F':
	    opts->force = 1;
	    break;
	case 'l':
	    optList = gvPluginList(opts->gvc, "layout", &cnt, NULL);
	    if (checkOpt(optarg, optList, cnt))
		opts->lay = optarg;
	    else {
		fprintf(stderr, "%s: unknown layout %s\n", progname,
			optarg);
		exit(1);
	    }
	    break;
	case 'T':
	    optList = gvPluginList(opts->gvc, "device", &cnt, NULL);
	    if (checkOpt(optarg, optList, cnt))
		opts->fmt = optarg;
	    else {
		fprintf(stderr, "%s: unknown format %s\n", progname,
			optarg);
		exit(1);
	    }
	    break;
	case 'v':
	    Verbose = 1;
	    break;
	case 'V':
	    Verbose = atoi(optarg);
	    break;
	case 'o':
	    opts->outf = openFile(optarg, "w");
	    opts->outfname = optarg;
	    break;
	default:
	    usage(progname);
	    exit(1);
	}
    }

    if (optind < argc) {
	opts->inf = openFile(argv[optind], "r");
	opts->infname = argv[optind];
    } else {
	opts->inf = stdin;
	opts->infname = "<stdin>";
    }

    if (!opts->outf) {
	opts->outf = stdout;
	opts->outfname = "<stdout>";
    }
}

int main(int argc, char *argv[])
{
    opts_t opts;

    init(argc, argv, &opts);
    doxlabel(&opts);
    return 0;
}
