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


#include    "convert.h"
#include    "agxbuf.h"
#ifdef HAVE_EXPAT
#include    <expat.h>
#include    <ctype.h>

#ifdef WIN32 //*dependencies
    #pragma comment( lib, "cgraph.lib" )
    #pragma comment( lib, "cdt.lib" )
    #pragma comment( lib, "ingraphs.lib" )
    #pragma comment( lib, "gvc.lib" )
    #pragma comment( lib, "expat.lib" )
#endif


#ifndef XML_STATUS_ERROR
#define XML_STATUS_ERROR 0
#endif

#define STACK_DEPTH	32
#define BUFSIZE		20000
#define SMALLBUF	1000
#define NAMEBUF		100

#define GRAPHML_ATTR	"_graphml_"
#define GRAPHML_ID      "_graphml_id"
#define GRAPHML_ROLE	"_graphml_role"
#define GRAPHML_HYPER	"_graphml_hypergraph"
#define GRAPHML_FROM    "_graphml_fromorder"
#define GRAPHML_TO      "_graphml_toorder"
#define GRAPHML_TYPE    "_graphml_type"
#define GRAPHML_COMP    "_graphml_composite_"
#define GRAPHML_LOC     "_graphml_locator_"

#define	TAG_NONE	-1
#define	TAG_GRAPH	0
#define TAG_NODE	1
#define TAG_EDGE	2

static FILE *outFile;
static char *CmdName;
static char **Files;
static int Verbose;
static char* gname = "";

typedef struct slist slist;
struct slist {
    slist *next;
    char buf[1];
};

#define NEW(t)      (t*)malloc(sizeof(t))
#define N_NEW(n,t)  (t*)malloc((n)*sizeof(t))
/* Round x up to next multiple of y, which is a power of 2 */
#define ROUND2(x,y) (((x) + ((y)-1)) & ~((y)-1))

static void pushString(slist ** stk, const char *s)
{
    int sz = ROUND2(sizeof(slist) + strlen(s), sizeof(void *));
    slist *sp = (slist *) N_NEW(sz, char);
    strcpy(sp->buf, s);
    sp->next = *stk;
    *stk = sp;
}

static void popString(slist ** stk)
{
    slist *sp = *stk;
    if (!sp) {
	fprintf(stderr, "PANIC: graphml2gv: empty element stack\n");
	exit(1);
    }
    *stk = sp->next;
    free(sp);
}

static char *topString(slist * stk)
{
    if (!stk) {
	fprintf(stderr, "PANIC: graphml2gv: empty element stack\n");
	exit(1);
    }
    return stk->buf;
}

static void freeString(slist * stk)
{
    slist *sp;

    while (stk) {
	sp = stk->next;
	free(stk);
	stk = sp;
    }
}

typedef struct userdata {
    agxbuf xml_attr_name;
    agxbuf xml_attr_value;
    agxbuf composite_buffer;
    char* gname;
    slist *elements;
    int listen;
    int closedElementType;
    int globalAttrType;
    int compositeReadState;
    int edgeinverted;
    Dt_t *nameMap;
} userdata_t;

static Agraph_t *root;		/* root graph */
static int Current_class;	/* Current element type */
static Agraph_t *G;		/* Current graph */
static Agnode_t *N;		/* Set if Current_class == TAG_NODE */
static Agedge_t *E;		/* Set if Current_class == TAG_EDGE */

static int GSP;
static Agraph_t *Gstack[STACK_DEPTH];

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
    free(np->unique_name);
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

static userdata_t *genUserdata(char* dfltname)
{
    userdata_t *user = NEW(userdata_t);
    agxbinit(&(user->xml_attr_name), NAMEBUF, 0);
    agxbinit(&(user->xml_attr_value), SMALLBUF, 0);
    agxbinit(&(user->composite_buffer), SMALLBUF, 0);
    user->listen = FALSE;
    user->elements = 0;
    user->closedElementType = TAG_NONE;
    user->globalAttrType = TAG_NONE;
    user->compositeReadState = FALSE;
    user->edgeinverted = FALSE;
    user->gname = dfltname;
    user->nameMap = dtopen(&nameDisc, Dtoset);
    return user;
}

static void freeUserdata(userdata_t * ud)
{
    dtclose(ud->nameMap);
    agxbfree(&(ud->xml_attr_name));
    agxbfree(&(ud->xml_attr_value));
    agxbfree(&(ud->composite_buffer));
    freeString(ud->elements);
    free(ud);
}

