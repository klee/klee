//===-- CexCachingSolver.cpp ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver.h"

#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/SolverImpl.h"
#include "klee/TimerStatIncrementer.h"
#include "klee/util/Assignment.h"
#include "klee/util/ExprUtil.h"
#include "klee/util/ExprVisitor.h"
#include "klee/Internal/ADT/MapOfSets.h"

#include "SolverStats.h"

#include "llvm/Support/CommandLine.h"

using namespace klee;
using namespace llvm;

namespace {
  cl::opt<bool>
  DebugCexCacheCheckBinding("debug-cex-cache-check-binding");

  cl::opt<bool>
  CexCacheTryAll("cex-cache-try-all",
                 cl::desc("try substituting all counterexamples before asking the SMT solver"),
                 cl::init(false));

  cl::opt<bool>
  CexCacheExperimental("cex-cache-exp", cl::init(false));

}

///

typedef std::set< ref<Expr> > KeyType;

struct AssignmentLessThan {
  bool operator()(const Assignment *a, const Assignment *b) {
    return a->bindings < b->bindings;
  }
};

/*
 * Contains a list of pairs of expressions and their answers that happen
 * to map to the same hash value.  Prevents a hash collision.
 */
class AssignmentBundle{
	typedef std::pair<std::set<unsigned>, Assignment *> answer;

private:
	//The list of possible answers to iterate through should a collision occur
	std::vector<answer> possibleAnswers;

public :
	AssignmentBundle(){}

	bool match(const KeyType &key, answer &possible){
		if(key.size() != possible.first.size()){
			return false;
		}

		for(std::set<ref<Expr> >::const_iterator it = key.begin(); it != key.end(); it ++){
			if(possible.first.count(it->get()->hash()) == 0){
				return false;
			}
		}

		return true;
	}

	bool get(const KeyType &key, Assignment * &assignment){
		for(std::vector<answer>::const_iterator it = possibleAnswers.begin(); it != possibleAnswers.end(); it++){
			answer a = * it;
			if(match(key, a)){
				assignment = it->second;
				return true;
			}
		}
		assignment = 0;
		return false;
	}

	bool get(const std::vector<ref<Expr> > & key, Assignment * &result){
		KeyType pass;
		for(std::vector<ref<Expr> >::const_iterator it = key.begin(); it != key.end(); it++){
			ref<Expr> e = * it;
			pass.insert(e);
		}
		return get(pass, result);
	}

	void put(const KeyType & key, Assignment * &result){
		for(std::vector<answer>::const_iterator it = possibleAnswers.begin(); it != possibleAnswers.end(); it++){
			answer a = * it;
			if(match(key, a)){
				return;
			}
		}
		std::set<unsigned> add;
		for(KeyType::const_iterator it = key.begin(); it != key.end(); it ++){
			ref<Expr> expr = *it;
			add.insert(expr.get()->hash());
		}

		answer a(add, result);
		possibleAnswers.push_back(a);
	}
};

class CexCachingSolver : public SolverImpl {
  typedef std::set<Assignment*, AssignmentLessThan> assignmentsTable_ty;

  Solver *solver;
  
  std::map<long, AssignmentBundle *> quickCache;

  MapOfSets<ref<Expr>, Assignment*> cache;
  // memo table
  assignmentsTable_ty assignmentsTable;

  bool searchForAssignment(KeyType &key, 
                           Assignment *&result);
  
  bool lookupAssignment(const Query& query, KeyType &key, Assignment *&result);

  bool lookupAssignment(const Query& query, Assignment *&result) {
    KeyType key;
    return lookupAssignment(query, key, result);
  }

  //Caching operations
  bool getFromQuickCache(const KeyType & key, Assignment * &assignment);
  void insertInQuickCache(const KeyType & key, Assignment * &binding);
  void insertInCaches(const KeyType & key, Assignment * &binding);

  bool quickMatch(const Query &query, const KeyType &key, Assignment *&result);

  bool checkPreviousSolutionHelper(const ref<Expr>, const std::set<ref<Expr> > &key, Assignment * &result);
  bool checkPreviousSolution(const Query &query, Assignment *&result);

  bool getAssignment(const Query& query, Assignment *&result);
  
public:
  CexCachingSolver(Solver *_solver) : solver(_solver) {}
  ~CexCachingSolver();
  
