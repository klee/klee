#ifndef KLEE_TAINT_H
#define KLEE_TAINT_H

#include <assert.h>

namespace klee
{
  typedef unsigned int TaintSet;
  extern TaintSet EMPTYTAINTSET;
  void mergeTaint (TaintSet & this_taint, TaintSet const other_taint);

  #define SIZEOFTAINT sizeof(TaintSet)*4

}

#endif