static void addToMap(Dt_t * map, char *name, char *uniqueName)
{
    namev_t obj;
    namev_t *objp;

    obj.name = name;
    objp = dtinsert(map, &obj);
    assert(objp->unique_name == 0);
    objp->unique_name = strdup(uniqueName);
}

static char *mapLookup(Dt_t * nm, char *name)
{
    namev_t *objp = dtmatch(nm, name);
    if (objp)
	return objp->unique_name;
    else
	return 0;
}

static int isAnonGraph(char *name)
{
    if (*name++ != '%')
	return 0;
    while (isdigit(*name))
	name++;			/* skip over digits */
    return (*name == '\0');
}

static void push_subg(Agraph_t * g)
{
    if (GSP == STACK_DEPTH) {
	fprintf(stderr, "graphml2gv: Too many (> %d) nestings of subgraphs\n",
		STACK_DEPTH);
	exit(1);
    } else if (GSP == 0)
	root = g;
    G = Gstack[GSP++] = g;
}

static Agraph_t *pop_subg(void)
{
    Agraph_t *g;
    if (GSP == 0) {
	fprintf(stderr, "graphml2gv: Gstack underflow in graph parser\n");
	exit(1);
    }
    g = Gstack[--GSP];
    if (GSP > 0)
	G = Gstack[GSP - 1];
    return g;
}

static Agnode_t *bind_node(const char *name)
{
    N = agnode(G, (char *) name, 1);
    return N;
}

static Agedge_t *bind_edge(const char *tail, const char *head)
{
    Agnode_t *tailNode, *headNode;
    char *key = 0;

    tailNode = agnode(G, (char *) tail, 1);
    headNode = agnode(G, (char *) head, 1);
    E = agedge(G, tailNode, headNode, key, 1);
    return E;
}

static int get_xml_attr(char *attrname, const char **atts)
{
    int count = 0;
    while (atts[count] != NULL) {
	if (strcmp(attrname, atts[count]) == 0) {
	    return count + 1;
	}
	count += 2;
    }
    return -1;
}

static void setName(Dt_t * names, Agobj_t * n, char *value)
{
    Agsym_t *ap;
    char *oldName;

    ap = agattr(root, AGTYPE(n), GRAPHML_ID, "");
    agxset(n, ap, agnameof(n));
    oldName = agxget(n, ap);	/* set/get gives us new copy */
    addToMap(names, oldName, value);
    agrename(n, value);
}

static char *defval = "";

static void
setNodeAttr(Agnode_t * np, char *name, char *value, userdata_t * ud)
{
    Agsym_t *ap;

    if (strcmp(name, "name") == 0) {
	setName(ud->nameMap, (Agobj_t *) np, value);
    } else {
	ap = agattr(root, AGNODE, name, 0);
	if (!ap)
	    ap = agattr(root, AGNODE, name, defval);
	agxset(np, ap, value);
    }
}

#define NODELBL "node:"
#define NLBLLEN (sizeof(NODELBL)-1)
#define EDGELBL "edge:"
#define ELBLLEN (sizeof(EDGELBL)-1)

/* setGlobalNodeAttr:
 * Set global node attribute.
 * The names must always begin with "node:".
 */
static void
setGlobalNodeAttr(Agraph_t * g, char *name, char *value, userdata_t * ud)
{
    if (strncmp(name, NODELBL, NLBLLEN))
	fprintf(stderr,
		"Warning: global node attribute %s in graph %s does not begin with the prefix %s\n",
		name, agnameof(g), NODELBL);
    else
	name += NLBLLEN;
    if ((g != root) && !agattr(root, AGNODE, name, 0))
	agattr(root, AGNODE, name, defval);
    agattr(G, AGNODE, name, value);
}

