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

/* Lefteris Koutsofios - AT&T Labs Research */

#ifndef _TXTVIEW_H
#define _TXTVIEW_H
void TXTinit (Grect_t);
void TXTterm (void);
int TXTmode (int argc, lvar_t *argv);
int TXTask (int argc, lvar_t *argv);
void TXTprocess (int, char *);
void TXTupdate (void);
void TXTtoggle (int, void *);
#endif /* _TXTVIEW_H */

#ifdef __cplusplus
}
#endif