  bool computeTruth(const Query&, bool &isValid);
  bool computeValidity(const Query&, Solver::Validity &result);
  bool computeValue(const Query&, ref<Expr> &result);
  bool computeInitialValues(const Query&,
                            const std::vector<const Array*> &objects,
                            std::vector< std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query& query);
  void setCoreSolverTimeout(double timeout);
};

///

struct NullAssignment {
  bool operator()(Assignment *a) const { return !a; }
};

struct NonNullAssignment {
  bool operator()(Assignment *a) const { return a!=0; }
};

struct NullOrSatisfyingAssignment {
  KeyType &key;
  
  NullOrSatisfyingAssignment(KeyType &_key) : key(_key) {}

  bool operator()(Assignment *a) const { 
    return !a || a->satisfies(key.begin(), key.end()); 
  }
};

/// searchForAssignment - Look for a cached solution for a query.
///
/// \param key - The query to look up.
/// \param result [out] - The cached result, if the lookup is succesful. This is
/// either a satisfying assignment (for a satisfiable query), or 0 (for an
/// unsatisfiable query).
/// \return - True if a cached result was found.
bool CexCachingSolver::searchForAssignment(KeyType &key, Assignment *&result) {
  Assignment * const *lookup = cache.lookup(key);
  if (lookup) {
    result = *lookup;
    return true;
  }

  if (CexCacheTryAll) {
    // Look for a satisfying assignment for a superset, which is trivially an
    // assignment for any subset.
    Assignment **lookup = cache.findSuperset(key, NonNullAssignment());
    
    // Otherwise, look for a subset which is unsatisfiable, see below.
    if (!lookup) 
      lookup = cache.findSubset(key, NullAssignment());

    // If either lookup succeeded, then we have a cached solution.
    if (lookup) {
      result = *lookup;
      return true;
    }

    // Otherwise, iterate through the set of current assignments to see if one
    // of them satisfies the query.
    for (assignmentsTable_ty::iterator it = assignmentsTable.begin(), 
           ie = assignmentsTable.end(); it != ie; ++it) {
      Assignment *a = *it;
      if (a->satisfies(key.begin(), key.end())) {
        result = a;
        return true;
      }
    }
  } else {
    // FIXME: Which order? one is sure to be better.

    // Look for a satisfying assignment for a superset, which is trivially an
    // assignment for any subset.
    Assignment **lookup = cache.findSuperset(key, NonNullAssignment());

    // Otherwise, look for a subset which is unsatisfiable -- if the subset is
    // unsatisfiable then no additional constraints can produce a valid
    // assignment. While searching subsets, we also explicitly the solutions for
    // satisfiable subsets to see if they solve the current query and return
    // them if so. This is cheap and frequently succeeds.
    if (!lookup) 
      lookup = cache.findSubset(key, NullOrSatisfyingAssignment(key));

    // If either lookup succeeded, then we have a cached solution.
    if (lookup) {
      result = *lookup;
      return true;
    }
  }
  
  return false;
}

bool
CexCachingSolver::getFromQuickCache(const KeyType &key, Assignment * &result){
	long comboHash = 0;
	for(KeyType::const_iterator it = key.begin(); it != key.end(); it++){
		comboHash += (*it).get()->hash();
	}

	if(quickCache.count(comboHash)){
		AssignmentBundle * bundle = quickCache[comboHash];
		return bundle->get(key, result);
	}
	result = 0;
	return false;
}

void
CexCachingSolver::insertInQuickCache(const KeyType &key, Assignment * &binding){
	long comboHash = 0;
	for(KeyType::iterator it = key.begin(); it != key.end(); it++){
		comboHash += (*it).get()->hash();
	}

	AssignmentBundle * b = quickCache[comboHash];
	if(! b){
		b = new AssignmentBundle();
		quickCache[comboHash] = b;
	}

	b->put(key, binding);
}

void
CexCachingSolver::insertInCaches(const KeyType &key, Assignment * &binding){
	insertInQuickCache(key, binding);
	cache.insert(key, binding);
}

bool CexCachingSolver::quickMatch(const Query &query,
								  const KeyType &key,
								  Assignment *&result) {
	if(getFromQuickCache(key, result)){
		return true;
	}
	result = 0;
	return false;
}

