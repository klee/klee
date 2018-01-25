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

#include <string.h>
#include <stdlib.h>
#include "gvc.h"

extern "C" {
extern void gv_string_writer_init(GVC_t *gvc);
extern void gv_channel_writer_init(GVC_t *gvc);
}

#ifdef WITH_CGRAPH
#define agfindattr(x,s) agattrsym(x,s)
#define agraphattr(g,n,s) agattr(g,AGRAPH,n,s)
#define agnodeattr(g,n,s) agattr(g,AGNODE,n,s)
#define agedgeattr(g,n,s) agattr(g,AGEDGE,n,s)
#endif

static char emptystring[] = {'\0'};

static GVC_t *gvc;

static void gv_init(void) {
    /* list of builtins, enable demand loading */
    gvc = gvContextPlugins(lt_preloaded_symbols, DEMAND_LOADING);
}

Agraph_t *graph(char *name)
{
    if (!gvc)
        gv_init();
#ifdef WITH_CGRAPH
    return agopen(name, Agundirected, 0);
#else
    return agopen(name, AGRAPH);
#endif
}

Agraph_t *digraph(char *name)
{
    if (!gvc)
        gv_init();
#ifdef WITH_CGRAPH
    return agopen(name, Agdirected, 0);
#else
    return agopen(name, AGDIGRAPH);
#endif
}

Agraph_t *strictgraph(char *name)
{
    if (!gvc)
        gv_init();
#ifdef WITH_CGRAPH
    return agopen(name, Agstrictundirected, 0);
#else
    return agopen(name, AGRAPHSTRICT);
#endif
}

Agraph_t *strictdigraph(char *name)
{
    if (!gvc)
        gv_init();
#ifdef WITH_CGRAPH
    return agopen(name, Agstrictdirected, 0);
#else
    return agopen(name, AGDIGRAPHSTRICT);
#endif
}

Agraph_t *readstring(char *string)
{
    if (!gvc)
        gv_init();
    return agmemread(string);
}

Agraph_t *read(FILE *f)
{
    if (!gvc)
        gv_init();
#ifdef WITH_CGRAPH
    return agread(f, NULL);
#else
    return agread(f);
#endif
}

Agraph_t *read(const char *filename)
{
    FILE *f;
    Agraph_t *g;

    f = fopen(filename, "r");
    if (!f)
        return NULL;
    if (!gvc)
        gv_init();
#ifdef WITH_CGRAPH
    g = agread(f, NULL);
#else
    g = agread(f);
#endif
    fclose(f);
    return g;
}

//-------------------------------------------------
Agraph_t *graph(Agraph_t *g, char *name)
{
    if (!gvc)
        gv_init();
#ifdef WITH_CGRAPH
    return agsubg(g, name, 1);
#else
    return agsubg(g, name);
#endif
}

Agnode_t *node(Agraph_t *g, char *name)
{
    if (!gvc)
        return NULL;
#ifdef WITH_CGRAPH
    return agnode(g, name, 1);
#else
    // creating a protonode is not permitted
    if (name[0] == '\001' && strcmp (name, "\001proto") == 0)
        return NULL;
    return agnode(g, name);
#endif
}

Agedge_t *edge(Agnode_t *t, Agnode_t *h)
{
    if (!gvc || !t || !h)
        return NULL;
#ifdef WITH_CGRAPH
    // edges from/to the protonode are not permitted
    if (AGTYPE(t) == AGRAPH || AGTYPE(h) == AGRAPH)
	return NULL;
    return agedge(agraphof(t), t, h, NULL, 1);
#else
    // edges from/to the protonode are not permitted
    if ((agnameof(t)[0] == '\001' && strcmp (agnameof(t), "\001proto") == 0)
      || (agnameof(h)[0] == '\001' && strcmp (agnameof(h), "\001proto") == 0))
        return NULL;
    return agedge(t->graph, t, h);
#endif
}

