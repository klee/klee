/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**
 * \brief A constraint determines a minimum or exact spacing required between
 * two variables.
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

#ifndef SEEN_REMOVEOVERLAP_CONSTRAINT_H
#define SEEN_REMOVEOVERLAP_CONSTRAINT_H

#include <iostream>
#include "variable.h"

class Constraint
{
	friend std::ostream& operator <<(std::ostream &os,const Constraint &c);
public:
	Variable *left;
	Variable *right;
	double gap;
	double lm;
	Constraint(Variable *left, Variable *right, double gap, bool equality=false);
	~Constraint();
	inline double slack() const { return right->position() - gap - left->position(); }
	long timeStamp;
	bool active;
	bool visited;
	bool equality;
};
#include <float.h>
#include "block.h"
static inline bool compareConstraints(Constraint *const &l, Constraint *const &r) {
	double const sl = 
		l->left->block->timeStamp > l->timeStamp
		||l->left->block==l->right->block
		?-DBL_MAX:l->slack();
	double const sr = 
		r->left->block->timeStamp > r->timeStamp
		||r->left->block==r->right->block
		?-DBL_MAX:r->slack();
	if(sl==sr) {
		// arbitrary choice based on id
		if(l->left->id==r->left->id) {
			if(l->right->id<r->right->id) return true;
			return false;
		}
		if(l->left->id<r->left->id) return true;
		return false;
	}
	return sl < sr;
}

#endif // SEEN_REMOVEOVERLAP_CONSTRAINT_H
