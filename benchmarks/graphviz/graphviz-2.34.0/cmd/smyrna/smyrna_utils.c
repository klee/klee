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
#include "smyrna_utils.h"
#include "memory.h"
/* many of these functions are available in libcommon.
 * We cannot use those because dependencies cause a great
 * deal of libcommon to be brought in, which complicates
 * and makes possible conflicts as some parts of libcommon
 * rely on not using libcgraph.
 */
int mapbool(char *p)
{
    if (p == NULL)
	return FALSE;
    if (!strcasecmp(p, "false"))
	return FALSE;
    if (!strcasecmp(p, "no"))
	return FALSE;
    if (!strcasecmp(p, "true"))
	return TRUE;
    if (!strcasecmp(p, "yes"))
	return TRUE;
    return atoi(p);
}

/* return true if *s points to &[A-Za-z]*;      (e.g. &Ccedil; )
 *                          or &#[0-9]*;        (e.g. &#38; )
 *                          or &#x[0-9a-fA-F]*; (e.g. &#x6C34; )
 */
static int xml_isentity(char *s)
{
    s++;			/* already known to be '&' */
    if (*s == '#') {
	s++;
	if (*s == 'x' || *s == 'X') {
	    s++;
	    while ((*s >= '0' && *s <= '9')
		   || (*s >= 'a' && *s <= 'f')
		   || (*s >= 'A' && *s <= 'F'))
		s++;
	} else {
	    while (*s >= '0' && *s <= '9')
		s++;
	}
    } else {
	while ((*s >= 'a' && *s <= 'z')
	       || (*s >= 'A' && *s <= 'Z'))
	    s++;
    }
    if (*s == ';')
	return 1;
    return 0;
}

char *xml_string(char *s)
{
    static char *buf = NULL;
    static int bufsize = 0;
    char *p, *sub, *prev = NULL;
    int len, pos = 0;

    if (!buf) {
	bufsize = 64;
	buf = N_NEW(bufsize, char);
    }

    p = buf;
    while (s && *s) {
	if (pos > (bufsize - 8)) {
	    bufsize *= 2;
	    buf = realloc(buf, bufsize);
	    p = buf + pos;
	}
	/* escape '&' only if not part of a legal entity sequence */
	if (*s == '&' && !(xml_isentity(s))) {
	    sub = "&amp;";
	    len = 5;
	}
	/* '<' '>' are safe to substitute even if string is already UTF-8 coded
	 * since UTF-8 strings won't contain '<' or '>' */
	else if (*s == '<') {
	    sub = "&lt;";
	    len = 4;
	}
	else if (*s == '>') {
	    sub = "&gt;";
	    len = 4;
	}
	else if (*s == '-') {	/* can't be used in xml comment strings */
	    sub = "&#45;";
	    len = 5;
	}
	else if (*s == ' ' && prev && *prev == ' ') {
	    /* substitute 2nd and subsequent spaces with required_spaces */
	    sub = "&#160;";  /* inkscape doesn't recognise &nbsp; */
	    len = 6;
	}
	else if (*s == '"') {
	    sub = "&quot;";
	    len = 6;
	}
	else if (*s == '\'') {
	    sub = "&#39;";
	    len = 5;
	}
	else {
	    sub = s;
	    len = 1;
	}
	while (len--) {
	    *p++ = *sub++;
	    pos++;
	}
	prev = s;
	s++;
    }
    *p = '\0';
    return buf;
}
#if 0
static int late_int(void *obj,Agsym_t* attr, int def, int low)
{
    char *p;
    int rv;
    if (attr == NULL)
	return def;
    p = agxget(obj, attr);
    if (!p || p[0] == '\0')
	return def;
    if ((rv = atoi(p)) < low)
	rv = low;
    return rv;
}

static double late_double(void *obj, Agsym_t* attr, double def, double low)
{
    char *p;
    double rv;

    if (!attr || !obj)
	return def;
    p = agxget(obj, attr);
    if (!p || p[0] == '\0')
	return def;
    if ((rv = atof(p)) < low)
	rv = low;
    return rv;
}

static char *late_nnstring(void *obj, Agsym_t * attr, char *def)
{
    char *rv = late_string(obj, attr, def);
    if (!rv || (rv[0] == '\0'))
	rv = def;
    return rv;
}

#endif

static char* late_string(void *obj, Agsym_t * attr, char *def)
{
    if (!attr || !obj)
	return def;
    return agxget(obj, attr);
}

static int late_bool(void *obj, Agsym_t * attr, int def)
{
    if (attr == NULL)
	return def;
    return mapbool(agxget(obj, attr));
}

int l_int(void *obj, Agsym_t * attr, int def)
{
    char *p;
    if (attr == NULL)
	return def;
    p = agxget(obj, attr);
    if (!p || p[0] == '\0')
	return def;
    return atoi(p);
}

