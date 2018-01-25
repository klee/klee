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

#include "render.h"
#include "agxbuf.h"
#include "htmltable.h"
#include "entities.h"
#include "logic.h"
#include "gvc.h"

#ifdef WIN32
#include "libltdl/lt_system.h"
#endif
#ifndef WIN32
#include <unistd.h>
#endif
#include <ctype.h>

/*
 *  a queue of nodes
 */
nodequeue *new_queue(int sz)
{
    nodequeue *q = NEW(nodequeue);

    if (sz <= 1)
	sz = 2;
    q->head = q->tail = q->store = N_NEW(sz, node_t *);
    q->limit = q->store + sz;
    return q;
}

void free_queue(nodequeue * q)
{
    free(q->store);
    free(q);
}

void enqueue(nodequeue * q, node_t * n)
{
    *(q->tail++) = n;
    if (q->tail >= q->limit)
	q->tail = q->store;
}

node_t *dequeue(nodequeue * q)
{
    node_t *n;
    if (q->head == q->tail)
	n = NULL;
    else {
	n = *(q->head++);
	if (q->head >= q->limit)
	    q->head = q->store;
    }
    return n;
}

int late_int(void *obj, attrsym_t * attr, int def, int low)
{
    char *p;
    char *endp;
    int rv;
    if (attr == NULL)
	return def;
    p = ag_xget(obj, attr);
    if (!p || p[0] == '\0')
	return def;
    rv = strtol (p, &endp, 10);
    if (p == endp) return def;  /* invalid int format */
    if (rv < low) return low;
    else return rv;
}

double late_double(void *obj, attrsym_t * attr, double def, double low)
{
    char *p;
    char *endp;
    double rv;

    if (!attr || !obj)
	return def;
    p = ag_xget(obj, attr);
    if (!p || p[0] == '\0')
	return def;
    rv = strtod (p, &endp);
    if (p == endp) return def;  /* invalid double format */
    if (rv < low) return low;
    else return rv;
}

char *late_string(void *obj, attrsym_t * attr, char *def)
{
    if (!attr || !obj)
	return def;
#ifndef WITH_CGRAPH
    return agxget(obj, attr->index);
#else /* WITH_CGRAPH */
    return agxget(obj, attr);
#endif /* WITH_CGRAPH */
}

char *late_nnstring(void *obj, attrsym_t * attr, char *def)
{
    char *rv = late_string(obj, attr, def);
    if (!rv || (rv[0] == '\0'))
	rv = def;
    return rv;
}

boolean late_bool(void *obj, attrsym_t * attr, int def)
{
    if (attr == NULL)
	return def;
#ifndef WITH_CGRAPH
    return mapbool(agxget(obj, attr->index));
#else /* WITH_CGRAPH */

    return mapbool(agxget(obj, attr));
#endif /* WITH_CGRAPH */
}

/* union-find */
node_t *UF_find(node_t * n)
{
    while (ND_UF_parent(n) && (ND_UF_parent(n) != n)) {
#ifndef WITH_CGRAPH
	if (ND_UF_parent(n)->u.UF_parent)
	    ND_UF_parent(n) = ND_UF_parent(n)->u.UF_parent;
#else /* WITH_CGRAPH */
	if (ND_UF_parent(ND_UF_parent(n)))
	    ND_UF_parent(n) = ND_UF_parent(ND_UF_parent(n));
#endif /* WITH_CGRAPH */
	n = ND_UF_parent(n);
    }
    return n;
}

