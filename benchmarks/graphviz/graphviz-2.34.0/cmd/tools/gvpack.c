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
 * Written by Emden R. Gansner
 */



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "compat_getopt.h"
#endif

#include <assert.h>
#include "gvc.h"
#include "render.h"
#include "neatoprocs.h"
#include "ingraphs.h"
#include "pack.h"

#ifndef WITH_CGRAPH
extern Agdict_t *agdictof(void *);
#endif


#ifdef WIN32 /*dependencies*/
#ifdef WITH_CGRAPH
    #pragma comment( lib, "cgraph.lib" )
#else
    #pragma comment( lib, "graph.lib" )
#endif
    #pragma comment( lib, "gvc.lib" )
    #pragma comment( lib, "ingraphs.lib" )
    #pragma comment( lib, "neatogen.lib" )
    #pragma comment( lib, "pathplan.lib" )
    #pragma comment( lib, "vpsc.lib" )
    #pragma comment( lib, "sparse.lib" )
    #pragma comment( lib, "gts.lib" )
    #pragma comment( lib, "glib-2.0.lib" )
    #pragma comment( lib, "cdt.lib" )
    #pragma comment( lib, "gvplugin_neato_layout.lib" )


#endif


/* gvpack:
 * Input consists of graphs in dot format.
 * The graphs should have pos, width and height set for nodes, 
 * bb set for clusters, and, optionally, spline info for edges.
 * The graphs are packed geometrically and combined
 * into a single output graph, ready to be sent to neato -s -n2.
 *  -m <i> specifies the margin, in points, about each graph.
 */

typedef struct {
    Dtlink_t link;
    char *name;
    char *value;
} attr_t;

typedef struct {
    Dtlink_t link;
    char *name;
    int cnt;
} pair_t;

static int verbose = 0;
static char **myFiles = 0;
static int nGraphs = 0;		/* Guess as to no. of graphs */
static FILE *outfp;		/* output; stdout by default */
#ifndef WITH_CGRAPH
static int kind;		/* type of graph */
#else
static Agdesc_t kind;		/* type of graph */
#endif
static int G_cnt;		/* No. of -G arguments */
static int G_sz;		/* Storage size for -G arguments */
static attr_t *G_args;		/* Storage for -G arguments */
static int doPack;              /* Do packing if true */
static char* gname = "root";

#define NEWNODE(n) ((node_t*)ND_alg(n))

static char *useString =
    "Usage: gvpack [-gnuv?] [-m<margin>] {-array[_rc][n]] [-o<outf>] <files>\n\
  -n          - use node granularity\n\
  -g          - use graph granularity\n\
  -array*     - pack as array of graphs\n\
  -G<n>=<v>   - attach name/value attribute to output graph\n\
  -m<n>       - set margin to <n> points\n\
  -s<gname>   - use <gname> for name of root graph\n\
  -o<outfile> - write output to <outfile>\n\
  -u          - no packing; just combine graphs\n\
  -v          - verbose\n\
  -?          - print usage\n\
If no files are specified, stdin is used\n";

static void usage(int v)
{
    printf("%s",useString);
    exit(v);
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
	fprintf(stderr, "gvpack: could not open file %s for %s\n",
		name, modestr);
	exit(1);
    }
    return (fp);
}

#define G_CHUNK 10

/* setNameValue:
 * If arg is a name-value pair, add it to the list
 * and return 0; otherwise, return 1.
 */
static int setNameValue(char *arg)
{
    char *p;
    char *rhs = "true";

    if ((p = strchr(arg, '='))) {
	*p++ = '\0';
	rhs = p;
    }
    if (G_cnt >= G_sz) {
	G_sz += G_CHUNK;
	G_args = RALLOC(G_sz, G_args, attr_t);
    }
    G_args[G_cnt].name = arg;
    G_args[G_cnt].value = rhs;
    G_cnt++;

    return 0;
}

/* setUInt:
 * If arg is an integer, value is stored in v
 * and function returns 0; otherwise, returns 1.
 */
static int setUInt(unsigned int *v, char *arg)
{
    char *p;
    unsigned int i;

    i = (unsigned int) strtol(arg, &p, 10);
    if (p == arg) {
	fprintf(stderr, "Error: bad value in flag -%s - ignored\n",
		arg - 1);
	return 1;
    }
    *v = i;
    return 0;
}

