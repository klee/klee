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

int graphcmd(ClientData clientData, Tcl_Interp * interp,
#ifndef TCLOBJ
		    int argc, char *argv[]
#else
		    int argc, Tcl_Obj * CONST objv[]
#endif
    )
{

    Agraph_t *g, *sg;
    Agnode_t *n, *tail, *head;
    Agedge_t *e;
#ifndef WITH_CGRAPH
    Agraph_t **gp, **sgp;
    Agnode_t **np;
    Agedge_t **ep;
    ictx_t *ictx = (ictx_t *)clientData;
    unsigned long id;
#else
    gctx_t *gctx = (gctx_t *)clientData;
    ictx_t *ictx = gctx->ictx;
#endif
    Agsym_t *a;
    char c, buf[256], **argv2;
    int i, j, length, argc2, rc;
    GVC_t *gvc = ictx->gvc;
    GVJ_t *job = gvc->job;

    if (argc < 2) {
	Tcl_AppendResult(interp, "Wrong # args: should be \"", argv[0], " option ?arg arg ...?\"", NULL);
	return TCL_ERROR;
    }
#ifndef WITH_CGRAPH
    if (!(gp = (Agraph_t **) tclhandleXlate(ictx->graphTblPtr, argv[0]))) {
	Tcl_AppendResult(interp, "Graph \"", argv[0], "\" not found", NULL);
	return TCL_ERROR;
    }
    g = *gp;
#else
    g = cmd2g(argv[0]);
    if (!g) {
	Tcl_AppendResult(interp, "Graph \"", argv[0], "\" not found", NULL);
	return TCL_ERROR;
    }
#endif

    c = argv[1][0];
    length = strlen(argv[1]);

    if ((c == 'a') && (strncmp(argv[1], "addedge", length) == 0)) {
	if ((argc < 4) || (argc % 2)) {
	    Tcl_AppendResult(interp, "Wrong # args: should be \"", argv[0],
			     " addedge tail head ?attributename attributevalue? ?...?\"",
			     NULL);
	    return TCL_ERROR;
	}
#ifndef WITH_CGRAPH
	if (!(np = (Agnode_t **) tclhandleXlate(ictx->nodeTblPtr, argv[2]))) {
	    if (!(tail = agfindnode(g, argv[2]))) {
		Tcl_AppendResult(interp, "Tail node \"", argv[2], "\" not found.", NULL);
		return TCL_ERROR;
	    }
        }
	else {
	    tail = *np;
	}
#else
        tail = cmd2n(argv[2]);
        if (!tail) {
	    if (!(tail = agfindnode(g, argv[2]))) {
		Tcl_AppendResult(interp, "Tail node \"", argv[2], "\" not found.", NULL);
		return TCL_ERROR;
	    }
        }
#endif
	if (agroot(g) != agroot(agraphof(tail))) {
	    Tcl_AppendResult(interp, "Tail node ", argv[2], " is not in the graph.", NULL);
	    return TCL_ERROR;
	}
#ifndef WITH_CGRAPH
	if (!(np = (Agnode_t **) tclhandleXlate(ictx->nodeTblPtr, argv[3]))) {
	    if (!(head = agfindnode(g, argv[3]))) {
		Tcl_AppendResult(interp, "Head node \"", argv[3], "\" not found.", NULL);
		return TCL_ERROR;
	    }
	}
	else {
	    head = *np;
	}
#else
        head = cmd2n(argv[2]);
        if (!head) {
	    if (!(head = agfindnode(g, argv[3]))) {
		Tcl_AppendResult(interp, "Head node \"", argv[3], "\" not found.", NULL);
		return TCL_ERROR;
	    }
        }
#endif
	if (agroot(g) != agroot(agraphof(head))) {
	    Tcl_AppendResult(interp, "Head node ", argv[3], " is not in the graph.", NULL);
	    return TCL_ERROR;
	}
#ifndef WITH_CGRAPH
	e = agedge(g, tail, head);
#else
	e = agedge(g, tail, head, NULL, 1);
	Tcl_AppendResult(interp, obj2cmd(e), NULL);
#endif
#ifndef WITH_CGRAPH
	if (!(ep = (Agedge_t **) tclhandleXlateIndex(ictx->edgeTblPtr, AGID(e))) || *ep != e) {
	    ep = (Agedge_t **) tclhandleAlloc(ictx->edgeTblPtr, Tcl_GetStringResult(interp), &id);
	    *ep = e;
	    AGID(e) = id;
#ifndef TCLOBJ
	    Tcl_CreateCommand(interp, Tcl_GetStringResult(interp), edgecmd,
			      (ClientData) ictx, (Tcl_CmdDeleteProc *) NULL);
#else				/* TCLOBJ */
	    Tcl_CreateObjCommand(interp, Tcl_GetStringResult(interp), edgecmd,
				 (ClientData) ictx, (Tcl_CmdDeleteProc *) NULL);
#endif				/* TCLOBJ */
	} else {
	    tclhandleString(ictx->edgeTblPtr, Tcl_GetStringResult(interp), AGID(e));
	}
#endif
	setedgeattributes(agroot(g), e, &argv[4], argc - 4);
	reset_layout(gvc, g);
	return TCL_OK;

    } else if ((c == 'a') && (strncmp(argv[1], "addnode", length) == 0)) {
	if (argc % 2) {
	    /* if odd number of args then argv[2] is name */
#ifdef WITH_CGRAPH
	    n = agnode(g, argv[2], 1);
#else
	    n = agnode(g, argv[2]);
	    if (!(np = (Agnode_t **) tclhandleXlateIndex(ictx->nodeTblPtr, AGID(n))) || *np != n) {
		np = (Agnode_t **) tclhandleAlloc(ictx->nodeTblPtr, Tcl_GetStringResult(interp), &id);
		*np = n;
		AGID(n) = id;
#ifndef TCLOBJ
		Tcl_CreateCommand(interp, Tcl_GetStringResult(interp), nodecmd,
				  (ClientData) ictx, (Tcl_CmdDeleteProc *) NULL);
#else /* TCLOBJ */
		Tcl_CreateObjCommand(interp, Tcl_GetStringResult(interp), nodecmd,
				     (ClientData) ictx, (Tcl_CmdDeleteProc *) NULL);
#endif /* TCLOBJ */
	    } else {
		tclhandleString(ictx->nodeTblPtr, Tcl_GetStringResult(interp), AGID(n));
	    }
#endif
	    i = 3;
	} else {
#ifdef WITH_CGRAPH
	    n = agnode(g, NULL, 1);  /* anon node */
#else
	    /* else use handle as name */
	    np = (Agnode_t **) tclhandleAlloc(ictx->nodeTblPtr, Tcl_GetStringResult(interp), &id);
	    n = agnode(g, Tcl_GetStringResult(interp));
	    *np = n;
	    AGID(n) = id;
#ifndef TCLOBJ
	    Tcl_CreateCommand(interp, Tcl_GetStringResult(interp), nodecmd,
			      (ClientData) ictx, (Tcl_CmdDeleteProc *) NULL);
#else				/* TCLOBJ */
	    Tcl_CreateObjCommand(interp, Tcl_GetStringResult(interp), nodecmd,
				 (ClientData) ictx, (Tcl_CmdDeleteProc *) NULL);
#endif				/* TCLOBJ */
#endif
	    i = 2;
	}
#ifndef WITH_CGRAPH
	np = (Agnode_t **)tclhandleXlateIndex(ictx->nodeTblPtr, AGID(n));
    	*np = n;
#else
	Tcl_AppendResult(interp, obj2cmd(n), NULL);
#endif
	setnodeattributes(agroot(g), n, &argv[i], argc - i);
	reset_layout(gvc, g);
	return TCL_OK;

    } else if ((c == 'a')
	       && (strncmp(argv[1], "addsubgraph", length) == 0)) {
	if (argc < 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     "\" addsubgraph ?name? ?attributename attributevalue? ?...?",
			     NULL);
	}
	if (argc % 2) {
	    /* if odd number of args then argv[2] is name */
#ifdef WITH_CGRAPH
	    sg = agsubg(g, argv[2], 1);
	    Tcl_AppendResult(interp, obj2cmd(sg), NULL);
#else
	    sg = agsubg(g, argv[2]);
	    if (!  (sgp = (Agraph_t **) tclhandleXlateIndex(ictx->graphTblPtr, AGID(sg))) || *sgp != sg) {
		sgp = (Agraph_t **) tclhandleAlloc(ictx->graphTblPtr, Tcl_GetStringResult(interp), &id);
		*sgp = sg;
		AGID(sg) = id;
#ifndef TCLOBJ
		Tcl_CreateCommand(interp, Tcl_GetStringResult(interp), graphcmd,
				  (ClientData) ictx, (Tcl_CmdDeleteProc *) NULL);
#else
		Tcl_CreateObjCommand(interp, Tcl_GetStringResult(interp), graphcmd,
				     (ClientData) ictx, (Tcl_CmdDeleteProc *) NULL);
#endif
	    } else {
		tclhandleString(ictx->graphTblPtr, Tcl_GetStringResult(interp), AGID(sg));
	    }
#endif
	    i = 3;
	} else {
#ifdef WITH_CGRAPH
	    sg = agsubg(g, NULL, 1);  /* anon subgraph */
#else
	    /* else use handle as name */
	    sgp = (Agraph_t **) tclhandleAlloc(ictx->graphTblPtr, Tcl_GetStringResult(interp), &id);
	    sg = agsubg(g, Tcl_GetStringResult(interp));
	    *sgp = sg;
	    AGID(sg) = id;
#ifndef TCLOBJ
	    Tcl_CreateCommand(interp, Tcl_GetStringResult(interp), graphcmd,
			      (ClientData) ictx, (Tcl_CmdDeleteProc *) NULL);
#else
	    Tcl_CreateObjCommand(interp, Tcl_GetStringResult(interp), graphcmd,
				 (ClientData) ictx, (Tcl_CmdDeleteProc *) NULL);
#endif
#endif
	    i = 2;
	}
#ifndef WITH_CGRAPH
	sgp = (Agraph_t **)tclhandleXlateIndex(ictx->graphTblPtr, AGID(sg));
    	*sgp = sg;
#endif
	setgraphattributes(sg, &argv[i], argc - i);
	reset_layout(gvc, g);
	return TCL_OK;

    } else if ((c == 'c') && (strncmp(argv[1], "countnodes", length) == 0)) {
	sprintf(buf, "%d", agnnodes(g));
	Tcl_AppendResult(interp, buf, NULL);
	return TCL_OK;

    } else if ((c == 'c') && (strncmp(argv[1], "countedges", length) == 0)) {
	sprintf(buf, "%d", agnedges(g));
	Tcl_AppendResult(interp, buf, NULL);
	return TCL_OK;

    } else if ((c == 'd') && (strncmp(argv[1], "delete", length) == 0)) {
	reset_layout(gvc, g);
#ifndef WITH_CGRAPH
	deleteGraph(ictx, g);
#else
	deleteGraph(gctx, g);
#endif
	return TCL_OK;

    } else if ((c == 'f') && (strncmp(argv[1], "findedge", length) == 0)) {
	if (argc < 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
			     argv[0], " findedge tailnodename headnodename\"", NULL);
	    return TCL_ERROR;
	}
	if (!(tail = agfindnode(g, argv[2]))) {
	    Tcl_AppendResult(interp, "Tail node \"", argv[2], "\" not found.", NULL);
	    return TCL_ERROR;
	}
	if (!(head = agfindnode(g, argv[3]))) {
	    Tcl_AppendResult(interp, "Head node \"", argv[3], "\" not found.", NULL);
	    return TCL_ERROR;
	}
	if (!(e = agfindedge(g, tail, head))) {
	    Tcl_AppendResult(interp, "Edge \"", argv[2], " - ", argv[3], "\" not found.", NULL);
	    return TCL_ERROR;
	}