// induce tail if necessary
Agedge_t *edge(char *tname, Agnode_t *h)
{
#ifdef WITH_CGRAPH
    return edge(node(agraphof(h), tname), h);
#else
    return edge(node(h->graph, tname), h);
#endif
}

// induce head if necessary
Agedge_t *edge(Agnode_t *t, char *hname)
{
#ifdef WITH_CGRAPH
    return edge(t, node(agraphof(t), hname));
#else
    return edge(t, node(t->graph, hname));
#endif
}

// induce tail/head if necessary
Agedge_t *edge(Agraph_t *g, char *tname, char *hname)
{
    return edge(node(g, tname), node(g, hname));
}

//-------------------------------------------------
static char* myagxget(void *obj, Agsym_t *a)
{
    int len;
    char *val, *hs;

    if (!obj || !a)
        return emptystring;
#ifndef WITH_CGRAPH
    val = agxget(obj, a->index);
#else
    val = agxget(obj, a);
#endif
    if (!val)
        return emptystring;
    if (a->name[0] == 'l' && strcmp(a->name, "label") == 0 && aghtmlstr(val)) {
        len = strlen(val);
        hs = (char*)malloc(len + 3);
        hs[0] = '<';
        strcpy(hs+1, val);
        hs[len+1] = '>';
        hs[len+2] = '\0';
        return hs;
    }
    return val;
}
char *getv(Agraph_t *g, Agsym_t *a)
{
    return myagxget(g, a);
}
char *getv(Agraph_t *g, char *attr)
{
    Agsym_t *a;

    if (!g || !attr)
        return NULL;
    a = agfindattr(agroot(g), attr);
    return myagxget(g, a);
}
static void myagxset(void *obj, Agsym_t *a, char *val)
{
    int len;
    char *hs;

    if (a->name[0] == 'l' && val[0] == '<' && strcmp(a->name, "label") == 0) {
        len = strlen(val);
        if (val[len-1] == '>') {
            hs = strdup(val+1);
                *(hs+len-2) = '\0';
#ifdef WITH_CGRAPH
            val = agstrdup_html(agraphof(obj),hs);
#else
            val = agstrdup_html(hs);
#endif
            free(hs);
        }
    }
#ifndef WITH_CGRAPH
    agxset(obj, a->index, val);
#else
    agxset(obj, a, val);
#endif
}
char *setv(Agraph_t *g, Agsym_t *a, char *val)
{
    if (!g || !a || !val)
        return NULL;
    myagxset(g, a, val);
    return val;
}
char *setv(Agraph_t *g, char *attr, char *val)
{
    Agsym_t *a;

    if (!g || !attr || !val)
        return NULL;
    a = agfindattr(agroot(g), attr);
    if (!a)
        a = agraphattr(g->root, attr, emptystring);
    myagxset(g, a, val);
    return val;
}
//-------------------------------------------------
char *getv(Agnode_t *n, Agsym_t *a)
{
    if (!n || !a)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(n) == AGRAPH) // protonode   
	return NULL;   // FIXME ??
#endif
    return myagxget(n, a);
}
char *getv(Agnode_t *n, char *attr)
{
    Agraph_t *g;
    Agsym_t *a;

    if (!n || !attr)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(n) == AGRAPH) // protonode   
	return NULL;   // FIXME ??
#endif
    g = agroot(agraphof(n));
#ifdef WITH_CGRAPH
    a = agattr(g, AGNODE, attr, NULL);
#else
    a = agfindattr(g->proto->n, attr);
#endif
    return myagxget(n, a);
}
char *setv(Agnode_t *n, Agsym_t *a, char *val)
{
    if (!n || !a || !val)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(n) == AGRAPH) // protonode   
	return NULL;   // FIXME ??
#endif
    myagxset(n, a, val);
    return val;
}
char *setv(Agnode_t *n, char *attr, char *val)
{
    Agraph_t *g;
    Agsym_t *a;

    if (!n || !attr || !val)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(n) == AGRAPH) { // protonode   
	g = (Agraph_t*)n;
    	a = agattr(g, AGNODE, attr, val); // create default attribute in psuodo protonode
	    // FIXME? - deal with html in "label" attributes
	return val;
    }