#ifdef WITH_CGRAPH
static Agsym_t *agraphattr(Agraph_t *g, char *name, char *value)
{
    return agattr(g, AGRAPH, name, value);
}

static Agsym_t *agnodeattr(Agraph_t *g, char *name, char *value)
{
    return agattr(g, AGNODE, name, value);
}

static Agsym_t *agedgeattr(Agraph_t *g, char *name, char *value)
{
    return agattr(g, AGEDGE, name, value);
}

#endif

/* init:
 */
static void init(int argc, char *argv[], pack_info* pinfo)
{
    int c, len;
    char buf[BUFSIZ];
    char* bp;

#ifndef WITH_CGRAPH
    aginit();
#endif
    agnodeattr(NULL, "label", NODENAME_ESC);
    pinfo->mode = l_clust;
    pinfo->margin = CL_OFFSET;
    pinfo->doSplines = TRUE; /* Use edges in packing */
    pinfo->fixed = 0;

    opterr = 0;
    while ((c = getopt(argc, argv, ":na:gvum:s:o:G:")) != -1) {
	switch (c) {
	case 'a':
	    len = strlen(optarg) + 2;
	    if (len > BUFSIZ)
		bp = N_GNEW(len, char);
	    else
		bp = buf;
	    sprintf (bp, "a%s\n", optarg);
	    parsePackModeInfo (bp, pinfo->mode, pinfo);
	    if (bp != buf)
		free (bp);
	    break;
	case 'n':
	    pinfo->mode = l_node;
	    break;
	case 's':
	    gname = optarg;
	    break;
	case 'g':
	    pinfo->mode = l_graph;
	    break;
	case 'm':
	    setUInt(&pinfo->margin, optarg);
	    break;
	case 'o':
	    outfp = openFile(optarg, "w");
	    break;
	case 'u':
	    pinfo->mode = l_undef;
	    break;
	case 'G':
	    if (*optarg)
		setNameValue(optarg);
	    else
		fprintf(stderr,
			"gvpack: option -G missing argument - ignored\n");
	    break;
	case 'v':
	    verbose = 1;
	    Verbose = 1;
	    break;
	case ':':
	    fprintf(stderr, "gvpack: option -%c missing argument - ignored\n", optopt);
	    break;
	case '?':
	    if (optopt == '?')
		usage(0);
	    else
		fprintf(stderr,
			"gvpack: option -%c unrecognized - ignored\n", optopt);
	    break;
	}
    }
    argv += optind;
    argc -= optind;

    if (argc > 0) {
	myFiles = argv;
	nGraphs = argc;		/* guess one graph per file */
    } else
	nGraphs = 10;		/* initial guess as to no. of graphs */
    if (!outfp)
	outfp = stdout;		/* stdout the default */
}

/* init_node_edge:
 * initialize node and edge attributes
 */
static void init_node_edge(Agraph_t * g)
{
    node_t *n;
    edge_t *e;
    int nG = agnnodes(g);
    attrsym_t *N_pos = agfindnodeattr(g, "pos");
    attrsym_t *N_pin = agfindnodeattr(g, "pin");

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	neato_init_node(n);
	user_pos(N_pos, N_pin, n, nG);	/* set user position if given */
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e))
	    common_init_edge(e);
    }
}

/* init_graph:
 * Initialize attributes. We always do the minimum required by
 * libcommon. If fill is true, we use init_nop (neato -n) to
 * read in attributes relevant to the layout.
 */
static void init_graph(Agraph_t * g, boolean fill, GVC_t* gvc)
{
    int d;
    node_t *n;
    edge_t *e;

#ifdef WITH_CGRAPH
    aginit (g, AGRAPH, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);
    aginit (g, AGNODE, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);
    aginit (g, AGEDGE, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);
#endif
    GD_gvc(g) = gvc;
    graph_init(g, FALSE);
    d = late_int(g, agfindgraphattr(g, "dim"), 2, 2);
    if (d != 2) {
	fprintf(stderr, "Error: graph %s has dim = %d (!= 2)\n", agnameof(g),
		d);
	exit(1);
    }
    Ndim = GD_ndim(g) = 2;
    init_node_edge(g);
    if (fill) {
	int ret = init_nop(g,0);
	if (ret) {
	    if (ret < 0)
		fprintf(stderr, "Error loading layout info from graph %s\n",
		    agnameof(g));
	    else if (ret > 0)
		fprintf(stderr, "gvpack does not support backgrounds as found in graph %s\n",
		    agnameof(g));
	    exit(1);
	}
	if (Concentrate) { /* check for edges without pos info */
	    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
		for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
		    if (ED_spl(e) == NULL) ED_edge_type(e) = IGNORED;
		}
	    }
	}
    }
}

