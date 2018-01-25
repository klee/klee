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

#ifndef PARSE_H
#define PARSE_H

    typedef enum { Begin =
	    0, End, BeginG, EndG, Node, Edge, Eof, Error } case_t;

    typedef struct _case_info {
	int gstart;
	char *guard;
	int astart;
	char *action;
	struct _case_info *next;
    } case_info;

    typedef struct _parse_block {
	int l_beging;
	char *begg_stmt;
	int n_nstmts;
	int n_estmts;
	case_info *node_stmts;
	case_info *edge_stmts;
	struct _parse_block *next;
    } parse_block; 

    typedef struct {
	char *source;
	int l_begin, l_end, l_endg;
	char *begin_stmt;
	int n_blocks;
	parse_block *blocks;
	char *endg_stmt;
	char *end_stmt;
    } parse_prog;

    extern parse_prog *parseProg(char *, int);
    extern void freeParseProg (parse_prog *);

#endif

#ifdef __cplusplus
}
#endif
