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

/* Krishnam Raju Pericherla */

#include <gtk/gtk.h>
#include "common.h"
#include "g.h"
#include "gcommon.h"
#include "mem.h"

int Gxfd;
GtkWidget *Groot;
GdkDisplay *Gdisplay;
int Gpopdownflag;
int Gscreenn;
int Gdepth;
Glazyq_t Glazyq;

PIXpoint_t *Gppp;
int Gppn, Gppi;

char *Gbufp = NULL;
int Gbufn = 0, Gbufi = 0;

Gfont_t *Gfontp;
GdkFont *deffont;
int Gfontn;

int argn;

int Ginitgraphics(void)
{

    gtk_init(0, NULL);

    Gpopdownflag = FALSE;
    Gdisplay = gdk_display_get_default();
    deffont = gdk_font_load("fixed");

    Gpopdownflag = FALSE;
    Glazyq.flag = 0;
    Gbufp = Marrayalloc((long) BUFINCR * BUFSIZE);
    Gbufn = BUFINCR;
    Gppp = Marrayalloc((long) PPINCR * PPSIZE);
    Gppn = BUFINCR;
    Gfontp = Marrayalloc((long) FONTSIZE);
    Gfontn = 1;
    Gfontp[0].name = strdup("default");
    if (!Gdefaultfont)
	Gfontp[0].font = deffont;
    else if (Gdefaultfont[0] != '\000')
	Gfontp[0].font = gdk_font_load(Gdefaultfont);
    else
	Gfontp[0].font = NULL;
    return 0;
}

int Gtermgraphics(void)
{
    int fi;

    for (fi = 0; fi < Gfontn; fi++)
	free(Gfontp[fi].name);

    Marrayfree(Gfontp), Gfontp = NULL, Gfontn = 0;
    Marrayfree(Gppp), Gppp = NULL, Gppn = 0;
    Marrayfree(Gbufp), Gbufp = NULL, Gbufn = 0;

    return 0;
}

void Gflushlazyq(void)
{

}

void Glazymanage(GtkWidget w)
{

}

int Gsync(void)
{
    if (Glazyq.flag)
	Gflushlazyq();
    gdk_display_sync(Gdisplay);
    return 0;
}

int Gresetbstate(int wi)
{
    Gcw_t *cw;
    int bn;

    cw = Gwidgets[wi].u.c;
    bn = cw->bstate[0] + cw->bstate[1] + cw->bstate[2];
    cw->bstate[0] = cw->bstate[1] = cw->bstate[2] = 0;
    cw->buttonsdown -= bn;
    Gbuttonsdown -= bn;
    return 0;
}

int Gprocessevents(int waitflag, Geventmode_t mode)
{
    int rtn;

    if (Glazyq.flag)
	Gflushlazyq();
    rtn = 0;
    switch (waitflag) {
    case TRUE:
	gtk_main_iteration();
	if (mode == G_ONEEVENT)
	    return 1;
	rtn = 1;
    case FALSE:
	while (gtk_events_pending()) {
	    gtk_main_iteration();
	    if (mode == G_ONEEVENT)
		return 1;
	    rtn = 1;
	}
	break;
    }
    return rtn;
}
