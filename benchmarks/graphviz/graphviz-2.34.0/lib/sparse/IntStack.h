/* $Id$Revision: */
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

#ifndef IntStack_H
#define IntStack_H

/* last in first out integer stack */
struct IntStack_struct{
  int last;/* position of the last element, If empty, last = -1 */
  int max_len;
  int *stack;
};

typedef struct IntStack_struct* IntStack;

IntStack IntStack_new(void);

void IntStack_delete(IntStack s);

#define IntStack_get_length(s) (1+(s)->last)

int IntStack_push(IntStack s, int i);/* add an item and return the pos (>=0).
					Return negative value of malloc failed */

int IntStack_pop(IntStack s, int *flag);/* remove the last item. If none exist, flag = -1, and return -1. */

void IntStack_print(IntStack s);

#endif
