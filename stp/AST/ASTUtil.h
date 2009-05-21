/********************************************************************
 * AUTHORS: Vijay Ganesh, David L. Dill
 *
 * BEGIN DATE: November, 2005
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/
// -*- c++ -*-

#ifndef ASTUTIL_H
#define ASTUTIL_H

#include <cstring>
#include <iostream>
#include <vector>
#ifdef EXT_HASH_MAP
#include <ext/hash_set>
#include <ext/hash_map>
#else
#include <hash_set>
#include <hash_map>
#endif

using namespace std; 
namespace BEEV {
#ifdef EXT_HASH_MAP
  using namespace __gnu_cxx;
#endif
  //some global variables that are set through commandline options. it
  //is best that these variables remain global. Default values set
  //here
  //
  //collect statistics on certain functions
  extern bool stats;
  //print DAG nodes
  extern bool print_nodes;
  //tentative global var to allow for variable activity optimization
  //in the SAT solver. deprecated.
  extern bool variable_activity_optimize;
  //run STP in optimized mode
  extern bool optimize;
  //do sat refinement, i.e. underconstraint the problem, and feed to
  //SAT. if this works, great. else, add a set of suitable constraints
  //to re-constraint the problem correctly, and call SAT again, until
  //all constraints have been added.
  extern bool arrayread_refinement;
  //switch to control write refinements
  extern bool arraywrite_refinement;
  //check the counterexample against the original input to STP
  extern bool check_counterexample;
  //construct the counterexample in terms of original variable based
  //on the counterexample returned by SAT solver
  extern bool construct_counterexample;
  extern bool print_counterexample;
  //if this option is true then print the way dawson wants using a
  //different printer. do not use this printer.
  extern bool print_arrayval_declaredorder;
  //flag to decide whether to print "valid/invalid" or not
  extern bool print_output;
  //do linear search in the array values of an input array. experimental
  extern bool linear_search;
  //print the variable order chosen by the sat solver while it is
  //solving.
  extern bool print_sat_varorder; 
  //turn on word level bitvector solver
  extern bool wordlevel_solve;
  //XOR flattening optimizations.
  extern bool xor_flatten;
  //this flag indicates that the BVSolver() succeeded
  extern bool toplevel_solved;
  //the smtlib parser has been turned on
  extern bool smtlib_parser_enable;
  //print the input back
  extern bool print_STPinput_back;

  extern void (*vc_error_hdlr)(const char* err_msg);
  /*Spacer class is basically just an int, but the new class allows
    overloading of << with a special definition that prints the int as
    that many spaces. */
  class Spacer {
  public:
    int _spaces;
    Spacer(int spaces) { _spaces = spaces; }
    friend ostream& operator<<(ostream& os, const Spacer &ind);
  };

  inline Spacer spaces(int width) {
    Spacer sp(width);
    return sp;
  }

  struct eqstr {
    bool operator()(const char* s1, const char* s2) const {
      return strcmp(s1, s2) == 0;
    }
  };
  
  typedef hash_map<const char*,int, 
		   hash<const char *>,eqstr> function_counters;
  void CountersAndStats(const char * functionname);

  //global function which accepts an integer and looks up the
  //corresponding ASTNode and prints a char* of that ASTNode
  void Convert_MINISATVar_To_ASTNode_Print(int minisat_var, 
					   int decision, int polarity=0);
}; // end namespace.
#endif
