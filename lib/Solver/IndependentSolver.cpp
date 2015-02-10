//===-- IndependentSolver.cpp ---------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "independent-solver"
#include "klee/Solver.h"

#include "klee/Expr.h"
#include "klee/Constraints.h"
#include "klee/SolverImpl.h"
#include "klee/Internal/Support/Debug.h"
#include "klee/util/Assignment.h"
#include "IndependenceAnalysis.h"

#include "klee/util/ExprUtil.h"

#include "llvm/Support/raw_ostream.h"
#include <map>
#include <vector>
#include <ostream>

using namespace klee;
using namespace llvm;

class IndependentSolver : public SolverImpl {
private:
  Solver *solver;

public:
  IndependentSolver(Solver *_solver) 
    : solver(_solver) {}
  ~IndependentSolver() { delete solver; }

  bool computeTruth(const Query&, bool &isValid);
  bool computeValidity(const Query&, Solver::Validity &result);
  bool computeValue(const Query&, ref<Expr> &result);
  bool computeInitialValues(const Query& query,
                            const std::vector<const Array*> &objects,
                            std::vector< std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query&);
  void setCoreSolverTimeout(double timeout);
};
  
bool IndependentSolver::computeValidity(const Query& query,
                                        Solver::Validity &result) {
  std::vector< ref<Expr> > required;
  IndependentElementSet eltsClosure =
    getFreshFactor(query, required);
  ConstraintManager tmp(required);
  return solver->impl->computeValidity(Query(tmp, query.expr), 
                                       result);
}

bool IndependentSolver::computeTruth(const Query& query, bool &isValid) {
  std::vector< ref<Expr> > required;
  IndependentElementSet eltsClosure = 
	getFreshFactor(query, required);
  ConstraintManager tmp(required);
  return solver->impl->computeTruth(Query(tmp, query.expr), 
                                    isValid);
}

bool IndependentSolver::computeValue(const Query& query, ref<Expr> &result) {
  std::vector< ref<Expr> > required;
  IndependentElementSet eltsClosure = 
	getFreshFactor(query, required);
  ConstraintManager tmp(required);
  return solver->impl->computeValue(Query(tmp, query.expr), result);
}

/*
 * Used only for assertions to make sure point created during computeInitialValues
 * is in fact correct.
 */
bool createdPointEvaluatesToTrue(const Query &query,
		const std::vector<const Array*> &objects,
		std::vector< std::vector<unsigned char> > &values){

	Assignment assign = Assignment(objects, values);

	for(ConstraintManager::constraint_iterator it = query.constraints.begin();
			it != query.constraints.end(); ++it){
		ref<Expr> ret = assign.evaluate(*it);
		if(! isa<ConstantExpr>(ret) || ! cast<ConstantExpr>(ret)->isTrue()){
			return false;
		}
	}

	ref<Expr> neg = Expr::createIsZero(query.expr);
	ref<Expr> q = assign.evaluate(neg);

	assert(isa<ConstantExpr>(q) && "assignment evaluation did not result in constant");
	return cast<ConstantExpr>(q)->isTrue();
}

bool IndependentSolver::computeInitialValues(const Query& query,
		const std::vector<const Array*> &objects,
		std::vector< std::vector<unsigned char> > &values,
		bool &hasSolution){

	std::list<IndependentElementSet> * factors = new std::list<IndependentElementSet>;
	getAllFactors(query, factors);

	//Used to rearrange all of the answers into the correct order
	std::map<const Array*, std::vector<unsigned char> > retMap;

	for (std::list<IndependentElementSet>::iterator it = factors->begin(); it != factors->end(); ++it) {

		std::vector<const Array*> arraysInFactor;
		calculateArrays(*it, arraysInFactor);

		//Going to use this as the "fresh" expression for the Query() invocation below
		assert(it->exprs.size() >= 1 && "No null/empty factors");
		if(arraysInFactor.size() == 0){
			continue;
		}

		ConstraintManager tmp(it->exprs);
		std::vector<std::vector<unsigned char> > tempValues;
		if(!solver->impl->computeInitialValues(Query(tmp, ConstantExpr::alloc(0, Expr::Bool)), arraysInFactor, tempValues, hasSolution)){
			values.clear();	//The above assertion is to make sure we are returning values in correct state
			return false;
		}else if(!hasSolution){
			values.clear();//The above assertion is to make sure we are returning values in correct state
			return true;
		}else{
			assert(tempValues.size() == arraysInFactor.size() && "Should be equal number arrays and answers");
			for(unsigned i = 0; i < tempValues.size(); i++){
				if(retMap.count(arraysInFactor[i])){
					//We already have an array with some partially correct answers,
					//so we need to place the answers to the new query into the right
					//spot while avoiding the undetermined values also in the array
					std::vector<unsigned char> * tempPtr = &retMap[arraysInFactor[i]];
					assert(tempPtr->size() == tempValues[i].size() && "we're talking about the same array here");
					::DenseSet<unsigned> * ds = &(it->elements[arraysInFactor[i]]);
					for(std::set<unsigned>::iterator it2 = ds->begin(); it2 != ds->end(); it2++){
						unsigned index = * it2;
						(* tempPtr)[index] = tempValues[i][index];
					}
				}else{
					//Dump all the new values into the array
					retMap[arraysInFactor[i]] = tempValues[i];
				}
			}
		}
	}

	for(std::vector<const Array *>::const_iterator it = objects.begin(); it != objects.end(); it++){
		const Array * arr = * it;
		if(!retMap.count(arr)){
			//this means we have an array that is somehow related to the
			//constraint, but whose values aren't actually required to
			//satisfy the query.
			std::vector<unsigned char> ret(arr->size);
			values.push_back(ret);
		}else{
			values.push_back(retMap[arr]);
		}
	}

	assert(createdPointEvaluatesToTrue(query, objects, values) && "should satisfy the equation");

	delete factors;
	return true;

}

SolverImpl::SolverRunStatus IndependentSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();      
}

char *IndependentSolver::getConstraintLog(const Query& query) {
  return solver->impl->getConstraintLog(query);
}

void IndependentSolver::setCoreSolverTimeout(double timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

Solver *klee::createIndependentSolver(Solver *s) {
  return new Solver(new IndependentSolver(s));
}
