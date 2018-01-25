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


#include "convert.h"
#include <ctype.h>

#define SMALLBUF    128

#define NEW(t)      (t*)malloc(sizeof(t))
#define N_NEW(n,t)  (t*)malloc((n)*sizeof(t))
#define EMPTY(s)	((s == 0) || (*s == '\0'))
#define SLEN(s)     (sizeof(s)-1)

#define NODE		1
#define EDGE		2
#define GRAPH		3

#define GXL_ATTR    "_gxl_"
#define GXL_ROLE	"_gxl_role"
#define GXL_HYPER	"_gxl_hypergraph"
#define GXL_ID      "_gxl_id"
#define GXL_FROM    "_gxl_fromorder"
#define GXL_TO      "_gxl_toorder"
#define GXL_TYPE    "_gxl_type"
#define GXL_COMP    "_gxl_composite_"
#define GXL_LOC     "_gxl_locator_"

#define GXL_ATTR_LEN (SLEN(GXL_ATTR))
#define GXL_COMP_LEN (SLEN(GXL_COMP))
#define GXL_LOC_LEN  (SLEN(GXL_LOC))

typedef struct {
    Agrec_t h;
    int written;
} Agnodeinfo_t;

static int Level;		/* level of tabs */
static Agsym_t *Tailport, *Headport;

typedef struct {
    Dtlink_t link;
    char *name;
    char *unique_name;
} namev_t;

static namev_t *make_nitem(Dt_t * d, namev_t * objp, Dtdisc_t * disc)
{
    namev_t *np = NEW(namev_t);
    np->name = objp->name;
    np->unique_name = 0;
    return np;
}

static void free_nitem(Dt_t * d, namev_t * np, Dtdisc_t * disc)
{
    free(np);
}

