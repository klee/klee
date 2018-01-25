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

#ifndef RECTANGLE_H
#define RECTANGLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Rect {
    int boundary[NUMSIDES];
} Rect_t;

void InitRect(Rect_t * r);
#ifdef RTDEBUG
void PrintRect(Rect_t *);
#endif
unsigned int RectArea(Rect_t *);
int Overlap(Rect_t *, Rect_t *);
int Contained(Rect_t *, Rect_t *);
Rect_t CombineRect(Rect_t *, Rect_t *);
Rect_t NullRect(void);

#ifdef __cplusplus
}
#endif

#endif				/*RECTANGLE_H */
