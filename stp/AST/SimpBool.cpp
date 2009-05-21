/********************************************************************
 * AUTHORS: Vijay Ganesh, David L. Dill
 *
 * BEGIN DATE: April, 2006
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/

// -*- c++ -*-

// Simplifying create methods for Boolean operations.
// These are only very simple local simplifications.

// This is somewhat redundant with Vijay's simplifier code.  They
// need to be merged.
// FIXME: control with optimize flag.

static bool _trace_simpbool = 0;
static bool _disable_simpbool = 0;

#include "AST.h"

// SMTLIB experimental hack.  Try allocating a single stack here for
// children to reduce growing of vectors.
//BEEV::ASTVec child_stack;

namespace BEEV {

  ASTNode BeevMgr::CreateSimpForm(Kind kind, ASTVec &children = _empty_ASTVec) {
    if (_disable_simpbool) {
      return CreateNode(kind, children);
    }
    else {
      switch (kind) {
      case NOT: return CreateSimpNot(children[0]); break; 
      case AND: return CreateSimpAndOr(1, children); break; 
      case OR: return CreateSimpAndOr(0, children); break;
      case NAND: return CreateSimpNot(CreateSimpAndOr(1, children)); break;
      case NOR: return CreateSimpNot(CreateSimpAndOr(0, children)); break;
      case IFF: {
	// Not sure children can ever be empty, but what the heck.
	//	if (children.size() == 0) {
	//	  return ASTTrue;
	//	}
	// Convert IFF to XOR ASAP.  IFF is not associative, so this makes
	// flattening much easier.
	children[0] = CreateSimpNot(children[0]);
	return CreateSimpXor(children); break;
      }
      case XOR: 
	return CreateSimpXor(children); break;
	// FIXME: Earlier, check that this only has two arguments
      case IMPLIES: return CreateSimpAndOr(0, CreateSimpNot(children[0]), children[1]); break;
      case ITE: return CreateSimpFormITE(children[0], children[1], children[2]);
      default: return CreateNode(kind, children);
      }
    }
  }

  // specialized versions
  
  ASTNode BeevMgr::CreateSimpForm(Kind kind,
				  const ASTNode& child0) {
    ASTVec children;
    //child_stack.clear();	// could just reset top pointer.
    children.push_back(child0);
    //child_stack.push_back(child0);
    return CreateSimpForm(kind, children);
    //return CreateSimpForm(kind, child_stack);
  }
  
  ASTNode BeevMgr::CreateSimpForm(Kind kind,
				  const ASTNode& child0,
				  const ASTNode& child1) {
    ASTVec children;
    //child_stack.clear();	// could just reset top pointer.
    children.push_back(child0);
    //child_stack.push_back(child0);
    children.push_back(child1);
    //child_stack.push_back(child1);
    return CreateSimpForm(kind, children);
    //return CreateSimpForm(kind, child_stack);
  }
  
  
  ASTNode BeevMgr::CreateSimpForm(Kind kind,
				  const ASTNode& child0,
				  const ASTNode& child1,
				  const ASTNode& child2) {
    ASTVec children;
    //child_stack.clear();	// could just reset top pointer.
    children.push_back(child0);
    //child_stack.push_back(child0);
    children.push_back(child1);
    //child_stack.push_back(child1);
    children.push_back(child2);
    //child_stack.push_back(child2);
    return CreateSimpForm(kind, children);
    //return CreateSimpForm(kind, child_stack);
  }
    
  ASTNode BeevMgr::CreateSimpNot(const ASTNode& form) {
    Kind k = form.GetKind();
    switch (k) {
    case FALSE: { return ASTTrue; }
    case TRUE: { return ASTFalse; }
    case NOT: { return form[0]; } // NOT NOT cancellation
    case XOR: {
      // Push negation down in this case.
      // FIXME: Separate pre-pass to push negation down?
      // CreateSimp should be local, and this isn't.  
      // It isn't memoized.  Arg.
      ASTVec children = form.GetChildren();
      children[0] = CreateSimpNot(children[0]);
      return CreateSimpXor(children);
    }
    default: { return CreateNode(NOT, form); }
    }
  }

  // I don't think this is even called, since it called
  // CreateSimpAndOr instead of CreateSimpXor until 1/9/07 with no
  // ill effects.  Calls seem to go to the version that takes a vector
  // of children.
  ASTNode BeevMgr::CreateSimpXor(const ASTNode& form1, const ASTNode& form2) {
    ASTVec children;
    children.push_back(form1);
    children.push_back(form2);
    return CreateSimpXor(children);
  }


  ASTNode BeevMgr::CreateSimpAndOr(bool IsAnd, const ASTNode& form1, const ASTNode& form2) {
    ASTVec children;
    children.push_back(form1);
    children.push_back(form2);
    return CreateSimpAndOr(IsAnd, children);
  }

  ASTNode BeevMgr::CreateSimpAndOr(bool IsAnd, ASTVec &children) {

    if (_trace_simpbool) {
      cout << "========" << endl << "CreateSimpAndOr " << (IsAnd ? "AND " : "OR ") ;
      lpvec(children);
      cout << endl;
    }

    ASTVec new_children;

    // sort so that identical nodes occur in sequential runs, followed by
    // their negations.

    SortByExprNum(children);
    
    ASTNode annihilator = (IsAnd ? ASTFalse : ASTTrue);
    ASTNode identity = (IsAnd ? ASTTrue : ASTFalse);

    ASTNode retval;

    ASTVec::const_iterator it_end = children.end();
    ASTVec::const_iterator next_it;
    for(ASTVec::const_iterator it = children.begin(); it != it_end; it = next_it) {
      next_it = it + 1;
      bool nextexists = (next_it < it_end);

      if (*it == annihilator) {
	retval = annihilator;
	if (_trace_simpbool) {
	  cout << "returns " << retval << endl;
	}
	return retval;
      }
      else if (*it == identity) {
	// just drop it
      }
      else if (nextexists && (*next_it == *it)) {
	// drop it
	//	cout << "Dropping [" << it->GetNodeNum() << "]" << endl;
      }
      else if (nextexists && (next_it->GetKind() == NOT) && ((*next_it)[0] == *it)) {
	// form and negation -- return FALSE for AND, TRUE for OR.
	retval = annihilator;
	// cout << "X and/or NOT X" << endl; 
	if (_trace_simpbool) {
	  cout << "returns " << retval << endl;
	}
	return retval;
      }
      else {
	// add to children
	new_children.push_back(*it);
      }
    }

    // If we get here, we saw no annihilators, and children should
    // be only the non-True nodes.
    if (new_children.size() < 2) {
      if (0 == new_children.size()) {
	retval = identity;
      }
      else {
	// there is just one child
	retval = new_children[0];
      }
    }
    else {
      // 2 or more children.  Create a new node.
      retval = CreateNode(IsAnd ? AND : OR, new_children);
    }
    if (_trace_simpbool) {
      cout << "returns " << retval << endl;
    }
    return retval;
  }


  // Constant children are accumulated in "accumconst".  
  ASTNode BeevMgr::CreateSimpXor(ASTVec &children) {

    if (_trace_simpbool) {
      cout << "========" << endl
	   << "CreateSimpXor ";
      lpvec(children);
      cout << endl;
    }

    // Change this not to init to children if flattening code is present.
    // ASTVec flat_children = children;		// empty vector

    ASTVec flat_children;		// empty vector

    ASTVec::const_iterator it_end = children.end();

    if (xor_flatten) {

      bool fflag = 0;		// ***Temp debugging
      
      // Experimental flattening code.
      
      for(ASTVec::iterator it = children.begin(); it != it_end; it++) {
	Kind ck = it->GetKind();
	const ASTVec &gchildren = it->GetChildren();
	if (XOR == ck) {
	  fflag = 1;
	  // append grandchildren to children
	  flat_children.insert(flat_children.end(), gchildren.begin(), gchildren.end());
	}
	else {
	  flat_children.push_back(*it);
	}
      }
      
      if (_trace_simpbool && fflag) {
	cout << "========" << endl;
	cout << "Flattening: " << endl;
	lpvec(children);
	
	cout << "--------" << endl;
	cout << "Flattening result: " << endl;
	lpvec(flat_children);
      }
    }
    else {
      flat_children = children;
    }
      

    // sort so that identical nodes occur in sequential runs, followed by
    // their negations.
    SortByExprNum(flat_children);

    ASTNode retval;

    // This is the C Boolean value of all constant args seen.  It is initially
    // 0.  TRUE children cause it to change value.
    bool accumconst = 0;

    ASTVec new_children;

    it_end = flat_children.end();
    ASTVec::iterator next_it;
    for(ASTVec::iterator it = flat_children.begin(); it != it_end; it++) {
      next_it = it + 1;
      bool nextexists = (next_it < it_end);

      if (ASTTrue == *it) {
	accumconst = !accumconst;
      }
      else if (ASTFalse == *it) {
	// Ignore it
      }
      else if (nextexists && (*next_it == *it)) {
	// x XOR x = FALSE.  Skip current, write "false" into next_it
	// so that it gets tossed, too.
	*next_it = ASTFalse;
      }
      else if (nextexists && (next_it->GetKind() == NOT) && ((*next_it)[0] == *it)) {
	// x XOR NOT x = TRUE.  Skip current, write "true" into next_it
	// so that it gets tossed, too.
	*next_it = ASTTrue;
      }
      else if (NOT == it->GetKind()) {
	// If child is (NOT alpha), we can flip accumconst and use alpha.
	// This is ok because (NOT alpha) == TRUE XOR alpha
	accumconst = !accumconst;
	// CreateSimpNot just takes child of not.
	new_children.push_back(CreateSimpNot(*it));
      }
      else {
	new_children.push_back(*it);
      }
    }

    // Children should be non-constant.
    if (new_children.size() < 2) {
      if (0 == new_children.size()) {
	// XOR(TRUE, FALSE) -- accumconst will be 1.
	if (accumconst) {
	  retval = ASTTrue;
	}
	else {
	  retval = ASTFalse;
	}
      }
      else {
	// there is just one child
	// XOR(x, TRUE) -- accumconst will be 1.
	if (accumconst) {
	  retval =  CreateSimpNot(new_children[0]);
	}
	else {
	  retval = new_children[0];
	}
      }
    }
    else {
      // negate first child if accumconst == 1
      if (accumconst) {
	new_children[0] = CreateSimpNot(new_children[0]);
      }
      retval = CreateNode(XOR, new_children);
    }

    if (_trace_simpbool) {    
      cout << "returns " << retval << endl;
    }
    return retval;
  }

  // FIXME:  How do I know whether ITE is a formula or not?
  ASTNode BeevMgr::CreateSimpFormITE(const ASTNode& child0,
				     const ASTNode& child1,
				     const ASTNode& child2) {

    ASTNode retval;
    
    if (_trace_simpbool) {
      cout << "========" << endl << "CreateSimpFormITE "
	   << child0 
	   << child1 
	   << child2 << endl;
    }

    if (ASTTrue == child0) {
      retval = child1;
    }
    else if (ASTFalse == child0) {
      retval = child2;
    }
    else if (child1 == child2) {
      retval = child1;
    }    
    // ITE(x, TRUE, y ) == x OR y
    else if (ASTTrue == child1) {
      retval = CreateSimpAndOr(0, child0, child2);
    }
    // ITE(x, FALSE, y ) == (!x AND y)
    else if (ASTFalse == child1) {
      retval = CreateSimpAndOr(1, CreateSimpNot(child0), child2); 
    }
    // ITE(x, y, TRUE ) == (!x OR y)
    else if (ASTTrue == child2) {
      retval = CreateSimpAndOr(0, CreateSimpNot(child0), child1); 
    }
    // ITE(x, y, FALSE ) == (x AND y)
    else if (ASTFalse == child2) {
      retval = CreateSimpAndOr(1, child0, child1); 
    }
    // ITE (x, !y, y) == x XOR y
//     else if (NOT == child1.GetKind() && (child1[0] == child2)) {
//       retval = CreateSimpXor(child0, child2);
//     }
//     // ITE (x, y, !y) == x IFF y.  I think other cases are covered
//     // by XOR/IFF optimizations
//     else if (NOT == child2.GetKind() && (child2[0] == child1)) {
//       retval = CreateSimpXor(CreateSimpNot(child0), child2);
//     }
    else {
      retval = CreateNode(ITE, child0, child1, child2);
    }

    if (_trace_simpbool) {
      cout << "returns " << retval << endl;
    }

    return retval;
  }
} // BEEV namespace