#endif
    g = agroot(agraphof(n));
#ifdef WITH_CGRAPH
    a = agattr(g, AGNODE, attr, NULL);
#else
    a = agfindattr(g->proto->n, attr);
#endif
    if (!a)
        a = agnodeattr(g, attr, emptystring);
    myagxset(n, a, val);
    return val;
}
//-------------------------------------------------
char *getv(Agedge_t *e, Agsym_t *a)
{
    if (!e || !a)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(e) == AGRAPH) // protoedge   
	return NULL;   // FIXME ??
#endif
    return myagxget(e, a);
}
char *getv(Agedge_t *e, char *attr)
{
    Agraph_t *g;
    Agsym_t *a;

    if (!e || !attr)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(e) == AGRAPH) // protoedge   
	return NULL;   // FIXME ??
#endif
    g = agraphof(agtail(e));
#ifndef WITH_CGRAPH
    a = agfindattr(g->proto->e, attr);
#else
    a = agattr(g, AGEDGE, attr, NULL);
#endif
    return myagxget(e, a);
}
char *setv(Agedge_t *e, Agsym_t *a, char *val)
{
    if (!e || !a || !val)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(e) == AGRAPH) // protoedge   
	return NULL;   // FIXME ??
#endif
    myagxset(e, a, val);
    return val;
}
char *setv(Agedge_t *e, char *attr, char *val)
{
    Agraph_t *g;
    Agsym_t *a;

    if (!e || !attr || !val)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(e) == AGRAPH) { // protoedge   
	g = (Agraph_t*)e;
    	a = agattr(g, AGEDGE, attr, val); // create default attribute in pseudo protoedge
	    // FIXME? - deal with html in "label" attributes
	return val;
    }
#endif
    g = agroot(agraphof(agtail(e)));
#ifndef WITH_CGRAPH
    a = agfindattr(g->proto->e, attr);
    if (!a)
        a = agedgeattr(g, attr, emptystring);
#else
    a = agattr(g, AGEDGE, attr, NULL);
    if (!a)
        a = agattr(g, AGEDGE, attr, emptystring);
#endif
    myagxset(e, a, val);
    return val;
}
//-------------------------------------------------
Agraph_t *findsubg(Agraph_t *g, char *name)
{
    if (!g || !name)
        return NULL;
#ifndef WITH_CGRAPH
    return agfindsubg(g, name);
#else
    return agsubg(g, name, 0);
#endif
}

Agnode_t *findnode(Agraph_t *g, char *name)
{
    if (!g || !name)
        return NULL;
#ifndef WITH_CGRAPH
    return agfindnode(g, name);
#else
    return agnode(g, name, 0);
#endif
}

Agedge_t *findedge(Agnode_t *t, Agnode_t *h)
{
    if (!t || !h)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(t) == AGRAPH || AGTYPE(h) == AGRAPH)
	return NULL;
#endif
    return agfindedge(agraphof(t), t, h);
}

Agsym_t *findattr(Agraph_t *g, char *name)
{
    if (!g || !name)
        return NULL;
    return agfindattr(g, name);
}

Agsym_t *findattr(Agnode_t *n, char *name)
{
    if (!n || !name)
        return NULL;
    return agfindattr(n, name);
}

Agsym_t *findattr(Agedge_t *e, char *name)
{
    if (!e || !name)
        return NULL;
    return agfindattr(e, name);
}

//-------------------------------------------------

Agnode_t *headof(Agedge_t *e)
{
    if (!e)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(e) == AGRAPH)
	return NULL;
#endif
    return aghead(e);
}

Agnode_t *tailof(Agedge_t *e)
{
    if (!e)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(e) == AGRAPH)
	return NULL;
#endif
    return agtail(e);
}

Agraph_t *graphof(Agraph_t *g)
{
    if (!g || g == g->root)
        return NULL;
    return agroot(g);
}

