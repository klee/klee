/* $Id$Revision: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#ifndef VECTOR_H
#define VECTOR_H
#include <stdlib.h>
struct vector_struct {
  int maxlen;
  int len;
  void *v;
  size_t size_of_elem;
  void (*deallocator)(void *v);
};

typedef struct vector_struct *Vector;

/* deallocator works on each element of the vector */
Vector Vector_new(int maxlen, size_t size_of_elem, void (*deallocator)(void *v));

Vector Vector_add(Vector v, void *stuff);

Vector Vector_reset(Vector v, void *stuff, int i);

void Vector_delete(Vector v);

void* Vector_get(Vector v, int i);

int Vector_get_length(Vector v);

Vector Vector_reset(Vector v, void *stuff, int i);

/*------------- vector of strings ----------- */

typedef Vector StringVector;

Vector StringVector_new(int len, int delete_element_strings);
Vector StringVector_add(Vector v, char *i);
void StringVector_delete(Vector v);
char** StringVector_get(Vector v, int i);
int StringVector_get_length(Vector v);
Vector StringVector_reset(Vector v, char *content, int pos);
void StringVector_fprint(FILE *fp, StringVector v);
void StringVector_fprint1(FILE *fp, StringVector v);
StringVector StringVector_part(StringVector v, int n, int *selected_list);
/*------------- integer vector ----------- */

typedef Vector IntegerVector;

Vector IntegerVector_new(int len);
Vector IntegerVector_add(Vector v, int i);
void IntegerVector_delete(Vector v);
int* IntegerVector_get(Vector v, int i);
int IntegerVector_get_length(Vector v);
Vector IntegerVector_reset(Vector v, int content, int pos);



#endif
