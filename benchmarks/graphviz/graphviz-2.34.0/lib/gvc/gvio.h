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

#ifndef GVDEVICE_H
#define GVDEVICE_H

#include "gvcjob.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GVDLL
#define extern __declspec(dllexport)
#else
#define extern
#endif

/*visual studio*/
#ifdef WIN32_DLL
#ifndef GVC_EXPORTS
#define extern __declspec(dllimport)
#endif
#endif
/*end visual studio*/

    extern size_t gvwrite (GVJ_t * job, const char *s, size_t len);
    extern size_t gvfwrite (const void *ptr, size_t size, size_t nmemb, FILE *stream);
    extern int gvferror (FILE *stream);
    extern int gvputc(GVJ_t * job, int c);
    extern int gvputs(GVJ_t * job, const char *s);
    extern int gvflush (GVJ_t * job);
    extern void gvprintf(GVJ_t * job, const char *format, ...);
    extern void gvprintdouble(GVJ_t * job, double num); 
    extern void gvprintpointf(GVJ_t * job, pointf p);
    extern void gvprintpointflist(GVJ_t * job, pointf *p, int n);

#undef extern

#ifdef __cplusplus
}
#endif

#endif				/* GVDEVICE_H */
