/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**
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
#ifndef SEEN_REMOVEOVERLAP_VARIABLE_H
#define SEEN_REMOVEOVERLAP_VARIABLE_H

#include <vector>
#include <iostream>
class Block;
class Constraint;
#include "block.h"

typedef std::vector<Constraint*> Constraints;
class Variable
{
	friend std::ostream& operator <<(std::ostream &os, const Variable &v);
public:
	const int id; // useful in log files
	double desiredPosition;
	const double weight;
	double offset;
	Block *block;
	bool visited;
	Constraints in;
	Constraints out;
	char *toString();
	inline Variable(const int id, const double desiredPos, const double weight)
		: id(id)
		, desiredPosition(desiredPos)
		, weight(weight)
		, offset(0)
		, visited(false)
	{
	}
	inline double position() const {
		return block->posn+offset;
	}
	//double position() const;
	~Variable(void){
		in.clear();
		out.clear();
	}
};
#endif // SEEN_REMOVEOVERLAP_VARIABLE_H