bool CexCachingSolver::checkPreviousSolutionHelper(const ref<Expr> queryExpr,	//If anything other than query.expr, then negated.
										   const std::set<ref<Expr> > &key,
										   Assignment * &result){
	Assignment * parentSolution;
	if(getFromQuickCache(key, parentSolution)){
		if(!parentSolution){
			//means that the the previous state was UNSAT and therefore the
			//new answer will also be necessarily UNSAT
			assert(!result);
			return true;
		}else{
			/*
			 * Means that there is in fact a parent solution.  In this case,
			 * we can now check whether this parent solution satisfies the
			 * child state.  There's a pretty good chance... at least 50/50
			 */
			ref<Expr> neg = Expr::createIsZero(queryExpr);
			ref<Expr> q = parentSolution->evaluate(neg);

			assert(isa<ConstantExpr>(q) && "assignment evaluation did not result in constant");
			if(cast<ConstantExpr>(q)->isTrue()){
				result = parentSolution;
				return true;
			}else{
				/*
				 * If goes false, that means that the point that had gotten us to our parent
				 * went along the opposing branch and it won't help us at this stage.  The
				 * value stored in oldAnswer could help someone though.
				 */
				assert(!result);
				return false;
			}
		}
	}
	assert(!parentSolution);
	assert(!result);
	return false;
}

bool CexCachingSolver::checkPreviousSolution(const Query &query,
										  Assignment *&result){
	if(query.constraints.size() == 0){
		return false;
	}

	std::set<ref<Expr> > parentKey;
	ref<Expr> queryExpr;
	if(isa<ConstantExpr>(query.expr)){
		assert(cast<ConstantExpr>(query.expr)->isFalse() && "query.expr == true should'd happen");
		for(unsigned i = 0; i < query.constraints.size() - 1; i ++){
			ref<Expr> ref = query.constraints.get(i);
			parentKey.insert(ref);
		}

        ref<Expr> toNeg = query.constraints.get(query.constraints.size() - 1);
        queryExpr = Expr::createIsZero(toNeg);
	}else{
		for(unsigned i = 0; i < query.constraints.size(); i ++){
			ref<Expr> ref = query.constraints.get(i);
			parentKey.insert(ref);
		}
		queryExpr = query.expr;
	}

	if(checkPreviousSolutionHelper(queryExpr, parentKey, result)){
		/*
		 * result may contain one of two things
		 * 	- A 0, meaning that the previous piece of the was UNSAT and therefore new is too
		 *	  (I put an assertion in checkPrevHelper to discount this case, but being careful)
		 * 	- An actual result which we should verify is correct.
		 */
		return true;
	}else{
		return false;
	}
}


/// lookupAssignment - Lookup a cached result for the given \arg query.
///
/// \param query - The query to lookup.
/// \param key [out] - On return, the key constructed for the query.
/// \param result [out] - The cached result, if the lookup is succesful. This is
/// either a satisfying assignment (for a satisfiable query), or 0 (for an
/// unsatisfiable query).
/// \return True if a cached result was found.
bool CexCachingSolver::lookupAssignment(const Query &query, 
                                        KeyType &key,
                                        Assignment *&result) {
  key = KeyType(query.constraints.begin(), query.constraints.end());
  ref<Expr> neg = Expr::createIsZero(query.expr);
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(neg)) {
    if (CE->isFalse()) {
      result = (Assignment*) 0;
      ++stats::queryCexCacheHits;
      return true;
    }
  } else {
    key.insert(neg);
  }

  bool found = quickMatch(query, key, result);

  if(!found){
  		found = checkPreviousSolution(query, result);
  		if(found){
  			insertInQuickCache(key, result);
  		}
  }

  if(!found){
	  found = searchForAssignment(key, result);
	  if(found){
		  insertInQuickCache(key, result);
	  }
  }

  if (found){
    ++stats::queryCexCacheHits;
  }else{
	  ++stats::queryCexCacheMisses;
  }
    
  return found;
}