static void
setEdgeAttr(Agedge_t * ep, char *name, char *value, userdata_t * ud)
{
    Agsym_t *ap;
    char *attrname;

    if (strcmp(name, "headport") == 0) {
	if (ud->edgeinverted)
	    attrname = "tailport";
	else
	    attrname = "headport";
	ap = agattr(root, AGEDGE, attrname, 0);
	if (!ap)
	    ap = agattr(root, AGEDGE, attrname, defval);
	agxset(ep, ap, value);
    } else if (strcmp(name, "tailport") == 0) {
	if (ud->edgeinverted)
	    attrname = "headport";
	else
	    attrname = "tailport";
	ap = agattr(root, AGEDGE, attrname, 0);
	if (!ap)
	    ap = agattr(root, AGEDGE, attrname, defval);
	agxset(ep, ap, value);
    } else {
	ap = agattr(root, AGEDGE, name, 0);
	if (!ap)
	    ap = agattr(root, AGEDGE, name, defval);
	agxset(ep, ap, value);
    }
}

/* setGlobalEdgeAttr:
 * Set global edge attribute.
 * The names always begin with "edge:".
 */
static void
setGlobalEdgeAttr(Agraph_t * g, char *name, char *value, userdata_t * ud)
{
    if (strncmp(name, EDGELBL, ELBLLEN))
	fprintf(stderr,
		"Warning: global edge attribute %s in graph %s does not begin with the prefix %s\n",
		name, agnameof(g), EDGELBL);
    else
	name += ELBLLEN;
    if ((g != root) && !agattr(root, AGEDGE, name, 0))
	agattr(root, AGEDGE, name, defval);
    agattr(g, AGEDGE, name, value);
}

static void
setGraphAttr(Agraph_t * g, char *name, char *value, userdata_t * ud)
{
    Agsym_t *ap;

    if ((g == root) && !strcmp(name, "strict") && !strcmp(value, "true")) {
	g->desc.strict = 1;
    } else if (strcmp(name, "name") == 0)
	setName(ud->nameMap, (Agobj_t *) g, value);
    else {
	ap = agattr(root, AGRAPH, name, 0);
	if (ap)
	    agxset(g, ap, value);
	else if (g == root)
	    agattr(root, AGRAPH, name, value);
	else {
	    ap = agattr(root, AGRAPH, name, defval);
	    agxset(g, ap, value);
	}
    }
}

static void setAttr(char *name, char *value, userdata_t * ud)
{
    switch (Current_class) {
    case TAG_GRAPH:
	setGraphAttr(G, name, value, ud);
	break;
    case TAG_NODE:
	setNodeAttr(N, name, value, ud);
	break;
    case TAG_EDGE:
	setEdgeAttr(E, name, value, ud);
	break;
    }
}

/*------------- expat handlers ----------------------------------*/

static void
startElementHandler(void *userData, const char *name, const char **atts)
{
    int pos;
    userdata_t *ud = (userdata_t *) userData;
    Agraph_t *g = NULL;

    if (strcmp(name, "graphml") == 0) {
	/* do nothing */
    } else if (strcmp(name, "graph") == 0) {
	const char *edgeMode = "";
	const char *id;
	Agdesc_t dir;
	char buf[NAMEBUF];	/* holds % + number */

	Current_class = TAG_GRAPH;
	if (ud->closedElementType == TAG_GRAPH) {
	    fprintf(stderr,
		    "Warning: Node contains more than one graph.\n");
	}
	pos = get_xml_attr("id", atts);
	if (pos > 0) {
	    id = atts[pos];
	}
	else
	    id = ud->gname;
	pos = get_xml_attr("edgedefault", atts);
	if (pos > 0) {
	    edgeMode = atts[pos];
	}

	if (GSP == 0) {
	    if (strcmp(edgeMode, "directed") == 0) {
		dir = Agdirected;
	    } else if (strcmp(edgeMode, "undirected") == 0) {
		dir = Agundirected;
	    } else {
		if (Verbose) {
		    fprintf(stderr,
			"Warning: graph has no edgedefault attribute - assume directed\n");
		}
		dir = Agdirected;
	    }
	    g = agopen((char *) id, dir, &AgDefaultDisc);
	    push_subg(g);
	} else {
	    Agraph_t *subg;
	    if (isAnonGraph((char *) id)) {
		static int anon_id = 1;
		sprintf(buf, "%%%d", anon_id++);
		id = buf;
	    }
	    subg = agsubg(G, (char *) id, 1);
	    push_subg(subg);
	}

	pushString(&ud->elements, id);
    } else if (strcmp(name, "node") == 0) {
	Current_class = TAG_NODE;
	pos = get_xml_attr("id", atts);
	if (pos > 0) {
	    const char *attrname;
	    attrname = atts[pos];

	    bind_node(attrname);

	    pushString(&ud->elements, attrname);
	}

    } else if (strcmp(name, "edge") == 0) {
	const char *head = "", *tail = "";
	char *tname;
	Agnode_t *t;

	Current_class = TAG_EDGE;
	pos = get_xml_attr("source", atts);
	if (pos > 0)
	    tail = atts[pos];
	pos = get_xml_attr("target", atts);
	if (pos > 0)
	    head = atts[pos];

	tname = mapLookup(ud->nameMap, (char *) tail);
	if (tname)
	    tail = tname;

	tname = mapLookup(ud->nameMap, (char *) head);
	if (tname)
	    head = tname;

	bind_edge(tail, head);

	t = AGTAIL(E);
	tname = agnameof(t);

	if (strcmp(tname, tail) == 0) {
	    ud->edgeinverted = FALSE;
	} else if (strcmp(tname, head) == 0) {
	    ud->edgeinverted = TRUE;
	}

	pos = get_xml_attr("id", atts);
	if (pos > 0) {
	    setEdgeAttr(E, GRAPHML_ID, (char *) atts[pos], ud);
	}
    } else {
	/* must be some extension */
	fprintf(stderr,
		"Unknown node %s - ignoring.\n",
		name);
    }
}

