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

#ifndef _GFXVIEW_H
#define _GFXVIEW_H
void GFXinit (void);
void GFXterm (void);
void GFXprune (void);
void GFXlabelcb (Gevent_t *);
void GFXviewcb (Gevent_t *);
void GFXevent (Gevent_t *);
void GFXmove (void);
void GFXredraw (void);
void GFXtextcb (int, char *);
void GFXbuttoncb (int, void *);
void GFXmonitorfile (int);
void GFXidle (void);

int GFXcreatewidget (int, lvar_t *);
int GFXsetwidgetattr (int, lvar_t *);
int GFXgetwidgetattr (int, lvar_t *);
int GFXdestroywidget (int, lvar_t *);
int GFXclear (int, lvar_t *);
int GFXsetgfxattr (int, lvar_t *);
int GFXgetgfxattr (int, lvar_t *);
int GFXarrow (int, lvar_t *);
int GFXline (int, lvar_t *);
int GFXbox (int, lvar_t *);
int GFXpolygon (int, lvar_t *);
int GFXsplinegon (int, lvar_t *);
int GFXarc (int, lvar_t *);
int GFXtext (int, lvar_t *);
int GFXtextsize (int, lvar_t *);
int GFXcreatebitmap (int, lvar_t *);
int GFXdestroybitmap (int, lvar_t *);
int GFXreadbitmap (int, lvar_t *);
int GFXwritebitmap (int, lvar_t *);
int GFXbitblt (int, lvar_t *);
int GFXclearpick (int, lvar_t *);
int GFXsetpick (int, lvar_t *);
int GFXdisplaymenu (int, lvar_t *);
int GFXcolormap (int, lvar_t *);
#endif /* _GFXVIEW_H */

#ifdef __cplusplus
}
#endif

