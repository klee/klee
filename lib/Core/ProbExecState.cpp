#include "ProbExecState.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Support/OptionCategories.h"


ProbExecState::ProbExecState(std::string data, uint32_t stateId) :
    data{data},
    stateId{stateId} {

}

ProbExecState::ProbExecState(std::string data, uint32_t stateId, ETreeNode* treeNode) :
    data{data},
    stateId{stateId}, 
    treeNode{ETreeNodePtr(treeNode)} {

}

ProbExecState::ProbExecState(bool forkflag, std::string data, uint32_t stateId, uint32_t assemblyLine, uint32_t codeLine, ETreeNode* treeNode) : 
    forkflag{forkflag}, 
    data{data}, 
    stateId{stateId}, 
    assemblyLine{assemblyLine}, 
    codeLine{codeLine}, 
    treeNode{ETreeNodePtr(treeNode) {

}