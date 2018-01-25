/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**
 * \brief Solve an instance of the "Variable Placement with Separation
 * Constraints" problem.
 *
 * Authors:
 *   Tim Dwyer <tgdwyer@gmail.com>
 *
 * Copyright (C) 2005 Authors
 * 
 * This version is released under the CPL (Common Public License) with
 * the Graphviz distribution.
 * A version is also available under the LGPL as part of the Adaptagrams
 * project: http://sourceforge.net/projects/adaptagrams.  
 * If you make improvements or bug fixes to this code it would be much
 * appreciated if you could also contribute those changes back to the
 * Adaptagrams repository.
 */
#ifndef SEEN_REMOVEOVERLAP_SOLVE_VPSC_H
#define SEEN_REMOVEOVERLAP_SOLVE_VPSC_H

#include <vector>
class Variable;
class Constraint;
class Blocks;

/**
 * Variable Placement with Separation Constraints problem instance
 */
class VPSC {
public:
	virtual void satisfy();
	virtual void solve();

	VPSC(const unsigned n, Variable *vs[], const unsigned m, Constraint *cs[]);
	virtual ~VPSC();
protected:
	Blocks *bs;
	Constraint **cs;
	unsigned m;
	void printBlocks();
private:
	void refine();
	bool constraintGraphIsCyclic(const unsigned n, Variable *vs[]);
	bool blockGraphIsCyclic();
};

class IncVPSC : VPSC {
public:
	unsigned splitCnt;
	void satisfy();
	void solve();
	void moveBlocks();
	void splitBlocks();
	IncVPSC(const unsigned n, Variable *vs[], const unsigned m, Constraint *cs[]);
private:
	typedef std::vector<Constraint*> ConstraintList;
	ConstraintList inactive;
	double mostViolated(ConstraintList &l,Constraint* &v);
};
#endif // SEEN_REMOVEOVERLAP_SOLVE_VPSC_H