node_t *UF_union(node_t * u, node_t * v)
{
    if (u == v)
	return u;
    if (ND_UF_parent(u) == NULL) {
	ND_UF_parent(u) = u;
	ND_UF_size(u) = 1;
    } else
	u = UF_find(u);
    if (ND_UF_parent(v) == NULL) {
	ND_UF_parent(v) = v;
	ND_UF_size(v) = 1;
    } else
	v = UF_find(v);
#ifndef WITH_CGRAPH
    if (u->id > v->id) {
#else /* WITH_CGRAPH */
    if (ND_id(u) > ND_id(v)) {
#endif /* WITH_CGRAPH */
	ND_UF_parent(u) = v;
	ND_UF_size(v) += ND_UF_size(u);
    } else {
	ND_UF_parent(v) = u;
	ND_UF_size(u) += ND_UF_size(v);
	v = u;
    }
    return v;
}

void UF_remove(node_t * u, node_t * v)
{
    assert(ND_UF_size(u) == 1);
    ND_UF_parent(u) = u;
    ND_UF_size(v) -= ND_UF_size(u);
}

void UF_singleton(node_t * u)
{
    ND_UF_size(u) = 1;
    ND_UF_parent(u) = NULL;
    ND_ranktype(u) = NORMAL;
}

void UF_setname(node_t * u, node_t * v)
{
    assert(u == UF_find(u));
    ND_UF_parent(u) = v;
    ND_UF_size(v) += ND_UF_size(u);
}

pointf coord(node_t * n)
{
    pointf r;

    r.x = POINTS_PER_INCH * ND_pos(n)[0];
    r.y = POINTS_PER_INCH * ND_pos(n)[1];
    return r;
}

/* from Glassner's Graphics Gems */
#define W_DEGREE 5

/*
 *  Bezier : 
 *	Evaluate a Bezier curve at a particular parameter value
 *      Fill in control points for resulting sub-curves if "Left" and
 *	"Right" are non-null.
 * 
 */
pointf Bezier(pointf * V, int degree, double t, pointf * Left, pointf * Right)
{
    int i, j;			/* Index variables      */
    pointf Vtemp[W_DEGREE + 1][W_DEGREE + 1];

    /* Copy control points  */
    for (j = 0; j <= degree; j++) {
	Vtemp[0][j] = V[j];
    }

    /* Triangle computation */
    for (i = 1; i <= degree; i++) {
	for (j = 0; j <= degree - i; j++) {
	    Vtemp[i][j].x =
		(1.0 - t) * Vtemp[i - 1][j].x + t * Vtemp[i - 1][j + 1].x;
	    Vtemp[i][j].y =
		(1.0 - t) * Vtemp[i - 1][j].y + t * Vtemp[i - 1][j + 1].y;
	}
    }

    if (Left != NULL)
	for (j = 0; j <= degree; j++)
	    Left[j] = Vtemp[j][0];
    if (Right != NULL)
	for (j = 0; j <= degree; j++)
	    Right[j] = Vtemp[degree - j][j];

    return (Vtemp[degree][0]);
}

#ifdef DEBUG
edge_t *debug_getedge(graph_t * g, char *s0, char *s1)
{
    node_t *n0, *n1;
    n0 = agfindnode(g, s0);
    n1 = agfindnode(g, s1);
    if (n0 && n1)
	return agfindedge(g, n0, n1);
    else
	return NULL;
}
#ifdef WITH_CGRAPH
Agraphinfo_t* GD_info(g) { return ((Agraphinfo_t*)AGDATA(g));}
Agnodeinfo_t* ND_info(n) { return ((Agnodeinfo_t*)AGDATA(n));}
#endif
#endif

#if !defined(MSWIN32) && !defined(WIN32)
#include	<pwd.h>

#if 0
static void cleanup(void)
{
    agxbfree(&xb);
}
#endif
#endif

/* Fgets:
 * Read a complete line.
 * Return pointer to line, 
 * or 0 on EOF
 */
char *Fgets(FILE * fp)
{
    static int bsize = 0;
    static char *buf;
    char *lp;
    int len;

    len = 0;
    do {
	if (bsize - len < BUFSIZ) {
	    bsize += BUFSIZ;
	    buf = grealloc(buf, bsize);
	}
	lp = fgets(buf + len, bsize - len, fp);
	if (lp == 0)
	    break;
	len += strlen(lp);	/* since lp != NULL, len > 0 */
    } while (buf[len - 1] != '\n');

    if (len > 0)
	return buf;
    else
	return 0;
}

/* safefile:
 * Check to make sure it is okay to read in files.
 * It returns NULL if the filename is trivial.
 *
 * If the application has set the SERVER_NAME environment variable, 
 * this indicates it is web-active. In this case, it requires that the GV_FILE_PATH
 * environment variable be set. This gives the legal directories
 * from which files may be read. safefile then derives the rightmost component
 * of filename, where components are separated by a slash, backslash or colon, 
 * It then checks for the existence of a file consisting of a directory from 
 * GV_FILE_PATH followed by the rightmost component of filename. It returns the
 * first such found, or NULL otherwise.
 * The filename returned is thus
 * Gvfilepath concatenated with the last component of filename,
 * where a component is determined by a slash, backslash or colon
 * character.  
 *
 * If filename contains multiple components, the user is
 * warned, once, that everything to the left is ignored.
 *
 * For non-server applications, we use the path list in Gvimagepath to 
 * resolve relative pathnames.
 *
 * N.B. safefile uses a fixed buffer, so functions using it should use the
 * value immediately or make a copy.
 */
#ifdef WIN32
#define PATHSEP ";"
#else
#define PATHSEP ":"
#endif

static char** mkDirlist (const char* list, int* maxdirlen)
{
    int cnt = 0;
    char* s = strdup (list);
    char* dir;
    char** dirs = NULL;
    int maxlen = 0;

    for (dir = strtok (s, PATHSEP); dir; dir = strtok (NULL, PATHSEP)) {
	dirs = ALLOC (cnt+2,dirs,char*);
	dirs[cnt++] = dir;
	maxlen = MAX(maxlen, strlen (dir));
    }
    dirs[cnt] = NULL;
    *maxdirlen = maxlen;
    return dirs;
}

static char* findPath (char** dirs, int maxdirlen, const char* str)
{
    static char *safefilename = NULL;
    char** dp;

	/* allocate a buffer that we are sure is big enough
         * +1 for null character.
         * +1 for directory separator character.
         */
    safefilename = realloc(safefilename, (maxdirlen + strlen(str) + 2));

    for (dp = dirs; *dp; dp++) {
	sprintf (safefilename, "%s%s%s", *dp, DIRSEP, str);
	if (access (safefilename, R_OK) == 0)
	    return safefilename;
    }
    return NULL;
}

const char *safefile(const char *filename)
{
    static boolean onetime = TRUE;
    static char *pathlist = NULL;
    static int maxdirlen;
    static char** dirs;
    const char *str, *p;

    if (!filename || !filename[0])
	return NULL;

    if (HTTPServerEnVar) {   /* If used as a server */
	/* 
	 * If we are running in an http server we allow
	 * files only from the directory specified in
	 * the GV_FILE_PATH environment variable.
	 */
	if (!Gvfilepath || (*Gvfilepath == '\0')) {
	    if (onetime) {
		agerr(AGWARN,
		      "file loading is disabled because the environment contains SERVER_NAME=\"%s\"\n"
		      "and the GV_FILE_PATH variable is unset or empty.\n",
		      HTTPServerEnVar);
		onetime = FALSE;
	    }
	    return NULL;
	}
	if (!pathlist) {
	    dirs = mkDirlist (Gvfilepath, &maxdirlen);
	    pathlist = Gvfilepath;
	}

	str = filename;
	if ((p = strrchr(str, '/')))
	    str = ++p;
	if ((p = strrchr(str, '\\')))
	    str = ++p;
	if ((p = strrchr(str, ':')))
	    str = ++p;

	if (onetime && str != filename) {
	    agerr(AGWARN, "Path provided to file: \"%s\" has been ignored"
		  " because files are only permitted to be loaded from the directories in \"%s\""
		  " when running in an http server.\n", filename, Gvfilepath);
	    onetime = FALSE;
	}

	return findPath (dirs, maxdirlen, str);
    }

    if (pathlist != Gvimagepath) {
	if (dirs) {
	    free (dirs[0]);
	    free (dirs);
	    dirs = NULL;
	}
	pathlist = Gvimagepath;
	if (pathlist && *pathlist)
	    dirs = mkDirlist (pathlist, &maxdirlen);
    }

    if ((*filename == DIRSEP[0]) || !dirs)
	return filename;

    return findPath (dirs, maxdirlen, filename);
}

int maptoken(char *p, char **name, int *val)
{
    int i;
    char *q;

    for (i = 0; (q = name[i]) != 0; i++)
	if (p && streq(p, q))
	    break;
    return val[i];
}

boolean mapBool(char *p, boolean dflt)
{
    if (!p || (*p == '\0'))
	return dflt;
    if (!strcasecmp(p, "false"))
	return FALSE;
    if (!strcasecmp(p, "no"))
	return FALSE;
    if (!strcasecmp(p, "true"))
	return TRUE;
    if (!strcasecmp(p, "yes"))
	return TRUE;
    if (isdigit(*p))
	return atoi(p);
    else
	return dflt;
}

boolean mapbool(char *p)
{
    return mapBool (p, FALSE);
}

pointf dotneato_closest(splines * spl, pointf pt)
{
    int i, j, k, besti, bestj;
    double bestdist2, d2, dlow2, dhigh2; /* squares of distances */
    double low, high, t;
    pointf c[4], pt2;
    bezier bz;

    besti = bestj = -1;
    bestdist2 = 1e+38;
    for (i = 0; i < spl->size; i++) {
	bz = spl->list[i];
	for (j = 0; j < bz.size; j++) {
	    pointf b;

	    b.x = bz.list[j].x;
	    b.y = bz.list[j].y;
	    d2 = DIST2(b, pt);
	    if ((bestj == -1) || (d2 < bestdist2)) {
		besti = i;
		bestj = j;
		bestdist2 = d2;
	    }
	}
    }

    bz = spl->list[besti];
    /* Pick best Bezier. If bestj is the last point in the B-spline, decrement.
     * Then set j to be the first point in the corresponding Bezier by dividing
     * then multiplying be 3. Thus, 0,1,2 => 0; 3,4,5 => 3, etc.
     */
    if (bestj == bz.size-1)
	bestj--;
    j = 3*(bestj / 3);
    for (k = 0; k < 4; k++) {
	c[k].x = bz.list[j + k].x;
	c[k].y = bz.list[j + k].y;
    }
    low = 0.0;
    high = 1.0;
    dlow2 = DIST2(c[0], pt);
    dhigh2 = DIST2(c[3], pt);
    do {
	t = (low + high) / 2.0;
	pt2 = Bezier(c, 3, t, NULL, NULL);
	if (fabs(dlow2 - dhigh2) < 1.0)
	    break;
	if (fabs(high - low) < .00001)
	    break;
	if (dlow2 < dhigh2) {
	    high = t;
	    dhigh2 = DIST2(pt2, pt);
	} else {
	    low = t;
	    dlow2 = DIST2(pt2, pt);
	}
    } while (1);
    return pt2;
}

pointf spline_at_y(splines * spl, double y)
{
    int i, j;
    double low, high, d, t;
    pointf c[4], p;
    static bezier bz;

/* this caching seems to prevent p.x from getting set from bz.list[0].x
	- optimizer problem ? */

#if 0
    static splines *mem = NULL;

    if (mem != spl) {
	mem = spl;
#endif
	for (i = 0; i < spl->size; i++) {
	    bz = spl->list[i];
	    if (BETWEEN(bz.list[bz.size - 1].y, y, bz.list[0].y))
		break;
	}
#if 0
    }
#endif
    if (y > bz.list[0].y)
	p = bz.list[0];
    else if (y < bz.list[bz.size - 1].y)
	p = bz.list[bz.size - 1];
    else {
	for (i = 0; i < bz.size; i += 3) {
	    for (j = 0; j < 3; j++) {
		if ((bz.list[i + j].y <= y) && (y <= bz.list[i + j + 1].y))
		    break;
		if ((bz.list[i + j].y >= y) && (y >= bz.list[i + j + 1].y))
		    break;
	    }
	    if (j < 3)
		break;
	}
	assert(i < bz.size);
	for (j = 0; j < 4; j++) {
	    c[j].x = bz.list[i + j].x;
	    c[j].y = bz.list[i + j].y;
	    /* make the spline be monotonic in Y, awful but it works for now */
	    if ((j > 0) && (c[j].y > c[j - 1].y))
		c[j].y = c[j - 1].y;
	}
	low = 0.0;
	high = 1.0;
	do {
	    t = (low + high) / 2.0;
	    p = Bezier(c, 3, t, NULL, NULL);
	    d = p.y - y;
	    if (ABS(d) <= 1)
		break;
	    if (d < 0)
		high = t;
	    else
		low = t;
	} while (1);
    }
    p.y = y;
    return p;
}

pointf neato_closest(splines * spl, pointf p)
{
/* this is a stub so that we can share a common emit.c between dot and neato */

    return spline_at_y(spl, p.y);
}

static int Tflag;
void gvToggle(int s)
{
    Tflag = !Tflag;
#if !defined(MSWIN32) && !defined(WIN32)
    signal(SIGUSR1, gvToggle);
#endif
}

int test_toggle()
{
    return Tflag;
}

struct fontinfo {
    double fontsize;
    char *fontname;
    char *fontcolor;
};

void common_init_node(node_t * n)
{
    struct fontinfo fi;
    char *str;
    ND_width(n) =
	late_double(n, N_width, DEFAULT_NODEWIDTH, MIN_NODEWIDTH);
    ND_height(n) =
	late_double(n, N_height, DEFAULT_NODEHEIGHT, MIN_NODEHEIGHT);
    ND_shape(n) =
	bind_shape(late_nnstring(n, N_shape, DEFAULT_NODESHAPE), n);
#ifndef WITH_CGRAPH
    str = agxget(n, N_label->index);
#else
    str = agxget(n, N_label);
#endif
    fi.fontsize = late_double(n, N_fontsize, DEFAULT_FONTSIZE, MIN_FONTSIZE);
    fi.fontname = late_nnstring(n, N_fontname, DEFAULT_FONTNAME);
    fi.fontcolor = late_nnstring(n, N_fontcolor, DEFAULT_COLOR);
    ND_label(n) = make_label((void*)n, str,
	        ((aghtmlstr(str) ? LT_HTML : LT_NONE) | ( (shapeOf(n) == SH_RECORD) ? LT_RECD : LT_NONE)),
		fi.fontsize, fi.fontname, fi.fontcolor);
#ifndef WITH_CGRAPH
    if (N_xlabel && (str = agxget(n, N_xlabel->index)) && (str[0])) {
#else
    if (N_xlabel && (str = agxget(n, N_xlabel)) && (str[0])) {
#endif
	ND_xlabel(n) = make_label((void*)n, str, (aghtmlstr(str) ? LT_HTML : LT_NONE),
				fi.fontsize, fi.fontname, fi.fontcolor);
	GD_has_labels(agraphof(n)) |= NODE_XLABEL;
    }

    ND_showboxes(n) = late_int(n, N_showboxes, 0, 0);
    ND_shape(n)->fns->initfn(n);
}

static void initFontEdgeAttr(edge_t * e, struct fontinfo *fi)
{
    fi->fontsize = late_double(e, E_fontsize, DEFAULT_FONTSIZE, MIN_FONTSIZE);
    fi->fontname = late_nnstring(e, E_fontname, DEFAULT_FONTNAME);
    fi->fontcolor = late_nnstring(e, E_fontcolor, DEFAULT_COLOR);
}

static void
initFontLabelEdgeAttr(edge_t * e, struct fontinfo *fi,
		      struct fontinfo *lfi)
{
    if (!fi->fontname) initFontEdgeAttr(e, fi);
    lfi->fontsize = late_double(e, E_labelfontsize, fi->fontsize, MIN_FONTSIZE);
    lfi->fontname = late_nnstring(e, E_labelfontname, fi->fontname);
    lfi->fontcolor = late_nnstring(e, E_labelfontcolor, fi->fontcolor);
}

/* noClip:
 * Return true if head/tail end of edge should not be clipped
 * to node.
 */
static boolean 
noClip(edge_t *e, attrsym_t* sym)
{
    char		*str;
    boolean		rv = FALSE;

    if (sym) {	/* mapbool isn't a good fit, because we want "" to mean true */
#ifndef WITH_CGRAPH
	str = agxget(e,sym->index);
#else /* WITH_CGRAPH */
	str = agxget(e,sym);
#endif /* WITH_CGRAPH */
	if (str && str[0]) rv = !mapbool(str);
	else rv = FALSE;
    }
    return rv;
}

/*chkPort:
 */
static port
chkPort (port (*pf)(node_t*, char*, char*), node_t* n, char* s)
{ 
    port pt;
#ifndef WITH_CGRAPH
    char* cp = strchr(s,':');

#else /* WITH_CGRAPH */
	char* cp=NULL;
	if(s)
		cp= strchr(s,':');
#endif /* WITH_CGRAPH */
    if (cp) {
	*cp = '\0';
	pt = pf(n, s, cp+1);
	*cp = ':';
	pt.name = cp+1;
    }
    else
	pt = pf(n, s, NULL);
	pt.name = s;
    return pt;
}

/* return true if edge has label */
int common_init_edge(edge_t * e)
{
    char *str;
    int r = 0;
    struct fontinfo fi;
    struct fontinfo lfi;
    graph_t *sg = agraphof(agtail(e));

    fi.fontname = NULL;
    lfi.fontname = NULL;
#ifndef WITH_CGRAPH
    if (E_label && (str = agxget(e, E_label->index)) && (str[0])) {
#else
    if (E_label && (str = agxget(e, E_label)) && (str[0])) {
#endif
	r = 1;
	initFontEdgeAttr(e, &fi);
	ED_label(e) = make_label((void*)e, str, (aghtmlstr(str) ? LT_HTML : LT_NONE),
				fi.fontsize, fi.fontname, fi.fontcolor);
	GD_has_labels(sg) |= EDGE_LABEL;
	ED_label_ontop(e) =
	    mapbool(late_string(e, E_label_float, "false"));
    }

#ifndef WITH_CGRAPH
    if (E_xlabel && (str = agxget(e, E_xlabel->index)) && (str[0])) {
#else
    if (E_xlabel && (str = agxget(e, E_xlabel)) && (str[0])) {
#endif
	if (!fi.fontname)
	    initFontEdgeAttr(e, &fi);
	ED_xlabel(e) = make_label((void*)e, str, (aghtmlstr(str) ? LT_HTML : LT_NONE),
				fi.fontsize, fi.fontname, fi.fontcolor);
	GD_has_labels(sg) |= EDGE_XLABEL;
    }


    /* vladimir */
#ifndef WITH_CGRAPH
    if (E_headlabel && (str = agxget(e, E_headlabel->index)) && (str[0])) {
#else
    if (E_headlabel && (str = agxget(e, E_headlabel)) && (str[0])) {
#endif
	initFontLabelEdgeAttr(e, &fi, &lfi);
	ED_head_label(e) = make_label((void*)e, str, (aghtmlstr(str) ? LT_HTML : LT_NONE),
				lfi.fontsize, lfi.fontname, lfi.fontcolor);
	GD_has_labels(sg) |= HEAD_LABEL;
    }
#ifndef WITH_CGRAPH
    if (E_taillabel && (str = agxget(e, E_taillabel->index)) && (str[0])) {
#else
    if (E_taillabel && (str = agxget(e, E_taillabel)) && (str[0])) {
#endif
	if (!lfi.fontname)
	    initFontLabelEdgeAttr(e, &fi, &lfi);
	ED_tail_label(e) = make_label((void*)e, str, (aghtmlstr(str) ? LT_HTML : LT_NONE),
				lfi.fontsize, lfi.fontname, lfi.fontcolor);
	GD_has_labels(sg) |= TAIL_LABEL;
    }
    /* end vladimir */

    /* We still accept ports beginning with colons but this is deprecated 
     * That is, we allow tailport = ":abc" as well as the preferred 
     * tailport = "abc".
     */
    str = agget(e, TAIL_ID);
#ifdef WITH_CGRAPH
    /* libgraph always defines tailport/headport; libcgraph doesn't */
    if (!str) str = "";
#endif
    if (str && str[0])
	ND_has_port(agtail(e)) = TRUE;
    ED_tail_port(e) = chkPort (ND_shape(agtail(e))->fns->portfn, agtail(e), str);
    if (noClip(e, E_tailclip))
	ED_tail_port(e).clip = FALSE;
    str = agget(e, HEAD_ID);
#ifdef WITH_CGRAPH
    /* libgraph always defines tailport/headport; libcgraph doesn't */
    if (!str) str = "";
#endif
    if (str && str[0])
	ND_has_port(aghead(e)) = TRUE;
    ED_head_port(e) = chkPort(ND_shape(aghead(e))->fns->portfn, aghead(e), str);
    if (noClip(e, E_headclip))
	ED_head_port(e).clip = FALSE;

    return r;
}

/* addLabelBB:
 */
static boxf addLabelBB(boxf bb, textlabel_t * lp, boolean flipxy)
{
    double width, height;
    pointf p = lp->pos;
    double min, max;

    if (flipxy) {
	height = lp->dimen.x;
	width = lp->dimen.y;
    }
    else {
	width = lp->dimen.x;
	height = lp->dimen.y;
    }
    min = p.x - width / 2.;
    max = p.x + width / 2.;
    if (min < bb.LL.x)
	bb.LL.x = min;
    if (max > bb.UR.x)
	bb.UR.x = max;

    min = p.y - height / 2.;
    max = p.y + height / 2.;
    if (min < bb.LL.y)
	bb.LL.y = min;
    if (max > bb.UR.y)
	bb.UR.y = max;

    return bb;
}

/* updateBB:
 * Reset graph's bounding box to include bounding box of the given label.
 * Assume the label's position has been set.
 */
void updateBB(graph_t * g, textlabel_t * lp)
{
    GD_bb(g) = addLabelBB(GD_bb(g), lp, GD_flip(g));
}

/* compute_bb:
 * Compute bounding box of g using nodes, splines, and clusters.
 * Assumes bb of clusters already computed.
 * store in GD_bb.
 */
void compute_bb(graph_t * g)
{
    node_t *n;
    edge_t *e;
    boxf b, bb;
    boxf BF;
    pointf ptf, s2;
    int i, j;

    if ((agnnodes(g) == 0) && (GD_n_cluster(g) ==0)) {
	bb.LL = pointfof(0, 0);
	bb.UR = pointfof(0, 0);
	return;
    }

    bb.LL = pointfof(INT_MAX, INT_MAX);
    bb.UR = pointfof(-INT_MAX, -INT_MAX);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	ptf = coord(n);
	s2.x = ND_xsize(n) / 2.0;
	s2.y = ND_ysize(n) / 2.0;
	b.LL = sub_pointf(ptf, s2);
	b.UR = add_pointf(ptf, s2);

	EXPANDBB(bb,b);
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (ED_spl(e) == 0)
		continue;
	    for (i = 0; i < ED_spl(e)->size; i++) {
#ifndef WITH_CGRAPH
		for (j = 0; j < ED_spl(e)->list[i].size; j++) {
#else
		for (j = 0; j < (((Agedgeinfo_t*)AGDATA(e))->spl)->list[i].size; j++) {
#endif
		    ptf = ED_spl(e)->list[i].list[j];
		    EXPANDBP(bb,ptf);
		}
	    }
	    if (ED_label(e) && ED_label(e)->set) {
		bb = addLabelBB(bb, ED_label(e), GD_flip(g));
	    }
	}
    }

    for (i = 1; i <= GD_n_cluster(g); i++) {
	B2BF(GD_bb(GD_clust(g)[i]), BF);
	EXPANDBB(bb,BF);
    }
    if (GD_label(g) && GD_label(g)->set) {
	bb = addLabelBB(bb, GD_label(g), GD_flip(g));
    }

    GD_bb(g) = bb;
}

int is_a_cluster (Agraph_t* g)
{
    return ((g == g->root) || (!strncasecmp(agnameof(g), "cluster", 7)));
}

/* setAttr:
 * Sets object's name attribute to the given value.
 * Creates the attribute if not already set.
 */
Agsym_t *setAttr(graph_t * g, void *obj, char *name, char *value,
			Agsym_t * ap)
{
    if (ap == NULL) {
	switch (agobjkind(obj)) {
#ifndef WITH_CGRAPH
	case AGGRAPH:
	    ap = agraphattr(g, name, "");
#else /* WITH_CGRAPH */
	case AGRAPH:
	    ap = agattr(g, AGRAPH,name, "");
#endif /* WITH_CGRAPH */
	    break;
	case AGNODE:
#ifndef WITH_CGRAPH
	    ap = agnodeattr(g, name, "");
#else /* WITH_CGRAPH */
	    ap = agattr(g,AGNODE, name, "");
#endif /* WITH_CGRAPH */
	    break;
	case AGEDGE:
#ifndef WITH_CGRAPH
	    ap = agedgeattr(g, name, "");
#else /* WITH_CGRAPH */
	    ap = agattr(g,AGEDGE, name, "");
#endif /* WITH_CGRAPH */
	    break;
	}
    }
#ifndef WITH_CGRAPH
    agxset(obj, ap->index, value);
#else /* WITH_CGRAPH */
    agxset(obj, ap, value);
#endif /* WITH_CGRAPH */
    return ap;
}

/* clustNode:
 * Generate a special cluster node representing the end node
 * of an edge to the cluster cg. n is a node whose name is the same
 * as the cluster cg. clg is the subgraph of all of
 * the original nodes, which will be deleted later.
 */
static node_t *clustNode(node_t * n, graph_t * cg, agxbuf * xb,
			 graph_t * clg)
{
    node_t *cn;
    static int idx = 0;
    char num[100];

    agxbput(xb, "__");
    sprintf(num, "%d", idx++);
    agxbput(xb, num);
    agxbputc(xb, ':');
    agxbput(xb, agnameof(cg));

#ifndef WITH_CGRAPH
    cn = agnode(agroot(cg), agxbuse(xb));
#else
    cn = agnode(agroot(cg), agxbuse(xb), 1);
    agbindrec(cn, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);
#endif /* WITH_CGRAPH */

    SET_CLUST_NODE(cn);
#ifndef WITH_CGRAPH
    aginsert(cg, cn);
    aginsert(clg, n);
#else /* WITH_CGRAPH */
	agsubnode(cg,cn,1);
	//aginsert(cg, cn);
	agsubnode(clg,n,1);
	//aginsert(clg, n);
#endif /* WITH_CGRAPH */

    /* set attributes */
    N_label = setAttr(agraphof(cn), cn, "label", "", N_label);
    N_style = setAttr(agraphof(cn), cn, "style", "invis", N_style);
    N_shape = setAttr(agraphof(cn), cn, "shape", "box", N_shape);
    /* N_width = setAttr (cn->graph, cn, "width", "0.0001", N_width); */

    return cn;
}

typedef struct {
    Dtlink_t link;		/* cdt data */
    void *p[2];			/* key */
    node_t *t;
    node_t *h;
} item;

static int cmpItem(Dt_t * d, void *p1[], void *p2[], Dtdisc_t * disc)
{
    NOTUSED(d);
    NOTUSED(disc);

    if (p1[0] < p2[0])
	return -1;
    else if (p1[0] > p2[0])
	return 1;
    else if (p1[1] < p2[1])
	return -1;
    else if (p1[1] > p2[1])
	return 1;
    else
	return 0;
}

/* newItem:
 */
static void *newItem(Dt_t * d, item * objp, Dtdisc_t * disc)
{
    item *newp = NEW(item);

    NOTUSED(disc);
    newp->p[0] = objp->p[0];
    newp->p[1] = objp->p[1];
    newp->t = objp->t;
    newp->h = objp->h;

    return newp;
}

/* freeItem:
 */
static void freeItem(Dt_t * d, item * obj, Dtdisc_t * disc)
{
    free(obj);
}

static Dtdisc_t mapDisc = {
    offsetof(item, p),
    sizeof(2 * sizeof(void *)),
    offsetof(item, link),
    (Dtmake_f) newItem,
    (Dtfree_f) freeItem,
    (Dtcompar_f) cmpItem,
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

/* cloneEdge:
 * Make a copy of e in e's graph but using ct and ch as nodes
 */
static edge_t *cloneEdge(edge_t * e, node_t * ct, node_t * ch)
{
    graph_t *g = agraphof(ct);
#ifndef WITH_CGRAPH
    edge_t *ce = agedge(g, ct, ch);
#else /* WITH_CGRAPH */
    edge_t *ce = agedge(g, ct, ch,NULL,1);
    agbindrec(ce, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);
#endif /* WITH_CGRAPH */
    agcopyattr(e, ce);

    return ce;
}

/* insertEdge:
 */
static void insertEdge(Dt_t * map, void *t, void *h, edge_t * e)
{
    item dummy;

    dummy.p[0] = t;
    dummy.p[1] = h;
    dummy.t = agtail(e);
    dummy.h = aghead(e);
    dtinsert(map, &dummy);

    dummy.p[0] = h;
    dummy.p[1] = t;
    dummy.t = aghead(e);
    dummy.h = agtail(e);
    dtinsert(map, &dummy);
}

/* mapEdge:
 * Check if we already have cluster edge corresponding to t->h,
 * and return it.
 */
static item *mapEdge(Dt_t * map, edge_t * e)
{
    void *key[2];

    key[0] = agtail(e);
    key[1] = aghead(e);
    return (item *) dtmatch(map, &key);
}

/* checkCompound:
 * If endpoint names a cluster, mark for temporary deletion and create 
 * special node and insert into cluster. Then clone the edge. Real edge
 * will be deleted when we delete the original node.
 * Invariant: new edge has same sense as old. That is, given t->h with
 * t and h mapped to ct and ch, the new edge is ct->ch.
 *
 * In the current model, we create a cluster node for each cluster edge
 * between the cluster and some other node or cluster, treating the
 * cluster node as a port on the cluster. This should help with better
 * routing to avoid edge crossings. At present, this is not implemented,
 * so we could use a simpler model in which we create a single cluster
 * node for each cluster used in a cluster edge.
 */
#ifndef WITH_CGRAPH
#define MAPC(n) (strncmp((n)->name,"cluster",7)?NULL:agfindsubg((n)->graph, (n)->name))
#else /* WITH_CGRAPH */
#define MAPC(n) (strncmp(agnameof(n),"cluster",7)?NULL:findCluster(cmap,agnameof(n)))
#endif /* WITH_CGRAPH */


#ifdef WITH_CGRAPH
static void
checkCompound(edge_t * e, graph_t * clg, agxbuf * xb, Dt_t * map, Dt_t* cmap)
#else
static void
checkCompound(edge_t * e, graph_t * clg, agxbuf * xb, Dt_t * map)
#endif
{
    graph_t *tg;
    graph_t *hg;
    node_t *cn;
    node_t *cn1;
    node_t *t = agtail(e);
    node_t *h = aghead(e);
    edge_t *ce;
    item *ip;

    if (IS_CLUST_NODE(h)) return;
    tg = MAPC(t);
    hg = MAPC(h);
    if (!tg && !hg)
	return;
    if (tg == hg) {
	agerr(AGWARN, "cluster cycle %s -- %s not supported\n", agnameof(t),
	      agnameof(t));
	return;
    }
    ip = mapEdge(map, e);
    if (ip) {
	cloneEdge(e, ip->t, ip->h);
	return;
    }

    if (hg) {
	if (tg) {
	    if (agcontains(hg, tg)) {
		agerr(AGWARN, "tail cluster %s inside head cluster %s\n",
		      agnameof(tg), agnameof(hg));
		return;
	    }
	    if (agcontains(tg, hg)) {
		agerr(AGWARN, "head cluster %s inside tail cluster %s\n",
		      agnameof(hg),agnameof(tg));
		return;
	    }
	    cn = clustNode(t, tg, xb, clg);
	    cn1 = clustNode(h, hg, xb, clg);
	    ce = cloneEdge(e, cn, cn1);
	    insertEdge(map, t, h, ce);
	} else {
	    if (agcontains(hg, t)) {
		agerr(AGWARN, "tail node %s inside head cluster %s\n",
		      agnameof(t), agnameof(hg));
		return;
	    }
	    cn = clustNode(h, hg, xb, clg);
	    ce = cloneEdge(e, t, cn);
	    insertEdge(map, t, h, ce);
	}
    } else {
	if (agcontains(tg, h)) {
	    agerr(AGWARN, "head node %s inside tail cluster %s\n", agnameof(h),
		  agnameof(tg));
	    return;
	}
	cn = clustNode(t, tg, xb, clg);
	ce = cloneEdge(e, cn, h);
	insertEdge(map, t, h, ce);
    }
}

/* processClusterEdges:
 * Look for cluster edges. Replace cluster edge endpoints
 * corresponding to a cluster with special cluster nodes.
 * Delete original nodes.
 * Return 0 if no cluster edges; 1 otherwise.
 */
int processClusterEdges(graph_t * g)
{
    int rv;
    node_t *n;
    node_t *nxt;
    edge_t *e;
    graph_t *clg;
    agxbuf xb;
    Dt_t *map;
#ifdef WITH_CGRAPH
    Dt_t *cmap = mkClustMap (g);
#endif
    unsigned char buf[SMALLBUF];

    map = dtopen(&mapDisc, Dtoset);
#ifndef WITH_CGRAPH
    clg = agsubg(g, "__clusternodes");
#else /* WITH_CGRAPH */
    clg = agsubg(g, "__clusternodes",1);
    agbindrec(clg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);
#endif /* WITH_CGRAPH */
    agxbinit(&xb, SMALLBUF, buf);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (IS_CLUST_NODE(n)) continue;
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
#ifndef WITH_CGRAPH
	    checkCompound(e, clg, &xb, map);
#else
	    checkCompound(e, clg, &xb, map, cmap);
#endif
	}
    }
    agxbfree(&xb);
    dtclose(map);
    rv = agnnodes(clg);
    for (n = agfstnode(clg); n; n = nxt) {
	nxt = agnxtnode(clg, n);
	agdelete(g, n);
    }
    agclose(clg);
    if (rv)
	SET_CLUST_EDGE(g);
#ifdef WITH_CGRAPH
    dtclose(cmap);
#endif
    return rv;
}

/* mapN:
 * Convert cluster nodes back to ordinary nodes
 * If n is already ordinary, return it.
 * Otherwise, we know node's name is "__i:xxx"
 * where i is some number and xxx is the nodes's original name.
 * Create new node of name xxx if it doesn't exist and add n to clg
 * for later deletion.
 */
static node_t *mapN(node_t * n, graph_t * clg)
{
#ifndef WITH_CGRAPH
    extern Agdict_t *agdictof(void *);
    Agdict_t *d;
    Agsym_t **list;
#endif /* WITH_CGRAPH */
    node_t *nn;
    char *name;
    graph_t *g = agraphof(n);
    Agsym_t *sym;

    if (!(IS_CLUST_NODE(n)))
	return n;
#ifndef WITH_CGRAPH
    aginsert(clg, n);
#else /* WITH_CGRAPH */
    agsubnode(clg, n, 1);
#endif /* WITH_CGRAPH */
    name = strchr(agnameof(n), ':');
    assert(name);
    name++;
    if ((nn = agfindnode(g, name)))
	return nn;
#ifndef WITH_CGRAPH
    nn = agnode(g, name);
#else /* WITH_CGRAPH */
    nn = agnode(g, name, 1);
    agbindrec(nn, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);
#endif /* WITH_CGRAPH */

    /* Set all attributes to default */
#ifndef WITH_CGRAPH
    d = agdictof(n);
    list = d->list;
    while ((sym = *list++)) {
	/* Can use pointer comparison because of ref strings. */
	if (agxget(nn, sym->index) != sym->value)
	    agxset(nn, sym->index, sym->value);
    }
#else /* WITH_CGRAPH */
    for (sym = agnxtattr(g, AGNODE, NULL); sym;  (sym = agnxtattr(g, AGNODE, sym))) {
	if (agxget(nn, sym) != sym->defval)
	    agxset(nn, sym, sym->defval);
    }
#endif /* WITH_CGRAPH */
    return nn;
}

static void undoCompound(edge_t * e, graph_t * clg)
{
    node_t *t = agtail(e);
    node_t *h = aghead(e);
    node_t *ntail;
    node_t *nhead;

    if (!(IS_CLUST_NODE(t) || IS_CLUST_NODE(h)))
	return;
    ntail = mapN(t, clg);
    nhead = mapN(h, clg);
    cloneEdge(e, ntail, nhead);
}

/* undoClusterEdges:
 * Replace cluster nodes with originals. Make sure original has
 * no attributes. Replace original edges. Delete cluster nodes,
 * which will also delete cluster edges.
 */
void undoClusterEdges(graph_t * g)
{
    node_t *n;
    edge_t *e;
    graph_t *clg;

#ifndef WITH_CGRAPH
    clg = agsubg(g, "__clusternodes");
#else /* WITH_CGRAPH */
    clg = agsubg(g, "__clusternodes",1);
	agbindrec(clg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);
#endif /* WITH_CGRAPH */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    undoCompound(e, clg);
	}
    }
    for (n = agfstnode(clg); n; n = agnxtnode(clg, n)) {
	agdelete(g, n);
    }
    agclose(clg);
}

/* safe_dcl:
 * Find the attribute belonging to graph g for objects like obj
 * with given name. If one does not exist, create it with the
 * default value def.
 */ 
#ifndef WITH_CGRAPH
attrsym_t*
safe_dcl(graph_t * g, void *obj, char *name, char *def,
         attrsym_t * (*fun) (Agraph_t *, char *, char *))
{
    attrsym_t *a = agfindattr(obj, name);
    if (a == NULL)
	a = fun(g, name, def);
    return a;
}
#else /* WITH_CGRAPH */
attrsym_t*
safe_dcl(graph_t * g, int obj_kind, char *name, char *def)
{
    attrsym_t *a = agattr(g,obj_kind,name, NULL);
    if (!a)	/* attribute does not exist */		
	a = agattr(g,obj_kind,name,def);
    return a;
}
#endif /* WITH_CGRAPH */

static int comp_entities(const void *e1, const void *e2) {
  return strcmp(((struct entities_s *)e1)->name, ((struct entities_s *)e2)->name);
}

#define MAXENTLEN 8

/* scanEntity:
 * Scan non-numeric entity, convert to &#...; form and store in xbuf.
 * t points to first char after '&'. Return after final semicolon.
 * If unknown, we return t and let libexpat flag the error.
 *     */
char* scanEntity (char* t, agxbuf* xb)
{
    char*  endp = strchr (t, ';');
    struct entities_s key, *res;
    int    len;
    char   buf[MAXENTLEN+1];

    agxbputc(xb, '&');
    if (!endp) return t;
    if (((len = endp-t) > MAXENTLEN) || (len < 2)) return t;
    strncpy (buf, t, len);
    buf[len] = '\0';
    key.name =  buf;
    res = bsearch(&key, entities, NR_OF_ENTITIES,
        sizeof(entities[0]), comp_entities);
    if (!res) return t;
    sprintf (buf, "%d", res->value);
    agxbputc(xb, '#');
    agxbput(xb, buf);
    agxbputc(xb, ';');
    return (endp+1);
}


/* htmlEntity:
 * Check for an HTML entity for a special character.
 * Assume *s points to first byte after '&'. 
 * If successful, return the corresponding value and update s to
 * point after the terminating ';'.
 * On failure, return 0 and leave s unchanged.
 */
static int
htmlEntity (char** s)
{
    char *p;
    struct entities_s key, *res;
    char entity_name_buf[ENTITY_NAME_LENGTH_MAX+1];
    unsigned char* str = *(unsigned char**)s;
    unsigned int byte;
    int i, n = 0;

    byte = *str;
    if (byte == '#') {
	byte = *(str + 1);
	if (byte == 'x' || byte == 'X') {
	    for (i = 2; i < 8; i++) {
		byte = *(str + i);
		if (byte >= 'A' && byte <= 'F')
                    byte = byte - 'A' + 10;
		else if (byte >= 'a' && byte <= 'f')
                    byte = byte - 'a' + 10;
		else if (byte >= '0' && byte <= '9')
                    byte = byte - '0';
		else
                    break;
		n = (n * 16) + byte;
	    }
	}
	else {
	    for (i = 1; i < 8; i++) {
		byte = *(str + i);
		if (byte >= '0' && byte <= '9')
		    n = (n * 10) + (byte - '0');
		else
		    break;
	    }
	}
	if (byte == ';') {
	    str += i+1;
	}
	else {
	    n = 0;
	}
    }
    else {
	key.name = p = entity_name_buf;
	for (i = 0; i <  ENTITY_NAME_LENGTH_MAX; i++) {
	    byte = *(str + i);
	    if (byte == '\0') break;
	    if (byte == ';') {
		*p++ = '\0';
		res = bsearch(&key, entities, NR_OF_ENTITIES,
		    sizeof(entities[0]), *comp_entities);
		if (res) {
		    n = res->value;
		    str += i+1;
		}
		break;
	    }
	    *p++ = byte;
	}
    }
    *s = (char*)str;
    return n;
}

static unsigned char
cvtAndAppend (unsigned char c, agxbuf* xb)
{
    char buf[2];
    char* s;
    char* p;
    int len;

    buf[0] = c;
    buf[1] = '\0';

    p = s = latin1ToUTF8 (buf);
    len = strlen(s);
    while (len-- > 1)
	agxbputc(xb, *p++);
    c = *p;
    free (s);
    return c;
}

/* htmlEntityUTF8:
 * substitute html entities like: &#123; and: &amp; with the UTF8 equivalents
 * check for invalid utf8. If found, treat a single byte as Latin-1, convert it to
 * utf8 and warn the user.
 */
char* htmlEntityUTF8 (char* s, graph_t* g)
{
    static graph_t* lastg;
    static boolean warned;
    char*  ns;
    agxbuf xb;
    unsigned char buf[BUFSIZ];
    unsigned char c;
    unsigned int v;
    int rc;

    if (lastg != g) {
	lastg = g;
	warned = 0;
    }

    agxbinit(&xb, BUFSIZ, buf);

    while ((c = *(unsigned char*)s++)) {
        if (c < 0xC0) {
	    /*
	     * Handles properly formed UTF-8 characters between
	     * 0x01 and 0x7F.  Also treats \0 and naked trail
	     * bytes 0x80 to 0xBF as valid characters representing
	     * themselves.
	     */
	    if (c == '&') {
		/* replace html entity sequences like: &amp;
		 * and: &#123; with their UTF8 equivalents */
	        v = htmlEntity (&s);
	        if (v) {
		    if (v < 0x7F) /* entity needs 1 byte in UTF8 */
			c = v;
		    else if (v < 0x07FF) { /* entity needs 2 bytes in UTF8 */
			rc = agxbputc(&xb, (v >> 6) | 0xC0);
			c = (v & 0x3F) | 0x80;
		    }
		    else { /* entity needs 3 bytes in UTF8 */
			rc = agxbputc(&xb, (v >> 12) | 0xE0);
			rc = agxbputc(&xb, ((v >> 6) & 0x3F) | 0x80);
			c = (v & 0x3F) | 0x80;
		    }
		}
            }
	}
        else if (c < 0xE0) { /* copy 2 byte UTF8 characters */
	    if ((*s & 0xC0) == 0x80) {
	        rc = agxbputc(&xb, c);
	        c = *(unsigned char*)s++;
	    }
	    else { 
		if (!warned) {
		    agerr(AGWARN, "Invalid 2-byte UTF8 found in input of graph %s - treated as Latin-1. Perhaps \"-Gcharset=latin1\" is needed?\n", agnameof(g));
		    warned = 1;
		}
		c = cvtAndAppend (c, &xb);
	    }
	}
	else if (c < 0xF0) { /* copy 3 byte UTF8 characters */
	    if (((*s & 0xC0) == 0x80) && ((s[1] & 0xC0) == 0x80)) {
	        rc = agxbputc(&xb, c);
	        c = *(unsigned char*)s++;
	        rc = agxbputc(&xb, c);
	        c = *(unsigned char*)s++;
	    }
	    else {
		if (!warned) {
		    agerr(AGWARN, "Invalid 3-byte UTF8 found in input of graph %s - treated as Latin-1. Perhaps \"-Gcharset=latin1\" is needed?\n", agnameof(g));
		    warned = 1;
		}
		c = cvtAndAppend (c, &xb);
	    }
	}
	else  {
	    if (!warned) {
		agerr(AGWARN, "UTF8 codes > 3 bytes are not currently supported (graph %s) - treated as Latin-1. Perhaps \"-Gcharset=latin1\" is needed?\n", agnameof(g));
		warned = 1;
	    }
	    c = cvtAndAppend (c, &xb);
        }
	rc = agxbputc(&xb, c);
    }
    ns = strdup (agxbuse(&xb));
    agxbfree(&xb);
    return ns;
}

/* latin1ToUTF8:
 * Converts string from Latin1 encoding to utf8
 * Also translates HTML entities.
 *
 */
char* latin1ToUTF8 (char* s)
{
    char*  ns;
    agxbuf xb;
    unsigned char buf[BUFSIZ];
    unsigned int  v;
    int rc;
    
    agxbinit(&xb, BUFSIZ, buf);

    /* Values are either a byte (<= 256) or come from htmlEntity, whose
     * values are all less than 0x07FF, so we need at most 3 bytes.
     */
    while ((v = *(unsigned char*)s++)) {
	if (v == '&') {
	    v = htmlEntity (&s);
	    if (!v) v = '&';
        }
	if (v < 0x7F)
	    rc = agxbputc(&xb, v);
	else if (v < 0x07FF) {
	    rc = agxbputc(&xb, (v >> 6) | 0xC0);
	    rc = agxbputc(&xb, (v & 0x3F) | 0x80);
	}
	else {
	    rc = agxbputc(&xb, (v >> 12) | 0xE0);
	    rc = agxbputc(&xb, ((v >> 6) & 0x3F) | 0x80);
	    rc = agxbputc(&xb, (v & 0x3F) | 0x80);
	}
    }
    ns = strdup (agxbuse(&xb));
    agxbfree(&xb);
    return ns;
}

/* utf8ToLatin1:
 * Converts string from utf8 encoding to Latin1
 * Note that it does not attempt to reproduce HTML entities.
 * We assume the input string comes from latin1ToUTF8.
 */
char*
utf8ToLatin1 (char* s)
{
    char*  ns;
    agxbuf xb;
    unsigned char buf[BUFSIZ];
    unsigned char c;
    unsigned char outc;
    int rc;
    
    agxbinit(&xb, BUFSIZ, buf);

    while ((c = *(unsigned char*)s++)) {
	if (c < 0x7F)
	    rc = agxbputc(&xb, c);
	else {
	    outc = (c & 0x03) << 6;
	    c = *(unsigned char*)s++;
	    outc = outc | (c & 0x3F);
	    rc = agxbputc(&xb, outc);
	}
    }
    ns = strdup (agxbuse(&xb));
    agxbfree(&xb);
    return ns;
}

boolean overlap_node(node_t *n, boxf b)
{
    inside_t ictxt;
    pointf p;

    if (! OVERLAP(b, ND_bb(n)))
        return FALSE;

/*  FIXME - need to do something better about CLOSEENOUGH */
    p = sub_pointf(ND_coord(n), mid_pointf(b.UR, b.LL));

    ictxt.s.n = n;
    ictxt.s.bp = NULL;

    return ND_shape(n)->fns->insidefn(&ictxt, p);
}

boolean overlap_label(textlabel_t *lp, boxf b)
{
    pointf s;
    boxf bb;

    s.x = lp->dimen.x / 2.;
    s.y = lp->dimen.y / 2.;
    bb.LL = sub_pointf(lp->pos, s);
    bb.UR = add_pointf(lp->pos, s);
    return OVERLAP(b, bb);
}

static boolean overlap_arrow(pointf p, pointf u, double scale, int flag, boxf b)
{
    if (OVERLAP(b, arrow_bb(p, u, scale, flag))) {
	/* FIXME - check inside arrow shape */
	return TRUE;
    }
    return FALSE;
}

static boolean overlap_bezier(bezier bz, boxf b)
{
    int i;
    pointf p, u;

    assert(bz.size);
    u = bz.list[0];
    for (i = 1; i < bz.size; i++) {
	p = bz.list[i];
	if (lineToBox(p, u, b) != -1)
	    return TRUE;
	u = p;
    }

    /* check arrows */
    if (bz.sflag) {
	if (overlap_arrow(bz.sp, bz.list[0], 1, bz.sflag, b))
	    return TRUE;
    }
    if (bz.eflag) {
	if (overlap_arrow(bz.ep, bz.list[bz.size - 1], 1, bz.eflag, b))
	    return TRUE;
    }
    return FALSE;
}

boolean overlap_edge(edge_t *e, boxf b)
{
    int i;
    splines *spl;
    textlabel_t *lp;

    spl = ED_spl(e);
    if (spl && boxf_overlap(spl->bb, b))
        for (i = 0; i < spl->size; i++)
            if (overlap_bezier(spl->list[i], b))
                return TRUE;

    lp = ED_label(e);
    if (lp && overlap_label(lp, b))
        return TRUE;

    return FALSE;
}

/* edgeType:
 * Convert string to edge type.
 */
int edgeType (char* s, int dflt)
{
    int et;

    if (!s || (*s == '\0')) return dflt;

    et = ET_NONE;
    switch (*s) {
    case '0' :    /* false */
	et = ET_LINE;
	break;
    case '1' :    /* true */
    case '2' :
    case '3' :
    case '4' :
    case '5' :
    case '6' :
    case '7' :
    case '8' :
    case '9' :
	et = ET_SPLINE;
	break;
    case 'c' :
    case 'C' :
	if (!strcasecmp (s+1, "urved"))
	    et = ET_CURVED;
	else if (!strcasecmp (s+1, "ompound"))
	    et = ET_COMPOUND;
	break;
    case 'f' :
    case 'F' :
	if (!strcasecmp (s+1, "alse"))
	    et = ET_LINE;
	break;
    case 'l' :
    case 'L' :
	if (!strcasecmp (s+1, "ine"))
	    et = ET_LINE;
	break;
    case 'n' :
    case 'N' :
	if (!strcasecmp (s+1, "one")) return et;
	if (!strcasecmp (s+1, "o")) return ET_LINE;
	break;
    case 'o' :
    case 'O' :
	if (!strcasecmp (s+1, "rtho"))
	    et = ET_ORTHO;
	break;
    case 'p' :
    case 'P' :
	if (!strcasecmp (s+1, "olyline"))
	    et = ET_PLINE;
	break;
    case 's' :
    case 'S' :
	if (!strcasecmp (s+1, "pline"))
	    et = ET_SPLINE;
	break;
    case 't' :
    case 'T' :
	if (!strcasecmp (s+1, "rue"))
	    et = ET_SPLINE;
	break;
    case 'y' :
    case 'Y' :
	if (!strcasecmp (s+1, "es"))
	    et = ET_SPLINE;
	break;
    }
    if (!et) {
	agerr(AGWARN, "Unknown \"splines\" value: \"%s\" - ignored\n", s);
	et = dflt;
    }
    return et;
}

/* setEdgeType:
 * Sets graph's edge type based on the "splines" attribute.
 * If the attribute is not defined, use default.
 * If the attribute is "", use NONE.
 * If attribute value matches (case indepedent), use match.
 *   ortho => ET_ORTHO
 *   none => ET_NONE
 *   line => ET_LINE
 *   polyline => ET_PLINE
 *   spline => ET_SPLINE
 * If attribute is boolean, true means ET_SPLINE, false means ET_LINE.
 * Else warn and use default.
 */
void setEdgeType (graph_t* g, int dflt)
{
    char* s = agget(g, "splines");
    int et;

    if (!s) {
	et = dflt;
    }
    else if (*s == '\0') {
	et = ET_NONE;
    }
    else et = edgeType (s, dflt);
    GD_flags(g) |= et;
}


/* get_gradient_points
 * Evaluates the extreme points of an ellipse or polygon
 * Determines the point at the center of the extreme points
 * If isRadial is true,sets the inner radius to half the distance to the min point;
 * else uses the angle parameter to identify two points on a line that defines the 
 * gradient direction
 * By default, this assumes a left-hand coordinate system (for svg); if RHS = 2 flag
 * is set, use standard coordinate system.
 */
void get_gradient_points(pointf * A, pointf * G, int n, float angle, int flags)
{
    int i;
    double rx, ry;
    pointf min,max,center;
    int isRadial = flags & 1;
    int isRHS = flags & 2;
    
    if (n == 2) {
      rx = A[1].x - A[0].x;
      ry = A[1].y - A[0].y;
      min.x = A[0].x - rx;
      max.x = A[0].x + rx;
      min.y = A[0].y - ry;
      max.y = A[0].y + ry;
    }    
    else {
      min.x = max.x = A[0].x;
      min.y = max.y = A[0].y;
      for (i = 0; i < n; i++){
	min.x = MIN(A[i].x,min.x);
	min.y = MIN(A[i].y,min.y);
	max.x = MAX(A[i].x,max.x);
	max.y = MAX(A[i].y,max.y);
      }
    }
      center.x = min.x + (max.x - min.x)/2;
      center.y = min.y + (max.y - min.y)/2;
    if (isRadial) {
	double inner_r, outer_r;
	outer_r = sqrt((center.x - min.x)*(center.x - min.x) +
		      (center.y - min.y)*(center.y - min.y));
	inner_r = outer_r /4.;
	if (isRHS) {
	    G[0].y = center.y;
	}
	else {
	    G[0].y = -center.y;
	}
	G[0].x = center.x;
	G[1].x = inner_r;
	G[1].y = outer_r;
    }
    else {
	double half_x = max.x - center.x;
	double half_y = max.y - center.y;
	double sina = sin(angle);
	double cosa = cos(angle);
	if (isRHS) {
	    G[0].y = center.y - half_y * sina;
	    G[1].y = center.y + half_y * sina;
	}
	else {
	    G[0].y = -center.y + (max.y - center.y) * sin(angle);
	    G[1].y = -center.y - (center.y - min.y) * sin(angle);
	}
	G[0].x = center.x - half_x * cosa;
	G[1].x = center.x + half_x * cosa;
    }
}

#ifndef WIN32_STATIC
#ifndef HAVE_STRCASECMP


#include <string.h>
//#include <ctype.h>


int strcasecmp(const char *s1, const char *s2)
{
    while ((*s1 != '\0')
	   && (tolower(*(unsigned char *) s1) ==
	       tolower(*(unsigned char *) s2))) {
	s1++;
	s2++;
    }

    return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

#endif				/* HAVE_STRCASECMP */
#endif				/* WIN32_STATIC */

#ifndef WIN32_STATIC
#ifndef HAVE_STRNCASECMP
#include <string.h>
//#include <ctype.h>

int strncasecmp(const char *s1, const char *s2, unsigned int n)
{
    if (n == 0)
	return 0;

    while ((n-- != 0)
	   && (tolower(*(unsigned char *) s1) ==
	       tolower(*(unsigned char *) s2))) {
	if (n == 0 || *s1 == '\0' || *s2 == '\0')
	    return 0;
	s1++;
	s2++;
    }

    return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

#endif				/* HAVE_STRNCASECMP */
#endif                          /* WIN32_STATIC */
void gv_free_splines(edge_t * e)
{
    int i;
    if (ED_spl(e)) {
        for (i = 0; i < ED_spl(e)->size; i++)
            free(ED_spl(e)->list[i].list);
        free(ED_spl(e)->list);
        free(ED_spl(e));
    }
    ED_spl(e) = NULL;
}

void gv_cleanup_edge(edge_t * e)
{
    free (ED_path(e).ps);
    gv_free_splines(e);
    free_label(ED_label(e));
    free_label(ED_xlabel(e));
    free_label(ED_head_label(e));
    free_label(ED_tail_label(e));
#ifndef WITH_CGRAPH
    memset(&(e->u), 0, sizeof(Agedgeinfo_t));
#else /* WITH_CGRAPH */
	/*FIX HERE , shallow cleaning may not be enough here */
	agdelrec(e, "Agedgeinfo_t");	
#endif /* WITH_CGRAPH */
}

void gv_cleanup_node(node_t * n)
{
    if (ND_pos(n)) free(ND_pos(n));
    if (ND_shape(n))
        ND_shape(n)->fns->freefn(n);
    free_label(ND_label(n));
    free_label(ND_xlabel(n));
#ifndef WITH_CGRAPH
    memset(&(n->u), 0, sizeof(Agnodeinfo_t));
#else /* WITH_CGRAPH */
	/*FIX HERE , shallow cleaning may not be enough here */
	agdelrec(n, "Agnodeinfo_t");	
#endif /* WITH_CGRAPH */
}

void gv_nodesize(node_t * n, boolean flip)
{
    double w;

    if (flip) {
        w = INCH2PS(ND_height(n));
        ND_lw(n) = ND_rw(n) = w / 2;
        ND_ht(n) = INCH2PS(ND_width(n));
    } 
    else {
        w = INCH2PS(ND_width(n));
        ND_lw(n) = ND_rw(n) = w / 2;
        ND_ht(n) = INCH2PS(ND_height(n));
    }
}

#ifdef WIN32	
void fix_fc(void)
{
    char buf[28192];
    char buf2[28192];
    int cur=0;
    FILE* fp;

    if((fp = fopen("fix-fc.exe", "r")) == NULL)
	    return ;
    fclose (fp);
    if (!system ("fix-fc.exe")) {
	system ("del fix-fc.exe");
	system ("dot -c");	//run dot -c once too since we already run things :)
    }
}
#endif

#ifndef HAVE_DRAND48
double drand48(void)
{
    double d;
    d = rand();
    d = d / RAND_MAX;
    return d;
}
#endif
#ifdef WITH_CGRAPH
typedef struct {
    Dtlink_t link;
    char* name;
    Agraph_t* clp;
} clust_t;

static void free_clust (Dt_t* dt, clust_t* clp, Dtdisc_t* disc)
{
    free (clp);
}

static Dtdisc_t strDisc = {
    offsetof(clust_t,name),
    -1,
    offsetof(clust_t,link),
    NIL(Dtmake_f),
    (Dtfree_f)free_clust,
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

static void fillMap (Agraph_t* g, Dt_t* map)
{
    Agraph_t* cl;
    int c;
    char* s;
    clust_t* ip;

    for (c = 1; c <= GD_n_cluster(g); c++) {
	cl = GD_clust(g)[c];
	s = agnameof(cl);
	if (dtmatch (map, &s)) {
	    agerr(AGWARN, "Two clusters named %s - the second will be ignored\n", s);
	}
	else {
	    ip = NEW(clust_t);
	    ip->name = s;
	    ip->clp = cl;
	    dtinsert (map, ip);
	}
	fillMap (cl, map);
    }
}

/* mkClustMap:
 * Generates a dictionary mapping cluster names to corresponding cluster.
 * Used with cgraph as the latter does not support a flat namespace of clusters.
 * Assumes G has already built a cluster tree using GD_n_cluster and GD_clust.
 */
Dt_t* mkClustMap (Agraph_t* g)
{
    Dt_t* map = dtopen (&strDisc, Dtoset);

    fillMap (g, map);
    
    return map;
}

Agraph_t*
findCluster (Dt_t* map, char* name)
{
    clust_t* clp = dtmatch (map, name);
    if (clp)
	return clp->clp;
    else
	return NULL;
}
#endif

#ifdef WITH_CGRAPH
Agnodeinfo_t* ninf(Agnode_t* n) {return (Agnodeinfo_t*)AGDATA(n);}
Agraphinfo_t* ginf(Agraph_t* g) {return (Agraphinfo_t*)AGDATA(g);}
Agedgeinfo_t* einf(Agedge_t* e) {return (Agedgeinfo_t*)AGDATA(e);}
#endif
