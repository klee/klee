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


#include "common.h"
#include "g.h"
#include "gcommon.h"


char *Gpscanvasname = "out.ps";

int GPcreatewidget(Gwidget_t * parent, Gwidget_t * widget,
		   int attrn, Gwattr_t * attrp)
{

    return 0;
}


int GPsetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{

    return 0;
}


int GPgetwidgetattr(Gwidget_t * widget, int attrn, Gwattr_t * attrp)
{

    return 0;
}


int GPdestroywidget(Gwidget_t * widget)
{

    return 0;
}


int GPsetgfxattr(Gwidget_t * widget, Ggattr_t * ap)
{

    return 0;
}


int GParrow(Gwidget_t * widget, Gpoint_t gp1, Gpoint_t gp2, Ggattr_t * ap)
{

    return 0;
}


int GPline(Gwidget_t * widget, Gpoint_t gp1, Gpoint_t gp2, Ggattr_t * ap)
{

    return 0;
}


int GPbox(Gwidget_t * widget, Grect_t gr, Ggattr_t * ap)
{

    return 0;
}


int GPpolygon(Gwidget_t * widget, int gpn, Gpoint_t * gpp, Ggattr_t * ap)
{

    return 0;
}


int GPsplinegon(Gwidget_t * widget, int gpn, Gpoint_t * gpp, Ggattr_t * ap)
{

    return 0;
}


int GParc(Gwidget_t * widget, Gpoint_t gc, Gsize_t gs, double ang1,
	  double ang2, Ggattr_t * ap)
{

    return 0;
}


int GPtext(Gwidget_t * widget, Gtextline_t * tlp, int n, Gpoint_t go,
	   char *fn, double fs, char *justs, Ggattr_t * ap)
{

    return 0;
}


int GPgettextsize(Gwidget_t * widget, Gtextline_t * tlp, int n, char *fn,
		  double fs, Gsize_t * gsp)
{

    return 0;
}


int GPcreatebitmap(Gwidget_t * widget, Gbitmap_t * bitmap, Gsize_t s)
{

    return 0;
}


int GPdestroybitmap(Gbitmap_t * bitmap)
{

    return 0;
}


int GPreadbitmap(Gwidget_t * widget, Gbitmap_t * bitmap, FILE * fp)
{

    return 0;
}


int GPwritebitmap(Gbitmap_t * bitmap, FILE * fp)
{

    return 0;
}


int GPbitblt(Gwidget_t * widget, Gpoint_t gp, Grect_t gr,
	     Gbitmap_t * bitmap, char *mode, Ggattr_t * ap)
{

    return 0;
}


int GPcanvasclear(Gwidget_t * widget)
{

    return 0;
}


int GPgetgfxattr(Gwidget_t * widget, Ggattr_t * ap)
{

    return 0;
}
