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

#ifndef ATT_GRAPHPVT_H
#define ATT_GRAPHPVT_H 1
#define _BLD_cgraph 1

#ifndef EXTERN
#define EXTERN extern
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cgraph.h>

#include	 	<ctype.h>

#ifdef HAVE_AST
#include		<ast.h>
#include		<vmalloc.h>
#else
#ifdef HAVE_VMALLOC
#include		<vmalloc.h>
#endif				/* HAVE_VMALLOC */
#include		<sys/types.h>
#include		<stdarg.h>
#include		<stdlib.h>
#include		<string.h>
#ifdef HAVE_UNISTD_H
#include	<unistd.h>
#endif				/* HAVE_UNISTD_H */
#endif				/* HAVE_AST */
#ifdef DEBUG
#include <assert.h>
#else
#define assert(x)
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifndef streq
#define streq(s,t)		((*s == *t) && !strcmp((s),(t)))
#endif
#ifdef offsetof
#undef offsetof
#endif
#ifdef HAVE_INTPTR_T
#define offsetof(typ,fld)  ((intptr_t)(&(((typ*)0)->fld)))
#else
#define offsetof(typ,fld)  ((int)(&(((typ*)0)->fld)))
#endif
#define NOTUSED(var)	(void) var

#define NILgraph			NIL(Agraph_t*)
#define NILnode				NIL(Agnode_t*)
#define NILedge				NIL(Agedge_t*)
#define NILsym				NIL(Agsym_t*)
#define NILstr				NIL(char*)

#define	SUCCESS				0
#define FAILURE				-1
#define LOCALNAMEPREFIX		'%'

#define AGDISC(g,d)			((g)->clos->disc.d)
#define AGCLOS(g,d)			((g)->clos->state.d)
#define AGNEW(g,t)			((t*)(agalloc(g,sizeof(t))))

#define ISALNUM(c) ((isalnum(c)) || ((c) == '_') || (!isascii(c)))

	/* functional definitions */
typedef Agobj_t *(*agobjsearchfn_t) (Agraph_t * g, Agobj_t * obj);
int agapply(Agraph_t * g, Agobj_t * obj, agobjfn_t fn, void *arg,
	    int preorder);

	/* global variables */
EXTERN Agraph_t *Ag_G_global;
extern char *AgDataRecName;

	/* set ordering disciplines */
extern Dtdisc_t Ag_subnode_id_disc;
extern Dtdisc_t Ag_subnode_seq_disc;
extern Dtdisc_t Ag_mainedge_id_disc;
extern Dtdisc_t Ag_subedge_id_disc;
extern Dtdisc_t Ag_mainedge_seq_disc;
extern Dtdisc_t Ag_subedge_seq_disc;
extern Dtdisc_t Ag_subgraph_id_disc;
extern Agcbdisc_t AgAttrdisc;

	/* internal constructor of graphs and subgraphs */
Agraph_t *agopen1(Agraph_t * g);
int agstrclose(Agraph_t * g);

	/* ref string management */
void agmarkhtmlstr(char *s);

	/* object set management */
Agnode_t *agfindnode_by_id(Agraph_t * g, unsigned long id);
Dtcompar_f agdictorder(Agraph_t *, Dict_t *, Dtcompar_f);
int agedgecmpf(Dict_t * d, void *arg_e0, void *arg_e1, Dtdisc_t * disc);
int agnamecmpf(Dict_t * d, void *, void *, Dtdisc_t * disc);
void agset_node_disc(Agraph_t * g, Dtdisc_t * disc);
unsigned long agnextseq(Agraph_t * g, int objtype);

/* dict helper functions */
Dict_t *agdtopen(Agraph_t * g, Dtdisc_t * disc, Dtmethod_t * method);
void agdtdisc(Agraph_t * g, Dict_t * dict, Dtdisc_t * disc);
long agdtdelete(Agraph_t * g, Dict_t * dict, void *obj);
int agdtclose(Agraph_t * g, Dict_t * dict);
void *agdictobjmem(Dict_t * dict, Void_t * p, size_t size,
		   Dtdisc_t * disc);
void agdictobjfree(Dict_t * dict, Void_t * p, Dtdisc_t * disc);

	/* name-value pair operations */
Agdatadict_t *agdatadict(Agraph_t * g, int cflag);
Agattr_t *agattrrec(void *obj);

void agraphattr_init(Agraph_t * g);
int agraphattr_delete(Agraph_t * g);
void agnodeattr_init(Agraph_t *g, Agnode_t * n);
void agnodeattr_delete(Agnode_t * n);
void agedgeattr_init(Agraph_t *g, Agedge_t * e);
void agedgeattr_delete(Agedge_t * e);

	/* parsing and lexing graph files */
int aagparse(void);
void aglexinit(Agdisc_t * disc, void *ifile);
int aaglex(void);
void aglexeof(void);

	/* ID management */
int agmapnametoid(Agraph_t * g, int objtype, char *str,
		  unsigned long *result, int allocflag);
int agallocid(Agraph_t * g, int objtype, unsigned long request);
void agfreeid(Agraph_t * g, int objtype, unsigned long id);
char *agprintid(Agobj_t * obj);
int aginternalmaplookup(Agraph_t * g, int objtype, char *str,
			unsigned long *result);
void aginternalmapinsert(Agraph_t * g, int objtype, char *str,
			 unsigned long result);
char *aginternalmapprint(Agraph_t * g, int objtype, unsigned long id);
int aginternalmapdelete(Agraph_t * g, int objtype, unsigned long id);
void aginternalmapclose(Agraph_t * g);
void agregister(Agraph_t * g, int objtype, void *obj);

	/* internal set operations */
void agedgesetop(Agraph_t * g, Agedge_t * e, int insertion);
void agdelnodeimage(Agraph_t * g, Agnode_t * node, void *ignored);
void agdeledgeimage(Agraph_t * g, Agedge_t * edge, void *ignored);
void *agrebind0(Agraph_t * g, void *obj);	/* unsafe */
int agrename(Agobj_t * obj, char *newname);
void agrecclose(Agobj_t * obj);

void agmethod_init(Agraph_t * g, void *obj);
void agmethod_upd(Agraph_t * g, void *obj, Agsym_t * sym);
void agmethod_delete(Agraph_t * g, void *obj);

#define CB_INITIALIZE	100
#define CB_UPDATE		101
#define CB_DELETION		102
void agsyspushdisc(Agraph_t * g, Agcbdisc_t * cb, void *state, int stack);
int agsyspopdisc(Agraph_t * g, Agcbdisc_t * cb, int stack);
void agrecord_callback(Agraph_t * g, Agobj_t * obj, int kind,
		       Agsym_t * optsym);
void aginitcb(Agraph_t * g, void *obj, Agcbstack_t * disc);
void agupdcb(Agraph_t * g, void *obj, Agsym_t * sym, Agcbstack_t * disc);
void agdelcb(Agraph_t * g, void *obj, Agcbstack_t * disc);

#endif				/* ATT_GRAPHPVT_H */