static Dtdisc_t nameDisc = {
    offsetof(namev_t, name),
    -1,
    offsetof(namev_t, link),
    (Dtmake_f) make_nitem,
    (Dtfree_f) free_nitem,
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

typedef struct {
    Dtlink_t link;
    char *name;
} idv_t;

static void free_iditem(Dt_t * d, idv_t * idp, Dtdisc_t * disc)
{
    free(idp->name);
    free(idp);
}

static Dtdisc_t idDisc = {
    offsetof(idv_t, name),
    -1,
    offsetof(idv_t, link),
    NIL(Dtmake_f),
    (Dtfree_f) free_iditem,
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

typedef struct {
    Dt_t *nodeMap;
    Dt_t *graphMap;
    Dt_t *synNodeMap;
    Dt_t *idList;
    Agraph_t *root;
    char attrsNotWritten;
    char directed;
} gxlstate_t;

static void writeBody(gxlstate_t *, Agraph_t * g, FILE * gxlFile);
static void iterateBody(gxlstate_t * stp, Agraph_t * g);

static void tabover(FILE * gxlFile)
{
    int temp;
    temp = Level;
    while (temp--)
	putc('\t', gxlFile);
}

/* legalGXLName:
 * By XML spec, 
 *   ID := (alpha|'_'|':')(NameChar)*
 *   NameChar := alpha|digit|'.'|':'|'-'|'_'
 */
static int legalGXLName(char *id)
{
    char c = *id++;
    if (!isalpha(c) && (c != '_') && (c != ':'))
	return 0;
    while ((c = *id++)) {
	if (!isalnum(c) && (c != '_') && (c != ':') &&
	    (c != '-') && (c != '.'))
	    return 0;
    }
    return 1;
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

static char *_xml_string(char *s, int notURL)
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
	else if ((*s == '-') && notURL) {	/* can't be used in xml comment strings */
	    sub = "&#45;";
	    len = 5;
	}
	else if (*s == ' ' && prev && *prev == ' ' && notURL) {
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

static char *xml_string(char *s)
{
    return _xml_string(s,1);
}

static char *xml_url_string(char *s)
{
    return _xml_string(s,0);
}

static int isGxlGrammar(char *name)
{
    return (strncmp(name, GXL_ATTR, (sizeof(GXL_ATTR) - 1)) == 0);
}

static int isLocatorType(char *name)
{
    return (strncmp(name, GXL_LOC, GXL_LOC_LEN) == 0);
}

static void *idexists(Dt_t * ids, char *id)
{
    return dtmatch(ids, id);
}

/* addid:
 * assume id is not in ids.
 */
static char *addid(Dt_t * ids, char *id)
{
    idv_t *idp = NEW(idv_t);

    idp->name = strdup(id);
    dtinsert(ids, idp);
    return idp->name;
}

static char *createGraphId(Dt_t * ids)
{
    static int graphIdCounter = 0;
    char buf[SMALLBUF];

    do {
	sprintf(buf, "G_%d", graphIdCounter++);
    } while (idexists(ids, buf));
    return addid(ids, buf);
}

static char *createNodeId(Dt_t * ids)
{
    static int nodeIdCounter = 0;
    char buf[SMALLBUF];

    do {
	sprintf(buf, "N_%d", nodeIdCounter++);
    } while (idexists(ids, buf));
    return addid(ids, buf);
}

static char *mapLookup(Dt_t * nm, char *name)
{
    namev_t *objp = dtmatch(nm, name);
    if (objp)
	return objp->unique_name;
    else
	return 0;
}

static char *nodeID(gxlstate_t * stp, Agnode_t * n)
{
    char *name, *uniqueName;

    name = agnameof(n);
    uniqueName = mapLookup(stp->nodeMap, name);
    assert(uniqueName);
    return uniqueName;
}

#define EXTRA 32		/* space for ':' followed by a number */
#define EDGEOP "--"		/* cannot use '>'; illegal in ID in GXL */

static char *createEdgeId(gxlstate_t * stp, Agedge_t * e)
{
    int edgeIdCounter = 1;
    char buf[BUFSIZ];
    char *hname = nodeID(stp, AGHEAD(e));
    char *tname = nodeID(stp, AGTAIL(e));
    int baselen = strlen(hname) + strlen(tname) + sizeof(EDGEOP);
    int len = baselen + EXTRA;
    char *bp;
    char *endp;			/* where to append ':' and number */
    char *rv;

    if (len <= BUFSIZ)
	bp = buf;
    else
	bp = N_NEW(len, char);
    endp = bp + (baselen - 1);

    sprintf(bp, "%s%s%s", tname, EDGEOP, hname);
    while (idexists(stp->idList, bp)) {
	sprintf(endp, ":%d", edgeIdCounter++);
    }

    rv = addid(stp->idList, bp);
    if (bp != buf)
	free(bp);
    return rv;
}

static void addToMap(Dt_t * map, char *name, char *uniqueName)
{
    namev_t obj;
    namev_t *objp;

    obj.name = name;
    objp = dtinsert(map, &obj);
    assert(objp->unique_name == 0);
    objp->unique_name = uniqueName;
}

static void graphAttrs(FILE * gxlFile, Agraph_t * g)
{
    char *val;

    val = agget(g, GXL_ROLE);
    if (!EMPTY(val)) {
	fprintf(gxlFile, " role=\"%s\"", xml_string(val));
    }
    val = agget(g, GXL_HYPER);
    if (!EMPTY(val)) {
	fprintf(gxlFile, " hypergraph=\"%s\"", xml_string(val));
    }
}

static void edgeAttrs(FILE * gxlFile, Agedge_t * e)
{
    char *val;

    val = agget(e, GXL_ID);
    if (!EMPTY(val)) {
	fprintf(gxlFile, " id=\"%s\"", xml_string(val));
    }
    val = agget(e, GXL_FROM);
    if (!EMPTY(val)) {
	fprintf(gxlFile, " fromorder=\"%s\"", xml_string(val));
    }
    val = agget(e, GXL_TO);
    if (!EMPTY(val)) {
	fprintf(gxlFile, " toorder=\"%s\"", xml_string(val));
    }
}


static void printHref(FILE * gxlFile, void *n)
{
    char *val;

    val = agget(n, GXL_TYPE);
    if (!EMPTY(val)) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<type xlink:href=\"%s\">\n", xml_url_string(val));
	tabover(gxlFile);
	fprintf(gxlFile, "\t</type>\n");
    }
}


static void
writeDict(Agraph_t * g, FILE * gxlFile, char *name, Dict_t * dict,
	  int isGraph)
{
    Dict_t *view;
    Agsym_t *sym, *psym;

    view = dtview(dict, NIL(Dict_t *));
    for (sym = (Agsym_t *) dtfirst(dict); sym;
	 sym = (Agsym_t *) dtnext(dict, sym)) {
	if (!isGxlGrammar(sym->name)) {
	    if (EMPTY(sym->defval)) {	/* try to skip empty str (default) */
		if (view == NIL(Dict_t *))
		    continue;	/* no parent */
		psym = (Agsym_t *) dtsearch(view, sym);
		/* assert(psym); */
		if (EMPTY(psym->defval))
		    continue;	/* also empty in parent */
	    }

	    if (isLocatorType(sym->defval)) {
		char *locatorVal;
		locatorVal = sym->defval;
		locatorVal += 13;

		tabover(gxlFile);
		fprintf(gxlFile, "\t<attr name=\"%s\">\n", xml_string(sym->name));
		tabover(gxlFile);
		fprintf(gxlFile, "\t\t<locator xlink:href=\"%s\"/>\n",
			xml_url_string(locatorVal));
		tabover(gxlFile);
		fprintf(gxlFile, "\t</attr>\n");
	    } else {
		tabover(gxlFile);
		if (isGraph) {
		    fprintf(gxlFile, "\t<attr name=\"%s\" ", xml_string(sym->name));
		    fprintf(gxlFile, "kind=\"%s\">\n", xml_string(name));
		}
		else {
		    fprintf(gxlFile, "\t<attr name=\"%s:", xml_string(name));
		    fprintf(gxlFile, "%s\" kind=\"", xml_string(sym->name));
		    fprintf(gxlFile, "%s\">\n", xml_string(name));
		}
		tabover(gxlFile);
		fprintf(gxlFile, "\t\t<string>%s</string>\n", xml_string(sym->defval));
		tabover(gxlFile);
		fprintf(gxlFile, "\t</attr>\n");
	    }
	} else {
	    /* gxl attr; check for special cases like composites */
	    if (strncmp(sym->name, GXL_COMP, GXL_COMP_LEN) == 0) {
		if (EMPTY(sym->defval)) {
		    if (view == NIL(Dict_t *))
			continue;
		    psym = (Agsym_t *) dtsearch(view, sym);
		    if (EMPTY(psym->defval))
			continue;
		}

		tabover(gxlFile);
		fprintf(gxlFile, "\t<attr name=\"%s\" ", xml_string(((sym->name) + GXL_COMP_LEN)));
		fprintf(gxlFile, "kind=\"%s\">\n", xml_string(name));
		tabover(gxlFile);
		fprintf(gxlFile, "\t\t%s\n", xml_string(sym->defval));
		tabover(gxlFile);
		fprintf(gxlFile, "\t</attr>\n");
	    }
	}
    }
    dtview(dict, view);		/* restore previous view */
}

static void writeDicts(Agraph_t * g, FILE * gxlFile)
{
    Agdatadict_t *def;
    if ((def = (Agdatadict_t *) agdatadict(g, FALSE))) {
	writeDict(g, gxlFile, "graph", def->dict.g, 1);
	writeDict(g, gxlFile, "node", def->dict.n, 0);
	writeDict(g, gxlFile, "edge", def->dict.e, 0);
    }
}

static void
writeHdr(gxlstate_t * stp, Agraph_t * g, FILE * gxlFile, int top)
{
    char *name;
    char *kind;
    char *uniqueName;
    char buf[BUFSIZ];
    char *bp;
    char *dynbuf = 0;
    int len;

    Level++;
    stp->attrsNotWritten = AGATTRWF(g);

    name = agnameof(g);
    if (g->desc.directed)
	kind = "directed";
    else
	kind = "undirected";
    if (!top && agparent(g)) {
	/* this must be anonymous graph */

	len = strlen(name) + sizeof("N_");
	if (len <= BUFSIZ)
	    bp = buf;
	else {
	    bp = dynbuf = N_NEW(len, char);
	}
	sprintf(bp, "N_%s", name);
	if (idexists(stp->idList, bp) || !legalGXLName(bp)) {
	    bp = createNodeId(stp->idList);
	} else {
	    bp = addid(stp->idList, bp);
	}
	addToMap(stp->synNodeMap, name, bp);

	tabover(gxlFile);
	fprintf(gxlFile, "<node id=\"%s\">\n", bp);
	if (dynbuf)
	    free(dynbuf);
	Level++;
    } else {
	Tailport = agattr(g, AGEDGE, "tailport", NIL(char *));
	Headport = agattr(g, AGEDGE, "headport", NIL(char *));
    }


    uniqueName = mapLookup(stp->graphMap, name);
    tabover(gxlFile);
    fprintf(gxlFile, "<graph id=\"%s\" edgeids=\"true\" edgemode=\"%s\"",
	    uniqueName, kind);
    graphAttrs(gxlFile, g);
    fprintf(gxlFile, ">\n");

    if (uniqueName && (strcmp(name, uniqueName) != 0)) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<attr name=\"name\">\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t\t<string>%s</string>\n", xml_string(name));
	tabover(gxlFile);
	fprintf(gxlFile, "\t</attr>\n");
    }

    if (agisstrict(g)) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<attr name=\"strict\">\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t\t<string>true</string>\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t</attr>\n");
    }

    writeDicts(g, gxlFile);
    printHref(gxlFile, g);
    AGATTRWF(g) = !(AGATTRWF(g));
}