bool CexCachingSolver::getAssignment(const Query& query, Assignment *&result) {
  KeyType key;
  if (lookupAssignment(query, key, result))
    return true;

  std::vector<const Array*> objects;
  findSymbolicObjects(key.begin(), key.end(), objects);

  std::vector< std::vector<unsigned char> > values;
  bool hasSolution;
  if (!solver->impl->computeInitialValues(query, objects, values, 
                                          hasSolution))
    return false;
    
  Assignment *binding;
  if (hasSolution) {
    binding = new Assignment(objects, values);

    // Memoize the result.
    std::pair<assignmentsTable_ty::iterator, bool>
      res = assignmentsTable.insert(binding);
    if (!res.second) {
      delete binding;
      binding = *res.first;
    }
    
    if (DebugCexCacheCheckBinding)
      assert(binding->satisfies(key.begin(), key.end()));
  } else {
    binding = (Assignment*) 0;
  }
  
  result = binding;
  insertInCaches(key, binding);

  return true;
}

///

CexCachingSolver::~CexCachingSolver() {
  cache.clear();
  delete solver;
  for (assignmentsTable_ty::iterator it = assignmentsTable.begin(), 
         ie = assignmentsTable.end(); it != ie; ++it)
    delete *it;
}

bool CexCachingSolver::computeValidity(const Query& query,
                                       Solver::Validity &result) {
  TimerStatIncrementer t(stats::cexCacheTime);
  Assignment *a;
  if (!getAssignment(query.withFalse(), a))
    return false;
  assert(a && "computeValidity() must have assignment");
  ref<Expr> q = a->evaluate(query.expr);
  assert(isa<ConstantExpr>(q) && 
         "assignment evaluation did not result in constant");

  if (cast<ConstantExpr>(q)->isTrue()) {
    if (!getAssignment(query, a))
      return false;
    result = !a ? Solver::True : Solver::Unknown;
  } else {
    if (!getAssignment(query.negateExpr(), a))
      return false;
    result = !a ? Solver::False : Solver::Unknown;
  }
  
  return true;
}

bool CexCachingSolver::computeTruth(const Query& query,
                                    bool &isValid) {
  TimerStatIncrementer t(stats::cexCacheTime);

  // There is a small amount of redundancy here. We only need to know
  // truth and do not really need to compute an assignment. This means
  // that we could check the cache to see if we already know that
  // state ^ query has no assignment. In that case, by the validity of
  // state, we know that state ^ !query must have an assignment, and
  // so query cannot be true (valid). This does get hits, but doesn't
  // really seem to be worth the overhead.

  if (CexCacheExperimental) {
    Assignment *a;
    if (lookupAssignment(query.negateExpr(), a) && !a)
      return false;
  }

  Assignment *a;
  if (!getAssignment(query, a))
    return false;

  isValid = !a;

  return true;
}

bool CexCachingSolver::computeValue(const Query& query,
                                    ref<Expr> &result) {
  TimerStatIncrementer t(stats::cexCacheTime);

  Assignment *a;
  if (!getAssignment(query.withFalse(), a))
    return false;
  assert(a && "computeValue() must have assignment");
  result = a->evaluate(query.expr);  
  assert(isa<ConstantExpr>(result) && 
         "assignment evaluation did not result in constant");
  return true;
}

bool 
CexCachingSolver::computeInitialValues(const Query& query,
                                       const std::vector<const Array*>  &objects,
									   std::vector< std::vector<unsigned char> > &values,
                                       bool &hasSolution) {
  TimerStatIncrementer t(stats::cexCacheTime);
  Assignment *a;
  if (!getAssignment(query, a))
    return false;
  hasSolution = !!a;
  
  if (!a)
    return true;

  // FIXME: We should use smarter assignment for result so we don't
  // need redundant copy.
  values = std::vector< std::vector<unsigned char> >(objects.size());
  for (unsigned i=0; i < objects.size(); ++i) {
    const Array *os = objects[i];
    Assignment::bindings_ty::iterator it = a->bindings.find(os);
    
    if (it == a->bindings.end()) {
      values[i] = std::vector<unsigned char>(os->size, 0);
    } else {
      values[i] = it->second;
    }
  }
  
  return true;
}

SolverImpl::SolverRunStatus CexCachingSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}

char *CexCachingSolver::getConstraintLog(const Query& query) {
  return solver->impl->getConstraintLog(query);
}

void CexCachingSolver::setCoreSolverTimeout(double timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

///

Solver *klee::createCexCachingSolver(Solver *_solver) {
  return new Solver(new CexCachingSolver(_solver));
}
