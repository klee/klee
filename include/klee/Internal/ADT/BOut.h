//===-- BOut.h --------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __COMMON_BOUT_H__
#define __COMMON_BOUT_H__


#ifdef __cplusplus
extern "C" {
#endif

  typedef struct BOutObject BOutObject;
  struct BOutObject {
    char *name;
    unsigned numBytes;
    unsigned char *bytes;
  };
  
  typedef struct BOut BOut;
  struct BOut {
    /* file format version */
    unsigned version; 
    
    unsigned numArgs;
    char **args;

    unsigned symArgvs;
    unsigned symArgvLen;

    unsigned numObjects;
    BOutObject *objects;
  };

  
  /* returns the current .bout file format version */
  unsigned bOut_getCurrentVersion();
  
  /* return true iff file at path matches BOut header */
  int   bOut_isBOutFile(const char *path);

  /* returns NULL on (unspecified) error */
  BOut* bOut_fromFile(const char *path);

  /* returns 1 on success, 0 on (unspecified) error */
  int   bOut_toFile(BOut *, const char *path);
  
  /* returns total number of object bytes */
  unsigned bOut_numBytes(BOut *);

  void  bOut_free(BOut *);

#ifdef __cplusplus
}
#endif

#endif
