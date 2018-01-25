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

#ifndef _GVGETFONTLIST_H
#define _GVGETFONTLIST_H

#ifdef HAVE_PANGOCAIRO
#include <pango/pangocairo.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* gv_ps_fontname;
    char* gv_font;
} gv_font_map;

extern gv_font_map* get_font_mapping(PangoFontMap * pfm);

#ifdef __cplusplus
}
#endif

#endif  /* HAVE_PANGOCAIRO */

#endif  /* _GVGETFONTLIST_H */
