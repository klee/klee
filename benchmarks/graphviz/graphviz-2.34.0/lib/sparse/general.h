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

#ifndef GENERAL_H
#define GENERAL_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
/* Applications that do not use the common library can define STANDALONE
 * to get definitions/definitions that are normally provided there.
 * In particular, note that Verbose is declared but undefined.
 */
#ifndef STANDALONE
#include <cgraph.h>
#include <globals.h>
#include <logic.h>
#include <arith.h>
#include <memory.h>
#endif  /* STANDALONE */

#define real double

#define set_flag(a, flag) ((a)=((a)|(flag)))
#define test_flag(a, flag) ((a)&(flag))
#define clear_flag(a, flag) ((a) &=(~(flag)))

#ifdef STANDALONE
#define MALLOC malloc
#define REALLOC realloc

#define N_NEW(n,t)   (t*)malloc((n)*sizeof(t))
#define NEW(t)       (t*)malloc(sizeof(t))
#define MAX(a,b) ((a)>(b)?(a):b)
#define MIN(a,b) ((a)<(b)?(a):b)
#define ABS(a) (((a)>0)?(a):(-(a)))

#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

#define MAXINT 1<<30
#define PI 3.14159

#define POINTS(inch) 72*(inch)

typedef unsigned int boolean;
extern unsigned char Verbose;

#else  /* STANDALONE */
#define MALLOC gmalloc
#define REALLOC grealloc
#endif    /* STANDALONE */

#define FREE free
#define MEMCPY memcpy

#ifndef DEBUG
#define NDEBUG /* switch off assert*/
#endif

#ifdef DEBUG
extern double _statistics[10];
#endif


extern int irand(int n);
extern real drand(void);
extern int *random_permutation(int n);/* random permutation of 0 to n-1 */


real* vector_subtract_to(int n, real *x, real *y);/* y = x-y */
real* vector_subtract_from(int n, real *x, real *y);/* y = y-x */
real* vector_add_to(int n, real *x, real *y);

real vector_product(int n, real *x, real *y);

real* vector_saxpy(int n, real *x, real *y, real beta); /* y = x+beta*y */


real* vector_saxpy2(int n, real *x, real *y, real beta);/* x = x+beta*y */

/* take m elements v[p[i]]],i=1,...,m and oput in u. u will be assigned if *u = NULL */
void vector_take(int n, real *v, int m, int *p, real **u);
void vector_float_take(int n, float *v, int m, int *p, float **u);

/* give the position of the lagest, second largest etc in vector v if ascending = TRUE
   or
   give the position of the smallest, second smallest etc  in vector v if ascending = TRUE.
   results in p. If *p == NULL, p is asigned.
*/
void vector_ordering(int n, real *v, int **p, int ascending);
void vector_sort_real(int n, real *v, int ascending);
void vector_sort_int(int n, int *v, int ascending);
real vector_median(int n, real *x);
real vector_percentile(int n, real *x, real y);/* find the value such that y% of element of vector x is <= that value.*/

void vector_print(char *s, int n, real *x);

#define MACHINEACC 1.0e-16

int excute_system_command3(char *s1, char *s2, char *s3);
int excute_system_command(char *s1, char *s2);

#define MINDIST 1.e-15

enum {UNMATCHED = -1};


real distance(real *x, int dim, int i, int j);
real distance_cropped(real *x, int dim, int i, int j);

real point_distance(real *p1, real *p2, int dim);

char *strip_dir(char *s);

void scale_to_box(real xmin, real ymin, real xmax, real ymax, int n, int dim, real *x);

#endif



