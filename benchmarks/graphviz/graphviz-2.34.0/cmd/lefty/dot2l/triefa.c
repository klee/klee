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

/*  File - TrieFA.ins.c
    This file contains code to be included in the scanner file using a
    generated trie-based FA.
*/

#include "triefa.h"

#ifdef UNDERLINE
static long CharMask[28] = {
    0x0000001, 0x0000000, 0x0000004, 0x0000008,
    0x0000010, 0x0000020, 0x0000040, 0x0000080,
    0x0000100, 0x0000200, 0x0000400, 0x0000800,
    0x0001000, 0x0002000, 0x0004000, 0x0008000,
    0x0010000, 0x0020000, 0x0040000, 0x0080000,
    0x0100000, 0x0200000, 0x0400000, 0x0800000,
    0x1000000, 0x2000000, 0x4000000, 0x8000000,
};

#define IN_MASK_RANGE(C) (islower (C) || ((C) == '_'))
#define MASK_INDEX(C)    ((C) - '_')

#else
static long CharMask[26] = {
    0x0000001, 0x0000002, 0x0000004, 0x0000008,
    0x0000010, 0x0000020, 0x0000040, 0x0000080,
    0x0000100, 0x0000200, 0x0000400, 0x0000800,
    0x0001000, 0x0002000, 0x0004000, 0x0008000,
    0x0010000, 0x0020000, 0x0040000, 0x0080000,
    0x0100000, 0x0200000, 0x0400000, 0x0800000,
    0x1000000, 0x2000000
};

#define IN_MASK_RANGE(C) islower (C)
#define MASK_INDEX(C)    ((C) - 'a')

#endif

static short TFA_State;

/*  TFA_Init:
    Initialize the trie FA.
*/
#define TFA_Init() TFA_State = 0

/*  TFA_Advance:
    Advance to the next state (or -1) on the lowercase letter c.
    This should be an inline routine, but the C++ implementation
    isn't advanced enough so we use a macro.
*/
#define TFA_Advance(C) { \
    char c = C; \
    if (TFA_State >= 0) { \
        if (isupper (c)) \
            c = tolower (c); \
        else if (! IN_MASK_RANGE (c)) { \
            TFA_State = -1; \
            goto TFA_done; \
        } \
        if (TrieStateTbl[TFA_State].mask & CharMask[MASK_INDEX (c)]) { \
            short i = TrieStateTbl[TFA_State].trans_base; \
            while (TrieTransTbl[i].c != c) \
                i++; \
            TFA_State = TrieTransTbl[i].next_state; \
        } \
        else \
            TFA_State = -1; \
    } \
TFA_done:; \
} /* end of TFA_Advance. */

/*  TFA_Definition:
    Return the definition (if any) associated with the current state.
*/
#define TFA_Definition() \
    ((TFA_State < 0) ? -1 : TrieStateTbl[TFA_State].def)
