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


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <generic_list.h>

#define DFLT_SIZE 100

generic_list_t *new_generic_list(unsigned long size)
{
    generic_list_t *list;

    list = (generic_list_t *) malloc(sizeof(generic_list_t));
    if (list == NULL) {
	perror("[new_generic_list()] Error allocating memory:");
	return NULL;
    }
    if (size != 0) {
	list->data = (gl_data *) malloc(size * sizeof(gl_data));
	if (list->data == NULL) {
	    perror("[new_generic_list()] Error allocating memory:");
	    return NULL;
	}
    } else
	list->data = NULL;
    list->size = size;
    list->used = 0;
    return list;
}

void free_generic_list(generic_list_t * list)
{
    if (list->size > 0) {
	free(list->data);
    }
    free(list);
}

generic_list_t *add_to_generic_list(generic_list_t * list, gl_data element)
{
    unsigned long new_size;
    gl_data *new_data;

    if (list->size == list->used) {
	if (list->size == 0) {
	    new_size = DFLT_SIZE;
	} else {
	    new_size = list->size * 2;
	}
	new_data =
	    (gl_data *) realloc(list->data, new_size * sizeof(gl_data));
	if (new_data == NULL) {
	    perror("[add_to_generic_list()] Error allocating memory:");
	    return NULL;
	}
	list->data = new_data;
	list->size = new_size;
    }
    list->data[list->used++] = element;
    return list;
}