/* cloneAttrs:
 * Copy all attributes from old object to new. Assume
 * attributes have been initialized.
 */
#ifdef WITH_CGRAPH
static void cloneDfltAttrs(Agraph_t *old, Agraph_t *new, int kind)
{
    Agsym_t *a;

    for (a = agnxtattr(old, kind, 0); a; a =  agnxtattr(old, kind, a)) {
	if (aghtmlstr(a->defval))
	    agattr (new, kind, a->name, agstrdup_html(new, a->defval));
	else
	    agattr (new, kind, a->name, a->defval);
    }
}
static void cloneAttrs(void *old, void *new)
{
    int kind = AGTYPE(old);
    Agsym_t *a;
    char* s;
    Agraph_t *g = agroot(old);
    Agraph_t *ng = agroot(new);

    for (a = agnxtattr(g, kind, 0); a; a =  agnxtattr(g, kind, a)) {
	s = agxget (old, a);
	if (aghtmlstr(s))
	    agset(new, a->name, agstrdup_html(ng, s));
	else
	    agset(new, a->name, s);
    }
}
#else
static void cloneAttrs(void *old, void *new)
{
    int j;
    Agsym_t *a;
    Agdict_t *dict = agdictof(old);

    for (j = 0; j < dtsize(dict->dict); j++) {
	a = dict->list[j];
	agset(new, a->name, agxget(old, a->index));
    }
}
#endif

/* cloneEdge:
 * Note that here, and in cloneNode and cloneCluster,
 * we do a shallow copy. We thus assume that attributes
 * are not disturbed. In particular, we assume they are
 * not deallocated.
 */
static void cloneEdge(Agedge_t * old, Agedge_t * new)
{
    cloneAttrs(old, new);
    ED_spl(new) = ED_spl(old);
    ED_edge_type(new) = ED_edge_type(old);
    ED_label(new) = ED_label(old);
    ED_head_label(new) = ED_head_label(old);
    ED_tail_label(new) = ED_tail_label(old);
}

/* cloneNode:
 */
static void cloneNode(Agnode_t * old, Agnode_t * new)
{
    cloneAttrs(old, new);
    ND_coord(new).x = POINTS(ND_pos(old)[0]);
    ND_coord(new).y = POINTS(ND_pos(old)[1]);
    ND_height(new) = ND_height(old);
    ND_ht(new) = ND_ht(old);
    ND_width(new) = ND_width(old);
    ND_lw(new) = ND_lw(old);
    ND_rw(new) = ND_rw(old);
    ND_shape(new) = ND_shape(old);
    ND_shape_info(new) = ND_shape_info(old);
}

/* cloneCluster:
 */
static void cloneCluster(Agraph_t * old, Agraph_t * new)
{
    /* string attributes were cloned as subgraphs */
    GD_label(new) = GD_label(old);
    GD_bb(new) = GD_bb(old);
}

/* freef:
 * Generic free function for dictionaries.
 */
static void freef(Dt_t * dt, Void_t * obj, Dtdisc_t * disc)
{
    free(obj);
}

static Dtdisc_t attrdisc = {
    offsetof(attr_t, name),	/* key */
    -1,				/* size */
    offsetof(attr_t, link),	/* link */
    (Dtmake_f) 0,
    (Dtfree_f) freef,
    (Dtcompar_f) 0,		/* use strcmp */
    (Dthash_f) 0,
    (Dtmemory_f) 0,
    (Dtevent_f) 0
};

/* fillDict:
 * Fill newdict with all the name-value attributes of
 * objp. If the attribute has already been defined and
 * has a different default, set default to "".
 */
