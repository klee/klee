/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**
 * \brief remove overlaps between a set of rectangles.
 *
 * Authors:
 *   Tim Dwyer <tgdwyer@gmail.com>
 *
 * Copyright (C) 2005 Authors
 *
 * This version is released under the CPL (Common Public License) with
 * the Graphviz distribution.
 * A version is also available under the LGPL as part of the Adaptagrams
 * project: http://sourceforge.net/projects/adaptagrams.  
 * If you make improvements or bug fixes to this code it would be much
 * appreciated if you could also contribute those changes back to the
 * Adaptagrams repository.
 */
#ifndef REMOVE_RECTANGLE_OVERLAP_H_SEEN
#define REMOVE_RECTANGLE_OVERLAP_H_SEEN

class Rectangle;

void removeRectangleOverlap(Rectangle *rs[], int n, double xBorder, double yBorder);


#endif /* !REMOVE_RECTANGLE_OVERLAP_H_SEEN */
