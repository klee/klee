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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>
#include <intset.h>
#include <memory.h>

static Void_t*
mkIntItem(Dt_t* d,intitem* obj,Dtdisc_t* disc)
{ 
    intitem* np = NEW(intitem);
    np->id = obj->id;
    return (Void_t*)np;
}

static void
freeIntItem(Dt_t* d,intitem* obj,Dtdisc_t* disc)
{
    free (obj);
}

static int
cmpid(Dt_t* d, int* key1, int* key2, Dtdisc_t* disc)
{
  if (*key1 > *key2) return 1;
  else if (*key1 < *key2) return -1;
  else return 0;
}   

static Dtdisc_t intSetDisc = {
    offsetof(intitem,id),
    sizeof(int),
    offsetof(intitem,link),
    (Dtmake_f)mkIntItem,
    (Dtfree_f)freeIntItem,
    (Dtcompar_f)cmpid,
    0,
    0,
    0
};

Dt_t* 
openIntSet (void)
{
    return dtopen(&intSetDisc,Dtoset);
}

void 
addIntSet (Dt_t* is, int v)
{
    intitem obj;

    obj.id = v;
    dtinsert(is, &obj);
}

int 
inIntSet (Dt_t* is, int v)
{
    return (dtmatch (is, &v) != 0);
}