#ifdef WITH_CGRAPH
static void fillDict(Dt_t * newdict, Agraph_t* g, int kind)
{
    Agsym_t *a;
    char *name;
    char *value;
    attr_t *rv;

    for (a = agnxtattr(g,kind,0); a; a = agnxtattr(g,kind,a)) {
	name = a->name;
	value = a->defval;
	rv = (attr_t *) dtmatch(newdict, name);
	if (!rv) {
	    rv = NEW(attr_t);
	    rv->name = name;
	    rv->value = value;
	    dtinsert(newdict, rv);
	} else if (strcmp(value, rv->value))
	    rv->value = "";
    }
}
#else
static void fillDict(Dt_t * newdict, void *objp)
{
    int j;
    Agsym_t *a;
    Agdict_t *olddict = agdictof(objp);
    char *name;
    char *value;
    attr_t *rv;

    for (j = 0; j < dtsize(olddict->dict); j++) {
	a = olddict->list[j];
	name = a->name;
	value = agxget(objp, a->index);
	rv = (attr_t *) dtmatch(newdict, name);
	if (!rv) {
	    rv = NEW(attr_t);
	    rv->name = name;
	    rv->value = value;
	    dtinsert(newdict, rv);
	} else if (strcmp(value, rv->value))
	    rv->value = "";
    }
}
#endif

/* fillGraph:
 * Use all the name-value entries in the dictionary d to define
 * to define universal node/edge/graph attributes for g.
 */
static void
fillGraph(Agraph_t * g, Dt_t * d,
	  Agsym_t * (*setf) (Agraph_t *, char *, char *))
{
    attr_t *av;
    for (av = (attr_t *) dtflatten(d); av;
	 av = (attr_t *) dtlink(d, (Dtlink_t *) av)) {
	setf(g, av->name, av->value);
    }
}

/* initAttrs:
 * Initialize the attributes of root as the union of the
 * attributes in the graphs gs.
 */
static void initAttrs(Agraph_t * root, Agraph_t ** gs, int cnt)
{
    Agraph_t *g;
    Dt_t *n_attrs;
    Dt_t *e_attrs;
    Dt_t *g_attrs;
    int i;

    n_attrs = dtopen(&attrdisc, Dtoset);
    e_attrs = dtopen(&attrdisc, Dtoset);
    g_attrs = dtopen(&attrdisc, Dtoset);

    for (i = 0; i < cnt; i++) {
	g = gs[i];
#ifdef WITH_CGRAPH
	fillDict(g_attrs, g, AGRAPH);
	fillDict(n_attrs, g, AGNODE);
	fillDict(e_attrs, g, AGEDGE);
#else
	fillDict(g_attrs, g);
	fillDict(n_attrs, g->proto->n);
	fillDict(e_attrs, g->proto->e);
#endif
    }

    fillGraph(root, g_attrs, agraphattr);
    fillGraph(root, n_attrs, agnodeattr);
    fillGraph(root, e_attrs, agedgeattr);

    dtclose(n_attrs);
    dtclose(e_attrs);
    dtclose(g_attrs);
}

/* cloneGraphAttr:
 */
static void cloneGraphAttr(Agraph_t * g, Agraph_t * ng)
{
    cloneAttrs(g, ng);
#ifdef WITH_CGRAPH
    cloneDfltAttrs(g, ng, AGNODE);
    cloneDfltAttrs(g, ng, AGEDGE);
#else
    cloneAttrs(g->proto->n, ng->proto->n);
    cloneAttrs(g->proto->e, ng->proto->e);
#endif
}

#ifdef UNIMPL
/* redoBB:
 * If "bb" attribute is valid, translate to reflect graph's
 * new packed position.
 */
static void redoBB(Agraph_t * g, char *s, Agsym_t * G_bb, point delta)
{
    box bb;
    char buf[100];

    if (sscanf(s, "%d,%d,%d,%d", &bb.LL.x, &bb.LL.y, &bb.UR.x, &bb.UR.y) ==
	4) {
	bb.LL.x += delta.x;
	bb.LL.y += delta.y;
	bb.UR.x += delta.x;
	bb.UR.y += delta.y;
	sprintf(buf, "%d,%d,%d,%d", bb.LL.x, bb.LL.y, bb.UR.x, bb.UR.y);
	agxset(g, G_bb->index, buf);
    }
}
#endif

/* xName:
 * Create a name for an object in the new graph using the
 * dictionary names and the old name. If the old name has not
 * been used, use it and add it to names. If it has been used,
 * create a new name using the old name and a number.
 * Note that returned string will immediately made into an agstring.
 */
