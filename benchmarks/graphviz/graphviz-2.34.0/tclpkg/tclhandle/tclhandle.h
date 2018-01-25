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

#ifndef TCL_ERROR
#define TCL_ERROR (1)
#endif

#ifndef TCL_OK
#define TCL_OK (0)
#endif

#ifndef NULL
#define NULL (0)
#endif

#define ISSPACE(c) (isspace ((unsigned char) c))

/*
 * Macro to rounded up a size to be a multiple of (void *).  This is required
 * for systems that have alignment restrictions on pointers and data.
 */
#define ROUND_ENTRY_SIZE(size) ((((size) + tclhandleEntryAlignment - 1) / \
		tclhandleEntryAlignment) * tclhandleEntryAlignment)

#define NULL_IDX      -1
#define ALLOCATED_IDX -2

    typedef unsigned char ubyte_t;
    typedef ubyte_t *ubyte_pt;

/*
 * This is the table header.  It is separately allocated from the table body,
 * since it must keep track of a table body that might move.  Each entry in the
 * table is preceded with a header which has the free list link, which is a
 * entry index of the next free entry.  Special values keep track of allocated
 * table is preceded with a header which has the free list link, which is a
 * entry index of the next free entry.  Special values keep track of allocated
 * entries.
 */

    typedef struct {
	int entrySize;		/* Entry size in bytes, including overhead */
	int tableSize;		/* Current number of entries in the table  */
	int freeHeadIdx;	/* Index of first free entry in the table  */
	char *handleFormat;	/* Malloc'ed copy of prefix string + "%lu" */
	ubyte_pt bodyPtr;	/* Pointer to table body                   */
    } tblHeader_t;
    typedef tblHeader_t *tblHeader_pt;

    typedef struct {
	int freeLink;
    } entryHeader_t;
    typedef entryHeader_t *entryHeader_pt;

#define ENTRY_HEADER_SIZE (ROUND_ENTRY_SIZE (sizeof (entryHeader_t)))

/*
 * This macro is used to return a pointer to an entry, given its index.
 */
#define TBL_INDEX(hdrPtr, idx) \
    ((entryHeader_pt) (hdrPtr->bodyPtr + (hdrPtr->entrySize * idx)))

/*
 * This macro is used to return an index to an entry, given its pointer.
 *    **** This macro provides no checks *****
 */
#define TBL_ENTRY(hdrPtr, entryPtr) \
    ((unsigned long) ((entryPtr - (hdrPtr->bodyPtr)) / (hdrPtr->entrySize)))

/*
 * This macros to convert between pointers to the user and header area of
 * an table entry.
 */
#define USER_AREA(entryPtr) \
 (void *) (((ubyte_pt) entryPtr) + ENTRY_HEADER_SIZE);
#define HEADER_AREA(entryPtr) \
 (entryHeader_pt) (((ubyte_pt) entryPtr) - ENTRY_HEADER_SIZE);

/*
 * Function prototypes.
 */

    void *tclhandleFreeIndex(tblHeader_pt headerPtr,
			     unsigned long entryIdx);
    void *tclhandleFree(tblHeader_pt headerPtr, char *handle);
    tblHeader_pt tclhandleInit(char *prefix, int entrySize,
			       int initEntries);
    int tclhandleReset(tblHeader_pt tblHdrPtr, int initEntries);
    int tclhandleDestroy(tblHeader_pt tblHdrPtr);
    void *tclhandleXlateIndex(tblHeader_pt headerPtr,
			      unsigned long entryIdx);
    void *tclhandleXlate(tblHeader_pt headerPtr, char *handle);
    entryHeader_pt tclhandleAlloc(tblHeader_pt tblHdrPtr, char *handle,
				  unsigned long *entryIdxPtr);
    void tclhandleString(tblHeader_pt tblHdrPtr, char *handle,
			 unsigned long entryIdx);
    int tclhandleIndex(tblHeader_pt tblHdrPtr, char *handle,
		       unsigned long *entryIdxPtr);

#ifdef __cplusplus
}
#endif