static void writeTrl(Agraph_t * g, FILE * gxlFile, int top)
{
    tabover(gxlFile);
    fprintf(gxlFile, "</graph>\n");
    Level--;
    if (!(top) && agparent(g)) {
	tabover(gxlFile);
	fprintf(gxlFile, "</node>\n");
	Level--;
    }
}


static void writeSubgs(gxlstate_t * stp, Agraph_t * g, FILE * gxlFile)
{
    Agraph_t *subg;

    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	writeHdr(stp, subg, gxlFile, FALSE);
	writeBody(stp, subg, gxlFile);
	writeTrl(subg, gxlFile, FALSE);
    }
}

static int writeEdgeName(Agedge_t * e, FILE * gxlFile, int terminate)
{
    int rv;
    char *p;

    p = agnameof(e);
    if (!(EMPTY(p))) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<attr name=\"key\">\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t\t<string>%s</string>\n", xml_string(p));
	tabover(gxlFile);
	fprintf(gxlFile, "\t</attr>\n");
	rv = TRUE;
    } else
	rv = FALSE;
    return rv;
}


static void
writeNondefaultAttr(void *obj, FILE * gxlFile, Dict_t * defdict)
{
    Agattr_t *data;
    Agsym_t *sym;
    int cnt = 0;

    if ((AGTYPE(obj) == AGINEDGE) || (AGTYPE(obj) == AGOUTEDGE)) {
	if (writeEdgeName(obj, gxlFile, FALSE))
	    cnt++;
    }
    data = (Agattr_t *) agattrrec(obj);
    if (data) {
	for (sym = (Agsym_t *) dtfirst(defdict); sym;
	     sym = (Agsym_t *) dtnext(defdict, sym)) {
	    if (!isGxlGrammar(sym->name)) {
		if ((AGTYPE(obj) == AGINEDGE)
		    || (AGTYPE(obj) == AGOUTEDGE)) {
		    if (Tailport && (sym->id == Tailport->id))
			continue;
		    if (Headport && (sym->id == Headport->id))
			continue;
		}
		if (data->str[sym->id] != sym->defval) {

		    if (strcmp(data->str[sym->id], "") == 0)
			continue;

		    if (isLocatorType(data->str[sym->id])) {
			char *locatorVal;
			locatorVal = data->str[sym->id];
			locatorVal += 13;

			tabover(gxlFile);
			fprintf(gxlFile, "\t<attr name=\"%s\">\n",
				xml_string(sym->name));
			tabover(gxlFile);
			fprintf(gxlFile,
				"\t\t<locator xlink:href=\"%s\"/>\n",
				xml_url_string(locatorVal));
			tabover(gxlFile);
			fprintf(gxlFile, "\t</attr>\n");
		    } else {
			tabover(gxlFile);
			fprintf(gxlFile, "\t<attr name=\"%s\">\n",
				xml_string(sym->name));
			tabover(gxlFile);
			fprintf(gxlFile, "\t\t<string>%s</string>\n", xml_string(data->str[sym->id]));
			tabover(gxlFile);
			fprintf(gxlFile, "\t</attr>\n");
		    }
		}
	    } else {
		/* gxl attr; check for special cases like composites */
		if (strncmp(sym->name, GXL_COMP, GXL_COMP_LEN) == 0) {
		    if (data->str[sym->id] != sym->defval) {

			tabover(gxlFile);
			fprintf(gxlFile, "\t<attr name=\"%s\">\n",
				xml_string(((sym->name) + GXL_COMP_LEN)));
			tabover(gxlFile);
			fprintf(gxlFile, "\t\t%s\n", xml_string(data->str[sym->id]));
			tabover(gxlFile);
			fprintf(gxlFile, "\t</attr>\n");
		    }
		}
	    }
	}
    }
    AGATTRWF((Agobj_t *) obj) = !(AGATTRWF((Agobj_t *) obj));
}