#ifndef WITH_CGRAPH
	tclhandleString(ictx->edgeTblPtr, buf, AGID(e));
	Tcl_AppendElement(interp, buf);
#else
	Tcl_AppendElement(interp, obj2cmd(e));
#endif
	return TCL_OK;

    } else if ((c == 'f') && (strncmp(argv[1], "findnode", length) == 0)) {
	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], " findnode nodename\"", NULL);
	    return TCL_ERROR;
	}
	if (!(n = agfindnode(g, argv[2]))) {
	    Tcl_AppendResult(interp, "Node not found.", NULL);
	    return TCL_ERROR;
	}
#ifndef WITH_CGRAPH
	tclhandleString(ictx->nodeTblPtr, buf, AGID(n));
	Tcl_AppendResult(interp, buf, NULL);
#else
	Tcl_AppendResult(interp, obj2cmd(n), NULL);
#endif
	return TCL_OK;

    } else if ((c == 'l')
	       && (strncmp(argv[1], "layoutedges", length) == 0)) {
	g = agroot(g);
#ifndef WITH_CGRAPH
	if (!GD_drawing(g))
#else
	if (!aggetrec (g, "Agraphinfo_t",0))
#endif
	    tcldot_layout(gvc, g, (argc > 2) ? argv[2] : NULL);
	return TCL_OK;

    } else if ((c == 'l')
	       && (strncmp(argv[1], "layoutnodes", length) == 0)) {
	g = agroot(g);
#ifndef WITH_CGRAPH
	if (!GD_drawing(g))
#else
	if (!aggetrec (g, "Agraphinfo_t",0))
#endif
	    tcldot_layout(gvc, g, (argc > 2) ? argv[2] : NULL);
	return TCL_OK;

    } else if ((c == 'l')
	       && (strncmp(argv[1], "listattributes", length) == 0)) {
	listGraphAttrs(interp, g);
	return TCL_OK;

    } else if ((c == 'l')
	       && (strncmp(argv[1], "listedgeattributes", length) == 0)) {
	listEdgeAttrs (interp, g);
	return TCL_OK;

    } else if ((c == 'l')
	       && (strncmp(argv[1], "listnodeattributes", length) == 0)) {
	listNodeAttrs (interp, g);
	return TCL_OK;

    } else if ((c == 'l') && (strncmp(argv[1], "listedges", length) == 0)) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
