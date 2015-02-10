/*
 * IndependenceAnalysis.cpp
 *
 *  Created on: Jan 29, 2015
 *      Author: ericrizzi
 */

#include "IndependenceAnalysis.h"

/*
 * Keeps track of all reads in a single Constraint.  Maintains
 * a list of indices that are accessed in a concrete way.  This
 * can be superseded, however, by a set of arrays (wholeObjects)
 * that have been symbolically accessed.
 */
IndependentElementSet::IndependentElementSet() {}

IndependentElementSet::IndependentElementSet(ref<Expr> e) {
	// Track all reads in the program.  Determines whether reads are
	//concrete or symbolic.  If they are symbolic, "collapses" array
	//by adding it to wholeObjects.  Otherwise, creates a mapping of
	//the form Map<array, set<index>> which tracks which parts of the
	//array are being accessed
	exprs.push_back(e);
	std::vector< ref<ReadExpr> > reads;
	findReads(e, /* visitUpdates= */ true, reads);
	for (unsigned i = 0; i != reads.size(); ++i) {
		ReadExpr *re = reads[i].get();
		const Array *array = re->updates.root;

		// Reads of a constant array don't alias.
		if (re->updates.root->isConstantArray() &&
				!re->updates.head)
			continue;

		if (!wholeObjects.count(array)) { //if haven't been scrambled yet
			if (ConstantExpr *CE = dyn_cast<ConstantExpr>(re->index)) {
				//if index constant, then add to set of constraints operating
				//on that array (actually, don't add constraint, just set index)
				::DenseSet<unsigned> &dis = elements[array];
				dis.add((unsigned) CE->getZExtValue(32));
			} else {
				//means symbolic access
				elements_ty::iterator it2 = elements.find(array);
				if (it2!=elements.end())
					//if first time seen, erase set
					elements.erase(it2);
				wholeObjects.insert(array);
			}
		}
	}
}

IndependentElementSet::IndependentElementSet(const IndependentElementSet &ies) :
	elements(ies.elements),
	wholeObjects(ies.wholeObjects),
	exprs(ies.exprs){}

void IndependentElementSet::print(llvm::raw_ostream &os) const {
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
bool IndependentElementSet::intersects(const IndependentElementSet &b) {
	//If there are any symbolic arrays in our query that b accesses
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
		//if any of the elements we access are symbolic in b
		if (b.wholeObjects.count(array))
			return true;
		elements_ty::const_iterator it2 = b.elements.find(array);
		//Or if we access the same index as b
		if (it2 != b.elements.end()) {
			if (it->second.intersects(it2->second))
				return true;
		}
	}
	return false;
}

