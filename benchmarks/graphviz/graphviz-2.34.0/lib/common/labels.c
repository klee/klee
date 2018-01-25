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
#include "htmltable.h"
#include <limits.h>

static char *strdup_and_subst_obj0 (char *str, void *obj, int escBackslash);

static void storeline(graph_t *g, textlabel_t *lp, char *line, char terminator)
{
    pointf size;
    textpara_t *para;
    int oldsz = lp->u.txt.nparas + 1;

    lp->u.txt.para = ZALLOC(oldsz + 1, lp->u.txt.para, textpara_t, oldsz);
    para = &(lp->u.txt.para[lp->u.txt.nparas]);
    para->str = line;
    para->just = terminator;
    if (line && line[0])
        size = textsize(g, para, lp->fontname, lp->fontsize);
    else {
	size.x = 0.0;
	para->height = size.y = (int)(lp->fontsize * LINESPACING);
    }

    lp->u.txt.nparas++;
    /* width = max line width */
    lp->dimen.x = MAX(lp->dimen.x, size.x);
    /* accumulate height */
    lp->dimen.y += size.y;
}

/* compiles <str> into a label <lp> */
void make_simple_label(graph_t * g, textlabel_t * lp)
{
    char c, *p, *line, *lineptr, *str = lp->text;
    unsigned char byte = 0x00;

    lp->dimen.x = lp->dimen.y = 0.0;
    if (*str == '\0')
	return;

    line = lineptr = NULL;
    p = str;
    line = lineptr = N_GNEW(strlen(p) + 1, char);
    *line = 0;
    while ((c = *p++)) {
	byte = (unsigned int) c;
	/* wingraphviz allows a combination of ascii and big-5. The latter
         * is a two-byte encoding, with the first byte in 0xA1-0xFE, and
         * the second in 0x40-0x7e or 0xa1-0xfe. We assume that the input
         * is well-formed, but check that we don't go past the ending '\0'.
         */
	if ((lp->charset == CHAR_BIG5) && 0xA1 <= byte && byte <= 0xFE) {
	    *lineptr++ = c;
	    c = *p++;
	    *lineptr++ = c;
	    if (!c) /* NB. Protect against unexpected string end here */
		break;
	} else {
	    if (c == '\\') {
		switch (*p) {
		case 'n':
		case 'l':
		case 'r':
		    *lineptr++ = '\0';
		    storeline(g, lp, line, *p);
		    line = lineptr;
		    break;
		default:
		    *lineptr++ = *p;
		}
		if (*p)
		    p++;
		/* tcldot can enter real linend characters */
	    } else if (c == '\n') {
		*lineptr++ = '\0';
		storeline(g, lp, line, 'n');
		line = lineptr;
	    } else {
		*lineptr++ = c;
	    }
	}
    }

    if (line != lineptr) {
	*lineptr++ = '\0';
	storeline(g, lp, line, 'n');
    }

    lp->space = lp->dimen;
}

/* make_label:
 * Assume str is freshly allocated for this instance, so it
 * can be freed in free_label.
 */
textlabel_t *make_label(void *obj, char *str, int kind, double fontsize, char *fontname, char *fontcolor)
{
    textlabel_t *rv = NEW(textlabel_t);
    graph_t *g = NULL, *sg = NULL;
    node_t *n = NULL;
    edge_t *e = NULL;
        char *s;

    switch (agobjkind(obj)) {
#ifndef WITH_CGRAPH
    case AGGRAPH:
#else
    case AGRAPH:
#endif
        sg = (graph_t*)obj;
	g = sg->root;
	break;
    case AGNODE:
        n = (node_t*)obj;
	g = agroot(agraphof(n));
	break;
    case AGEDGE:
        e = (edge_t*)obj;
	g = agroot(agraphof(aghead(e)));
	break;
    }
    rv->fontname = fontname;
    rv->fontcolor = fontcolor;
    rv->fontsize = fontsize;
    rv->charset = GD_charset(g);
    if (kind & LT_RECD) {
	rv->text = strdup(str);
        if (kind & LT_HTML) {
	    rv->html = TRUE;
	}
    }
    else if (kind == LT_HTML) {
	rv->text = strdup(str);
	rv->html = TRUE;
	if (make_html_label(obj, rv)) {
	    switch (agobjkind(obj)) {
#ifndef WITH_CGRAPH
	    case AGGRAPH:
#else
	    case AGRAPH:
#endif
	        agerr(AGPREV, "in label of graph %s\n",agnameof(sg));
		break;
	    case AGNODE:
	        agerr(AGPREV, "in label of node %s\n", agnameof(n));
		break;
	    case AGEDGE:
		agerr(AGPREV, "in label of edge %s %s %s\n",
		        agnameof(agtail(e)), agisdirected(g)?"->":"--", agnameof(aghead(e)));
		break;
	    }
	}
    }
    else {
        assert(kind == LT_NONE);
	/* This call just processes the graph object based escape sequences. The formatting escape
         * sequences (\n, \l, \r) are processed in make_simple_label. That call also replaces \\ with \.
         */
	rv->text = strdup_and_subst_obj0(str, obj, 0);
        switch (rv->charset) {
	case CHAR_LATIN1:
	    s = latin1ToUTF8(rv->text);
	    break;
	default: /* UTF8 */
	    s = htmlEntityUTF8(rv->text, g);
	    break;
	}
        free(rv->text);
        rv->text = s;
	make_simple_label(g, rv);
    }
    return rv;
}

