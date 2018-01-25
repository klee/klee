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

#ifndef _LEX_H
#define _LEX_H

#define L_SEMI      0
#define L_ASSIGN    1
#define L_OR        2
#define L_AND       3
#define L_EQ        4
#define L_NE        5
#define L_LT        6
#define L_LE        7
#define L_GT        8
#define L_GE        9
#define L_PLUS     10
#define L_MINUS    11
#define L_MUL      12
#define L_DIV      13
#define L_MOD      14
#define L_NOT      15
#define L_STRING   16
#define L_NUMBER   17
#define L_ID       18
#define L_DOT      19
#define L_LB       20
#define L_RB       21
#define L_FUNCTION 22
#define L_LP       23
#define L_RP       24
#define L_LCB      25
#define L_RCB      26
#define L_LOCAL    27
#define L_COLON    28
#define L_COMMA    29
#define L_IF       30
#define L_ELSE     31
#define L_WHILE    32
#define L_FOR      33
#define L_IN       34
#define L_BREAK    35
#define L_CONTINUE 36
#define L_RETURN   37
#define L_INTERNAL 38
#define L_EOF      39
#define L_SIZE     40

#define MAXTOKEN 1000
typedef char *Lname_t;

extern int Ltok;
extern char Lstrtok[];
extern Lname_t Lnames[];

void Lsetsrc (int, char *, FILE *, int, int);
void Lgetsrc (int *, char **, FILE **, int *, int *);
void Lprintpos (void);
void Lgtok (void);
#endif /* _LEX_H */

#ifdef __cplusplus
}
#endif