float l_float(void *obj, Agsym_t * attr, float def)
{
    char *p;
    if (!attr || !obj)
	return def;
    p = agxget(obj, attr);
    if (!p || p[0] == '\0')
	return def;
    return atof(p);
}
int getAttrBool(Agraph_t* g,void* obj,char* attr_name,int def)
{
    Agsym_t* attr;
    attr = agattr(g, AGTYPE(obj), attr_name,0);
    return late_bool(obj, attr,def);
}
int getAttrInt(Agraph_t* g,void* obj,char* attr_name,int def)
{
    Agsym_t* attr;
    attr = agattr(g, AGTYPE(obj), attr_name,0);
    return l_int(obj,attr,def);
}
float getAttrFloat(Agraph_t* g,void* obj,char* attr_name,float def)
{
    Agsym_t* attr;
    attr = agattr(g, AGTYPE(obj), attr_name,0);
    return l_float(obj,attr,def);
}
char* getAttrStr(Agraph_t* g,void* obj,char* attr_name,char* def)
{
    Agsym_t* attr;
    attr = agattr(g, AGTYPE(obj), attr_name,0);
    return late_string(obj, attr,def);
}


void setColor(glCompColor* c,GLfloat R,GLfloat G,GLfloat B,GLfloat A)
{
    c->R=R;
    c->G=G;
    c->B=B;
    c->A=A;
}
glCompPoint getPointFromStr(char* str)
{

    glCompPoint p;
    char* a;
    char bf[512];
    strcpy(bf,str);
    p.x=0;
    p.y=0;
    p.z=0;
    a=strtok(bf,",");
    if(a)
	p.x=atof(a);
    a=strtok(NULL,",");
    if(a)
	p.y=atof(a);
    a=strtok(NULL,",");
    if(a)
	p.z=atof(a);
    return p;
}


#if 0
static float interpol(float minv, float maxv, float minc, float maxc, float x)
{
    return ((x - minv) * (maxc - minc) / (maxv - minv) + minc);
}


void 
getcolorfromschema(colorschemaset * sc, float l, float maxl,glCompColor * c)
{
    int ind;
    float percl = l / maxl * 100.00;
    for (ind = 0; ind < sc->schemacount; ind++) {
	if (percl < sc->s[ind].perc)
	    break;
    }

    if (sc->s[ind].smooth) {
	c->R =
	    interpol(sc->s[ind - 1].perc, sc->s[ind].perc,
		     sc->s[ind - 1].c.R, sc->s[ind].c.R, percl);
	c->G =
	    interpol(sc->s[ind - 1].perc, sc->s[ind].perc,
		     sc->s[ind - 1].c.G, sc->s[ind].c.G, percl);
	c->B =
	    interpol(sc->s[ind - 1].perc, sc->s[ind].perc,
		     sc->s[ind - 1].c.B, sc->s[ind].c.B, percl);
	c->A = 1;
    } else {
	c->R = sc->s[ind].c.R;
	c->G = sc->s[ind].c.G;
	c->B = sc->s[ind].c.B;
	c->A = 1;
    }
}
#endif

int point_within_ellips_with_coords(float ex, float ey, float ea, float eb,
				    float px, float py)
{

    float dx, dy;
    float a;
    dx = px - ex;
    dy = py - ey;
    a = (dx * dx) / (ea * ea) + (dy * dy) / (eb * eb);
    return (a <= 1);
}
int point_within_sphere_with_coords(float x0, float y0, float z0, float r,
				    float x, float y, float z)
{
    float rr =
	(x - x0) * (x - x0) + (y - y0) * (y - y0) + (z - z0) * (z - z0);
    rr = (float) pow(rr, 0.5);
    if (rr <= r)
	return 1;
    return 0;
}
#if 0
float distance_to_line(float ax, float ay, float bx, float by, float cx,
		       float cy)
{
    //this function returns the distance between a line(a-b) segment and a point(c) in 2D plane
    return (float)
	sqrt(pow(((by - ay) * (cx - ax) + (bx - ax) * (cy - ay)), 2)
	     / (pow((bx - ax), 2) + pow((by - ay), 2))
	);
}

int _point_in_polygon(int npol, float *xp, float *yp, float x, float y)
{
    int i, j, c = 0;
      for (i = 0, j = npol-1; i < npol; j = i++) {
        if ((((yp[i] <= y) && (y < yp[j])) ||
             ((yp[j] <= y) && (y < yp[i]))) &&
            (x < (xp[j] - xp[i]) * (y - yp[i]) / (yp[j] - yp[i]) + xp[i]))
          c = !c;
      }
      return c;
}
#endif


int point_in_polygon(glCompPoly* selPoly,glCompPoint p)
{
    int npol=selPoly->cnt;

    int i, j, c = 0;
      for (i = 0, j = npol-1; i < npol; j = i++) 
      {
        if ((((selPoly->pts[i].y <= p.y) && (p.y < selPoly->pts[j].y)) ||
             ((selPoly->pts[j].y <= p.y) && (p.y < selPoly->pts[i].y))) &&
            (p.x < (selPoly->pts[j].x - selPoly->pts[i].x) * (p.y - selPoly->pts[i].y) / (selPoly->pts[j].y - selPoly->pts[i].y) + selPoly->pts[i].x))
          c = !c;
      }
      return c;
    }