Agraph_t *graphof(Agedge_t *e)
{
    if (!e)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(e) == AGRAPH)
	return (Agraph_t*)e; /* graph of protoedge is itself recast */
#endif
    return agraphof(agtail(e));
}

Agraph_t *graphof(Agnode_t *n)
{
    if (!n)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(n) == AGRAPH)
	return (Agraph_t*)n;  /* graph of protonode is itself recast */
#endif
    return agraphof(n);
}

Agraph_t *rootof(Agraph_t *g)
{
    if (!g)
        return NULL;
    return agroot(g);
}

//-------------------------------------------------
Agnode_t *protonode(Agraph_t *g)
{
    if (!g)
        return NULL;
#ifdef WITH_CGRAPH
    return (Agnode_t *)g;    // gross abuse of the type system!
#else
    return g->proto->n;
#endif
}

Agedge_t *protoedge(Agraph_t *g)
{
    if (!g)
        return NULL;
#ifdef WITH_CGRAPH
    return (Agedge_t *)g;    // gross abuse of the type system!
#else
    return g->proto->e;
#endif
}

//-------------------------------------------------
char *nameof(Agraph_t *g)
{
    if (!g)
        return NULL;
    return agnameof(g);
}
char *nameof(Agnode_t *n)
{
    if (!n)
        return NULL;
#ifdef WITH_CGRAPH
    if (AGTYPE(n) == AGRAPH)
	return NULL;
#endif
    return agnameof(n);
}
//char *nameof(Agedge_t *e)
//{
//    if (!e)
//        return NULL;
//#ifdef WITH_CGRAPH
//    if (AGTYPE(e) == AGRAPH)
//	return NULL;
//#endif
//    return agnameof(e);
//}
char *nameof(Agsym_t *a)
{
    if (!a)
        return NULL;
    return a->name;
}

//-------------------------------------------------
bool ok(Agraph_t *g)
{
    if (!g) 
        return false;
    return true;
}
bool ok(Agnode_t *n)
{
    if (!n) 
        return false;
    return true;
}
bool ok(Agedge_t *e)
{
    if (!e) 
        return false;
    return true;
}
bool ok(Agsym_t *a)
{
    if (!a) 
        return false;
    return true;
}
//-------------------------------------------------
Agraph_t *firstsubg(Agraph_t *g)
{
#ifndef WITH_CGRAPH
    Agraph_t *mg;
    Agnode_t *n;
    Agedge_t *e;
#endif

    if (!g)
        return NULL;
#ifdef WITH_CGRAPH
    return agfstsubg(g);
#else
    n = g->meta_node;
    if (!n) 
        return NULL;
    mg = agraphof(n);
    if (!mg) 
        return NULL;
    e = agfstout(mg, n);
    if (!e) 
        return NULL;
    return agusergraph(aghead(e));
#endif
}

Agraph_t *nextsubg(Agraph_t *g, Agraph_t *sg)
{
#ifndef WITH_CGRAPH
    Agraph_t *mg;
    Agnode_t *ng, *nsg;
    Agedge_t *e;
#endif

    if (!g || !sg)
        return NULL;
#ifdef WITH_CGRAPH
    return agnxtsubg(sg);
#else
    ng = g->meta_node;
    nsg = sg->meta_node;
    if (!ng || !nsg) 
        return NULL;
    mg = agraphof(ng);
    if (!mg) 
        return NULL;
    e = agfindedge(mg, ng, nsg);
    if (!e) 
        return NULL;
    e = agnxtout(mg, e);
    if (!e) 
        return NULL;
    return agusergraph(aghead(e));
#endif
}

#ifdef WITH_CGRAPH
Agraph_t *firstsupg(Agraph_t *g)
{
    return g->parent;
}

Agraph_t *nextsupg(Agraph_t *g, Agraph_t *sg)
{
    return NULL;
}
#else
Agraph_t *firstsupg(Agraph_t *g)
{
    Agraph_t *mg;
    Agnode_t *n;
    Agedge_t *e;

    if (!g)
        return NULL;
    n = g->meta_node;
    if (!n) 
        return NULL;
    mg = agraphof(n);
    if (!mg) 
        return NULL;
    e = agfstin(mg, n);
    if (!e) 
        return NULL;
    return agusergraph(agtail(e));
}

