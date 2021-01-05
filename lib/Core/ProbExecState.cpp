#include "ProbExecState.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"
#include <memory.h>

using namespace klee;
using namespace llvm;

ProbExecState::ProbExecState(std::string data, uint32_t stateId)
    : data{data}, stateId{stateId} {}

ProbExecState::ProbExecState(std::string data, uint32_t stateId,
                             ETreeNodePtr treeNode)
    : data{data}, stateId{stateId}, treeNode{treeNode} {}

ProbExecState::ProbExecState(bool forkflag, std::string data, uint32_t stateId,
                             uint32_t assemblyLine, uint32_t codeLine,
                             ETreeNodePtr treeNode)
    : forkflag{forkflag}, data{data}, stateId{stateId}, treeNode{treeNode} {}