#ifndef WITH_CGRAPH
		tclhandleString(ictx->edgeTblPtr, buf, AGID(e));
		Tcl_AppendElement(interp, buf);
#else
		Tcl_AppendElement(interp, obj2cmd(e));
#endif
	    }
	}
	return TCL_OK;

    } else if ((c == 'l') && (strncmp(argv[1], "listnodes", length) == 0)) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
#ifndef WITH_CGRAPH
	    tclhandleString(ictx->nodeTblPtr, buf, AGID(n));
	    Tcl_AppendElement(interp, buf);
#else
	    Tcl_AppendElement(interp, obj2cmd(n));
#endif
	    
	}
	return TCL_OK;

    } else if ((c == 'l')
	       && (strncmp(argv[1], "listnodesrev", length) == 0)) {
	for (n = aglstnode(g); n; n = agprvnode(g, n)) {
#ifndef WITH_CGRAPH
	    tclhandleString(ictx->nodeTblPtr, buf, AGID(n));
	    Tcl_AppendElement(interp, buf);
#else
	    Tcl_AppendElement(interp, obj2cmd(n));
#endif
	}
	return TCL_OK;

    } else if ((c == 'l')
	       && (strncmp(argv[1], "listsubgraphs", length) == 0)) {
#ifdef WITH_CGRAPH
	for (sg = agfstsubg(g); sg; sg = agnxtsubg(sg)) {
	    Tcl_AppendElement(interp, obj2cmd(g));
	}
#else
	if (g->meta_node) {
	    for (e = agfstout(g->meta_node->graph, g->meta_node); e;
		 e = agnxtout(g->meta_node->graph, e)) {
		sg = agusergraph(aghead(e));
		tclhandleString(ictx->graphTblPtr, buf, AGID(sg));
		Tcl_AppendElement(interp, buf);
	    }
	}
#endif
	return TCL_OK;

    } else if ((c == 'q')
	       && (strncmp(argv[1], "queryattributes", length) == 0)) {
	for (i = 2; i < argc; i++) {
	    if (Tcl_SplitList
		(interp, argv[i], &argc2,
		 (CONST84 char ***) &argv2) != TCL_OK)
		return TCL_ERROR;
	    for (j = 0; j < argc2; j++) {
		if ((a = agfindgraphattr(g, argv2[j]))) {
#ifndef WITH_CGRAPH
		    Tcl_AppendElement(interp, agxget(g, a->index));
#else
		    Tcl_AppendElement(interp, agxget(g, a));
#endif
		} else {
		    Tcl_AppendResult(interp, " No attribute named \"", argv2[j], "\"", NULL);
		    return TCL_ERROR;
		}
	    }
	    Tcl_Free((char *) argv2);
	}
	return TCL_OK;

    } else if ((c == 'q')
	       && (strncmp(argv[1], "queryattributevalues", length) ==
		   0)) {
	for (i = 2; i < argc; i++) {
	    if (Tcl_SplitList
		(interp, argv[i], &argc2,
		 (CONST84 char ***) &argv2) != TCL_OK)
		return TCL_ERROR;
	    for (j = 0; j < argc2; j++) {
		if ((a = agfindgraphattr(g, argv2[j]))) {
		    Tcl_AppendElement(interp, argv2[j]);
#ifndef WITH_CGRAPH
		    Tcl_AppendElement(interp, agxget(g, a->index));
#else
		    Tcl_AppendElement(interp, agxget(g, a));
#endif
		} else {
		    Tcl_AppendResult(interp, " No attribute named \"", argv2[j], "\"", NULL);
		    return TCL_ERROR;
		}
	    }
	    Tcl_Free((char *) argv2);
	}
	return TCL_OK;

    } else if ((c == 'q')
	       && (strncmp(argv[1], "queryedgeattributes", length) == 0)) {
	for (i = 2; i < argc; i++) {
	    if (Tcl_SplitList
		(interp, argv[i], &argc2,
		 (CONST84 char ***) &argv2) != TCL_OK)
		return TCL_ERROR;
	    for (j = 0; j < argc2; j++) {
		if ((a = agfindedgeattr(g, argv2[j]))) {
#ifndef WITH_CGRAPH
		    Tcl_AppendElement(interp, agxget(g->proto->e, a->index));
#else
		    Tcl_AppendElement(interp, agxget(g, a));
#endif
		} else {
		    Tcl_AppendResult(interp, " No attribute named \"", argv2[j], "\"", NULL);
		    return TCL_ERROR;
		}
	    }
	    Tcl_Free((char *) argv2);
	}
	return TCL_OK;

    } else if ((c == 'q')
	       && (strncmp(argv[1], "queryedgeattributevalues", length) == 0)) {
	for (i = 2; i < argc; i++) {
	    if (Tcl_SplitList
		(interp, argv[i], &argc2,
		 (CONST84 char ***) &argv2) != TCL_OK)
		return TCL_ERROR;
	    for (j = 0; j < argc2; j++) {
		if ((a = agfindedgeattr(g, argv2[j]))) {
		    Tcl_AppendElement(interp, argv2[j]);
#ifndef WITH_CGRAPH
		    Tcl_AppendElement(interp, agxget(g->proto->e, a->index));
#else
		    Tcl_AppendElement(interp, agxget(g, a));
#endif
		} else {
		    Tcl_AppendResult(interp, " No attribute named \"",
				     argv2[j], "\"", NULL);
		    return TCL_ERROR;
		}
	    }
	    Tcl_Free((char *) argv2);
	}
	return TCL_OK;

    } else if ((c == 'q')
	       && (strncmp(argv[1], "querynodeattributes", length) == 0)) {
	for (i = 2; i < argc; i++) {
	    if (Tcl_SplitList
		(interp, argv[i], &argc2,
		 (CONST84 char ***) &argv2) != TCL_OK)
		return TCL_ERROR;
	    for (j = 0; j < argc2; j++) {
		if ((a = agfindnodeattr(g, argv2[j]))) {
#ifndef WITH_CGRAPH
		    Tcl_AppendElement(interp, agxget(g->proto->n, a->index));
#else
		    Tcl_AppendElement(interp, agxget(g, a));
#endif
		} else {
		    Tcl_AppendResult(interp, " No attribute named \"",
				     argv2[j], "\"", NULL);
		    return TCL_ERROR;
		}
	    }
	    Tcl_Free((char *) argv2);
	}
	return TCL_OK;

    } else if ((c == 'q')
	       && (strncmp(argv[1], "querynodeattributevalues", length) ==
		   0)) {
	for (i = 2; i < argc; i++) {
	    if (Tcl_SplitList
		(interp, argv[i], &argc2,
		 (CONST84 char ***) &argv2) != TCL_OK)
		return TCL_ERROR;
	    for (j = 0; j < argc2; j++) {
		if ((a = agfindnodeattr(g, argv2[j]))) {
		    Tcl_AppendElement(interp, argv2[j]);
#ifndef WITH_CGRAPH
		    Tcl_AppendElement(interp, agxget(g->proto->n, a->index));
#else
		    Tcl_AppendElement(interp, agxget(g, a));
#endif
		} else {
		    Tcl_AppendResult(interp, " No attribute named \"", argv2[j], "\"", NULL);
		    return TCL_ERROR;
		}
	    }
	    Tcl_Free((char *) argv2);
	}
	return TCL_OK;

    } else if ((c == 'r') && (strncmp(argv[1], "render", length) == 0)) {
	char *canvas;

	if (argc < 3) {
	    canvas = "$c";
	} else {
	    canvas = argv[2];
#if 0				/* not implemented */
	    if (argc < 4) {
		tkgendata.eval = FALSE;
	    } else {
		if ((Tcl_GetBoolean(interp, argv[3], &tkgendata.eval)) !=
		    TCL_OK) {
		    Tcl_AppendResult(interp, " Invalid boolean: \"",
				     argv[3], "\"", NULL);
		    return TCL_ERROR;
		}
	    }
#endif
	}
        rc = gvjobs_output_langname(gvc, "tk");
	if (rc == NO_SUPPORT) {
	    Tcl_AppendResult(interp, " Format: \"tk\" not recognized.\n", NULL);
	    return TCL_ERROR;
	}

        gvc->write_fn = Tcldot_string_writer;
	job = gvc->job;
	job->imagedata = canvas;
	job->context = (void *)interp;
	job->external_context = TRUE;
	job->output_file = stdout;

	/* make sure that layout is done */
	g = agroot(g);
#ifndef WITH_CGRAPH
	if (!GD_drawing(g) || argc > 3)
#else
	if (!aggetrec (g, "Agraphinfo_t",0) || argc > 3)
#endif
	    tcldot_layout (gvc, g, (argc > 3) ? argv[3] : NULL);

	/* render graph TK canvas commands */
	gvc->common.viewNum = 0;
	gvRenderJobs(gvc, g);
	gvrender_end_job(job);
	gvdevice_finalize(job);
	fflush(job->output_file);
	gvjobs_delete(gvc);
	return TCL_OK;

#if HAVE_LIBGD
    } else if ((c == 'r') && (strncmp(argv[1], "rendergd", length) == 0)) {
	void **hdl;

	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " rendergd gdhandle ?DOT|NEATO|TWOPI|FDP|CIRCO?\"", NULL);
	    return TCL_ERROR;
	}
	rc = gvjobs_output_langname(gvc, "gd:gd:gd");
	if (rc == NO_SUPPORT) {
	    Tcl_AppendResult(interp, " Format: \"gd\" not recognized.\n", NULL);
	    return TCL_ERROR;
	}
        job = gvc->job;

	if (!  (hdl = tclhandleXlate(GDHandleTable, argv[2]))) {
	    Tcl_AppendResult(interp, "GD Image not found.", NULL);
	    return TCL_ERROR;
	}
	job->context = *hdl;
	job->external_context = TRUE;

	/* make sure that layout is done */
	g = agroot(g);
