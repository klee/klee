#include <klee/Taint.h>
#include <assert.h>

namespace klee
{
  TaintSet EMPTYTAINTSET = 0;

  bool hasTaint (TaintSet this_taint, unsigned int bit)
  {
    assert (bit < SIZEOFTAINT);
    return this_taint >> bit & 1;
  }

  void addTaint (TaintSet & this_taint, unsigned int bit)
  {
    assert (bit < SIZEOFTAINT);
    this_taint |= 1 << bit;
  }
  
  void delTaint (TaintSet & this_taint, unsigned int bit)
  {
    assert (bit < SIZEOFTAINT);
    this_taint &= ~(1 << bit);
  }
  
  void clearTaint (TaintSet & this_taint)
  {
    this_taint = 0;
  }
  
  void mergeTaint (TaintSet & this_taint, TaintSet const other_taint)
  {
    this_taint |= other_taint;
  }
/*
  void setTaint (TaintSet & this_taint, unsigned int taint const)
  {
    this_taint = taint;
  };
  TaintSet& getTaint (TaintSet & this_taint)
  {
    return this_taint;
  };
*/

}

