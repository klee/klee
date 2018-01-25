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

#ifndef _POINTSET_H
#define _POINTSET_H 1

#include <cdt.h>
#include <geom.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef Dict_t PointSet;
    typedef Dict_t PointMap;
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

	extern PointSet *newPS(void);
    extern void freePS(PointSet *);
    extern void insertPS(PointSet *, point);
    extern void addPS(PointSet *, int, int);
    extern int inPS(PointSet *, point);
    extern int isInPS(PointSet *, int, int);
    extern int sizeOf(PointSet *);
    extern point *pointsOf(PointSet *);

    extern PointMap *newPM(void);
    extern void clearPM(PointMap *);
    extern void freePM(PointMap *);
    extern int insertPM(PointMap *, int, int, int);
    extern int updatePM(PointMap * pm, int x, int y, int v);

#undef extern
#ifdef __cplusplus
}
#endif

#endif /* _POINTSET_H */
