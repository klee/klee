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

%{
#if _PACKAGE_ast
#include <ast.h>
#endif
#include <stdio.h>

typedef void *Tobj;

#include "config.h"
#include "dot2l.h"

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#include <string.h>

static char portstr[SMALLBUF];

extern void yyerror(const char *fmt, ...);
%}

%union {
    long i;
    char *s;
    void *o;
}

%token <i> T_graph T_digraph T_strict
%token <i> T_node T_edge T_edgeop
%token <s> T_id
%type <o> node_name node_id subg_stmt
%type <s> optgraphname
%left <i> T_subgraph /* to eliminate subgraph hdr shift/reduce conflict */
%left '{'
%%

file: graph_type optgraphname
    { D2Lbegin ($2); free ($2); }
  '{' stmt_list '}'
    { D2Lend (); }
| error
    { D2Labort (); }
| /* empty*/
    { D2Labort (); }
;

optgraphname:   T_id {$$=$1;} | /* empty */ {$$=0;} ;

graph_type: T_graph /* safe to change graph type/name before contents appear */
    { gtype = "graph"; etype = "--"; }
| T_strict T_graph
    { gtype = "strict graph"; etype = "--"; }
| T_digraph
    { gtype = "digraph"; etype = "->"; }
| T_strict T_digraph
    { gtype = "strict digraph"; etype = "->"; }
;

stmt_list: stmt_list1
| /* empty */
;

stmt_list1: stmt
| stmt_list1 stmt
;

stmt: stmt1
| stmt1 ';'
;

stmt1: node_stmt /* create nodes and set attributes */
| edge_stmt /* create edges and set attributes */
| attr_stmt /* reset value of attributes */
| subg_stmt {}
;

node_stmt: node_id { attrclass = NODE; portstr[0] = '\000'; }
    opt_attr_list { attrclass = GRAPH; }
;

node_id: node_name node_port
    { $$ = $1; }
;

node_name: T_id
    { $$ = D2Linsertnode ($1); free ($1); }
;

edge_stmt: node_id
    { D2Lbeginedge (NODE, $1, portstr); portstr[0] = '\000'; }
  edgeRHS
    { attrclass = EDGE; }
  opt_attr_list
    { D2Lendedge (); attrclass = GRAPH; }
| subg_stmt
    { D2Lbeginedge (GRAPH, $1, ""); }
  edgeRHS
    { attrclass = EDGE; }
  opt_attr_list
    { D2Lendedge (); attrclass = GRAPH; }
;

edgeRHS: T_edgeop node_id
    { D2Lmidedge (NODE, $2, portstr); portstr[0] = '\000'; }
| T_edgeop node_id
    { D2Lmidedge (NODE, $2, portstr); portstr[0] = '\000'; } edgeRHS
| T_edgeop subg_stmt
    { D2Lmidedge (GRAPH, $2, ""); portstr[0] = '\000'; }
| T_edgeop subg_stmt
    { D2Lmidedge (GRAPH, $2, ""); portstr[0] = '\000'; } edgeRHS
;

node_port: /* empty */
| port_location
| port_angle
| port_angle port_location
| port_location port_angle
;

port_location: ':' T_id
    {
        strcat (portstr, $2); free ($2);
    }
| ':' '(' T_id ',' T_id ')'
    {
        strcat (portstr, "("); strcat (portstr, $3);
        strcat (portstr, ","); strcat (portstr, $5);
        strcat (portstr, ")");
        free ($3), free ($5);
    }
;

port_angle: '@' T_id
    {
        strcat (portstr, "@"); strcat (portstr, $2); free ($2);
    }
;

attr_stmt: attr_class
    { inattrstmt = TRUE; }
  attr_list
    { attrclass = GRAPH; inattrstmt = FALSE; }
| attr_set
    { attrclass = GRAPH; }
;

attr_class: T_graph
    { attrclass = GRAPH; }
| T_node
    { attrclass = NODE; }
| T_edge
    { attrclass = EDGE; }
;

opt_attr_list: rec_attr_list
;

rec_attr_list: rec_attr_list attr_list
| /* empty */
;

attr_list: '[' inside_attr_list ']'
;

inside_attr_list: attr_set optcomma inside_attr_list
| /* empty */
;

attr_set: T_id '=' T_id
    { D2Lsetattr ($1, $3); free ($1); free ($3); }
;

optcomma: /* empty */
| ','
;

subg_stmt: subg_hdr '{' stmt_list '}' %prec '{'
    { $$ = D2Lpopgraph (); }
| '{' { D2Lpushgraph (NULL); } stmt_list '}'
    { $$ = D2Lpopgraph (); }
| subg_hdr %prec T_subgraph
    { $$ = D2Lpopgraph (); }
;

subg_hdr: T_subgraph T_id
    { D2Lpushgraph ($2); free ($2); }
;
