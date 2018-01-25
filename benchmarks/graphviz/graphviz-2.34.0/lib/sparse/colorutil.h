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

#ifndef COLORUTIL_H
#define COLORUTIL_H

#include <color.h>

extern int colorxlate(char *str, gvcolor_t * color, color_type_t target_type);

void rgb2hex(float r, float g, float b, char *cstring, char* opacity);/* dimension of cstring must be >=7 */
char* hue2rgb(real hue, char *color);
void hue2rgb_real(real hue, real *color);

#endif