static void endElementHandler(void *userData, const char *name)
{
    userdata_t *ud = (userdata_t *) userData;

    if (strcmp(name, "graph") == 0) {
	pop_subg();
	popString(&ud->elements);
	ud->closedElementType = TAG_GRAPH;
    } else if (strcmp(name, "node") == 0) {
	char *ele_name = topString(ud->elements);
	if (ud->closedElementType == TAG_GRAPH) {
	    Agnode_t *node = agnode(root, ele_name, 0);
	    agdelete(root, node);
	}
	popString(&ud->elements);
	Current_class = TAG_GRAPH;
	N = 0;
	ud->closedElementType = TAG_NODE;
    } else if (strcmp(name, "edge") == 0) {
	Current_class = TAG_GRAPH;
	E = 0;
	ud->closedElementType = TAG_EDGE;
	ud->edgeinverted = FALSE;
    } else if (strcmp(name, "attr") == 0) {
	char *name;
	char *value;
	char buf[SMALLBUF] = GRAPHML_COMP;
	char *dynbuf = 0;

	ud->closedElementType = TAG_NONE;
	if (ud->compositeReadState) {
	    int len = sizeof(GRAPHML_COMP) + agxblen(&ud->xml_attr_name);
	    if (len <= SMALLBUF) {
		name = buf;
	    } else {
		name = dynbuf = N_NEW(len, char);
		strcpy(name, GRAPHML_COMP);
	    }
	    strcpy(name + sizeof(GRAPHML_COMP) - 1,
		   agxbuse(&ud->xml_attr_name));
	    value = agxbuse(&ud->composite_buffer);
	    agxbclear(&ud->xml_attr_value);
	    ud->compositeReadState = FALSE;
	} else {
	    name = agxbuse(&ud->xml_attr_name);
	    value = agxbuse(&ud->xml_attr_value);
	}

	switch (ud->globalAttrType) {
	case TAG_NONE:
	    setAttr(name, value, ud);
	    break;
	case TAG_NODE:
	    setGlobalNodeAttr(G, name, value, ud);
	    break;
	case TAG_EDGE:
	    setGlobalEdgeAttr(G, name, value, ud);
	    break;
	case TAG_GRAPH:
	    setGraphAttr(G, name, value, ud);
	    break;
	}
	if (dynbuf)
	    free(dynbuf);
	ud->globalAttrType = TAG_NONE;
    } 
}

static void characterDataHandler(void *userData, const char *s, int length)
{
    userdata_t *ud = (userdata_t *) userData;

    if (!ud->listen)
	return;

    if (ud->compositeReadState) {
	agxbput_n(&ud->composite_buffer, (char *) s, length);
	return;
    }

    agxbput_n(&ud->xml_attr_value, (char *) s, length);
}