#ifndef WITH_CGRAPH
	if (!GD_drawing(g) || argc > 4)
#else
	if (!aggetrec (g, "Agraphinfo_t",0) || argc > 4)
#endif
	    tcldot_layout(gvc, g, (argc > 4) ? argv[4] : NULL);
	
	gvc->common.viewNum = 0;
	gvRenderJobs(gvc, g);
	gvrender_end_job(job);
	gvdevice_finalize(job);
	fflush(job->output_file);
	gvjobs_delete(gvc);
	Tcl_AppendResult(interp, argv[2], NULL);
	return TCL_OK;
#endif

    } else if ((c == 's')
	       && (strncmp(argv[1], "setattributes", length) == 0)) {
	if (argc == 3) {
	    if (Tcl_SplitList
		(interp, argv[2], &argc2,
		 (CONST84 char ***) &argv2) != TCL_OK)
		return TCL_ERROR;
	    if ((argc2 == 0) || (argc2 % 2)) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
				 "\" setattributes attributename attributevalue ?attributename attributevalue? ?...?",
				 NULL);
		Tcl_Free((char *) argv2);
		return TCL_ERROR;
	    }
	    setgraphattributes(g, argv2, argc2);
	    Tcl_Free((char *) argv2);
	    reset_layout(gvc, g);
	}
	if (argc == 4 && strcmp(argv[2], "viewport") == 0) {
	    /* special case to allow viewport to be set without resetting layout */
	    setgraphattributes(g, &argv[2], argc - 2);
	} else {
	    if ((argc < 4) || (argc % 2)) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
				 "\" setattributes attributename attributevalue ?attributename attributevalue? ?...?",
				 NULL);
		return TCL_ERROR;
	    }
	    setgraphattributes(g, &argv[2], argc - 2);
	    reset_layout(gvc, g);
	}
	return TCL_OK;

    } else if ((c == 's')
	       && (strncmp(argv[1], "setedgeattributes", length) == 0)) {
	if (argc == 3) {
	    if (Tcl_SplitList
		(interp, argv[2], &argc2,
		 (CONST84 char ***) &argv2) != TCL_OK)
		return TCL_ERROR;
	    if ((argc2 == 0) || (argc2 % 2)) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
				 "\" setedgeattributes attributename attributevalue ?attributename attributevalue? ?...?",
				 NULL);
		Tcl_Free((char *) argv2);
		return TCL_ERROR;
	    }
