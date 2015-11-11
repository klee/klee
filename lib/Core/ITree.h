/*
 * ITree.h
 *
 *  Created on: Oct 15, 2015
 *      Author: felicia
 */

#ifndef ITREE_H_
#define ITREE_H_

#include <klee/Expr.h>
#include "klee/ExecutionState.h"

enum Status { NoInterpolant, HalfInterpolant, FullInterpolant};
enum Operation { Add, Sub, Mul, UDiv, SDiv, URem, SRem, And, Or, Xor, Shl, LShr, AShr};
enum Comparison {Eq, Ne, Ult, Ule, Ugt, Uge, Slt, Sle, Sgt, Sge, Neg, Not};

namespace klee {
    class ExecutionState;
	struct PathCondition{
    	ref<Expr> base;
		ref<Expr> value;
		Operation operationName;
	};
	struct BranchCondition{
		ref<Expr> base;
		ref<Expr> value;
		Comparison compareName;
	};
	struct Subsumption{
		unsigned int * programPoint;
		ref<Expr> interpolant;
	};

	class ITree{
	    typedef ExecutionState* data_type;
		typedef std::vector< ref<Expr> > vectorExpr_type;
		typedef vectorExpr_type::iterator iterator;
		typedef vectorExpr_type::const_iterator const_iterator;

	public:
	    typedef class ITreeNode INode;
		INode *root;

		ITree(const data_type &_root);
		~ITree();

		std::pair<INode*, INode*> split(INode *n,
	                                 const data_type  &leftData,
	                                 const data_type  &rightData);
	    void addCondition(INode *n, ref<Expr>);
	    ITreeNode *currentINode;
	    std::vector<Subsumption> subsumptionStore;

	};

	class ITreeNode{
		friend class ITree;
		typedef ref<Expr> expression_type;
		typedef std::pair <expression_type, expression_type> pair_type;

	public:
		typedef class PathCondition pathCond;
		unsigned int * programPoint;
		ITreeNode *parent, *left, *right;
	    ExecutionState *data;
	    std::vector< ref<Expr> > conditions;
	    std::vector< PathCondition > newPathConds;
	    std::vector< PathCondition > allPathConds;
	    ref<Expr> interpolant;
	    Status InterpolantStatus;
	    bool isSubsumed;
	    std::vector <pair_type> variablesTracking;
	    BranchCondition latestBranchCond;

	private:
	    ITreeNode(ITreeNode *_parent, ExecutionState *_data);
	    ~ITreeNode();
	};

}
#endif /* ITREE_H_ */
