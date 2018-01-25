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

#ifndef _VMSTUB_H
#define _VMSTUB_H
#include <stdlib.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
typedef void Vmalloc_t;
#define vmalloc(heap,size) malloc(size)
#define vmopen(x,y,z) (Vmalloc_t*)(0)
#define vmclose(x) while (0)
#define vmresize(heap,ptr,size,oktomoveflag)  realloc((ptr),(size))
#define vmfree(heap,ptr) free(ptr)
#ifndef EXTERN
#define EXTERN extern
#endif
EXTERN void *Vmregion;
#endif