/* free_textpara:
 * Free resources related to textpara_t.
 * tl is an array of cnt textpara_t's.
 * It is also assumed that the text stored in the str field
 * is all stored in one large buffer shared by all of the textpara_t,
 * so only the first one needs to free its tlp->str.
 */
void free_textpara(textpara_t * tl, int cnt)
{
    int i;
    textpara_t* tlp = tl;

    if (!tl) return;
    for (i = 0; i < cnt; i++) { 
	if ((i == 0) && tlp->str)
	    free(tlp->str);
	if (tlp->layout && tlp->free_layout)
	    tlp->free_layout (tlp->layout);
	tlp++;
    }
    free(tl);
}

void free_label(textlabel_t * p)
{
    if (p) {
	free(p->text);
	if (p->html) {
	    if (p->u.html) free_html_label(p->u.html, 1);
	} else {
	    free_textpara(p->u.txt.para, p->u.txt.nparas);
	}
	free(p);
    }
}

void emit_label(GVJ_t * job, emit_state_t emit_state, textlabel_t * lp)
{
    obj_state_t *obj = job->obj;
    int i;
    pointf p;
    emit_state_t old_emit_state;

    old_emit_state = obj->emit_state;
    obj->emit_state = emit_state;

    if (lp->html) {
	emit_html_label(job, lp->u.html, lp);
	obj->emit_state = old_emit_state;
	return;
    }

    /* make sure that there is something to do */
    if (lp->u.txt.nparas < 1)
	return;

    gvrender_begin_label(job, LABEL_PLAIN);
    gvrender_set_pencolor(job, lp->fontcolor);

    /* position for first para */
    switch (lp->valign) {
	case 't':
    	    p.y = lp->pos.y + lp->space.y / 2.0 - lp->fontsize;
	    break;
	case 'b':
    	    p.y = lp->pos.y - lp->space.y / 2.0 + lp->dimen.y - lp->fontsize;
	    break;
	case 'c':
	default:	
    	    p.y = lp->pos.y + lp->dimen.y / 2.0 - lp->fontsize;
	    break;
    }
    for (i = 0; i < lp->u.txt.nparas; i++) {
	switch (lp->u.txt.para[i].just) {
	case 'l':
	    p.x = lp->pos.x - lp->space.x / 2.0;
	    break;
	case 'r':
	    p.x = lp->pos.x + lp->space.x / 2.0;
	    break;
	default:
	case 'n':
	    p.x = lp->pos.x;
	    break;
	}
	gvrender_textpara(job, p, &(lp->u.txt.para[i]));

	/* UL position for next para */
	p.y -= lp->u.txt.para[i].height;
    }

    gvrender_end_label(job);
    obj->emit_state = old_emit_state;
}

/* strdup_and_subst_obj0:
 * Replace various escape sequences with the name of the associated
 * graph object. A double backslash \\ can be used to avoid a replacement.
 * If escBackslash is true, convert \\ to \; else leave alone. All other dyads 
 * of the form \. are passed through unchanged.
 */
