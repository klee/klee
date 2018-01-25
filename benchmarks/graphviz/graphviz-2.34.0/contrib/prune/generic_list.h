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

#ifdef __cplusplus
extern "C" {
#endif


#ifndef GENERIC_LIST_H
#define GENERIC_LIST_H

    typedef void *gl_data;

    typedef struct generic_list_s {
	unsigned long used;	/* number of elements in the list */
	unsigned long size;	/* number of elements that the list can hold */
	gl_data *data;		/* pointer to first element */
    } generic_list_t;

    extern generic_list_t *new_generic_list(unsigned long size);
    extern generic_list_t *add_to_generic_list(generic_list_t * list,
					       gl_data element);
    extern void free_generic_list(generic_list_t * list);

#endif				/* GENERIC_LIST_H */

#ifdef __cplusplus
}
#endif
