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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ACTIONS_H
#define ACTIONS_H

#include "cgraph.h"
#include "expr.h"

    extern void nodeInduce(Agraph_t * selected);
    extern Agobj_t *clone(Agraph_t * g, Agobj_t * obj);
    extern Agraph_t *cloneG(Agraph_t * g, char* name);
    extern Agobj_t *copy(Agraph_t * g, Agobj_t * obj);
    extern int copyAttr(Agobj_t * obj, Agobj_t * obj1);
    extern int indexOf(char *s1, char *s2);
    extern int rindexOf(char *s1, char *s2);
    extern int match(char *str, char *pat);
    extern int lockGraph(Agraph_t * g, int);
    extern Agraph_t *compOf(Agraph_t * g, Agnode_t * n);
    extern Agedge_t *isEdge(Agraph_t* g, Agnode_t * t, Agnode_t * h, char *key);
    extern Agnode_t *addNode(Agraph_t * g, Agnode_t * n, int doAdd);
    extern Agedge_t *addEdge(Agraph_t * g, Agedge_t * e, int doAdd);
    extern Agraph_t *sameG(void *p1, void *p2, char *fn, char *msg);
    extern int compare(Agobj_t *, Agobj_t *);
    extern int sfioWrite(Agraph_t *, Sfio_t*, Agiodisc_t*);
    extern int writeFile(Agraph_t *, char *, Agiodisc_t*);
    extern int fwriteFile(Expr_t *, Agraph_t *, int, Agiodisc_t*);
    extern Agraph_t *readFile(char *);
    extern Agraph_t *freadFile(Expr_t *, int);
    extern int openFile(Expr_t *, char *, char *);
    extern int closeFile(Expr_t *, int);
    extern char *readLine(Expr_t *, int);
    extern char *canon(Expr_t * pgm, char *);
    extern char *toHtml(Agraph_t*, char *);
    extern char *toLower(Expr_t * pgm, char *, Sfio_t*);
    extern char *toUpper(Expr_t * pgm, char *, Sfio_t*);
    extern int deleteObj(Agraph_t * g, Agobj_t * obj);
    extern char *colorx (Expr_t* ex, char* incolor, char* fmt, Sfio_t* fp);
    extern void gvstart_timer(void);
    extern double gvelapsed_sec(void);

#endif

#ifdef __cplusplus
}
#endif
