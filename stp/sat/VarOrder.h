/**************************************************************************************[VarOrder.h]
MiniSat -- Copyright (c) 2003-2005, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef VarOrder_h
#define VarOrder_h

#include "SolverTypes.h"
#include "Solver.h"
#include "Heap.h"
#include "../AST/ASTUtil.h"

namespace MINISAT {
  //=================================================================================================

  struct VarOrder_lt {
    const vec<double>&  activity;
    bool operator () (Var x, Var y) { return activity[x] > activity[y]; }
    VarOrder_lt(const vec<double>&  act) : activity(act) { }
  };
  

  enum { p_decisionvar = 0, p_polarity = 1, p_frozen = 2, p_dontcare = 3 };
  
  
  class VarOrder {
    const vec<char>&    assigns;     // var->val. Pointer to external assignment table.
    const vec<double>&  activity;    // var->act. Pointer to external activity table.
    vec<char>           properties;
    //Heap<VarOrder_lt>   heap;
    //double              random_seed; // For the internal random number generator
    
    friend class VarFilter;
  public:
    //FIXME: Vijay: delete after experiments
    Heap<VarOrder_lt>   heap;
    double              random_seed; // For the internal random number generator
    //FIXME ENDS HERE

    VarOrder(const vec<char>& ass, const vec<double>& act) :
      assigns(ass), activity(act), heap(VarOrder_lt(act)), random_seed(2007)
      //assigns(ass), activity(act), heap(VarOrder_lt(act))
    { }
    
    int  size       ()                         { return heap.size(); }
    void setVarProp (Var v, uint prop, bool b) { properties[v] = (properties[v] & ~(1 << prop)) | (b << prop); }
    bool hasVarProp (Var v, uint prop) const   { return properties[v] & (1 << prop); }
    inline void cleanup    ();
    
    inline void newVar(bool polarity, bool dvar);
    inline void update(Var x);                  // Called when variable increased in activity.
    inline void undo(Var x);                    // Called when variable is unassigned and may be selected again.
    //Selects a new, unassigned variable (or 'var_Undef' if none exists).
    inline Lit  select(double random_freq =.0, int decision_level = 0); 
  };
  
  
  struct VarFilter {
    const VarOrder& o;
    VarFilter(const VarOrder& _o) : o(_o) {}
    bool operator()(Var v) const { return toLbool(o.assigns[v]) == l_Undef  && o.hasVarProp(v, p_decisionvar); }
    //bool operator()(Var v) const { return toLbool(o.assigns[v]) == l_Undef; }
  };
  
  void VarOrder::cleanup()
  {
    VarFilter f(*this);
    heap.filter(f);
  }
  
  void VarOrder::newVar(bool polarity, bool dvar)
  {
    Var v = assigns.size()-1;
    heap.setBounds(v+1);
    properties.push(0);
    setVarProp(v, p_decisionvar, dvar);
    setVarProp(v, p_polarity, polarity);
    undo(v);
  }
  
  
  void VarOrder::update(Var x)
  {
    if (heap.inHeap(x))
      heap.increase(x);
  }
  
  
  void VarOrder::undo(Var x)
  {
    if (!heap.inHeap(x) && hasVarProp(x, p_decisionvar))
      heap.insert(x);
  }
  
  
  Lit VarOrder::select(double random_var_freq, int decision_level)
  {
    Var next = var_Undef;
    
    if (drand(random_seed) < random_var_freq && !heap.empty())
      next = irand(random_seed,assigns.size());

    // Activity based decision:
    while (next == var_Undef || toLbool(assigns[next]) != l_Undef || !hasVarProp(next, p_decisionvar))
      if (heap.empty()){
	next = var_Undef;
	break;
      }else
	next = heap.getmin();
    
    //printing
    if(BEEV::print_sat_varorder) {
      if (next != var_Undef) {
	BEEV::Convert_MINISATVar_To_ASTNode_Print(next,
						  decision_level,
						  hasVarProp(next, p_polarity));
	// fprintf(stderr,"var = %d, prop = %d, decision = %d, polarity = %d, frozen = %d\n", 
	// 		next+1, properties[next], hasVarProp(next, p_decisionvar), 
	// 		hasVarProp(next, p_polarity), hasVarProp(next, p_frozen));
      }
      else
	fprintf(stderr, "var = undef\n");
    }
    
    return next == var_Undef ? lit_Undef : Lit(next, hasVarProp(next, p_polarity));
  }
  
  
  //=================================================================================================
};
#endif