static char *xName(Dt_t * names, char *oldname)
{
    static char* name = NULL;
    static int namelen = 0;
    char *namep;
    pair_t *p;
    int len;

    p = (pair_t *) dtmatch(names, oldname);
    if (p) {
	p->cnt++;
	len = strlen(oldname) + 100; /* 100 for "_gv" and decimal no. */
	if (namelen < len) {
	    if (name) free (name);
	    name = N_NEW(len, char);
	    namelen = len;
	}
	sprintf(name, "%s_gv%d", oldname, p->cnt);
	namep = name;
    } else {
	p = NEW(pair_t);
	p->name = oldname;
	dtinsert(names, p);
	namep = oldname;
    }
    return namep;
}

#define MARK(e) (ED_alg(e) = e)
#define MARKED(e) (ED_alg(e))
#define ISCLUSTER(g) (!strncmp(agnameof(g),"cluster",7))
#define SETCLUST(g,h) (GD_alg(g) = h)
#define GETCLUST(g) ((Agraph_t*)GD_alg(g))

/* cloneSubg:
 * Create a copy of g in ng, copying attributes, inserting nodes
 * and adding edges.
 */
static void
cloneSubg(Agraph_t * g, Agraph_t * ng, Agsym_t * G_bb, Dt_t * gnames)
{
#ifndef WITH_CGRAPH
    graph_t *mg;
    edge_t *me;
    node_t *mn;
#endif
    node_t *n;
    node_t *nn;
    edge_t *e;
    edge_t *ne;
    node_t *nt;
    node_t *nh;
    Agraph_t *subg;
    Agraph_t *nsubg;

    cloneGraphAttr(g, ng);
    if (doPack)
#ifdef WITH_CGRAPH
	agxset(ng, G_bb, "");	/* Unset all subgraph bb */
#else
	agxset(ng, G_bb->index, "");	/* Unset all subgraph bb */
#endif

    /* clone subgraphs */
#ifdef WITH_CGRAPH
    for (subg = agfstsubg (g); subg; subg = agfstsubg (subg)) {
	nsubg = agsubg(ng, xName(gnames, agnameof(subg)), 1);
#else
    mg = g->meta_node->graph;
    for (me = agfstout(mg, g->meta_node); me; me = agnxtout(mg, me)) {
	mn = aghead(me);
	subg = agusergraph(mn);
	nsubg = agsubg(ng, xName(gnames, agnameof(subg)));
#endif
	cloneSubg(subg, nsubg, G_bb, gnames);
	/* if subgraphs are clusters, point to the new 
	 * one so we can find it later.
	 */
	if (ISCLUSTER(subg))
	    SETCLUST(subg, nsubg);
    }

    /* add remaining nodes */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	nn = NEWNODE(n);
#ifndef WITH_CGRAPH
	if (!agfindnode(ng, agnameof(nn)))
	    aginsert(ng, nn);
#else
        agsubnode(ng, nn, 1);
#endif
    }

    /* add remaining edges. libgraph doesn't provide a way to find
     * multiedges, so we add edges bottom up, marking edges when added.
     */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (MARKED(e))
		continue;
	    nt = NEWNODE(agtail(e));
	    nh = NEWNODE(aghead(e));
#ifndef WITH_CGRAPH
	    ne = agedge(ng, nt, nh);
#else
	    ne = agedge(ng, nt, nh, NULL, 1);
	    agbindrec (ne, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);
#endif
	    cloneEdge(e, ne);
	    MARK(e);
	}
    }
}

/* cloneClusterTree:
 * Given old and new subgraphs which are corresponding
 * clusters, recursively create the subtree of clusters
 * under ng using the subtree of clusters under g.
 */
static void cloneClusterTree(Agraph_t * g, Agraph_t * ng)
{
    int i;

    cloneCluster(g, ng);

    if (GD_n_cluster(g)) {
	GD_n_cluster(ng) = GD_n_cluster(g);
	GD_clust(ng) = N_NEW(1 + GD_n_cluster(g), Agraph_t *);
	for (i = 1; i <= GD_n_cluster(g); i++) {
	    Agraph_t *c = GETCLUST(GD_clust(g)[i]);
	    GD_clust(ng)[i] = c;
	    cloneClusterTree(GD_clust(g)[i], c);
	}
    }
}

static Dtdisc_t pairdisc = {
    offsetof(pair_t, name),	/* key */
    -1,				/* size */
    offsetof(attr_t, link),	/* link */
    (Dtmake_f) 0,
    (Dtfree_f) freef,
    (Dtcompar_f) 0,		/* use strcmp */
    (Dthash_f) 0,
    (Dtmemory_f) 0,
    (Dtevent_f) 0
};