Agraph_t *nextsupg(Agraph_t *g, Agraph_t *sg)
{
    Agraph_t *mg;
    Agnode_t *ng, *nsg;
    Agedge_t *e;

    if (!g || !sg)
        return NULL;
    ng = g->meta_node;
    nsg = sg->meta_node;
    if (!ng || !nsg) 
        return NULL;
    mg = agraphof(ng);
    if (!mg) 
        return NULL;
    e = agfindedge(mg, nsg, ng);
    if (!e) 
        return NULL;
    e = agnxtin(mg, e);
    if (!e) 
        return NULL;
    return agusergraph(agtail(e));
}
#endif

Agedge_t *firstout(Agraph_t *g)
{
    Agnode_t *n;
    Agedge_t *e;

    if (!g)
        return NULL;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	e = agfstout(g, n);
	if (e) return e;
    }
    return NULL;
}

Agedge_t *nextout(Agraph_t *g, Agedge_t *e)
{
    Agnode_t *n;
    Agedge_t *ne;

    if (!g || !e)
        return NULL;
    ne = agnxtout(g, e);
    if (ne)
        return (ne);
    for (n = agnxtnode(g, agtail(e)); n; n = agnxtnode(g, n)) {
	ne = agfstout(g, n);
	if (ne) return ne;
    }
    return NULL;
}

Agedge_t *firstedge(Agraph_t *g)
{
    return firstout(g);
} 

Agedge_t *nextedge(Agraph_t *g, Agedge_t *e)
{
    return nextout(g, e);
} 

Agedge_t *firstout(Agnode_t *n)
{
    if (!n)
        return NULL;
    return agfstout(agraphof(n), n);
}

Agedge_t *nextout(Agnode_t *n, Agedge_t *e)
{
    if (!n || !e)
        return NULL;
    return agnxtout(agraphof(n), e);
}

Agnode_t *firsthead(Agnode_t *n)
{
    Agedge_t *e;

    if (!n)
        return NULL;
    e = agfstout(agraphof(n), n);
    if (!e)
        return NULL;
    return aghead(e);
}

Agnode_t *nexthead(Agnode_t *n, Agnode_t *h)
{
    Agedge_t *e;
    Agraph_t *g;

    if (!n || !h)
        return NULL;
    g = agraphof(n);
    e = agfindedge(g, n, h);
    if (!e)
        return NULL;
    do {
        e = agnxtout(g, e);
        if (!e)
            return NULL;
    } while (aghead(e) == h);
    return aghead(e);
}

Agedge_t *firstedge(Agnode_t *n)
{
    if (!n)
        return NULL;
    return agfstedge(agraphof(n), n);
} 

Agedge_t *nextedge(Agnode_t *n, Agedge_t *e)
{
    if (!n || !e)
        return NULL;
    return agnxtedge(agraphof(n), e, n); 
} 

Agedge_t *firstin(Agraph_t *g)
{
    Agnode_t *n;

    if (!g)
        return NULL;
    n = agfstnode(g);
    if (!n)
        return NULL;
    return agfstin(g, n);
}

Agedge_t *nextin(Agraph_t *g, Agedge_t *e)
{
    Agnode_t *n;
    Agedge_t *ne;

    if (!g || !e)
        return NULL;
    ne = agnxtin(g, e);
    if (ne)
        return (ne);
    n = agnxtnode(g, aghead(e));
    if (!n)
        return NULL;
    return agfstin(g, n);
}

Agedge_t *firstin(Agnode_t *n)
{
    if (!n)
        return NULL;
    return agfstin(agraphof(n), n);
}

Agedge_t *nextin(Agnode_t *n, Agedge_t *e)
{
    if (!n || !e)
        return NULL;
    return agnxtin(agraphof(n), e);
}

