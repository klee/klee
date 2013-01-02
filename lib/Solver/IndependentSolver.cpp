//===-- IndependentSolver.cpp ---------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver.h"

#include "klee/Expr.h"
#include "klee/Constraints.h"
#include "klee/SolverImpl.h"

#include "klee/util/ExprUtil.h"

#include <map>
#include <vector>
#include <ostream>
#include <iostream>

using namespace klee;
using namespace llvm;

template<class T>
class DenseSet {
  typedef std::set<T> set_ty;
  set_ty s;

public:
  DenseSet() {}

  void add(T x) {
    s.insert(x);
  }
  void add(T start, T end) {
    for (; start<end; start++)
      s.insert(start);
  }

  // returns true iff set is changed by addition
  bool add(const DenseSet &b) {
    bool modified = false;
    for (typename set_ty::const_iterator it = b.s.begin(), ie = b.s.end(); 
         it != ie; ++it) {
      if (modified || !s.count(*it)) {
        modified = true;
        s.insert(*it);
      }
    }
    return modified;
  }

  bool intersects(const DenseSet &b) {
    for (typename set_ty::iterator it = s.begin(), ie = s.end(); 
         it != ie; ++it)
      if (b.s.count(*it))
        return true;
    return false;
  }

  void print(std::ostream &os) const {
    bool first = true;
    os << "{";
    for (typename set_ty::iterator it = s.begin(), ie = s.end(); 
         it != ie; ++it) {
      if (first) {
        first = false;
      } else {
        os << ",";
      }
      os << *it;
    }
    os << "}";
  }
};

template<class T>
inline std::ostream &operator<<(std::ostream &os, const ::DenseSet<T> &dis) {
  dis.print(os);
  return os;
}

class IndependentElementSet {
  typedef std::map<const Array*, ::DenseSet<unsigned> > elements_ty;
  elements_ty elements;
  std::set<const Array*> wholeObjects;

public:
  IndependentElementSet() {}
  IndependentElementSet(ref<Expr> e) {
    std::vector< ref<ReadExpr> > reads;
    findReads(e, /* visitUpdates= */ true, reads);
    for (unsigned i = 0; i != reads.size(); ++i) {
      ReadExpr *re = reads[i].get();
      const Array *array = re->updates.root;
      
      // Reads of a constant array don't alias.
      if (re->updates.root->isConstantArray() &&
          !re->updates.head)
        continue;

      if (!wholeObjects.count(array)) {
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(re->index)) {
          ::DenseSet<unsigned> &dis = elements[array];
          dis.add((unsigned) CE->getZExtValue(32));
        } else {
          elements_ty::iterator it2 = elements.find(array);
          if (it2!=elements.end())
            elements.erase(it2);
          wholeObjects.insert(array);
        }
      }
    }
  }
  IndependentElementSet(const IndependentElementSet &ies) : 
    elements(ies.elements),
    wholeObjects(ies.wholeObjects) {}    

  IndependentElementSet &operator=(const IndependentElementSet &ies) {
    elements = ies.elements;
    wholeObjects = ies.wholeObjects;
    return *this;
  }

  void print(std::ostream &os) const {
    os << "{";
    bool first = true;
    for (std::set<const Array*>::iterator it = wholeObjects.begin(), 
           ie = wholeObjects.end(); it != ie; ++it) {
      const Array *array = *it;

      if (first) {
        first = false;
      } else {
        os << ", ";
      }

      os << "MO" << array->name;
    }
    for (elements_ty::const_iterator it = elements.begin(), ie = elements.end();
         it != ie; ++it) {
      const Array *array = it->first;
      const ::DenseSet<unsigned> &dis = it->second;

      if (first) {
        first = false;
      } else {
        os << ", ";
      }

      os << "MO" << array->name << " : " << dis;
    }
    os << "}";
  }

  // more efficient when this is the smaller set
  bool intersects(const IndependentElementSet &b) {
    for (std::set<const Array*>::iterator it = wholeObjects.begin(), 
           ie = wholeObjects.end(); it != ie; ++it) {
      const Array *array = *it;
      if (b.wholeObjects.count(array) || 
          b.elements.find(array) != b.elements.end())
        return true;
    }
    for (elements_ty::iterator it = elements.begin(), ie = elements.end();
         it != ie; ++it) {
      const Array *array = it->first;
      if (b.wholeObjects.count(array))
        return true;
      elements_ty::const_iterator it2 = b.elements.find(array);
      if (it2 != b.elements.end()) {
        if (it->second.intersects(it2->second))
          return true;
      }
    }
    return false;
  }

  // returns true iff set is changed by addition
  bool add(const IndependentElementSet &b) {
    bool modified = false;
    for (std::set<const Array*>::const_iterator it = b.wholeObjects.begin(), 
           ie = b.wholeObjects.end(); it != ie; ++it) {
      const Array *array = *it;
      elements_ty::iterator it2 = elements.find(array);
      if (it2!=elements.end()) {
        modified = true;
        elements.erase(it2);
        wholeObjects.insert(array);
      } else {
        if (!wholeObjects.count(array)) {
          modified = true;
          wholeObjects.insert(array);
        }
      }
    }
    for (elements_ty::const_iterator it = b.elements.begin(), 
           ie = b.elements.end(); it != ie; ++it) {
      const Array *array = it->first;
      if (!wholeObjects.count(array)) {
        elements_ty::iterator it2 = elements.find(array);
        if (it2==elements.end()) {
          modified = true;
          elements.insert(*it);
        } else {
          if (it2->second.add(it->second))
            modified = true;
        }
      }
    }
    return modified;
  }
};

