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


#include "libgraph.h"

typedef struct printdict_t {
    Dict_t *nodesleft, *edgesleft, *subgleft, *e_insubg, *n_insubg;
} printdict_t;

/*
 * memgets - same api as gets
 *
 * gets one line at a time from a memory buffer and places it in a user buffer
 *    up to a maximum of n characters
 *
 * returns pointer to obtained line in user buffer, or
 * returns NULL when last line read from memory buffer
 */
static char *memgets(char *ubuf, int n, FILE * mbuf)
{
    static char *mempos;
    char *to, *clp;		/* clp = current line pointer */
    int i;

    if (!n) {			/* a call with n==0 (from aglexinit) resets */
	mempos = (char *) mbuf;	/* cast from FILE* required by API */
    }

    clp = to = ubuf;
    for (i = 0; i < n - 1; i++) {	/* leave room for terminator */
	if (*mempos == '\0') {
	    if (i) {		/* if mbuf doesn't end in \n, provide one */
		*to++ = '\n';
	    } else {		/* all done */
		clp = NULL;
		mempos = NULL;
	    }
	    break;		/* last line or end-of-buffer */
	}
	if (*mempos == '\n') {
	    *to++ = *mempos++;
	    break;		/* all done with this line */
	}
	*to++ = *mempos++;	/* copy character */
    }
    *to++ = '\0';		/* place terminator in ubuf */
    return clp;
}

static Agraph_t*
finish (int rv, Agraph_t *g)
{
    if (rv) {
	if (g) agclose (g);
	return NULL;
    }
    else
	return g;
}

Agraph_t *agread(FILE * fp)
{
    int rv;
    aglexinit(fp, NULL);	/* use fgets from current io discipline */
    rv = agparse();
    return finish(rv, AG.parsed_g);
}

Agraph_t *agmemread(char *cp)
{
    int rv;
    gets_f savefgets = AG.fgets;
 
    AG.fgets = memgets;  /* memgets defined above */
    /* cast cp into a file pointer */
    aglexinit((FILE *) cp, NULL);
    rv = agparse();
    AG.fgets = savefgets;
    return finish(rv, AG.parsed_g);
}

Agraph_t *agread_usergets(FILE * fp, gets_f usergets)
{
    int rv;
    gets_f savefgets = AG.fgets;

    AG.fgets = usergets;		/* usergets provided externally */
    aglexinit(fp, NULL);
    rv = agparse();
    AG.fgets = savefgets;
    return finish(rv, AG.parsed_g);
}

static int
_is_number_char(char c)
{
    return (isdigit(c) || (c == '.'));
}

/* _agstrcanon:
 * Canonicalize an ordinary string if necessary.
 */
static char* 
_agstrcanon (char* arg, char* buf)
{
    char *s = arg;
    unsigned char uc;
    char *p = buf;
    int cnt = 0, dotcnt = 0;
    int has_special = FALSE;
    int maybe_num;
    int backslash_pending = FALSE;

    if (ISEMPTYSTR(arg))
	return "\"\"";
    *p++ = '\"';
    uc = *(unsigned char *) s++;
    maybe_num = _is_number_char(uc) || (uc == '-');
    while (uc) {
	if (uc == '\"') {
	    *p++ = '\\';
	    has_special = TRUE;
	} 
	else if (maybe_num) {
	    if (uc == '-') {
		if (cnt) {
		    maybe_num = FALSE;
		    has_special = TRUE;
		}
	    }
	    else if (uc == '.') {
		if (dotcnt++) {
		    maybe_num = FALSE;
		    has_special = TRUE;
		}
	    }
	    else if (!isdigit(uc)) {
		maybe_num = FALSE;
		has_special = TRUE;
	    }
	}
	else if (!ISALNUM(uc))
	    has_special = TRUE;
	*p++ = (char) uc;
	uc = *(unsigned char *) s++;
	cnt++;
        if (uc && backslash_pending && !((_is_number_char(p[-1]) || isalpha(p[-1]) || (p[-1] == '\\')) && (_is_number_char(uc) || isalpha(uc)))) {
            *p++ = '\\';
            *p++ = '\n';
            has_special = TRUE;
            backslash_pending = FALSE;
        } else if (uc && cnt % SMALLBUF == 0) {
            if (!((_is_number_char(p[-1]) || isalpha(p[-1]) || (p[-1] == '\\')) && (_is_number_char(uc) || isalpha(uc)))) {
	        *p++ = '\\';
    	        *p++ = '\n';
	        has_special = TRUE;
            } else {
                backslash_pending = TRUE;
            }
	}
    }
    *p++ = '\"';
    *p = '\0';
    if (has_special || ((cnt == 1) && ((*arg == '.') || (*arg == '-'))))
	return buf;

    /* use quotes to protect tokens (example, a node named "node") */
    if (agtoken(arg) >= 0)
	return buf;
    return arg;
}