Agnode_t *firsttail(Agnode_t *n)
{
    Agedge_t *e;

    if (!n)
        return NULL;
    e = agfstin(agraphof(n), n);
    if (!e)
        return NULL;
    return agtail(e);
}

Agnode_t *nexttail(Agnode_t *n, Agnode_t *t)
{
    Agedge_t *e;
    Agraph_t *g;

    if (!n || !t)
        return NULL;
    g = agraphof(n);
    e = agfindedge(g, t, n);
    if (!e)
        return NULL;
    do {
        e = agnxtin(g, e);
        if (!e)
            return NULL;
    } while (agtail(e) == t);
    return agtail(e);
}

Agnode_t *firstnode(Agraph_t *g)
{
    if (!g)
        return NULL;
    return agfstnode(g);
}

Agnode_t *nextnode(Agraph_t *g, Agnode_t *n)
{
    if (!g || !n)
        return NULL;
    return agnxtnode(g, n);
}

Agnode_t *firstnode(Agedge_t *e)
{
    if (!e)
        return NULL;
    return agtail(e);
}

Agnode_t *nextnode(Agedge_t *e, Agnode_t *n)
{
    if (!e || n != agtail(e))
        return NULL;
    return aghead(e);
}

Agsym_t *firstattr(Agraph_t *g)
{
    if (!g)
        return NULL;
    g = agroot(g);
#ifdef WITH_CGRAPH
    return agnxtattr(g,AGRAPH,NULL);
#else
    if (dtsize(g->univ->globattr->dict) == 0)
        return NULL;
    return g->univ->globattr->list[0];
#endif
}

Agsym_t *nextattr(Agraph_t *g, Agsym_t *a)
{
    int i;

    if (!g || !a)
        return NULL;
    g = agroot(g);
#ifdef WITH_CGRAPH
    return agnxtattr(g,AGRAPH,a);
#else
    for (i = 0; i < dtsize(g->univ->globattr->dict); i++)
        if (a == g->univ->globattr->list[i])
            break;
    i++;
    if (i > dtsize(g->univ->globattr->dict))
        return NULL;
    return g->univ->globattr->list[i];
#endif
}

Agsym_t *firstattr(Agnode_t *n)
{
    Agraph_t *g;

    if (!n)
        return NULL;
    g = agraphof(n);
#ifdef WITH_CGRAPH
    return agnxtattr(g,AGNODE,NULL);
#else
    if (dtsize(g->univ->nodeattr->dict) == 0)
        return NULL;
    return g->univ->nodeattr->list[0];
#endif
}

Agsym_t *nextattr(Agnode_t *n, Agsym_t *a)
{
    Agraph_t *g;
    int i;

    if (!n || !a)
        return NULL;
    g = agraphof(n);
#ifdef WITH_CGRAPH
    return agnxtattr(g,AGNODE,a);
#else
    for (i = 0; i < dtsize(g->univ->nodeattr->dict); i++)
        if (a == g->univ->nodeattr->list[i])
            break;
    i++;
    if (i > dtsize(g->univ->nodeattr->dict))
        return NULL;
    return g->univ->nodeattr->list[i];
#endif
}

Agsym_t *firstattr(Agedge_t *e)
{
    Agraph_t *g;

    if (!e)
        return NULL;
    g = agraphof(agtail(e));
#ifdef WITH_CGRAPH
    return agnxtattr(g,AGEDGE,NULL);
#else
    if (dtsize(g->univ->edgeattr->dict) == 0)
        return NULL;
    return g->univ->edgeattr->list[0];
#endif
}

Agsym_t *nextattr(Agedge_t *e, Agsym_t *a)
{
    Agraph_t *g;
    int i;

    if (!e || !a)
        return NULL;
    g = agraphof(agtail(e));
#ifdef WITH_CGRAPH
    return agnxtattr(g,AGEDGE,a);
#else
    for (i = 0; i < dtsize(g->univ->edgeattr->dict); i++)
        if (a == g->univ->edgeattr->list[i])
            break;
    i++;
    if (i > dtsize(g->univ->edgeattr->dict))
        return NULL;
    return g->univ->edgeattr->list[i];
#endif
}

