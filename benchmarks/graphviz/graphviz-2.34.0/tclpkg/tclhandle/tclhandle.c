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

/*
 * tclhandle.c --
 *
 * adapted from tclXhandles.c tclXutil.c and tclExtend.h
 */

/*
 *
 * tclXhandles.c --
 *
 * Tcl handles.  Provides a mechanism for managing expandable tables that are
 * addressed by textual handles.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1994 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 * $Id$
 *-----------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include "tclhandle.h"
/* Added 2000-09-19 by KG for memcpy, ... decls */
#include <string.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/*
 * Variable set to contain the alignment factor (in bytes) for this machine.
 * It is set on the first table initialization.
 */
static int tclhandleEntryAlignment = 0;

/*=============================================================================
 * tclhandleLinkInNewEntries --
 *   Build free links through the newly allocated part of a table.
 *   
 * Parameters:
 *   o tblHdrPtr (I) - A pointer to the table header.
 *   o newIdx (I) - Index of the first new entry.
 *   o numEntries (I) - The number of new entries.
 *-----------------------------------------------------------------------------
 */
static void tclhandleLinkInNewEntries(tblHeader_pt tblHdrPtr, int newIdx,
				      int numEntries)
{
    int entIdx, lastIdx;
    entryHeader_pt entryPtr;

    lastIdx = newIdx + numEntries - 1;

    for (entIdx = newIdx; entIdx < lastIdx; entIdx++) {
	entryPtr = TBL_INDEX(tblHdrPtr, entIdx);
	entryPtr->freeLink = entIdx + 1;
    }
    entryPtr = TBL_INDEX(tblHdrPtr, lastIdx);
    entryPtr->freeLink = tblHdrPtr->freeHeadIdx;
    tblHdrPtr->freeHeadIdx = newIdx;

}

/*=============================================================================
 * tclhandleExpandTable --
 *   Expand a handle table, doubling its size.
 * Parameters:
 *   o tblHdrPtr (I) - A pointer to the table header.
 *   o neededIdx (I) - If positive, then the table will be expanded so that
 *     this entry is available.  If -1, then just expand by the number of 
 *     entries specified on table creation.  MUST be smaller than this size.
 *-----------------------------------------------------------------------------
 */
static void tclhandleExpandTable(tblHeader_pt tblHdrPtr, int neededIdx)
{
    ubyte_pt oldbodyPtr = tblHdrPtr->bodyPtr;
    int numNewEntries;
    int newSize;

    if (neededIdx < 0)
	numNewEntries = tblHdrPtr->tableSize;
    else
	numNewEntries = (neededIdx - tblHdrPtr->tableSize) + 1;
    newSize =
	(tblHdrPtr->tableSize + numNewEntries) * tblHdrPtr->entrySize;

    tblHdrPtr->bodyPtr = (ubyte_pt) malloc(newSize);
    memcpy(tblHdrPtr->bodyPtr, oldbodyPtr,
	   (tblHdrPtr->tableSize * tblHdrPtr->entrySize));
    tclhandleLinkInNewEntries(tblHdrPtr, tblHdrPtr->tableSize,
			      numNewEntries);
    tblHdrPtr->tableSize += numNewEntries;
    free(oldbodyPtr);

}

/*=============================================================================
 * tclhandleAlloc --
 *   Allocate a table entry, expanding if necessary.
 *
 * Parameters:
 *   o headerPtr (I) - A pointer to the table header.
 *   o handle (O) - If not NULL, the handle is returned here.
 *   o entryIdxPtr (O) - If not NULL, the index is returned here.
 * Returns:
 *    The a pointer to the entry.
 *-----------------------------------------------------------------------------
 */
entryHeader_pt tclhandleAlloc(tblHeader_pt headerPtr, char *handle,
			      unsigned long *entryIdxPtr)
{
    tblHeader_pt tblHdrPtr = (tblHeader_pt) headerPtr;
    entryHeader_pt entryPtr;
    int entryIdx;

    if (tblHdrPtr->freeHeadIdx == NULL_IDX)
	tclhandleExpandTable(tblHdrPtr, -1);

    entryIdx = tblHdrPtr->freeHeadIdx;
    entryPtr = TBL_INDEX(tblHdrPtr, entryIdx);
    tblHdrPtr->freeHeadIdx = entryPtr->freeLink;
    entryPtr->freeLink = ALLOCATED_IDX;

    if (handle)
	sprintf(handle, tblHdrPtr->handleFormat, entryIdx);
    if (entryIdxPtr)
	*entryIdxPtr = entryIdx;
    return USER_AREA(entryPtr);

}