/* cloneGraph:
 * Create and return a new graph which is the logical union
 * of the graphs gs. 
 */
static Agraph_t *cloneGraph(Agraph_t ** gs, int cnt, GVC_t * gvc)
{
    Agraph_t *root;
    Agraph_t *g;
    Agraph_t *subg;
    Agnode_t *n;
    Agnode_t *np;
    int i;
    Dt_t *gnames;		/* dict of used subgraph names */
    Dt_t *nnames;		/* dict of used node names */
    Agsym_t *G_bb;
    Agsym_t *rv;
    boolean doWarn = TRUE;

    if (verbose)
	fprintf(stderr, "Creating clone graph\n");
#ifndef WITH_CGRAPH
    root = agopen(gname, kind);
#else
    root = agopen(gname, kind, &AgDefaultDisc);
#endif
    initAttrs(root, gs, cnt);
    G_bb = agfindgraphattr(root, "bb");
    if (doPack) assert(G_bb);

    /* add command-line attributes */
    for (i = 0; i < G_cnt; i++) {
	rv = agfindgraphattr(root, G_args[i].name);
	if (rv)
#ifndef WITH_CGRAPH
	    agxset(root, rv->index, G_args[i].value);
	else
	    agraphattr(root, G_args[i].name, G_args[i].value);
#else
	    agxset(root, rv, G_args[i].value);
	else
	    agattr(root, AGRAPH, G_args[i].name, G_args[i].value);
#endif
    }

    /* do common initialization. This will handle root's label. */
    init_graph(root, FALSE, gvc);
    State = GVSPLINES;

    gnames = dtopen(&pairdisc, Dtoset);
    nnames = dtopen(&pairdisc, Dtoset);
    for (i = 0; i < cnt; i++) {
	g = gs[i];
	if (verbose)
	    fprintf(stderr, "Cloning graph %s\n", agnameof(g));
	GD_n_cluster(root) += GD_n_cluster(g);

	/* Clone nodes, checking for node name conflicts */
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (doWarn && agfindnode(root, agnameof(n))) {
		fprintf(stderr,
			"Warning: node %s in graph[%d] %s already defined\n",
			agnameof(n), i, agnameof(g));
		fprintf(stderr, "Some nodes will be renamed.\n");
		doWarn = FALSE;
	    }
#ifndef WITH_CGRAPH
	    np = agnode(root, xName(nnames, agnameof(n)));
#else
	    np = agnode(root, xName(nnames, agnameof(n)), 1);
	    agbindrec (np, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);
#endif
	    ND_alg(n) = np;
	    cloneNode(n, np);
	}

	/* wrap the clone of g in a subgraph of root */
#ifndef WITH_CGRAPH
	subg = agsubg(root, xName(gnames, agnameof(g)));
#else
	subg = agsubg(root, xName(gnames, agnameof(g)), 1);
#endif
	cloneSubg(g, subg, G_bb, gnames);
    }
    dtclose(gnames);
    dtclose(nnames);

    /* set up cluster tree */
    if (GD_n_cluster(root)) {
	int j, idx;
	GD_clust(root) = N_NEW(1 + GD_n_cluster(root), graph_t *);

	idx = 1;
	for (i = 0; i < cnt; i++) {
	    g = gs[i];
	    for (j = 1; j <= GD_n_cluster(g); j++) {
		Agraph_t *c = GETCLUST(GD_clust(g)[j]);
		GD_clust(root)[idx++] = c;
		cloneClusterTree(GD_clust(g)[j], c);
	    }
	}
    }

    return root;
}

#ifdef WITH_CGRAPH
static Agraph_t *gread(FILE * fp)
{
    return agread(fp, (Agdisc_t *) 0);
}
#else
static Agraph_t *gread(FILE * fp)
{
    return agread(fp);
}
#endif

/* readGraphs:
 * Read input, parse the graphs, use init_nop (neato -n) to
 * read in all attributes need for layout.
 * Return the list of graphs. If cp != NULL, set it to the number
 * of graphs read.
 * We keep track of the types of graphs read. They all must be
 * either directed or undirected. If all graphs are strict, the
 * combined graph will be strict; other, the combined graph will
 * be non-strict.
 */