static char *strdup_and_subst_obj0 (char *str, void *obj, int escBackslash)
{
    char c, *s, *p, *t, *newstr;
    char *tp_str = "", *hp_str = "";
    char *g_str = "\\G", *n_str = "\\N", *e_str = "\\E",
	*h_str = "\\H", *t_str = "\\T", *l_str = "\\L";
    int g_len = 2, n_len = 2, e_len = 2,
	h_len = 2, t_len = 2, l_len = 2,
	tp_len = 0, hp_len = 0;
    int newlen = 0;
    int isEdge = 0;
    textlabel_t *tl;
    port pt;

    /* prepare substitution strings */
    switch (agobjkind(obj)) {
#ifndef WITH_CGRAPH
	case AGGRAPH:
#else
	case AGRAPH:
#endif
	    g_str = agnameof((graph_t *)obj);
	    g_len = strlen(g_str);
	    tl = GD_label((graph_t *)obj);
	    if (tl) {
		l_str = tl->text;
	    	if (str) l_len = strlen(l_str);
	    }
	    break;
	case AGNODE:
	    g_str = agnameof(agraphof((node_t *)obj));
	    g_len = strlen(g_str);
	    n_str = agnameof((node_t *)obj);
	    n_len = strlen(n_str);
	    tl = ND_label((node_t *)obj);
	    if (tl) {
		l_str = tl->text;
	    	if (str) l_len = strlen(l_str);
	    }
	    break;
	case AGEDGE:
	    isEdge = 1;
	    g_str = agnameof(agroot(agraphof(agtail(((edge_t *)obj)))));
	    g_len = strlen(g_str);
	    t_str = agnameof(agtail(((edge_t *)obj)));
	    t_len = strlen(t_str);
	    pt = ED_tail_port((edge_t *)obj);
	    if ((tp_str = pt.name))
	        tp_len = strlen(tp_str);
	    h_str = agnameof(aghead(((edge_t *)obj)));
	    h_len = strlen(h_str);
	    pt = ED_head_port((edge_t *)obj);
	    if ((hp_str = pt.name))
		hp_len = strlen(hp_str);
	    h_len = strlen(h_str);
	    tl = ED_label((edge_t *)obj);
	    if (tl) {
		l_str = tl->text;
	    	if (str) l_len = strlen(l_str);
	    }
	    if (agisdirected(agroot(agraphof(agtail(((edge_t*)obj))))))
		e_str = "->";
	    else
		e_str = "--";
	    e_len = t_len + (tp_len?tp_len+1:0) + 2 + h_len + (hp_len?hp_len+1:0);
	    break;
    }

    /* two passes over str.
     *
     * first pass prepares substitution strings and computes 
     * total length for newstring required from malloc.
     */
    for (s = str; (c = *s++);) {
	if (c == '\\') {
	    switch (c = *s++) {
	    case 'G':
		newlen += g_len;
		break;
	    case 'N':
		newlen += n_len;
		break;
	    case 'E':
		newlen += e_len;
		break;
	    case 'H':
		newlen += h_len;
		break;
	    case 'T':
		newlen += t_len;
		break; 
	    case 'L':
		newlen += l_len;
		break; 
	    case '\\':
		if (escBackslash) {
		    newlen += 1;
		    break; 
		}
		/* Fall through */
	    default:  /* leave other escape sequences unmodified, e.g. \n \l \r */
		newlen += 2;
	    }
	} else {
	    newlen++;
	}
    }
    /* allocate new string */
    newstr = gmalloc(newlen + 1);

    /* second pass over str assembles new string */
    for (s = str, p = newstr; (c = *s++);) {
	if (c == '\\') {
	    switch (c = *s++) {
	    case 'G':
		for (t = g_str; (*p = *t++); p++);
		break;
	    case 'N':
		for (t = n_str; (*p = *t++); p++);
		break;
	    case 'E':
		if (isEdge) {
		    for (t = t_str; (*p = *t++); p++);
		    if (tp_len) {
			*p++ = ':';
			for (t = tp_str; (*p = *t++); p++);
		    }
		    for (t = e_str; (*p = *t++); p++);
		    for (t = h_str; (*p = *t++); p++);
		    if (hp_len) {
			*p++ = ':';
			for (t = hp_str; (*p = *t++); p++);
		    }
		}
		break;
	    case 'T':
		for (t = t_str; (*p = *t++); p++);
		break;
	    case 'H':
		for (t = h_str; (*p = *t++); p++);
		break;
	    case 'L':
		for (t = l_str; (*p = *t++); p++);
		break;
	    case '\\':
		if (escBackslash) {
		    *p++ = '\\';
		    break; 
		}
		/* Fall through */
	    default:  /* leave other escape sequences unmodified, e.g. \n \l \r */
		*p++ = '\\';
		*p++ = c;
		break;
	    }
	} else {
	    *p++ = c;
	}
    }
    *p++ = '\0';
    return newstr;
}

/* strdup_and_subst_obj:
 * Processes graph object escape sequences; also collapses \\ to \.
 */
char *strdup_and_subst_obj(char *str, void *obj)
{
    return strdup_and_subst_obj0 (str, obj, 1);
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
	buf = gmalloc(bufsize);
    }

    p = buf;
    while (s && *s) {
	if (pos > (bufsize - 8)) {
	    bufsize *= 2;
	    buf = grealloc(buf, bufsize);
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

/* a variant of xml_string for urls in hrefs */
char *xml_url_string(char *s)
{
    static char *buf = NULL;
    static int bufsize = 0;
    char *p, *sub, *prev = NULL;
    int len, pos = 0;

    if (!buf) {
	bufsize = 64;
	buf = gmalloc(bufsize);
    }

    p = buf;
    while (s && *s) {
	if (pos > (bufsize - 8)) {
	    bufsize *= 2;
	    buf = grealloc(buf, bufsize);
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
#if 0
	else if (*s == '-') {	/* can't be used in xml comment strings */
	    sub = "&#45;";
	    len = 5;
	}
	else if (*s == ' ' && prev && *prev == ' ') {
	    /* substitute 2nd and subsequent spaces with required_spaces */
	    sub = "&#160;";  /* inkscape doesn't recognise &nbsp; */
	    len = 6;
	}
#endif
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
