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

/* Lefteris Koutsofios - AT&T Labs Research */

#ifndef _CODE_H
#define _CODE_H
#define C_NULL -1

#define C_ISSTMT(ct) (ct >= C_STMT && ct <= C_RETURN)

#define C_CODE      0
#define C_ASSIGN    1
#define C_INTEGER   2
#define C_REAL      3
#define C_STRING    4
#define C_OR        5
#define C_AND       6
#define C_EQ        7
#define C_NE        8
#define C_LT        9
#define C_LE       10
#define C_GT       11
#define C_GE       12
#define C_PLUS     13
#define C_MINUS    14
#define C_MUL      15
#define C_DIV      16
#define C_MOD      17
#define C_UMINUS   18
#define C_NOT      19
#define C_PEXPR    20
#define C_FCALL    21
#define C_GVAR     22
#define C_LVAR     23
#define C_PVAR     24
#define C_FUNCTION 25
#define C_TCONS    26
#define C_DECL     27
#define C_STMT     28
#define C_IF       29
#define C_WHILE    30
#define C_FOR      31
#define C_FORIN    32
#define C_BREAK    33
#define C_CONTINUE 34
#define C_RETURN   35
#define C_INTERNAL 36
#define C_ARGS     37
#define C_NOP      38
#define C_SIZE     39

typedef struct Code_t {
    int ctype;
    int next;
    union {
        char s[1];
        double d;
        long i;
        int fp;
        void *o;
    } u;
} Code_t;
#define C_CODESIZE sizeof (Code_t)

#define Cgetstring(i) (char *) &cbufp[i].u.s[0]
#define Cgetindex() cbufi

#define Csettype(a, b) cbufp[a].ctype = b
#define Csetfp(a, b) cbufp[a].u.fp = b
#define Csetnext(a, b) cbufp[a].next = b
#define Csetinteger(a, b) cbufp[a].u.i = b
#define Csetobject(a, b) cbufp[a].u.o = b
#define Csetreal(a, b) cbufp[a].u.d = b

extern Code_t *cbufp;
extern int cbufn, cbufi;

void Cinit (void);
void Cterm (void);
void Creset (void);
int Cnew (int);
int Cinteger (long);
int Creal (double);
int Cstring (char *);
#endif /* _CODE_H */

#ifdef __cplusplus
}
#endif

