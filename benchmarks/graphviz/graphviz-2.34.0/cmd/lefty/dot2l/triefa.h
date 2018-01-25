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

/*  File - TrieFA.h
    The data types for the generated trie-baseed finite automata.
*/

struct TrieState {  /* An entry in the FA state table */
    short def;         /* If this state is an accepting state then */
                       /* this is the definition, otherwise -1.    */
    short trans_base;  /* The base index into the transition table.*/
    long  mask;        /* The transition mask.                     */
};

struct TrieTrans {  /* An entry in the FA transition table. */
    short c;           /* The transition character (lowercase). */
    short next_state;  /* The next state.                       */
};

typedef struct TrieState TrieState;
typedef struct TrieTrans TrieTrans;

extern TrieState TrieStateTbl[];
extern TrieTrans TrieTransTbl[];

#ifdef __cplusplus
}
#endif