/* agstrcanon:
 * handles both html and ordinary strings.
 * canonicalize a string for printing.
 * changes to the semantics of this function
 * also involve the string scanner in lexer.c
 * Unsafe if buf is not large enough.
 */
char *agstrcanon(char *arg, char *buf)
{
    char *s = arg;
    char *p = buf;

    if (aghtmlstr(arg)) {
	*p++ = '<';
	while (*s)
	    *p++ = *s++;
	*p++ = '>';
	*p = '\0';
	return buf;
    }
    else
	return (_agstrcanon(arg, buf));
}

void agsetiodisc(
    char * (*myfgets) (char *s, int size, FILE *stream),
    size_t (*myfwrite) (const void *ptr, size_t size, size_t nmemb, FILE *stream),
    int (*myferror) (FILE *stream)
)
{
    if (myfgets) AG.fgets = myfgets;
    if (myfwrite) AG.fwrite = myfwrite;
#if defined(__SUNPRO_C) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__FreeBSD__)
#undef ferror
#endif
    if (myferror) AG.ferror = myferror;
}


int agputs(const char *s, FILE *fp)
{
    size_t len = strlen(s);

    if (AG.fwrite(s, sizeof(char), len, fp) != len) {
	return EOF;
    }
    return +1;
}

int agputc(int c, FILE *fp)
{
    const char cc = c;

    if (AG.fwrite (&cc, sizeof(char), 1, fp) != 1) {
	return EOF;
    }
    return c;
}

static void tabover(FILE * fp, int tab)
{
    while (tab--)
	agputc('\t', fp);
}

static char *getoutputbuffer(char *str)
{
    static char *rv;
    static int len;
    int req;

    req = MAX(2 * strlen(str) + 2, BUFSIZ);
    if (req > len) {
	if (rv)
	    rv = (char*)realloc(rv, req);
	else
	    rv = (char*)malloc(req);
	len = req;
    }
    return rv;
}

/* agcanonical:
 * Safe version of agstrcanon.
 */
char*
agcanonical(char *str)
{
    return agstrcanon(str, getoutputbuffer(str));
}

/* agcanon:
 * Safe version of agstrcanon but using only double quotes.
 * Any string can be used, but there is no check for html strings.
 */
char*
agcanon(char *str)
{
    return _agstrcanon(str, getoutputbuffer(str));
}

static void write_dict(Agdict_t * dict, FILE * fp)
{
    int i, cnt = 0;
    Agsym_t *a;

    for (i = 0; i < dtsize(dict->dict); i++) {
	a = dict->list[i];
	if (ISEMPTYSTR(a->value) == FALSE) {
	    if (cnt++ == 0) {
		agputc('\t', fp);
		agputs(dict->name, fp);
		agputs(" [", fp);
	    }
	    else {
		agputs(", ", fp);
            }
	    agputs(a->name, fp);
	    agputc('=', fp);
	    agputs(agcanonical(a->value), fp);
	}
    }
    if (cnt > 0)
	agputs("];\n", fp);
}

static void write_diffattr(FILE * fp, int indent, void *obj, void *par,
			   Agdict_t * dict)
{
    Agsym_t *a;
    int i;
    char *p, *q;
    int cnt = 0;

    for (i = 0; i < dtsize(dict->dict); i++) {
	a = dict->list[i];
	if (a->printed == FALSE)
	    continue;
	p = agxget(obj, a->index);
	if (par)
	    q = agxget(par, a->index);
	else
	    q = a->value;
	if (strcmp(p, q)) {
	    if (cnt++ == 0) {
		tabover(fp, indent);
	        agputs(dict->name, fp);
	        agputs(" [", fp);
	    } else {
	        agputs(",\n", fp);
		tabover(fp, indent + 1);
	    }
	    agputs(agcanonical(a->name), fp);
	    agputc('=', fp);
	    agputs(agcanonical(p), fp);
	}
    }
    if (cnt > 0)
	agputs("];\n", fp);
}

static void writeattr(FILE * fp, int *npp, char *name, char *val)
{
    agputs(++(*npp) > 1 ? ", " : " [", fp);
    agputs(agcanonical(name), fp);
    agputc('=', fp);
    agputs(agcanonical(val), fp);
}

