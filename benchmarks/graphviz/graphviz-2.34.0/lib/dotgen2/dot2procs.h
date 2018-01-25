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

#ifndef DOT2PROCS_H
#define DOT2PROCS_H

#ifdef _BEGIN_EXTERNS_
_BEGIN_EXTERNS_                 /* public data */
#endif
/* tabs at 8, or you're a sorry loser */
#ifdef __cplusplus
extern "C" {
#endif

extern int is_nonconstraint(edge_t * e);

#if defined(_BLD_dot) && defined(_DLL)
#   define extern __EXPORT__
#endif
    extern Agraph_t* dot2_mincross(Agraph_t *);
    /* extern void dot_position(Agraph_t *, aspect_t*); */
    extern void dot2_levels(Agraph_t *);
    /* extern void dot_sameports(Agraph_t *); */
    /* extern void dot_splines(Agraph_t *); */
#undef extern

#ifdef _END_EXTERNS_
     _END_EXTERNS_
#endif
#ifdef __cplusplus
}
#endif

#endif