#ifndef WITH_CGRAPH
	    setedgeattributes(g, g->proto->e, argv2, argc2);
#else
	    setedgeattributes(g, NULL, argv2, argc2);
#endif
	    Tcl_Free((char *) argv2);
	} else {
	    if ((argc < 4) || (argc % 2)) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
				 "\" setedgeattributes attributename attributevalue ?attributename attributevalue? ?...?",
				 NULL);
	    }
#ifndef WITH_CGRAPH
	    setedgeattributes(g, g->proto->e, &argv[2], argc - 2);
#else
	    setedgeattributes(g, NULL, &argv[2], argc - 2);
#endif
	}
	reset_layout(gvc, g);
	return TCL_OK;

    } else if ((c == 's')
	       && (strncmp(argv[1], "setnodeattributes", length) == 0)) {
	if (argc == 3) {
	    if (Tcl_SplitList
		(interp, argv[2], &argc2,
		 (CONST84 char ***) &argv2) != TCL_OK)
		return TCL_ERROR;
	    if ((argc2 == 0) || (argc2 % 2)) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
				 "\" setnodeattributes attributename attributevalue ?attributename attributevalue? ?...?",
				 NULL);
		Tcl_Free((char *) argv2);
		return TCL_ERROR;
	    }
#ifndef WITH_CGRAPH
	    setnodeattributes(g, g->proto->n, argv2, argc2);