void agwrnode(Agraph_t * g, FILE * fp, Agnode_t * n, int full, int indent)
{
    char *myval, *defval;
    int i, didwrite = FALSE;
    int nprint = 0;
    Agdict_t *d = n->graph->univ->nodeattr;
    Agsym_t *a;

    if (full) {
	for (i = 0; i < dtsize(d->dict); i++) {
	    a = d->list[i];
	    if (a->printed == FALSE)
		continue;
	    myval = agget(n, a->name);
	    if (g == n->graph)
		defval = a->value;
	    else
		defval = agget(g->proto->n, a->name);
	    if (strcmp(defval, myval)) {
		if (didwrite == FALSE) {
		    tabover(fp, indent);
		    agputs(agcanonical(n->name), fp);
		    didwrite = TRUE;
		}
		writeattr(fp, &nprint, a->name, myval);
	    }
	}
	if (didwrite) {
	    agputs(nprint > 0 ? "];\n" : ";\n", fp);
	    return;
	}
    }
    if ((agfstout(g, n) == NULL) && (agfstin(g, n) == NULL)) {
	tabover(fp, indent);
	agputs(agcanonical(n->name), fp);
	agputs(";\n", fp);
    }
}

/* writenodeandport:
 */
static void writenodeandport(FILE * fp, char *node, char *port)
{
    char *ss;
    agputs(agcanonical(node), fp);	/* slimey i know */
    if (port && *port) {
	if (aghtmlstr(port)) {
            agputc(':', fp);
	    agputs(agstrcanon(port, getoutputbuffer(port)), fp);
	}
	else {
	    ss = strchr (port, ':');
	    if (ss) {
		*ss = '\0';
                agputc(':', fp);
		agputs(_agstrcanon(port, getoutputbuffer(port)), fp);
                agputc(':', fp);
		agputs(_agstrcanon(ss+1, getoutputbuffer(ss+1)), fp);
		*ss = ':';
	    }
	    else {
                agputc(':', fp);
		agputs(_agstrcanon(port, getoutputbuffer(port)), fp);
	    }
	}
    }
}

void agwredge(Agraph_t * g, FILE * fp, Agedge_t * e, int list_all)
{
    char *myval, *defval, *tport, *hport;
    int i, nprint = 0;
    Agdict_t *d = e->tail->graph->univ->edgeattr;
    Agsym_t *a;

    if (e->attr) {
	tport = e->attr[TAILX];
	hport = e->attr[HEADX];
    }
    else tport = hport = "";
    writenodeandport(fp, e->tail->name, tport);
    agputs(((g->kind & AGFLAG_DIRECTED) ? " -> " : " -- "), fp);
    writenodeandport(fp, e->head->name, hport);
    if (list_all) {
	for (i = 0; i < dtsize(d->dict); i++) {
	    a = d->list[i];
	    if ((a->printed == FALSE)
		|| ((i == KEYX) && (e->printkey != MUSTPRINT)))
		continue;
	    myval = agget(e, a->name);
	    if (g == g->root)
		defval = a->value;
	    else
		defval = agget(g->proto->e, a->name);
	    if (strcmp(defval, myval))
		writeattr(fp, &nprint, a->name, myval);
	}
    }
    agputs(nprint > 0 ? "];\n" : ";\n", fp);
}