/*=============================================================================
 * tclhandleInit --
 *   Create and initialize a Tcl dynamic handle table. 
 * Parameters:
 *   o prefix (I) - The character string prefix to be used.
 *   o entrySize (I) - The size of an entry, in bytes.
 *   o initEntries (I) - Initial size of the table, in entries.
 * Returns:
 *   A pointer to the table header.  
 *-----------------------------------------------------------------------------
 */
tblHeader_pt tclhandleInit(char *prefix, int entrySize, int initEntries)
{
    tblHeader_pt tblHdrPtr;

    /*
     * It its not been calculated yet, determine the entry alignment required
     * for this machine.
     */
    if (tclhandleEntryAlignment == 0) {
	tclhandleEntryAlignment = sizeof(void *);
	if (sizeof(unsigned long) > tclhandleEntryAlignment)
	    tclhandleEntryAlignment = sizeof(unsigned long);
	if (sizeof(double) > tclhandleEntryAlignment)
	    tclhandleEntryAlignment = sizeof(double);
    }

    /*
     * Set up the table entry.
     */
    tblHdrPtr = (tblHeader_pt) malloc(sizeof(tblHeader_t));

    /* 
     * Calculate entry size, including header, rounded up to sizeof (void *). 
     */
    tblHdrPtr->entrySize = ENTRY_HEADER_SIZE + ROUND_ENTRY_SIZE(entrySize);
    tblHdrPtr->freeHeadIdx = NULL_IDX;
    tblHdrPtr->tableSize = initEntries;
    tblHdrPtr->handleFormat = (char *) malloc(strlen(prefix) + 4);
    strcpy(tblHdrPtr->handleFormat, prefix);
    strcat(tblHdrPtr->handleFormat, "%lu");
    tblHdrPtr->bodyPtr =
	(ubyte_pt) malloc(initEntries * tblHdrPtr->entrySize);
    tclhandleLinkInNewEntries(tblHdrPtr, 0, initEntries);

    return tblHdrPtr;

}

/*=============================================================================
 * tclhandleDestroy --
 *   Destroy a Tcl dynamic handle table. 
 * Parameters:
 *   tblHdrPtr - A pointer to the table header.
 * Returns:
 *   TCL_OK if success
 *   TCL_ERROR if the table was not empty.
 *-----------------------------------------------------------------------------
 */
int tclhandleDestroy(tblHeader_pt tblHdrPtr)
{
    int entIdx, lastIdx;
    entryHeader_pt entryPtr;

    lastIdx = tblHdrPtr->tableSize;
    for (entIdx = 0; entIdx < lastIdx; entIdx++) {
	entryPtr = TBL_INDEX(tblHdrPtr, entIdx);
	if (entryPtr->freeLink == ALLOCATED_IDX)
	    return TCL_ERROR;
    }
    free(tblHdrPtr->bodyPtr);
    free(tblHdrPtr->handleFormat);
    free(tblHdrPtr);

    return TCL_OK;
}

/*=============================================================================
 * tclhandleReset --
 *   Reset a Tcl dynamic handle table.  
 * Parameters:
 *   tblHdrPtr - A pointer to the table header.
 *   initEntries - The number of initial entries in the reset table.
 * Returns:
 *   TCL_OK if success
 *   TCL_ERROR if any handles still in use in the old handle table.
 *-----------------------------------------------------------------------------
 */

int tclhandleReset(tblHeader_pt tblHdrPtr, int initEntries)
{
    int entIdx, lastIdx;
    entryHeader_pt entryPtr;

    lastIdx = tblHdrPtr->tableSize;
    for (entIdx = 0; entIdx < lastIdx; entIdx++) {
	entryPtr = TBL_INDEX(tblHdrPtr, entIdx);
	if (entryPtr->freeLink == ALLOCATED_IDX)
	    return TCL_ERROR;
    }
    free(tblHdrPtr->bodyPtr);
    tblHdrPtr->freeHeadIdx = NULL_IDX;
    tblHdrPtr->tableSize = initEntries;
    tblHdrPtr->bodyPtr =
	(ubyte_pt) malloc(initEntries * tblHdrPtr->entrySize);
    tclhandleLinkInNewEntries(tblHdrPtr, 0, initEntries);

    return TCL_OK;
}

/*=============================================================================
 * tclhandleIndex --
 *   Translate a string handle to a handleTable Index.
 *
 * Parameters:
 *   o headerPtr (I) - A pointer to the table header.
 *   o handle (I) - The handle string -- <prefix><int>
 *   o entryIdxPtr (O) - If not NULL, the index is returned here.
 * Returns:
 *   TCL_OK if the handle was valid.
 *   TCL_ERROR if the handle was invalid.
 *-----------------------------------------------------------------------------
 */
