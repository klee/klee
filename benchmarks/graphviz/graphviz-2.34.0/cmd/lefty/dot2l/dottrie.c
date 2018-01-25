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

#include "common.h"
#include "dotparse.h"
#include "triefa.h"

TrieState TrieStateTbl[34] = {
    { -1,          0,  0x42058 },
    { -1,          5,    0x100 },
    { -1,          6,     0x40 },
    { -1,          7,  0x20000 },
    { -1,          8,      0x1 },
    { -1,          9,   0x8000 },
    { -1,         10,     0x80 },
    { T_digraph,  11,      0x0 },
    { -1,         11,      0x8 },
    { -1,         12,     0x40 },
    { -1,         13,     0x10 },
    { T_edge,     14,      0x0 },
    { -1,         14,  0x20000 },
    { -1,         15,      0x1 },
    { -1,         16,   0x8000 },
    { -1,         17,     0x80 },
    { T_graph,    18,      0x0 },
    { -1,         18,   0x4000 },
    { -1,         19,      0x8 },
    { -1,         20,     0x10 },
    { T_node,     21,      0x0 },
    { -1,         21, 0x180000 },
    { -1,         23,  0x20000 },
    { -1,         24,    0x100 },
    { -1,         25,      0x4 },
    { -1,         26,  0x80000 },
    { T_strict,   27,      0x0 },
    { -1,         27,      0x2 },
    { -1,         28,     0x40 },
    { -1,         29,  0x20000 },
    { -1,         30,      0x1 },
    { -1,         31,   0x8000 },
    { -1,         32,     0x80 },
    { T_subgraph, 33,      0x0 },
};
TrieTrans TrieTransTbl[33] = {
    /* State   0 */  { 's',  21 }, { 'e',   8 }, { 'g',  12 },
                     { 'n',  17 }, { 'd',   1 },
    /* State   1 */  { 'i',   2 },
    /* State   2 */  { 'g',   3 },
    /* State   3 */  { 'r',   4 },
    /* State   4 */  { 'a',   5 },
    /* State   5 */  { 'p',   6 },
    /* State   6 */  { 'h',   7 },
    /* State   8 */  { 'd',   9 },
    /* State   9 */  { 'g',  10 },
    /* State  10 */  { 'e',  11 },
    /* State  12 */  { 'r',  13 },
    /* State  13 */  { 'a',  14 },
    /* State  14 */  { 'p',  15 },
    /* State  15 */  { 'h',  16 },
    /* State  17 */  { 'o',  18 },
    /* State  18 */  { 'd',  19 },
    /* State  19 */  { 'e',  20 },
    /* State  21 */  { 't',  22 }, { 'u',  27 },
    /* State  22 */  { 'r',  23 },
    /* State  23 */  { 'i',  24 },
    /* State  24 */  { 'c',  25 },
    /* State  25 */  { 't',  26 },
    /* State  27 */  { 'b',  28 },
    /* State  28 */  { 'g',  29 },
    /* State  29 */  { 'r',  30 },
    /* State  30 */  { 'a',  31 },
    /* State  31 */  { 'p',  32 },
    /* State  32 */  { 'h',  33 },
};