/* nodeID:
 * Return id associated with the given node.
 */
static int attrs_written(gxlstate_t * stp, void *obj)
{
    return !(AGATTRWF((Agobj_t *) obj) == stp->attrsNotWritten);
}

static void
writeNode(gxlstate_t * stp, Agnode_t * n, FILE * gxlFile, Dict_t * d)
{
    char *name, *uniqueName;

    name = agnameof(n);
    uniqueName = nodeID(stp, n);
    Level++;
    tabover(gxlFile);
    fprintf(gxlFile, "<node id=\"%s\">\n", uniqueName);

    printHref(gxlFile, n);

    if (strcmp(name, uniqueName)) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<attr name=\"name\">\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t\t<string>%s</string>\n", xml_string(name));
	tabover(gxlFile);
	fprintf(gxlFile, "\t</attr>\n");
    }

    if (!attrs_written(stp, n))
	writeNondefaultAttr(n, gxlFile, d);
    tabover(gxlFile);
    fprintf(gxlFile, "</node>\n");
    Level--;
}

static void writePort(Agedge_t * e, FILE * gxlFile, char *name)
{
    char *val;

    val = agget(e, name);
    if (val && val[0]) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<attr name=\"%s\">\n", xml_string(name));
	tabover(gxlFile);
	fprintf(gxlFile, "\t\t<string>%s</string>\n", xml_string(val));
	tabover(gxlFile);
	fprintf(gxlFile, "\t</attr>\n");
    }
}

