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


/*
 * gpr state
 *
 */

#ifdef WIN32
#include "windows.h"
#include "Shlwapi.h"
#endif

#include <gprstate.h>
#include <error.h>
#include <sfstr.h>

static int name_used;

int validTVT(int c)
{
    return ((TV_flat <= c) && (c <= TV_prepostrev));
}

void initGPRState(Gpr_t * state, Vmalloc_t * vm)
{
    state->tgtname = vmstrdup(vm, "gvpr_result");
}

Gpr_t *openGPRState(gpr_info* info)
{
    Gpr_t *state;

    if (!(state = newof(0, Gpr_t, 1, 0))) {
	error(ERROR_ERROR, "Could not create gvpr state: out of memory");
	return state;
    }

    if (!(state->tmp = sfstropen())) {
	error(ERROR_ERROR, "Could not create state tmpfile");
	free (state);
	return 0;
    }

    state->tvt = TV_flat;
    state->name_used = name_used;
    state->tvroot = 0;
    state->tvnext = 0;
    state->tvedge = 0;
    state->outFile = info->outFile;
    state->argc = info->argc;
    state->argv = info->argv;
    state->errf = info->errf;
    state->flags = info->flags;

    return state;
}


static int
bindingcmpf (const void *key, const void *ip)
{
    return strcmp (((gvprbinding*)key)->name, ((gvprbinding*)ip)->name);
}

/* findBinding:
 */
gvprbinding* 
findBinding (Gpr_t* state, char* fname)
{
    gvprbinding key;
    gvprbinding* bp;

    if (!state->bindings) {
	error(ERROR_ERROR,"call(\"%s\") failed: no bindings", fname);
	return NULL;
    }
    if (!fname) {
	error(ERROR_ERROR,"NULL function name for call()");
	return NULL;
    }

    key.name = fname;
    bp = (gvprbinding*)bsearch(&key, state->bindings, state->n_bindings, sizeof(gvprbinding), bindingcmpf); 
    if (!bp)
	error(ERROR_ERROR, "No binding for \"%s\" in call()", fname);
    return bp;
}

/* addBindings:
 * Validate input, sort lexicographically, and attach
 */
void addBindings (Gpr_t* state, gvprbinding* bindings)
{
    int n = 0;
    gvprbinding* bp = bindings;
    gvprbinding* buf;
    gvprbinding* bufp;

    while (bp && bp->name) {
	if (bp->fn) n++;
	bp++;
    }

    if (n == 0) return;
    bufp = buf = newof(0, gvprbinding, n, 0);
    bp = bindings;
    while (bp->name) {
        if (bp->fn) {
	    *bufp = *bp;
	    bufp++;
	}
	bp++;
    }
    qsort (buf, n, sizeof(gvprbinding), bindingcmpf);

    state->bindings = buf;
    state->n_bindings = n;
}

void closeGPRState(Gpr_t* state)
{
    if (!state) return;
    name_used = state->name_used;
    if (state->tmp)
	sfclose (state->tmp);
    free (state->dp);
    free (state);
}

#ifdef WIN32_DLL
int pathisrelative (char* path)
{
    return PathIsRelative(path);

}
#endif

