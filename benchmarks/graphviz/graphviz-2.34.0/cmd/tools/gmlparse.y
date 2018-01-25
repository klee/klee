/* $Id$Revision: */
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

%{
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <arith.h>
#include <gml2gv.h>
#include <agxbuf.h>
#include <assert.h>

#define NEW(t)       (t*)malloc(sizeof(t))
#define N_NEW(n,t)   (t*)malloc((n)*sizeof(t))
#define RALLOC(size,ptr,type) ((type*)realloc(ptr,(size)*sizeof(type)))

typedef unsigned short ushort;

static gmlgraph* G;
static gmlnode* N;
static gmledge* E;
static Dt_t* L;
static Dt_t** liststk;
static int liststk_sz, liststk_cnt;
 
static void free_attr (Dt_t*d, gmlattr* p, Dtdisc_t* ds); /* forward decl */
static char* sortToStr (int sort);    /* forward decl */

static void
free_node (Dt_t*d, gmlnode* p, Dtdisc_t* ds)
{
    if (!p) return;
    if (p->attrlist) dtclose (p->attrlist);
    free (p);
}

static void
free_edge (Dt_t*d, gmledge* p, Dtdisc_t* ds)
{
    if (!p) return;
    if (p->attrlist) dtclose (p->attrlist);
    free (p);
}

static void
free_graph (Dt_t*d, gmlgraph* p, Dtdisc_t* ds)
{
    if (!p) return;
    if (p->nodelist)
	dtclose (p->nodelist);
    if (p->edgelist)
	dtclose (p->edgelist);
    if (p->attrlist)
	dtclose (p->attrlist);
    if (p->graphlist)
	dtclose (p->graphlist);
    free (p);
}

static Dtdisc_t nodeDisc = {
    offsetof(gmlnode,attrlist),
    sizeof(Dt_t*),
    offsetof(gmlnode,link),
    NIL(Dtmake_f),
    (Dtfree_f)free_node,
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

static Dtdisc_t edgeDisc = {
    offsetof(gmledge,attrlist),
    sizeof(Dt_t*),
    offsetof(gmledge,link),
    NIL(Dtmake_f),
    (Dtfree_f)free_edge,
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

static Dtdisc_t attrDisc = {
    offsetof(gmlattr,name),
    sizeof(char*),
    offsetof(gmlattr,link),
    NIL(Dtmake_f),
    (Dtfree_f)free_attr,
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

static Dtdisc_t graphDisc = {
    offsetof(gmlgraph,nodelist),
    sizeof(Dt_t*),
    offsetof(gmlgraph,link),
    NIL(Dtmake_f),
    (Dtfree_f)free_graph,
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

static void
initstk (void)
{
    liststk_sz = 10;
    liststk_cnt = 0;
    liststk = N_NEW(liststk_sz, Dt_t*);
}

static void
cleanup (void)
{
    int i;

    if (liststk) {
	for (i = 0; i < liststk_cnt; i++)
	    dtclose (liststk[i]);
	free (liststk);
	liststk = NULL;
    }
    if (L) {
	dtclose (L);
	L = NULL;
    }
    if (N) {
	free_node (0, N, 0);
	N = NULL;
    }
    if (E) {
	free_edge (0, E, 0);
	E = NULL;
    }
    if (G) {
	free_graph (0, G, 0);
	G = NULL;
    }
}

static void
pushAlist (void)
{
    Dt_t* lp = dtopen (&attrDisc, Dtqueue);

    if (L) {
	if (liststk_cnt == liststk_sz) {
	    liststk_sz *= 2;
	    liststk = RALLOC(liststk_sz, liststk, Dt_t*);
	}
	liststk[liststk_cnt++] = L;
    }
    L = lp;
}

static Dt_t*
popAlist (void)
{
    Dt_t* lp = L;

    if (liststk_cnt)
	L = liststk[--liststk_cnt];
    else
	L = NULL;

    return lp;
}

static void
popG (void)
{
    G = G->parent;
}

static void
pushG (void)
{
    gmlgraph* g = NEW(gmlgraph);

    g->attrlist = dtopen(&attrDisc, Dtqueue);
    g->nodelist = dtopen(&nodeDisc, Dtqueue);
    g->edgelist = dtopen(&edgeDisc, Dtqueue);
    g->graphlist = dtopen(&graphDisc, Dtqueue);
    g->parent = G;
    g->directed = -1;

    if (G)
	dtinsert (G->graphlist, g);

    G = g;
}

static gmlnode*
mkNode (void)
{
    gmlnode* np = NEW(gmlnode);
    np->attrlist = dtopen (&attrDisc, Dtqueue);
    np->id = NULL;
    return np;
}

static gmledge*
mkEdge (void)
{
    gmledge* ep = NEW(gmledge);
    ep->attrlist = dtopen (&attrDisc, Dtqueue);
    ep->source = NULL;
    ep->target = NULL;
    return ep;
}

static gmlattr*
mkAttr (char* name, int sort, int kind, char* str, Dt_t* list)
{
    gmlattr* gp = NEW(gmlattr);

    assert (name || sort);
    if (!name)
	name = strdup (sortToStr (sort));
    gp->sort = sort;
    gp->kind = kind;
    gp->name = name;
    if (str)
	gp->u.value = str;
    else {
	if (dtsize (list) == 0) {
	    dtclose (list);
	    list = 0;
	}
	gp->u.lp = list;
    }
/* fprintf (stderr, "[%x] %d %d \"%s\" \"%s\" \n", gp, sort, kind, (name?name:""),  (str?str:"")); */
    return gp;
}

static int
setDir (char* d)
{
    gmlgraph* g;
    int dir = atoi (d);

    free (d);
    if (dir < 0) dir = -1;
    else if (dir > 0) dir = 1;
    else dir = 0;
    G->directed = dir;

    if (dir >= 0) {
	for (g = G->parent; g; g = g->parent) {
	    if (g->directed < 0)
		g->directed = dir;
	    else if (g->directed != dir)
		return 1;
        }
    }

    return 0;
}

%}
%union  {
    int i;
    char *str;
    gmlnode* np;
    gmledge* ep;
    gmlattr* ap;
    Dt_t*    list;
}

%token GRAPH NODE EDGE DIRECTED SOURCE TARGET
%token XVAL YVAL WVAL HVAL LABEL GRAPHICS LABELGRAPHICS TYPE FILL
%token OUTLINE OUTLINESTYLE OUTLINEWIDTH WIDTH
%token STYLE LINE POINT
%token TEXT FONTSIZE FONTNAME COLOR
%token <str> INTEGER REAL STRING ID NAME
%token <list> LIST

%type <np> node
%type <ep> edge
%type <list> attrlist
%type <ap> alistitem

%%
graph : optalist hdr body  {gmllexeof(); if (G->parent) popG(); }
      | error { cleanup(); YYABORT; }
      | 
      ;

hdr   : GRAPH { pushG(); }
      ;

body  : '[' optglist ']'
      ;

optglist : glist
         |  /* empty */ 
         ;

glist  : glist glistitem
       | glistitem
       ;

glistitem : node { dtinsert (G->nodelist, $1); }
          | edge { dtinsert (G->edgelist, $1); }
          | hdr body 
          | DIRECTED INTEGER { 
		if (setDir($2)) { 
		    yyerror("mixed directed and undirected graphs"); 
		    cleanup ();
		    YYABORT;
		}
	  }
	  | ID INTEGER { dtinsert (G->attrlist, mkAttr (strdup("id"), 0, INTEGER, $2, 0)); }
          | alistitem { dtinsert (G->attrlist, $1); }
          ;

node :  NODE { N = mkNode(); } '[' nlist ']' { $$ = N; N = NULL; }
     ;

nlist : nlist nlistitem
      | nlistitem
      ;

nlistitem : ID INTEGER { N->id = $2; }
          | alistitem { dtinsert (N->attrlist, $1); }
          ;

edge :  EDGE { E = mkEdge(); } '[' elist ']' { $$ = E; E = NULL; }
     ;

elist : elist elistitem
      | elistitem
      ;

elistitem : SOURCE INTEGER { E->source = $2; }
          | TARGET INTEGER { E->target = $2; }
	  | ID INTEGER { dtinsert (E->attrlist, mkAttr (strdup("id"), 0, INTEGER, $2, 0)); }
          | alistitem { dtinsert (E->attrlist, $1); }
          ;

attrlist  : '[' {pushAlist(); } optalist ']' { $$ = popAlist(); }
          ;

optalist  : alist
          | /* empty */ 
          ;

alist  : alist alistitem  { dtinsert (L, $2); }
       | alistitem  { dtinsert (L, $1); }
       ;

alistitem : NAME INTEGER { $$ = mkAttr ($1, 0, INTEGER, $2, 0); }
          | NAME REAL    { $$ = mkAttr ($1, 0, REAL, $2, 0); }
          | NAME STRING  { $$ = mkAttr ($1, 0, STRING, $2, 0); }
          | NAME attrlist  { $$ = mkAttr ($1, 0, LIST, 0, $2); }
          | XVAL REAL  { $$ = mkAttr (0, XVAL, REAL, $2, 0); }
          | XVAL INTEGER  { $$ = mkAttr (0, XVAL, REAL, $2, 0); }
          | YVAL REAL  { $$ = mkAttr (0, YVAL, REAL, $2, 0); }
          | WVAL REAL  { $$ = mkAttr (0, WVAL, REAL, $2, 0); }
          | HVAL REAL  { $$ = mkAttr (0, HVAL, REAL, $2, 0); }
          | LABEL STRING  { $$ = mkAttr (0, LABEL, STRING, $2, 0); }
          | GRAPHICS attrlist  { $$ = mkAttr (0, GRAPHICS, LIST, 0, $2); }
          | LABELGRAPHICS attrlist  { $$ = mkAttr (0, LABELGRAPHICS, LIST, 0, $2); }
          | TYPE STRING  { $$ = mkAttr (0, TYPE, STRING, $2, 0); }
          | FILL STRING  { $$ = mkAttr (0, FILL, STRING, $2, 0); }
          | OUTLINE STRING  { $$ = mkAttr (0, OUTLINE, STRING, $2, 0); }
          | OUTLINESTYLE STRING  { $$ = mkAttr (0, OUTLINESTYLE, STRING, $2, 0); }
          | OUTLINEWIDTH INTEGER  { $$ = mkAttr (0, OUTLINEWIDTH, INTEGER, $2, 0); }
          | WIDTH REAL  { $$ = mkAttr (0, OUTLINEWIDTH, REAL, $2, 0); }
          | STYLE STRING  { $$ = mkAttr (0, STYLE, STRING, $2, 0); }
          | STYLE attrlist  { $$ = mkAttr (0, STYLE, LIST, 0, $2); }
          | LINE attrlist  { $$ = mkAttr (0, LINE, LIST, 0, $2); }
          | POINT attrlist  { $$ = mkAttr (0, POINT, LIST, 0, $2); }
          | TEXT STRING  { $$ = mkAttr (0, TEXT, STRING, $2, 0); }
          | FONTNAME STRING  { $$ = mkAttr (0, FONTNAME, STRING, $2, 0); }
          | FONTSIZE INTEGER  { $$ = mkAttr (0, FONTNAME, INTEGER, $2, 0); }
          | COLOR STRING  { $$ = mkAttr (0, COLOR, STRING, $2, 0); }
          ;

%%

static void
free_attr (Dt_t*d, gmlattr* p, Dtdisc_t* ds)
{
    if (!p) return;
    if ((p->kind == LIST) && p->u.lp)
	dtclose (p->u.lp);
    else
	free (p->u.value);
    free (p->name);
    free (p);
}

static void deparseList (Dt_t* alist, agxbuf* xb); /* forward declaration */

static void
deparseAttr (gmlattr* ap, agxbuf* xb)
{
    if (ap->kind == LIST) {
	agxbput (xb, ap->name);
	agxbputc (xb, ' ');
	deparseList (ap->u.lp, xb);
    }
    else if (ap->kind == STRING) {
	agxbput (xb, ap->name);
	agxbput (xb, " \"");
	agxbput (xb, ap->u.value);
	agxbput (xb, "\"");
    }
    else {
	agxbput (xb, ap->name);
	agxbputc (xb, ' ');
	agxbput (xb, ap->u.value);
    }
}

static void
deparseList (Dt_t* alist, agxbuf* xb)
{
    gmlattr* ap;

    agxbput (xb, "[ "); 
    if (alist) for (ap = dtfirst(alist); ap; ap = dtnext (alist, ap)) {
	deparseAttr (ap, xb);
	agxbputc (xb, ' ');
    }
    agxbput (xb, "]"); 
  
}

static void
unknown (Agobj_t* obj, gmlattr* ap, agxbuf* xb)
{
    char* str;

    if (ap->kind == LIST) {
	deparseList (ap->u.lp, xb);
	str = agxbuse (xb);
    }
    else
	str = ap->u.value;

    agsafeset (obj, ap->name, str, "");
}

static void
addNodeLabelGraphics (Agnode_t* np, Dt_t* alist, agxbuf* xb, agxbuf* unk)
{
    gmlattr* ap;
    int cnt = 0;

    if (!alist)
	return;

    for (ap = dtfirst(alist); ap; ap = dtnext (alist, ap)) {
	if (ap->sort == TEXT) {
	    agsafeset (np, "label", ap->u.value, "");
	}
	else if (ap->sort == COLOR) {
	    agsafeset (np, "fontcolor", ap->u.value, "");
	}
	else if (ap->sort == FONTSIZE) {
	    agsafeset (np, "fontsize", ap->u.value, "");
	}
	else if (ap->sort == FONTNAME) {
	    agsafeset (np, "fontname", ap->u.value, "");
	}
	else {
	    if (cnt)
		agxbputc (unk, ' '); 
	    else {
		agxbput (unk, "[ "); 
	    }
	    deparseAttr (ap, unk);
	    cnt++;
	}
    }

    if (cnt) {
	agxbput (unk, " ]"); 
	agsafeset (np, "LabelGraphics", agxbuse (unk), "");
    }
    else
	agxbclear (unk); 
}

static void
addEdgeLabelGraphics (Agedge_t* ep, Dt_t* alist, agxbuf* xb, agxbuf* unk)
{
    gmlattr* ap;
    char* x = "0";
    char* y = "0";
    int cnt = 0;

    if (!alist)
	return;

    for (ap = dtfirst(alist); ap; ap = dtnext (alist, ap)) {
	if (ap->sort == TEXT) {
	    agsafeset (ep, "label", ap->u.value, "");
	}
	else if (ap->sort == COLOR) {
	    agsafeset (ep, "fontcolor", ap->u.value, "");
	}
	else if (ap->sort == FONTSIZE) {
	    agsafeset (ep, "fontsize", ap->u.value, "");
	}
	else if (ap->sort == FONTNAME) {
	    agsafeset (ep, "fontname", ap->u.value, "");
	}
	else if (ap->sort == XVAL) {
	    x = ap->u.value;
	}
	else if (ap->sort == YVAL) {
	    y = ap->u.value;
	}
	else {
	    if (cnt)
		agxbputc (unk, ' '); 
	    else {
		agxbput (unk, "[ "); 
	    }
	    deparseAttr (ap, unk);
	    cnt++;
	}
    }

    agxbput (xb, x);
    agxbputc (xb, ',');
    agxbput (xb, y);
    agsafeset (ep, "lp", agxbuse (xb), "");

    if (cnt) {
	agxbput (unk, " ]"); 
	agsafeset (ep, "LabelGraphics", agxbuse (unk), "");
    }
    else
	agxbclear (unk); 
}

static void
addNodeGraphics (Agnode_t* np, Dt_t* alist, agxbuf* xb, agxbuf* unk)
{
    gmlattr* ap;
    char* x = "0";
    char* y = "0";
    char buf[BUFSIZ];
    double d;
    int cnt = 0;

    for (ap = dtfirst(alist); ap; ap = dtnext (alist, ap)) {
	if (ap->sort == XVAL) {
	    x = ap->u.value;
	}
	else if (ap->sort == YVAL) {
	    y = ap->u.value;
	}
	else if (ap->sort == WVAL) {
	    d = atof (ap->u.value);
	    sprintf (buf, "%.04f", d/72.0);
	    agsafeset (np, "width", buf, "");
	}
	else if (ap->sort == HVAL) {
	    d = atof (ap->u.value);
	    sprintf (buf, "%.04f", d/72.0);
	    agsafeset (np, "height", buf, "");
	}
	else if (ap->sort == TYPE) {
	    agsafeset (np, "shape", ap->u.value, "");
	}
	else if (ap->sort == FILL) {
	    agsafeset (np, "color", ap->u.value, "");
	}
	else if (ap->sort == OUTLINE) {
	    agsafeset (np, "pencolor", ap->u.value, "");
	}
	else if ((ap->sort == WIDTH) && (ap->sort == OUTLINEWIDTH )) {
	    agsafeset (np, "penwidth", ap->u.value, "");
	}
	else if ((ap->sort == OUTLINESTYLE) && (ap->sort == OUTLINEWIDTH )) {
	    agsafeset (np, "style", ap->u.value, "");
	}
	else {
	    if (cnt)
		agxbputc (unk, ' '); 
	    else {
		agxbput (unk, "[ "); 
	    }
	    deparseAttr (ap, unk);
	    cnt++;
	}
    }

    agxbput (xb, x);
    agxbputc (xb, ',');
    agxbput (xb, y);
    agsafeset (np, "pos", agxbuse (xb), "");

    if (cnt) {
	agxbput (unk, " ]"); 
	agsafeset (np, "graphics", agxbuse (unk), "");
    }
    else
	agxbclear (unk); 
}

static void
addEdgePoint (Agedge_t* ep, Dt_t* alist, agxbuf* xb)
{
    gmlattr* ap;
    char* x = "0";
    char* y = "0";

    for (ap = dtfirst(alist); ap; ap = dtnext (alist, ap)) {
        if (ap->sort == XVAL) {
	    x = ap->u.value;
	}
	else if (ap->sort == YVAL) {
	    y = ap->u.value;
	}
	else {
	    fprintf (stderr, "non-X/Y field in point attribute");
	    unknown ((Agobj_t*)ep, ap, xb);
	}
    }

    if (agxblen(xb)) agxbputc (xb, ' ');
    agxbput (xb, x);
    agxbputc (xb, ',');
    agxbput (xb, y);
}

static void
addEdgePos (Agedge_t* ep, Dt_t* alist, agxbuf* xb)
{
    gmlattr* ap;
    
    if (!alist) return;
    for (ap = dtfirst(alist); ap; ap = dtnext (alist, ap)) {
	if (ap->sort == POINT) {
	    addEdgePoint (ep, ap->u.lp, xb);
	}
	else {
	    fprintf (stderr, "non-point field in line attribute");
	    unknown ((Agobj_t*)ep, ap, xb);
	}
    }
    agsafeset (ep, "pos", agxbuse (xb), "");
}

static void
addEdgeGraphics (Agedge_t* ep, Dt_t* alist, agxbuf* xb, agxbuf* unk)
{
    gmlattr* ap;
    int cnt = 0;

    for (ap = dtfirst(alist); ap; ap = dtnext (alist, ap)) {
	if (ap->sort == WIDTH) {
	    agsafeset (ep, "penwidth", ap->u.value, "");
	}
	else if (ap->sort == STYLE) {
	    agsafeset (ep, "style", ap->u.value, "");
	}
	else if (ap->sort == FILL) {
	    agsafeset (ep, "color", ap->u.value, "");
	}
	else if (ap->sort == LINE) {
	    addEdgePos (ep, ap->u.lp, xb);
	}
	else {
	    if (cnt)
		agxbputc (unk, ' '); 
	    else {
		agxbput (unk, "[ "); 
	    }
	    deparseAttr (ap, unk);
	    cnt++;
	}
    }

    if (cnt) {
	agxbput (unk, " ]"); 
	agsafeset (ep, "graphics", agxbuse (unk), "");
    }
    else
	agxbclear(unk);
}

static void
addAttrs (Agobj_t* obj, Dt_t* alist, agxbuf* xb, agxbuf* unk)
{
    gmlattr* ap;

    for (ap = dtfirst(alist); ap; ap = dtnext (alist, ap)) {
	if (ap->sort == GRAPHICS) {
	    if (AGTYPE(obj) == AGNODE)
		addNodeGraphics ((Agnode_t*)obj, ap->u.lp, xb, unk);
	    else if (AGTYPE(obj) == AGEDGE)
		addEdgeGraphics ((Agedge_t*)obj, ap->u.lp, xb, unk);
	    else
		unknown (obj, ap, xb);
	}
	else if (ap->sort == LABELGRAPHICS) {
	    if (AGTYPE(obj) == AGNODE)
		addNodeLabelGraphics ((Agnode_t*)obj, ap->u.lp, xb, unk);
	    else if (AGTYPE(obj) == AGEDGE)
		addEdgeLabelGraphics ((Agedge_t*)obj, ap->u.lp, xb, unk);
	    else
		unknown (obj, ap, xb);
	}
	else if ((ap->sort == LABEL) && (AGTYPE(obj) != AGRAPH)) {
	    agsafeset (obj, "name", ap->u.value, "");
	}
	else
	    unknown (obj, ap, xb);
    }
}

static Agraph_t*
mkGraph (gmlgraph* G, Agraph_t* parent, char* name, agxbuf* xb, agxbuf* unk)
{
    Agraph_t* g;
    Agnode_t* n;
    Agnode_t* h;
    Agedge_t* e;
    gmlnode*  np;
    gmledge*  ep;
    gmlgraph* gp;

    if (parent) {
	g = agsubg (parent, NULL, 1);
    }
    else if (G->directed >= 1)
	g = agopen (name, Agdirected, 0);
    else
	g = agopen (name, Agundirected, 0);

    if (!parent && L) {
	addAttrs ((Agobj_t*)g, L, xb, unk);
    } 
    for (np = dtfirst(G->nodelist); np; np = dtnext (G->nodelist, np)) {
	if (!np->id) {
	   fprintf (stderr, "node without an id attribute"); 
	   exit (1);
        }
	n = agnode (g, np->id, 1);
	addAttrs ((Agobj_t*)n, np->attrlist, xb, unk);
    }

    for (ep = dtfirst(G->edgelist); ep; ep = dtnext (G->edgelist, ep)) {
	if (!ep->source) {
	   fprintf (stderr, "edge without an source attribute"); 
	   exit (1);
        }
	if (!ep->target) {
	   fprintf (stderr, "node without an target attribute"); 
	   exit (1);
        }
	n = agnode (g, ep->source, 1);
	h = agnode (g, ep->target, 1);
	e = agedge (g, n, h, NULL, 1);
	addAttrs ((Agobj_t*)e, ep->attrlist, xb, unk);
    }
    for (gp = dtfirst(G->graphlist); gp; gp = dtnext (G->graphlist, gp)) {
	mkGraph (gp, g, NULL, xb, unk);
    }

    addAttrs ((Agobj_t*)g, G->attrlist, xb, unk);

    return g;
}

Agraph_t*
gml_to_gv (char* name, FILE* fp, int cnt, int* errors)
{
    Agraph_t* g;
    agxbuf xb;
    unsigned char buf[BUFSIZ];
    agxbuf unk;
    unsigned char unknownb[BUFSIZ];
    int error;

    if (cnt == 0)
	initgmlscan(fp);
    else
	initgmlscan(0);
		
    initstk();
    L = NULL;
    pushAlist ();
    gmlparse ();

    error = gmlerrors();
    *errors |= error;
    if (!G || error)
	g = NULL;
    else {
	agxbinit (&xb, BUFSIZ, buf);
	agxbinit (&unk, BUFSIZ, unknownb);
	g = mkGraph (G, NULL, name, &xb, &unk);
	agxbfree (&xb);
    }

    cleanup ();

    return g;
}

static char*
sortToStr (int sort)
{
    char* s;

    switch (sort) {
    case GRAPH : 
	s = "graph"; break;
    case NODE : 
	s = "node"; break;
    case EDGE : 
	s = "edge"; break;
    case DIRECTED : 
	s = "directed"; break;
    case ID : 
	s = "id"; break;
    case SOURCE : 
	s = "source"; break;
    case TARGET : 
	s = "target"; break;
    case XVAL : 
	s = "xval"; break;
    case YVAL : 
	s = "yval"; break;
    case WVAL : 
	s = "wval"; break;
    case HVAL : 
	s = "hval"; break;
    case LABEL : 
	s = "label"; break;
    case GRAPHICS : 
	s = "graphics"; break;
    case LABELGRAPHICS : 
	s = "labelGraphics"; break;
    case TYPE : 
	s = "type"; break;
    case FILL : 
	s = "fill"; break;
    case OUTLINE : 
	s = "outline"; break;
    case OUTLINESTYLE : 
	s = "outlineStyle"; break;
    case OUTLINEWIDTH : 
	s = "outlineWidth"; break;
    case WIDTH : 
	s = "width"; break;
    case STYLE : 
	s = "style"; break;
    case LINE : 
	s = "line"; break;
    case POINT : 
	s = "point"; break;
    case TEXT : 
	s = "text"; break;
    case FONTSIZE : 
	s = "fontSize"; break;
    case FONTNAME : 
	s = "fontName"; break;
    case COLOR : 
	s = "color"; break;
    case INTEGER : 
	s = "integer"; break;
    case REAL : 
	s = "real"; break;
    case STRING : 
	s = "string"; break;
    case NAME : 
	s = "name"; break;
    case LIST : 
	s = "list"; break;
    case '[' : 
	s = "["; break;
    case ']' : 
	s = "]"; break;
    default : 
	s = NULL;break;
    }

    return s;
}

#if 0
int gmllex ()
{
    int tok = _gmllex();
    char* s = sortToStr (tok);

    if (s)
        fprintf (stderr, "token = %s\n", s);
    else
        fprintf (stderr, "token = <%d>\n", tok);
    return tok;
}
#endif

