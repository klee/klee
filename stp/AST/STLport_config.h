/********************************************************************
 * AUTHORS: Vijay Ganesh, David L. Dill
 *
 * BEGIN DATE: November, 2005
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/
// -*- c++ -*-

// STLport debug checking, if we use STLport threads flag is to get
// rid of link errors, since iostreams compiles with threads.  alloc
// and uninitialized are extra checks Later on, if used with Purify or
// Valgrind, may want to set flags to prevent reporting of false
// leaks.  For some reason, _STLP_THREADS works on the command line
// but not here (?)
#define _STLP_THREADS
#define _STLP_DEBUG 1
#define _STLP_DEBUG_LEVEL _STLP_STANDARD_DBG_LEVEL 
#define _STLP_DEBUG_ALLOC 1
#define _STLP_DEBUG_UNINITIALIZED 1