bool rm(Agraph_t *g)
{
    Agedge_t *e;

    if (!g)
        return false;
#ifdef WITH_CGRAPH
    Agraph_t* sg;
    for (sg = agfstsubg (g); sg; sg = agnxtsubg (sg))
	rm(sg);
    if (g == agroot(g))
	agclose(g);
    else
        agdelete(agroot(g), g);
    return true;
#else
    if (g->meta_node) {
        for (e = agfstout(g->meta_node->graph, g->meta_node); e;
        		e = agnxtout(g->meta_node->graph, e)) {
            rm(agusergraph(aghead(e)));
        }
        if (g == agroot(g)) {
            agclose(g);
        } else {
            agdelete(g->meta_node->graph, g);
        }
        return true;
    }
    fprintf(stderr, "subgraph has no meta_node\n");
    return false;
#endif
}

bool rm(Agnode_t *n)
{
    if (!n)
        return false;
    // removal of the protonode is not permitted
    if (agnameof(n)[0] == '\001' && strcmp (agnameof(n), "\001proto") ==0)
        return false;
    agdelete(agraphof(n), n);
    return true;
}

bool rm(Agedge_t *e)
{
    if (!e)
        return false;
    // removal of the protoedge is not permitted
    if ((agnameof(aghead(e))[0] == '\001' && strcmp (agnameof(aghead(e)), "\001proto") == 0)
     || (agnameof(agtail(e))[0] == '\001' && strcmp (agnameof(agtail(e)), "\001proto") == 0))
        return false;
    agdelete(agroot(agraphof(aghead(e))), e);
    return true;
}

bool layout(Agraph_t *g, const char *engine)
{
    int err;

    if (!g)
        return false;
    err = gvFreeLayout(gvc, g);  /* ignore errors */
    err = gvLayout(gvc, g, engine);
    return (! err);
}

// annotate the graph with layout information
bool render(Agraph_t *g)
{
    if (!g)
        return false;
    attach_attrs(g);
    return true;
}

// render to stdout
bool render(Agraph_t *g, const char *format)
{
    int err;

    if (!g)
        return false;
    err = gvRender(gvc, g, format, stdout);
    return (! err);
}

// render to an open FILE
bool render(Agraph_t *g, const char *format, FILE *f)
{
    int err;

    if (!g)
        return false;
    err = gvRender(gvc, g, format, f);
    return (! err);
}

// render to an open channel  
bool renderchannel(Agraph_t *g, const char *format, const char *channelname)
{
    int err;

    if (!g)
        return false;
    gv_channel_writer_init(gvc);
    err = gvRender(gvc, g, format, (FILE*)channelname);
    return (! err);
}

// render to a filename 
bool render(Agraph_t *g, const char *format, const char *filename)
{
    int err;

    if (!g)
        return false;
    err = gvRenderFilename(gvc, g, format, filename);
    return (! err);
}

// render to string result, using binding-dependent gv_string_writer()
void renderresult(Agraph_t *g, const char *format, char *outdata)
{
    int err;

    if (!g)
        return;
    gv_string_writer_init(gvc);
    err = gvRender(gvc, g, format, (FILE*)outdata);
}

// render to a malloc'ed data string, to be free'd by caller.
char* renderdata(Agraph_t *g, const char *format)
{
    int err;
    char *data;
    unsigned int length;

    if (!g)
	return NULL;
    err = gvRenderData(gvc, g, format, &data, &length);
    if (err)
	return NULL;
    data = (char*)realloc(data, length + 1);
    return data;
}

bool write(Agraph_t *g, FILE *f)
{
    int err;

    if (!g)
        return false;
    err = agwrite(g, f);
    return (! err);
}

bool write(Agraph_t *g, const char *filename)
{
    FILE *f;
    int err;

    if (!g)
        return false;
    f = fopen(filename, "w");
    if (!f)
        return false;
    err = agwrite(g, f);
    fclose(f);
    return (! err);
}
