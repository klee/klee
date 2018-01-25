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

 /* Implements graph.h  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rawgraph.h"
#include "memory.h"
#include "intset.h"

#define UNSCANNED 0
#define SCANNING  1
#define SCANNED   2

rawgraph*
make_graph(int n)
{
    int i;
    rawgraph* g = NEW(rawgraph);
    g->nvs = n;
    g->vertices = N_NEW(n, vertex);
    for(i=0;i<n;i++) {
        g->vertices[i].adj_list = openIntSet ();
        g->vertices[i].color = UNSCANNED;
    }
    return g;
}

void
free_graph(rawgraph* g)
{
    int i;
    for(i=0;i<g->nvs;i++)
        dtclose(g->vertices[i].adj_list);
    free (g->vertices);
    free (g);
}
 
void 
insert_edge(rawgraph* g, int v1, int v2)
{
    intitem obj;

    obj.id = v2;
    dtinsert(g->vertices[v1].adj_list,&obj);
}

void
remove_redge(rawgraph* g, int v1, int v2)
{
    intitem obj;
    obj.id = v2;
    dtdelete (g->vertices[v1].adj_list, &obj);
    obj.id = v1;
    dtdelete (g->vertices[v2].adj_list, &obj);
}

int
edge_exists(rawgraph* g, int v1, int v2)
{
    return (dtmatch (g->vertices[v1].adj_list, &v2) != 0);
}

typedef struct {
  int top;
  int* vals;
} stack;

static stack*
mkStack (int i)
{
    stack* sp = NEW(stack);
    sp->vals = N_NEW(i,int);
    sp->top = -1;
    return sp;
}

static void
freeStack (stack* s)
{
    free (s->vals);
    free (s);
}

static void
pushStack (stack* s, int i)
{
    s->top++;
    s->vals[s->top] = i;
}

static int
popStack (stack* s)
{
    int v;

    if (s->top == -1) return -1;
    v = s->vals[s->top];
    s->top--;
    return v;
}

static int
DFS_visit(rawgraph* g, int v, int time, stack* sp)
{
    Dt_t* adj;
    Dtlink_t* link;
    int id;
    vertex* vp;

    vp = g->vertices + v;
    vp->color = SCANNING;
    /* g->vertices[v].d = time; */
    adj = vp->adj_list;
    time = time + 1;

    for(link = dtflatten (adj); link; link = dtlink(adj,link)) {
        id = ((intitem*)dtobj(adj,link))->id;
        if(g->vertices[id].color == UNSCANNED)
            time = DFS_visit(g, id, time, sp);
    }
    vp->color = SCANNED;
    /* g->vertices[v].f = time; */
    pushStack (sp, v);
    return (time + 1);
}

void
top_sort(rawgraph* g)
{
    int i, v;
    int time = 0;
    int count = 0;
    stack* sp;

    if (g->nvs == 0) return;
    if (g->nvs == 1) {
        g->vertices[0].topsort_order = count;
		return;
	}

    sp = mkStack (g->nvs);
    for(i=0;i<g->nvs;i++) {
        if(g->vertices[i].color == UNSCANNED)
            time = DFS_visit(g, i, time, sp);
    }
    while((v = popStack(sp)) >= 0) {
        g->vertices[v].topsort_order = count;
        count++;
    }
    freeStack (sp);
}