static int writeEdgeTest(Agraph_t * g, Agedge_t * e)
{
    Agraph_t *subg;

    /* can use agedge() because we subverted the dict compar_f */
    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	if (agsubedge(subg, e, FALSE))
	    return FALSE;
    }
    return TRUE;
}

static void
writeEdge(gxlstate_t * stp, Agedge_t * e, FILE * gxlFile, Dict_t * d)
{
    Agnode_t *t, *h;
    char *bp;
    char *edge_id;

    t = AGTAIL(e);
    h = AGHEAD(e);

    Level++;
    tabover(gxlFile);
    fprintf(gxlFile, "<edge from=\"%s\" ", nodeID(stp, t));
    fprintf(gxlFile, "to=\"%s\"", nodeID(stp, h));
    edgeAttrs(gxlFile, e);

    if (stp->directed) {
	fprintf(gxlFile, " isdirected=\"true\"");
    } else {
	fprintf(gxlFile, " isdirected=\"false\"");
    }

    edge_id = agget(e, GXL_ID);
    if (!EMPTY(edge_id)) {
	fprintf(gxlFile, ">\n");
    } else {
	bp = createEdgeId(stp, e);
	fprintf(gxlFile, " id=\"%s\">\n", bp);
    }

    printHref(gxlFile, e);

    writePort(e, gxlFile, "tailport");
    writePort(e, gxlFile, "headport");
    if (!(attrs_written(stp, e)))
	writeNondefaultAttr(e, gxlFile, d);
    else
	writeEdgeName(e, gxlFile, TRUE);
    tabover(gxlFile);
    fprintf(gxlFile, "</edge>\n");
    Level--;
}


