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

#ifndef XLABELS_H
#define XLABELS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <geom.h>

typedef struct {
    pointf sz;			/* Size of label (input) */
    pointf pos;			/* Position of lower-left corner of label (output) */
    void *lbl;			/* Pointer to label in the graph */
    unsigned char set;		/* True if the position has been set (input/output) */
} xlabel_t;

typedef struct {
    pointf pos;			/* Position of lower-left corner of object */
    pointf sz;			/* Size of object; may be zero for a point */
    xlabel_t *lbl;		/* Label attached to object, or NULL */
} object_t;

typedef struct {
    boxf bb;			/* Bounding box of all objects */
    unsigned char force;	/* If true, all labels must be placed */
} label_params_t;

int placeLabels(object_t * objs, int n_objs,
		xlabel_t * lbls, int n_lbls, label_params_t * params);

#ifdef XLABEL_INT
#include <index.h>
#include <logic.h>
#include <cdt.h>

#ifndef XLNDSCALE
#define XLNDSCALE 72.0
#endif /*XLNDSCALE*/
#ifndef XLNBR
#define XLNBR    9
#endif /*XLNBR*/
#ifndef XLXDENOM
#define XLXDENOM 8
#endif /*XLXDENOM*/
#ifndef XLYDENOM
#define XLYDENOM 2
#endif /*XLYDENOM*/
#define XLNBR    9
#define XLCNR    4
#define XLNDSCALE 72.0
#define XLODCR   -1
// indexes of neighbors in certain arrays
// the node of interest is usually in node 4
// 6 7 8
// 3 4 5
// 0 1 2
#define XLPXPY   0
#define XLCXPY   1
#define XLNXPY   2
#define XLPXCY   3
#define XLCXCY   4
#define XLNXCY   5
#define XLPXNY   6
#define XLCXNY   7
#define XLNXNY   8
    typedef struct best_p_s {
    int n;
    double area;
    pointf pos;
} BestPos_t;

typedef struct obyh {
    Dtlink_t link;
    int key;
    Leaf_t d;
} HDict_t;

typedef struct XLabels_s {
    object_t *objs;
    int n_objs;
    xlabel_t *lbls;
    int n_lbls;
    label_params_t *params;

    Dt_t *hdx;			// splay tree keyed with hilbert spatial codes
    RTree_t *spdx;		// rtree

} XLabels_t;

#endif				/* XLABEL_INT */

#ifdef __cplusplus
}
#endif

#endif				/*XLABELS_H */