#else
	    setnodeattributes(g, NULL, argv2, argc2);
#endif
	    Tcl_Free((char *) argv2);
	} else {
	    if ((argc < 4) || (argc % 2)) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
				 "\" setnodeattributes attributename attributevalue ?attributename attributevalue? ?...?",
				 NULL);
	    }
#ifndef WITH_CGRAPH
	    setnodeattributes(g, g->proto->n, &argv[2], argc - 2);
#else
	    setnodeattributes(g, NULL, &argv[2], argc - 2);
#endif
	}
	reset_layout(gvc, g);
	return TCL_OK;

    } else if ((c == 's') && (strncmp(argv[1], "showname", length) == 0)) {
	Tcl_SetResult(interp, agnameof(g), TCL_STATIC);
	return TCL_OK;

    } else if ((c == 'w') && (strncmp(argv[1], "write", length) == 0)) {
	g = agroot(g);
	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
	      " write fileHandle ?language ?DOT|NEATO|TWOPI|FDP|CIRCO|NOP??\"",
	      NULL);
	    return TCL_ERROR;
	}

	/* process lang first to create job */
	if (argc < 4) {
	    i = gvjobs_output_langname(gvc, "dot");
	} else {
	    i = gvjobs_output_langname(gvc, argv[3]);
	}
	if (i == NO_SUPPORT) {
	    const char *s = gvplugin_list(gvc, API_render, argv[3]);
	    Tcl_AppendResult(interp, "Bad langname: \"", argv[3], "\". Use one of:", s, NULL);
	    return TCL_ERROR;
	}

	gvc->write_fn = Tcldot_channel_writer;
	job = gvc->job;

	/* populate new job struct with output language and output file data */
	job->output_lang = gvrender_select(job, job->output_langname);