inline std::ostream &operator<<(std::ostream &os, const IndependentElementSet &ies) {
  ies.print(os);
  return os;
}

static 
IndependentElementSet getIndependentConstraints(const Query& query,
                                                std::vector< ref<Expr> > &result) {
  IndependentElementSet eltsClosure(query.expr);
  std::vector< std::pair<ref<Expr>, IndependentElementSet> > worklist;

  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    worklist.push_back(std::make_pair(*it, IndependentElementSet(*it)));

  // XXX This should be more efficient (in terms of low level copy stuff).
  bool done = false;
  do {
    done = true;
    std::vector< std::pair<ref<Expr>, IndependentElementSet> > newWorklist;
    for (std::vector< std::pair<ref<Expr>, IndependentElementSet> >::iterator
           it = worklist.begin(), ie = worklist.end(); it != ie; ++it) {
      if (it->second.intersects(eltsClosure)) {
        if (eltsClosure.add(it->second))
          done = false;
        result.push_back(it->first);
      } else {
        newWorklist.push_back(*it);
      }
    }
    worklist.swap(newWorklist);
  } while (!done);

  if (0) {
    std::set< ref<Expr> > reqset(result.begin(), result.end());
    std::cerr << "--\n";
    std::cerr << "Q: " << query.expr << "\n";
    std::cerr << "\telts: " << IndependentElementSet(query.expr) << "\n";
    int i = 0;
  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it) {
      std::cerr << "C" << i++ << ": " << *it;
      std::cerr << " " << (reqset.count(*it) ? "(required)" : "(independent)") << "\n";
      std::cerr << "\telts: " << IndependentElementSet(*it) << "\n";
    }
    std::cerr << "elts closure: " << eltsClosure << "\n";
  }

  return eltsClosure;
}

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
                            bool &hasSolution) {
    return solver->impl->computeInitialValues(query, objects, values,
                                              hasSolution);
  }
  SolverRunStatus getOperationStatusCode();
};
  
bool IndependentSolver::computeValidity(const Query& query,
                                        Solver::Validity &result) {
  std::vector< ref<Expr> > required;
  IndependentElementSet eltsClosure =
    getIndependentConstraints(query, required);
  ConstraintManager tmp(required);
  return solver->impl->computeValidity(Query(tmp, query.expr), 
                                       result);
}

bool IndependentSolver::computeTruth(const Query& query, bool &isValid) {
  std::vector< ref<Expr> > required;
  IndependentElementSet eltsClosure = 
    getIndependentConstraints(query, required);
  ConstraintManager tmp(required);
  return solver->impl->computeTruth(Query(tmp, query.expr), 
                                    isValid);
}

bool IndependentSolver::computeValue(const Query& query, ref<Expr> &result) {
  std::vector< ref<Expr> > required;
  IndependentElementSet eltsClosure = 
    getIndependentConstraints(query, required);
  ConstraintManager tmp(required);
  return solver->impl->computeValue(Query(tmp, query.expr), result);
}

SolverImpl::SolverRunStatus IndependentSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();      
}

Solver *klee::createIndependentSolver(Solver *s) {
  return new Solver(new IndependentSolver(s));
}