// returns true iff set is changed by addition
bool IndependentElementSet::add(const IndependentElementSet &b) {

	for(unsigned i = 0; i < b.exprs.size(); i ++){
		ref<Expr> expr = b.exprs[i];
		exprs.push_back(expr);
	}
	std::set<const Array*> seen;
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

/*
 * Breaks down a constraint into all of it's individual pieces, returning a
 * list of IndependentElementSets or the independent factors.
 */
void getAllFactors(const Query& query, std::list<IndependentElementSet> * &factors ){
	assert(factors && "should not pass in a null vector");
	ConstantExpr *CE = dyn_cast<ConstantExpr>(query.expr);

	/*
	 * If the query.expr is false, we can simply ignore it.  Otherwise, we need to
	 * negate it and add it to the list of factors.
	 */
	if(CE){
		assert(CE && CE->isFalse() && "the expr should always be false and therefore not included in factors");
	}else{
		ref<Expr> neg = Expr::createIsZero(query.expr);
		factors->push_back(IndependentElementSet(neg));
	}

	for (ConstraintManager::const_iterator it = query.constraints.begin(),
			ie = query.constraints.end(); it != ie; ++it)
		//iterate through all the previously separated constraints.  Until we
		//actually return, factors is treated as a queue of expressions to be
		//evaluated.  If the queue property isn't maintained, then the exprs
		//could be returned in an order different from how they came it, negatively
		//affecting later stages.
		factors->push_back(IndependentElementSet(*it));

	bool doneLoop = false;
	do {
		doneLoop = true;
		std::list<IndependentElementSet> * done = new std::list<IndependentElementSet>;
		while(factors->size() > 0){
			IndependentElementSet current = factors->front();
			factors->pop_front();
			std::set<const Array*> seen;

			//This list represents the set of factors that are separate from current.
			//Those that are not inserted into this list (queue) intersect with current.
			std::list<IndependentElementSet> *keep = new std::list<IndependentElementSet>;
			while(factors->size() > 0){
				IndependentElementSet compare = factors->front();
				factors->pop_front();
				if(current.intersects(compare)){
					if (current.add(compare)){
						/*
						 * Means that we have added (z=y)added to (x=y)
						 * Now need to see if there are any (z=?)'s
						 */
						doneLoop = false;
					}
				}else{
					keep->push_back(compare);
				}
			}
			done->push_back(current);
			delete factors;
			factors = keep;

		}
		delete factors;
		factors = done;
	}while(!doneLoop);
}

/*
 * Extracts which arrays are referenced from a particular independent set.  Examines both
 * the actual known array accesses arr[1] plus the undetermined accesses arr[x].
 */
void calculateArrays(const IndependentElementSet & ie, std::vector<const Array *> &returnVector){
	//Now need to some how extract the arrays from these pieces...
	std::set<const Array*> thisSeen;
	for(std::map<const Array*, ::DenseSet<unsigned> >::const_iterator it = ie.elements.begin(); it != ie.elements.end(); it ++){
		const Array* arr = it->first;
		thisSeen.insert(arr);
	}

	for(std::set<const Array *>::iterator it = ie.wholeObjects.begin(); it != ie.wholeObjects.end(); it ++){
		const Array* arr = * it;
		thisSeen.insert(arr);
	}

	for(std::set<const Array *>::iterator it = thisSeen.begin(); it != thisSeen.end(); it ++){
		const Array * arr = * it;
		returnVector.push_back(arr);
	}
}

/*
 * Returns a pair.
 * 	First: Contains the elements related to the new part of the query ("fresh" in Green terms)
 * 	Second: Contains a set of all of the independent factors in the entire query.
 *
 * 	The reason we need to add this second part (the set) is for getInitialValues
 *
 * 	Note: This could be improved with caching, but since it's C++,
 * 	it's still pretty damn fast.
 */
IndependentElementSet getFreshFactor(const Query& query,
		std::vector< ref<Expr> > &result) {
	IndependentElementSet eltsClosure(query.expr); //The new thing we're testing
	std::vector< std::pair<ref<Expr>, IndependentElementSet> > worklist;

	for (ConstraintManager::const_iterator it = query.constraints.begin(),
			ie = query.constraints.end(); it != ie; ++it)
		//iterate through all the previously separated constraints
		worklist.push_back(std::make_pair(*it, IndependentElementSet(*it)));

	// XXX This should be more efficient (in terms of low level copy stuff).
	bool done = false;
	do {
		done = true;
		//The pair below keeps track of <Expr, Set<Variables in Expression>>
		std::vector< std::pair<ref<Expr>, IndependentElementSet> > newWorklist;
		for (std::vector< std::pair<ref<Expr>, IndependentElementSet> >::iterator
				it = worklist.begin(), ie = worklist.end(); it != ie; ++it) {
			if (it->second.intersects(eltsClosure)) {
				if (eltsClosure.add(it->second))
					done = false;	//Means that we have added (z=y)added to (x=y)
				//Now need to see if there are any (z=?)'s
				result.push_back(it->first);
			} else {
				newWorklist.push_back(*it);
			}
		}
		worklist.swap(newWorklist);
	} while (!done);

	KLEE_DEBUG(
			std::set< ref<Expr> > reqset(result.begin(), result.end());
	errs() << "--\n";
	errs() << "Q: " << query.expr << "\n";
	errs() << "\telts: " << IndependentElementSet(query.expr) << "\n";
	int i = 0;
	for (ConstraintManager::const_iterator it = query.constraints.begin(),
			ie = query.constraints.end(); it != ie; ++it) {
		errs() << "C" << i++ << ": " << *it;
		errs() << " " << (reqset.count(*it) ? "(required)" : "(independent)") << "\n";
		errs() << "\telts: " << IndependentElementSet(*it) << "\n";
	}
	errs() << "elts closure: " << eltsClosure << "\n";
	);


	return eltsClosure;
}
