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
#ifndef SMYRNA_UTILS_H
#define SMYRNA_UTILS_H

#include "smyrnadefs.h"
#include "cgraph.h"

extern char *xml_string(char *s);
extern int l_int(void *obj, Agsym_t * attr, int def);
extern float l_float(void *obj, Agsym_t * attr, float def);
extern int mapbool(char *p);
extern int getAttrBool(Agraph_t* g,void* obj,char* attr_name,int def);
extern int getAttrInt(Agraph_t* g,void* obj,char* attr_name,int def);
extern float getAttrFloat(Agraph_t* g,void* obj,char* attr_name,float def);
extern char* getAttrStr(Agraph_t* g,void* obj,char* attr_name,char* def);
extern void setColor(glCompColor* c,GLfloat R,GLfloat G,GLfloat B,GLfloat A);
extern void getcolorfromschema(colorschemaset * sc, float l, float maxl,glCompColor * c);
extern glCompPoint getPointFromStr(char* str);
#if 0
extern float distance_to_line(float ax, float ay, float bx, float by, float cx,float cy);
#endif
extern int point_in_polygon(glCompPoly* selPoly,glCompPoint p);
extern int point_within_ellips_with_coords(float ex, float ey,
					       float ea, float eb,
					       float px, float py);
extern int point_within_sphere_with_coords(float x0, float y0, float z0,
					float r, float x, float y,
					float z);

#endif
