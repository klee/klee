/*
 * IndependenceAnalysis.h
 *
 *  Created on: Jan 29, 2015
 *      Author: ericrizzi
 */

#ifndef KLEE_UTIL_INDEPENDENCEANALYSIS_H_
#define KLEE_UTIL_INDEPENDENCEANALYSIS_H_

#include "klee/Solver.h"

#include "klee/Expr.h"
#include "klee/Constraints.h"
#include "klee/SolverImpl.h"
#include "klee/Internal/Support/Debug.h"
#include "klee/Common.h"

#include "klee/util/ExprUtil.h"

#include "llvm/Support/raw_ostream.h"
#include <map>
#include <vector>
#include <ostream>
#include <list>

using namespace klee;
using namespace llvm;


template<class T>
class DenseSet {
	typedef std::set<T> set_ty;

public:
	set_ty s;

	DenseSet(){}

	void add(T x){
		s.insert(x);
	}

	void add(T start, T end) {
		for (; start<end; start++)
			s.insert(start);
	}

	// returns true iff set is changed by addition
	bool add(const DenseSet &b){
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

	bool intersects(const DenseSet &b){
		for (typename set_ty::iterator it = s.begin(), ie = s.end();
				it != ie; ++it)
			if (b.s.count(*it))
				return true;
		return false;
	}

	std::set<unsigned>::iterator begin(){
	    return s.begin();
	}

	std::set<unsigned>::iterator end(){
		return s.end();
	}

	void print(llvm::raw_ostream &os) const{
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

template <class T>
inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const ::DenseSet<T> &dis) {
	dis.print(os);
	return os;
}

/*
 * Keeps track of all reads in a single Constraint.  Maintains
 * a list of indices that are accessed in a concrete way.  This
 * can be superseded, however, by a set of arrays (wholeObjects)
 * that have been symbolically accessed.
 */
class IndependentElementSet {


public:
	typedef std::map<const Array*, ::DenseSet<unsigned> > elements_ty;

	elements_ty elements;
	std::set<const Array*> wholeObjects;	//Pretty sure represents symbolic accessed arrays
	std::vector<ref<Expr> > exprs;

	IndependentElementSet();
	IndependentElementSet(ref<Expr> e);
	IndependentElementSet(const IndependentElementSet &ies);

	void print(llvm::raw_ostream &os) const;

	// more efficient when this is the smaller set
	bool intersects(const IndependentElementSet &b);

	// returns true iff set is changed by addition
	bool add(const IndependentElementSet &b);

	IndependentElementSet &operator=(const IndependentElementSet &ies) {
		elements = ies.elements;
		wholeObjects = ies.wholeObjects;

		for(unsigned i = 0; i < ies.exprs.size(); i ++){
			ref<Expr> expr = ies.exprs[i];
			exprs.push_back(expr);
		}

		return *this;
	}
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const IndependentElementSet &ies) {
	ies.print(os);
	return os;
}

void getAllFactors(const Query& query, std::list<IndependentElementSet> * &factors );

void calculateArrays(const IndependentElementSet & ie, std::vector<const Array *> &returnVector);

IndependentElementSet getFreshFactor(const Query& query, std::vector<ref<Expr> > &result);

#endif /* KLEE_UTIL_INDEPENDENCEANALYSIS_H_ */