#define writeval(n) (((Agnodeinfo_t*)((n)->base.data))->written)

static void writeBody(gxlstate_t * stp, Agraph_t * g, FILE * gxlFile)
{
    Agnode_t *n;
    Agnode_t *realn;
    Agedge_t *e;
    Agdatadict_t *dd;

    writeSubgs(stp, g, gxlFile);
    dd = (Agdatadict_t *) agdatadict(g, FALSE);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	realn = agidnode(stp->root, AGID(n), 0);
	if (!writeval(realn)) {
	    writeval(realn) = 1;
	    writeNode(stp, n, gxlFile, dd->dict.n);
	}

	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (writeEdgeTest(g, e))
		writeEdge(stp, e, gxlFile, dd->dict.e);
	}
    }
}

static void iterateHdr(gxlstate_t * stp, Agraph_t * g)
{
    char *gxlId;
    char *name;

    name = agnameof(g);
    gxlId = agget(g, GXL_ID);
    if (EMPTY(gxlId))
	gxlId = name;

    if (idexists(stp->idList, gxlId) || !legalGXLName(gxlId))
	gxlId = createGraphId(stp->idList);
    else
	gxlId = addid(stp->idList, gxlId);
    addToMap(stp->graphMap, name, gxlId);
}

static void iterate_subgs(gxlstate_t * stp, Agraph_t * g)
{
    Agraph_t *subg;

    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	iterateHdr(stp, subg);
	iterateBody(stp, subg);
    }
}


static void iterateBody(gxlstate_t * stp, Agraph_t * g)
{
    Agnode_t *n;
    Agedge_t *e;

    iterate_subgs(stp, g);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	char *gxlId;
	char *nodename = agnameof(n);

	if (!mapLookup(stp->nodeMap, nodename)) {
	    gxlId = agget(n, GXL_ID);
	    if (EMPTY(gxlId))
		gxlId = nodename;
	    if (idexists(stp->idList, gxlId) || !legalGXLName(gxlId))
		gxlId = createNodeId(stp->idList);
	    else
		gxlId = addid(stp->idList, gxlId);
	    addToMap(stp->nodeMap, nodename, gxlId);
	}

	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (writeEdgeTest(g, e)) {
		char *edge_id = agget(e, GXL_ID);
		if (!EMPTY(edge_id))
		    addid(stp->idList, edge_id);
	    }
	}
    }
}

static gxlstate_t *initState(Agraph_t * g)
{
    gxlstate_t *stp = NEW(gxlstate_t);
    stp->nodeMap = dtopen(&nameDisc, Dtoset);
    stp->graphMap = dtopen(&nameDisc, Dtoset);
    stp->synNodeMap = dtopen(&nameDisc, Dtoset);
    stp->idList = dtopen(&idDisc, Dtoset);
    stp->attrsNotWritten = 0;
    stp->root = g;
    stp->directed = agisdirected(g);
    return stp;
}

static void freeState(gxlstate_t * stp)
{
    dtclose(stp->nodeMap);
    dtclose(stp->graphMap);
    dtclose(stp->synNodeMap);
    dtclose(stp->idList);
    free(stp);
}

void gv_to_gxl(Agraph_t * g, FILE * gxlFile)
{
    gxlstate_t *stp = initState(g);
    aginit(g, AGNODE, "node", sizeof(Agnodeinfo_t), TRUE);

    iterateHdr(stp, g);
    iterateBody(stp, g);

    Level = 0;

    fprintf(gxlFile, "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
    fprintf(gxlFile, "<gxl>\n");

    writeHdr(stp, g, gxlFile, TRUE);
    writeBody(stp, g, gxlFile);
    writeTrl(g, gxlFile, TRUE);

    fprintf(gxlFile, "</gxl>\n");

    freeState(stp);
}
