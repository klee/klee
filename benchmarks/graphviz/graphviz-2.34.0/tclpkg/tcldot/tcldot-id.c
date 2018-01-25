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


#include "tcldot.h"

#ifdef WITH_CGRAPH

// Agiddisc functions
static void *myiddisc_open(Agraph_t *g, Agdisc_t *disc) {
    ictx_t *ictx = (ictx_t *)disc;
    gctx_t *gctx;

    gctx = (gctx_t *)malloc(sizeof(gctx_t));
    gctx->g = g;
    gctx->ictx = ictx;
    return (void *)gctx;
}
static long myiddisc_map(void *state, int objtype, char *str, unsigned long *id, int createflag) {
    gctx_t *gctx = (gctx_t *)state;
    ictx_t *ictx = gctx->ictx;
    char *s;

    if (str) {
        if (createflag)
            s = agstrdup(gctx->g, str);
        else
            s = agstrbind(gctx->g, str);
        *id = (unsigned long) s;
    } else {
        *id = ictx->ctr;  /* counter maintained in per-interp space, so that
		ids are unique across all graphs in the interp */
        ictx->ctr += 2;
    }
    return TRUE;
}
/* we don't allow users to explicitly set IDs, either */
static long myiddisc_alloc(void *state, int objtype, unsigned long request_id) {
    NOTUSED(state);
    NOTUSED(objtype);
    NOTUSED(request_id);
    return FALSE;
}
static void myiddisc_free(void *state, int objtype, unsigned long id) {
    gctx_t *gctx = (gctx_t *)state;

/* FIXME no obj* available
    ictx_t *ictx = gctx->ictx;
    char buf[32] = "";

    switch (objtype) {
        case AGRAPH: sprintf(buf,"graph%lu",id); break;
        case AGNODE: sprintf(buf,"node%lu",id); break;
        case AGINEDGE:
        case AGOUTEDGE: sprintf(buf,"edge%lu",id); break;
    }
    Tcl_DeleteCommand(ictx->interp, buf);
*/
    if (id % 2 == 0)
        agstrfree(gctx->g, (char *) id);
}
static char *myiddisc_print(void *state, int objtype, unsigned long id) {
    NOTUSED(state);
    NOTUSED(objtype);
    if (id % 2 == 0)
        return (char *) id;
    else
        return "";
}
static void myiddisc_close(void *state) {
    free(state);
}
static void myiddisc_idregister(void *state, int objtype, void *obj) {
    gctx_t *gctx = (gctx_t *)state;
    ictx_t *ictx = gctx->ictx;
    Tcl_Interp *interp = ictx->interp;
    Tcl_CmdProc *proc = NULL;

    switch (objtype) {
        case AGRAPH: proc=graphcmd; break;
        case AGNODE: proc=nodecmd; break;
        case AGINEDGE:
        case AGOUTEDGE: proc=edgecmd; break;
    }
#ifndef TCLOBJ
    Tcl_CreateCommand(interp, obj2cmd(obj), proc, (ClientData) gctx, (Tcl_CmdDeleteProc *) NULL);
#else
    Tcl_CreateObjCommand(interp, obj2cmd(obj), proc, (ClientData) gctx, (Tcl_CmdDeleteProc *) NULL);
#endif           
}
Agiddisc_t myiddisc = {
    myiddisc_open,
    myiddisc_map,
    myiddisc_alloc,
    myiddisc_free,
    myiddisc_print,
    myiddisc_close,
    myiddisc_idregister
};
#endif