//	if (Tcl_GetOpenFile (interp, argv[2], 1, 1, &outfp) != TCL_OK)
//	    return TCL_ERROR;
//	job->output_file = (FILE *)outfp;
	
	{
	    Tcl_Channel chan;
	    int mode;

	    chan = Tcl_GetChannel(interp, argv[2], &mode);

	    if (!chan) {
	        Tcl_AppendResult(interp, "Channel not open: \"", argv[2], NULL);
	        return TCL_ERROR;
	    }
	    if (!(mode & TCL_WRITABLE)) {
	        Tcl_AppendResult(interp, "Channel not writable: \"", argv[2], NULL);
	        return TCL_ERROR;
	    }
	    job->output_file = (FILE *)chan;
	}
	job->output_filename = NULL;

	/* make sure that layout is done  - unless canonical output */
#ifndef WITH_CGRAPH
	if ((!GD_drawing(g) || argc > 4) && !(job->flags & LAYOUT_NOT_REQUIRED))
#else
	if ((!aggetrec (g, "Agraphinfo_t",0) || argc > 4) && !(job->flags & LAYOUT_NOT_REQUIRED))
#endif
	    tcldot_layout(gvc, g, (argc > 4) ? argv[4] : NULL);

	gvc->common.viewNum = 0;
	gvRenderJobs(gvc, g);
	gvdevice_finalize(job);
//	fflush(job->output_file);
	gvjobs_delete(gvc);
	return TCL_OK;

    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
	 "\": must be one of:",
	 "\n\taddedge, addnode, addsubgraph, countedges, countnodes,",
	 "\n\tlayout, listattributes, listedgeattributes, listnodeattributes,",
	 "\n\tlistedges, listnodes, listsubgraphs, render, rendergd,",
	 "\n\tqueryattributes, queryedgeattributes, querynodeattributes,",
	 "\n\tqueryattributevalues, queryedgeattributevalues, querynodeattributevalues,",
	 "\n\tsetattributes, setedgeattributes, setnodeattributes,",
	 "\n\tshowname, write.", NULL);
	return TCL_ERROR;
    }
}				/* graphcmd */
