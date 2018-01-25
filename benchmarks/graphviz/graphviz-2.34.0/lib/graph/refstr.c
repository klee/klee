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


#include	"libgraph.h"
#include	<assert.h>

static unsigned int HTML_BIT;
static unsigned int CNT_BITS;

#ifdef DMALLOC
#include "dmalloc.h"
#endif

typedef struct refstr_t {
    Dtlink_t link;
    unsigned int refcnt;
    char s[1];
} refstr_t;

static Dtdisc_t Refstrdisc = {
    offsetof(refstr_t, s[0]),
    0,
    0,
    ((Dtmake_f) 0),
    ((Dtfree_f) 0),
    ((Dtcompar_f) 0),		/* use strcmp */
    ((Dthash_f) 0),
    ((Dtmemory_f) 0),
    ((Dtevent_f) 0)
};

static Dict_t *StringDict;

#ifdef DEBUG
static int refstrprint(Dt_t * d, Void_t * obj, Void_t * env)
{
    refstr_t *r = (refstr_t *) obj;
    fprintf(stderr, "%s\n", r->s);
    return 0;
}

void agrefstrdump(void)
{
    dtwalk(StringDict, refstrprint, 0);
}
#endif

/* initialize_strings:
 * Create dictionary and masks for HTML strings.
 * HTML_BIT must be the most significant bit. We assume 8-bit bytes.
 */
static void initialize_strings(void)
{
    StringDict = dtopen(&Refstrdisc, Dttree);
    HTML_BIT = ((unsigned int) 1) << (sizeof(unsigned int) * 8 - 1);
    CNT_BITS = ~HTML_BIT;
}

char *agstrdup(char *s)
{
    refstr_t *key, *r;

    if (StringDict == NULL)
	initialize_strings();
    if (s == NULL)
	return s;

    key = (refstr_t *) (s - offsetof(refstr_t, s[0]));
    r = (refstr_t *) dtsearch(StringDict, key);
    if (r)
	r->refcnt++;
    else {
	r = (refstr_t *) malloc(sizeof(refstr_t) + strlen(s));
	r->refcnt = 1;
	strcpy(r->s, s);
	dtinsert(StringDict, r);
    }
    return r->s;
}

/* agstrdup_html:
 * For various purposes, including deparsing, we have to recognize
 * strings coming from <...> rather than "...". To do this, we set
 * the top bit of the refcnt field. Since the use of html strings
 * is localized, we allow the application to make the distinction.
 */
char *agstrdup_html(char *s)
{
    refstr_t *key, *r;

    if (StringDict == NULL)
	initialize_strings();
    if (s == NULL)
	return s;

    key = (refstr_t *) (s - offsetof(refstr_t, s[0]));
    r = (refstr_t *) dtsearch(StringDict, key);
    if (r)
	r->refcnt++;
    else {
	r = (refstr_t *) malloc(sizeof(refstr_t) + strlen(s));
	r->refcnt = 1 | HTML_BIT;
	strcpy(r->s, s);
	dtinsert(StringDict, r);
    }
    return r->s;
}

void agstrfree(char *s)
{
    refstr_t *key, *r;

    if ((StringDict == NULL) || (s == NULL))
	return;
    key = (refstr_t *) (s - offsetof(refstr_t, s[0]));
    r = (refstr_t *) dtsearch(StringDict, key);

    if (r) {
	r->refcnt--;
	if ((r->refcnt && CNT_BITS) == 0) {
	    dtdelete(StringDict, r);
	    free(r);
	}
    } else
	agerr(AGERR, "agstrfree lost %s\n", s);
}

/* aghtmlstr:
 * Return true if s is an HTML string.
 * We assume s points to the datafield s[0] of a refstr.
 */
int aghtmlstr(char *s)
{
    refstr_t *key;

    if ((StringDict == NULL) || (s == NULL))
	return 0;
    key = (refstr_t *) (s - offsetof(refstr_t, s[0]));
    return (key->refcnt & HTML_BIT);
}
