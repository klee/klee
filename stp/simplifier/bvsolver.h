/********************************************************************
 * AUTHORS: Vijay Ganesh, David L. Dill
 *
 * BEGIN DATE: November, 2005
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/
// -*- c++ -*-

#include "../AST/AST.h"
#include "../AST/ASTUtil.h"
namespace BEEV {

  //This class represents the bitvector arithmetic linear solver.
  //
  //The bitvector solver is a partial solver, i.e. it does not solve
  //for all variables in the system of equations. it is
  //best-effort. it relies on the SAT solver to be complete.
  //
  //The BVSolver assumes that the input equations are normalized, and
  //have liketerms combined etc.
  //
  //0. Traverse top-down over the input DAG, looking for a conjunction
  //0. of equations. if you find one, then for each equation in the
  //0. conjunction, do the following steps.
  //
  //1. check for Linearity of the input equation
  //
  //2. Solve for a "chosen" variable. The variable should occur
  //2. exactly once and must have an odd coeff. Refer STP's CAV 2007
  //2. paper for actual solving procedure
  //
  //4. Outside the solver, Substitute and Re-normalize the input DAG 
  class BVSolver {
    //Ptr to toplevel manager that manages bit-vector expressions
    //(i.e. construct various kinds of expressions), and also has
    //member functions that simplify bit-vector expressions
    BeevMgr * _bm;
    ASTNode ASTTrue, ASTFalse;

    //Those formulas which have already been solved. If the same
    //formula occurs twice then do not solve the second occurence, and
    //instead drop it
    ASTNodeMap FormulasAlreadySolvedMap;

    //this map is useful while traversing terms and uniquely
    //identifying variables in the those terms. Prevents double
    //counting.
    ASTNodeMap TermsAlreadySeenMap;
    ASTNodeMap TermsAlreadySeenMap_ForArrays;

    //count is used in the creation of new variables
    unsigned int _symbol_count;

    //solved variables list: If a variable has been solved for then do
    //not solve for it again
    ASTNodeSet DoNotSolve_TheseVars;

    //checks if var has been solved for or not. if yes, then return
    //true else return false
    bool DoNotSolveThis(const ASTNode& var);

    //traverses a term, and creates a multiset of all variables in the
    //term. Does memoization to avoid double counting.
    void VarsInTheTerm(const ASTNode& lhs, ASTNodeMultiSet& v);
    void VarsInTheTerm_TopLevel(const ASTNode& lhs, ASTNodeMultiSet& v);

    //choose a suitable var from the term
    ASTNode ChooseMonom(const ASTNode& eq, ASTNode& modifiedterm);
    //accepts an equation and solves for a variable or a monom in it
    ASTNode BVSolve_Odd(const ASTNode& eq);

    //solves equations of the form a*x=t where 'a' is even. Has a
    //return value, unlike the normal BVSolve()
    ASTNode BVSolve_Even(const ASTNode& eq);
    ASTNode CheckEvenEqn(const ASTNode& input, bool& evenflag);

    //Checks for arrayreads in a term. if yes then returns true, else
    //return false
    bool CheckForArrayReads(const ASTNode& term);
    bool CheckForArrayReads_TopLevel(const ASTNode& term);

    //Creates new variables used in solving
    ASTNode NewVar(unsigned int n);

    //this function return true if the var occurs in term, else the
    //function returns false
    bool VarSeenInTerm(const ASTNode& var, const ASTNode& term);
    
    //takes an even number "in" as input, and returns an odd number
    //(return value) and a power of 2 (as number_shifts by reference),
    //such that in = (odd_number * power_of_2).
    //
    //Refer STP's CAV 2007 (or Clark Barrett's 1998 paper on
    //bit-vector arithmetic published in DAC 1998) paper for precise
    //understanding of the algorithm
    ASTNode SplitEven_into_Oddnum_PowerOf2(const ASTNode& in, unsigned int& number_shifts);

    //Once a formula has been solved, then update the alreadysolvedmap
    //with the formula, and the solved value. The solved value can be
    //described using the following example: Suppose input to the
    //solver is
    //
    // input key: x = 2 AND y = x + t
    //
    // output value: y = 2 + t
    void UpdateAlreadySolvedMap(const ASTNode& key, const ASTNode& value);

    //This function checks if the key (formula) has already been
    //solved for.
    //
    //If yes it returns TRUE and fills the "output" with the
    //solved-value (call by reference argument),
    //
    //else returns FALSE
    bool CheckAlreadySolvedMap(const ASTNode& key, ASTNode& output);
  public:
    //constructor
    BVSolver(BeevMgr * bm) : _bm(bm), _symbol_count(0) {
      ASTTrue = _bm->CreateNode(TRUE);
      ASTFalse = _bm->CreateNode(FALSE);
    };

    //Destructor
    ~BVSolver() {
      TermsAlreadySeenMap.clear();
      DoNotSolve_TheseVars.clear();
    }

    //Top Level Solver: Goes over the input DAG, identifies the
    //equation to be solved, solves them,
    ASTNode TopLevelBVSolve(const ASTNode& a);
  }; //end of class bvsolver
};//end of namespace BEEV
