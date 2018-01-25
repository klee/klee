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

#ifndef GV_MACROS_H
#define GV_MACROS_H

#ifndef NOTUSED
#define NOTUSED(var) (void) var
#endif

#ifndef NIL
#define NIL(type)  ((type)0)
#endif

#define isPinned(n)     (ND_pinned(n) == P_PIN)
#define hasPos(n)       (ND_pinned(n) > 0)
#define isFixed(n)      (ND_pinned(n) > P_SET)

#define SET_CLUST_NODE(n) (ND_clustnode(n) = TRUE)
#define IS_CLUST_NODE(n)  (ND_clustnode(n))
#define HAS_CLUST_EDGE(g) (GD_flags(g) & 1)
#define SET_CLUST_EDGE(g) (GD_flags(g) |= 1)
#define EDGE_TYPE(g) (GD_flags(g) & (7 << 1))

#ifndef streq
#define streq(a,b)		(*(a)==*(b)&&!strcmp(a,b))
#endif

#define XPAD(d) ((d).x += 4*GAP)
#define YPAD(d) ((d).y += 2*GAP)
#define PAD(d)  {XPAD(d); YPAD(d);}

#define OTHERDIR(dir) ((dir == CCW) ? CW : CCW)

#define NEXTSIDE(side, dir) ((dir == CCW) ? \
		((side & 0x8) ? BOTTOM : (side << 1)) : \
		((side & 0x1) ? LEFT : (side >> 1)))

#endif