Dtdisc_t agEdgedisc = {
    offsetof(Agedge_t, id),
    sizeof(int),
    -1,
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    (Dtcompar_f) agcmpid,
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};
static void
write_subg(Agraph_t * g, FILE * fp, Agraph_t * par, int indent,
	   printdict_t * state)
{
    Agraph_t *subg, *meta;
    Agnode_t *n, *pn;
    Agedge_t *e, *pe;
    Dict_t *save_e, *save_n;

    if (indent) {
	tabover(fp, indent++);
	if (dtsearch(state->subgleft, g->meta_node)) {
	    if (strncmp(g->name, "_anonymous", 10)) {
		agputs("subgraph ", fp);
		agputs(agcanonical(g->name), fp);
		agputs(" {\n", fp);
	    }
	    else {
		agputs("{\n", fp);	/* no name printed for anonymous subg */
	    }
	    write_diffattr(fp, indent, g, par, g->univ->globattr);
	    /* The root node and edge environment use the dictionaries,
	     * not the proto node or edge, so the next level down must
	     * record differences with the dictionaries.
	     */
	    if (par == g->root) {
		pn = NULL;
		pe = NULL;
	    } else {
		pn = par->proto->n;
		pe = par->proto->e;
	    }
	    write_diffattr(fp, indent, g->proto->n, pn, g->univ->nodeattr);
	    write_diffattr(fp, indent, g->proto->e, pe, g->univ->edgeattr);
	    dtdelete(state->subgleft, g->meta_node);
	} else {
	    agputs("subgraph ", fp);
	    agputs(agcanonical(g->name), fp);
	    agputs(";\n", fp);
	    return;
	}
    } else
	write_diffattr(fp, ++indent, g, NULL, g->univ->globattr);

    save_n = state->n_insubg;
    save_e = state->e_insubg;
    meta = g->meta_node->graph;
    state->n_insubg = dtopen(&agNamedisc, Dttree);
    state->e_insubg = dtopen(&agOutdisc, Dttree);
    for (e = agfstout(meta, g->meta_node); e; e = agnxtout(meta, e)) {
	subg = agusergraph(e->head);
	write_subg(subg, fp, g, indent, state);
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (dtsearch(state->nodesleft, n)) {
	    agwrnode(g, fp, n, TRUE, indent);
	    dtdelete(state->nodesleft, n);
	} else {
	    if (dtsearch(state->n_insubg, n) == NULL) {
		agwrnode(g, fp, n, FALSE, indent);
	    }
	}
	dtinsert(save_n, n);
    }

    dtdisc(g->outedges, &agEdgedisc, 0);	/* sort by id */
    for (e = (Agedge_t *) dtfirst(g->outedges); e;
	 e = (Agedge_t *) dtnext(g->outedges, e)) {
	if (dtsearch(state->edgesleft, e)) {
	    tabover(fp, indent);
	    agwredge(g, fp, e, TRUE);
	    dtdelete(state->edgesleft, e);
	} else {
	    if (dtsearch(state->e_insubg, e) == NULL) {
		tabover(fp, indent);
		agwredge(g, fp, e, FALSE);
	    }
	}
	dtinsert(save_e, e);
    }
    dtdisc(g->outedges, &agOutdisc, 0);	/* sort by name */
    dtclose(state->n_insubg);
    state->n_insubg = save_n;
    dtclose(state->e_insubg);
    state->e_insubg = save_e;

    if (indent > 1) {
	tabover(fp, indent - 1);
	agputs("}\n", fp);
    }
}

static Dict_t *Copy;
static int copydictf(Dict_t * d, void *a, void *ignored)
{
    dtinsert(Copy, (Agsym_t *) a);
    return 0;
}

static void copydict(Dict_t * from, Dict_t * to)
{
    Copy = to;
    dtwalk(from, copydictf, 0);
}

static printdict_t *new_printdict_t(Agraph_t * g)
{
    printdict_t *rv = NEW(printdict_t);
    rv->nodesleft = dtopen(&agNodedisc, Dttree);
    copydict(g->nodes, rv->nodesleft);
    rv->edgesleft = dtopen(&agEdgedisc, Dttree);
    copydict(g->outedges, rv->edgesleft);
    rv->n_insubg = dtopen(&agNodedisc, Dttree);
    rv->e_insubg = dtopen(&agOutdisc, Dttree);
    rv->subgleft = dtopen(&agNodedisc, Dttree);
    copydict(g->meta_node->graph->nodes, rv->subgleft);
    return rv;
}

static void free_printdict_t(printdict_t * dict)
{
    dtclose(dict->nodesleft);
    dtclose(dict->n_insubg);
    dtclose(dict->edgesleft);
    dtclose(dict->e_insubg);
    dtclose(dict->subgleft);
    free(dict);
}

#ifdef ferror
/* if ferror is a macro (__SUNPRO_C __CYGWIN__ __MINGW32__ __FreeBSD__ and poss others)
 * then wrap it in a function */
static int agferror(FILE *stream)
{
        return ferror(stream);
}
#endif


int agwrite(Agraph_t * g, FILE * fp)
{
    printdict_t *p;

    if (AG.fwrite == NULL) {
        AG.fwrite = fwrite;   /* init to system version of fwrite() */
    }
    if (AG.ferror == NULL) {
#ifdef ferror
#undef ferror
	/* if ferror is a macro, then use our wrapper function, but 
 	 * undef the macro first so it doesn't subst in "AG.ferror" */
	AG.ferror = agferror; /* init to ferror macro wrapper function */
#else
	AG.ferror = ferror;   /* init to system version of ferror() */
#endif
    }

    /* write the graph header */
    agputs((AG_IS_STRICT(g)) ? "strict " : "", fp);
    agputs((AG_IS_DIRECTED(g)) ? "digraph" : "graph", fp);
    if (strncmp(g->name, "_anonymous", 10)) {
	agputc(' ', fp);
	agputs(agcanonical(g->name), fp);
    }
    agputs(" {\n", fp);

    /* write the top level attribute defs */
    write_dict(g->univ->globattr, fp);
    write_dict(g->univ->nodeattr, fp);
    write_dict(g->univ->edgeattr, fp);

    /* write the graph contents */
    p = new_printdict_t(g);
    write_subg(g, fp, (Agraph_t *) 0, 0, p);
    agputs("}\n", fp);
    free_printdict_t(p);
    return AG.ferror(fp);
}
