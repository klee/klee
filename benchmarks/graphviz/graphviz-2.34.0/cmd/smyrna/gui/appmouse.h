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

#ifndef APPMOUSE_H
#define APPMOUSE_H
#include "smyrnadefs.h"

extern void appmouse_left_click_down(ViewInfo* v,int x,int y);
extern void appmouse_left_click_up(ViewInfo* v,int x,int y);
extern void appmouse_left_drag(ViewInfo* v,int x,int y);

extern void appmouse_right_click_down(ViewInfo* v,int x,int y);
extern void appmouse_right_click_up(ViewInfo* v,int x,int y);
extern void appmouse_right_drag(ViewInfo* v,int x,int y);

extern void appmouse_middle_click_down(ViewInfo* v,int x,int y);
extern void appmouse_middle_click_up(ViewInfo* v,int x,int y);
extern void appmouse_middle_drag(ViewInfo* v,int x,int y);
extern void appmouse_move(ViewInfo* v,int x,int y);
extern void appmouse_key_release(ViewInfo* v,int key);
extern void appmouse_key_press(ViewInfo* v,int key);



#endif