Agraph_t *graphml_to_gv(char* gname, FILE * graphmlFile, int* rv)
{
    char buf[BUFSIZE];
    int done;
    userdata_t *udata = genUserdata(gname);
    XML_Parser parser = XML_ParserCreate(NULL);

    *rv = 0;
    XML_SetUserData(parser, udata);
    XML_SetElementHandler(parser, startElementHandler, endElementHandler);
    XML_SetCharacterDataHandler(parser, characterDataHandler);

    Current_class = TAG_GRAPH;
    root = 0;
    do {
	size_t len = fread(buf, 1, sizeof(buf), graphmlFile);
	if (len == 0)
	    break;
	done = len < sizeof(buf);
	if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR) {
	    fprintf(stderr,
		    "%s at line %lu\n",
		    XML_ErrorString(XML_GetErrorCode(parser)),
		    XML_GetCurrentLineNumber(parser));
	    *rv = 1;
	    break;
	}
    } while (!done);
    XML_ParserFree(parser);
    freeUserdata(udata);

    return root;
}

static FILE *getFile(void)
{
    FILE *rv = NULL;
    static FILE *savef = NULL;
    static int cnt = 0;

    if (Files == NULL) {
	if (cnt++ == 0) {
	    rv = stdin;
	}
    } else {
	if (savef)
	    fclose(savef);
	while (Files[cnt]) {
	    if ((rv = fopen(Files[cnt++], "r")) != 0)
		break;
	    else
		fprintf(stderr, "Can't open %s\n", Files[cnt - 1]);
	}
    }
    savef = rv;
    return rv;
}

static FILE *openFile(char *name, char *mode)
{
    FILE *fp;
    char *modestr;

    fp = fopen(name, mode);
    if (!fp) {
	if (*mode == 'r')
	    modestr = "reading";
	else
	    modestr = "writing";
	fprintf(stderr, "%s: could not open file %s for %s\n",
		CmdName, name, modestr);
	perror(name);
	exit(1);
    }
    return fp;
}

static const char *use = "Usage: %s [-gd?] [-o<file>] [<graphs>]\n\
 -g<name>  : use <name> as template for graph names\n\
 -o<file>  : output to <file> (stdout)\n\
 -v        : verbose mode\n\
 -?        : usage\n";

static void usage(int v)
{
    fprintf(stderr, use, CmdName);
    exit(v);
}

static char *cmdName(char *path)
{
    char *sp;

    sp = strrchr(path, '/');
    if (sp)
	sp++;
    else
	sp = path;
    return sp;
}

static void initargs(int argc, char **argv)
{
    int c;

    CmdName = cmdName(argv[0]);
    opterr = 0;
    while ((c = getopt(argc, argv, ":vg:o:")) != -1) {
	switch (c) {
	case 'g':
	    gname = optarg;
	    break;
	case 'v':
	    Verbose = 1;
	    break;
	case 'o':
	    outFile = openFile(optarg, "w");
	    break;
	case ':':
	    fprintf(stderr, "%s: option -%c missing argument\n", CmdName, optopt);
	    usage(1);
	    break;
	case '?':
	    if (optopt == '?')
		usage(0);
	    else {
		fprintf(stderr, "%s: option -%c unrecognized\n", CmdName,
			optopt);
		usage(1);
	    }
	}
    }

    argv += optind;
    argc -= optind;

    if (argc)
	Files = argv;
    if (!outFile)
	outFile = stdout;
}

static char*
nameOf (char* name, int cnt)
{
    static char* buf = 0;

    if (*name == '\0')
	return name;
    if (cnt) {
	if (!buf)
	    buf = N_NEW (strlen(name)+32,char);  /* 32 to handle any integer plus null byte */
	sprintf (buf, "%s%d", name, cnt);
	return buf;
    }
    else
	return name;
}

#endif

int main(int argc, char **argv)
{
    Agraph_t *G;
    Agraph_t *prev = 0;
    FILE *inFile;
    int rv, gcnt = 0;

#ifdef HAVE_EXPAT
    initargs(argc, argv);
    while ((inFile = getFile())) {
	while ((G = graphml_to_gv(nameOf(gname, gcnt), inFile, &rv))) {
	    gcnt++;
	    if (prev)
		agclose(prev);
	    prev = G;
	    if (Verbose) 
		fprintf (stderr, "%s: %d nodes %d edges\n",
		    agnameof (G), agnnodes(G), agnedges(G));
	    agwrite(G, outFile);
	    fflush(outFile);
	}
    }
    exit(rv);
#else
    fputs("cvtgxl: not configured for conversion from GXL to GV\n", stderr);
    exit(1);
#endif
}