static Agraph_t **readGraphs(int *cp, GVC_t* gvc)
{
    Agraph_t *g;
    Agraph_t **gs = 0;
    ingraph_state ig;
    int cnt = 0;
    int sz = 0;
    int kindUnset = 1;

    /* set various state values */
    PSinputscale = POINTS_PER_INCH;
    Nop = 2;

    newIngraph(&ig, myFiles, gread);
    while ((g = nextGraph(&ig)) != 0) {
	if (verbose)
	    fprintf(stderr, "Reading graph %s\n", agnameof(g));
	if (agnnodes(g) == 0) {
	    fprintf(stderr, "Graph %s is empty - ignoring\n", agnameof(g));
	    continue;
	}
	if (cnt >= sz) {
	    sz += nGraphs;
	    gs = ALLOC(sz, gs, Agraph_t *);
	}
#ifndef WITH_CGRAPH
	if (kindUnset) {
	    kindUnset = 0;
	    kind = g->kind;
	}
	else if ((kind & AGFLAG_DIRECTED) != AG_IS_DIRECTED(g)) {
	    fprintf(stderr,
		    "Error: all graphs must be directed or undirected\n");
	    exit(1);
	} else if (!AG_IS_STRICT(g))
	    kind = g->kind;
#else
	if (kindUnset) {
	    kindUnset = 0;
	    kind = g->desc;
	}
	else if (kind.directed != g->desc.directed) {
	    fprintf(stderr,
		    "Error: all graphs must be directed or undirected\n");
	    exit(1);
	} else if (!agisstrict(g))
	    kind = g->desc;
#endif
	init_graph(g, doPack, gvc);
	gs[cnt++] = g;
    }

    gs = RALLOC(cnt, gs, Agraph_t *);
    if (cp)
	*cp = cnt;
    return gs;
}

/* compBB:
 * Compute the bounding box containing the graphs.
 * We can just use the bounding boxes of the graphs.
 */
boxf compBB(Agraph_t ** gs, int cnt)
{
    boxf bb, bb2;
    int i;

    bb = GD_bb(gs[0]);

    for (i = 1; i < cnt; i++) {
	bb2 = GD_bb(gs[i]);
	bb.LL.x = MIN(bb.LL.x, bb2.LL.x);
	bb.LL.y = MIN(bb.LL.y, bb2.LL.y);
	bb.UR.x = MAX(bb.UR.x, bb2.UR.x);
	bb.UR.y = MAX(bb.UR.y, bb2.UR.y);
    }

    return bb;
}

#ifdef DEBUG
void dump(Agraph_t * g)
{
    node_t *v;
    edge_t *e;

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	fprintf(stderr, "%s\n", agnameof(v));
	for (e = agfstout(g, v); e; e = agnxtout(g, e)) {
	    fprintf(stderr, "  %s -- %s\n", agnameof(agtail(e)), agnameof(aghead(e)));
	}
    }
}

#ifndef WITH_CGRAPH
void dumps(Agraph_t * g)
{
    graph_t *subg;
    graph_t *mg;
    edge_t *me;
    node_t *mn;

    mg = g->meta_node->graph;
    for (me = agfstout(mg, g->meta_node); me; me = agnxtout(mg, me)) {
	mn = aghead(me);
	subg = agusergraph(mn);
	dump(subg);
	fprintf(stderr, "====\n");
    }
}
#else
void dumps(Agraph_t * g)
{
    graph_t *subg;

    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	dump(subg);
	fprintf(stderr, "====\n");
    }
}
#endif
#endif

int main(int argc, char *argv[])
{
    Agraph_t **gs;
    Agraph_t *g;
    int cnt;
    pack_info pinfo;
    GVC_t * gvc;

    init(argc, argv, &pinfo);

    doPack = (pinfo.mode != l_undef);

    gvc = gvNEWcontext(lt_preloaded_symbols, DEMAND_LOADING);
    gs = readGraphs(&cnt, gvc);
    if (cnt == 0)
	exit(0);

    /* pack graphs */
    if (doPack) {
	if (packGraphs(cnt, gs, 0, &pinfo)) {
	    fprintf(stderr, "gvpack: packing of graphs failed.\n");
	    exit(1);
	}
    }

    /* create union graph and copy attributes */
    g = cloneGraph(gs, cnt, gvc);

    /* compute new top-level bb and set */
    if (doPack) {
	GD_bb(g) = compBB(gs, cnt);
	dotneato_postprocess(g);
	attach_attrs(g);
    }
    agwrite(g, outfp);
    exit(0);
}