int tclhandleIndex(tblHeader_pt tblHdrPtr, char *handle,
		   unsigned long *entryIdxPtr)
{
    unsigned long entryIdx;

    if ((sscanf(handle, tblHdrPtr->handleFormat, &entryIdx)) != 1)
	return TCL_ERROR;
    if (entryIdxPtr)
	*entryIdxPtr = entryIdx;
    return TCL_OK;
}

/*=============================================================================
 * tclhandleString --
 *   Translate a handleTable Index into a string handle
 *       NB. No check is performed on the Index provided.
 *
 * Parameters:
 *   o headerPtr (I) - A pointer to the table header.
 *   o handle (O) - The handle string -- <prefix><int>
 *   o entryIdxPtr (I) - If not NULL, the index is returned here.
 *-----------------------------------------------------------------------------
 */
void tclhandleString(tblHeader_pt tblHdrPtr, char *handle,
		     unsigned long entryIdx)
{
    sprintf(handle, tblHdrPtr->handleFormat, entryIdx);
}


/*=============================================================================
 * tclhandleXlateIndex --
 *   Translate an index to an entry pointer.
 *
 * Parameters:
 *   o headerPtr (I) - A pointer to the table header.
 *   o index (I) - The index of an entry.
 * Returns:
 *   A pointer to the entry, or NULL if an error occured.
 *-----------------------------------------------------------------------------
 */
void *tclhandleXlateIndex(tblHeader_pt headerPtr, unsigned long entryIdx)
{
    tblHeader_pt tblHdrPtr = headerPtr;
    entryHeader_pt entryPtr;

    entryPtr = TBL_INDEX(tblHdrPtr, entryIdx);

    if ((entryIdx >= tblHdrPtr->tableSize) ||
	(entryPtr->freeLink != ALLOCATED_IDX)) {
	return NULL;
    }

    return USER_AREA(entryPtr);
}

/*=============================================================================
 * tclhandleXlate --
 *   Translate a string handle to an entry pointer.
 *
 * Parameters:
 *   o headerPtr (I) - A pointer to the table header.
 *   o handle (I) - The handle string -- <prefix><int>
 * Returns:
 *   A pointer to the entry, or NULL if an error occured.
 *-----------------------------------------------------------------------------
 */
void *tclhandleXlate(tblHeader_pt tblHdrPtr, char *handle)
{
    unsigned long entryIdx;

    if ((tclhandleIndex(tblHdrPtr, handle, &entryIdx)) != TCL_OK)
	return NULL;
    return (tclhandleXlateIndex(tblHdrPtr, entryIdx));
}

/*============================================================================
 * tclhandleFreeIndex --
 *   Frees a handle table entry by index.
 *
 * Parameters:
 *   o headerPtr (I) - A pointer to the table header.
 *   o index (I) - The index of an entry.
 *----------------------------------------------------------------------------
 * Returns:
 *   The contents of the entry, if success, or NULL if an error occured.
 *----------------------------------------------------------------------------
 */
void *tclhandleFreeIndex(tblHeader_pt headerPtr, unsigned long entryIdx)
{
    tblHeader_pt tblHdrPtr = headerPtr;
    entryHeader_pt entryPtr, freeentryPtr;

    entryPtr = TBL_INDEX(tblHdrPtr, entryIdx);

    if ((entryIdx >= tblHdrPtr->tableSize) ||
	(entryPtr->freeLink != ALLOCATED_IDX)) {
	return NULL;
    }

    entryPtr = USER_AREA(entryPtr);
    freeentryPtr = HEADER_AREA(entryPtr);
    freeentryPtr->freeLink = tblHdrPtr->freeHeadIdx;
    tblHdrPtr->freeHeadIdx = (((ubyte_pt) entryPtr) - tblHdrPtr->bodyPtr) /
	tblHdrPtr->entrySize;

    return entryPtr;
}

/*============================================================================
 * tclhandleFree --
 *   Frees a handle table entry by handle.
 *
 * Parameters:
 *   o headerPtr (I) - A pointer to the table header.
 *   o handle (I) - The handle of an entry.
 *----------------------------------------------------------------------------
 * Returns:
 *   The contents of the entry, if success, or NULL if an error occured.
 *----------------------------------------------------------------------------
 */
void *tclhandleFree(tblHeader_pt tblHdrPtr, char *handle)
{
    unsigned long entryIdx;

    if ((tclhandleIndex(tblHdrPtr, handle, &entryIdx)) != TCL_OK)
	return NULL;
    return (tclhandleFreeIndex(tblHdrPtr, entryIdx));
}
