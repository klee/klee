//===-- SpecialFunctionHandler.h --------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SPECIALFUNCTIONHANDLER_H
#define KLEE_SPECIALFUNCTIONHANDLER_H

#include "llvm/IR/Type.h"
//#include "llvm/Target/TargetData.h"
#include "AddressSpace.h"
#include <iterator>
#include <map>
#include <vector>
#include <string>

namespace llvm {
  class Function;
}

namespace klee {

  class Executor;
  class Expr;
  class ExecutionState;
  struct KInstruction;
  template<typename T> class ref;
  
  class SpecialFunctionHandler {
  public:
    typedef void (SpecialFunctionHandler::*Handler)(ExecutionState &state,
                                                    KInstruction *target, 
                                                    std::vector<ref<Expr> > 
                                                      &arguments);
    typedef std::map<const llvm::Function*, 
                     std::pair<Handler,bool> > handlers_ty;

    handlers_ty handlers;
    class Executor &executor;

    struct HandlerInfo {
      const char *name;
      SpecialFunctionHandler::Handler handler;
      bool doesNotReturn; /// Intrinsic terminates the process
      bool hasReturnValue; /// Intrinsic has a return value
      bool doNotOverride; /// Intrinsic should not be used if already defined
    };

    // const_iterator to iterate over stored HandlerInfo
    // FIXME: Implement >, >=, <=, < operators
    class const_iterator : public std::iterator<std::random_access_iterator_tag, HandlerInfo>
    {
      private:
        value_type* base;
        int index;
      public:
      const_iterator(value_type* hi) : base(hi), index(0) {};
      const_iterator& operator++();  // pre-fix
      const_iterator operator++(int); // post-fix
      const value_type& operator*() { return base[index];}
      const value_type* operator->() { return &(base[index]);}
      const value_type& operator[](int i) { return base[i];}
      bool operator==(const_iterator& rhs) { return (rhs.base + rhs.index) == (this->base + this->index);}
      bool operator!=(const_iterator& rhs) { return !(*this == rhs);}
    };

    static const_iterator begin();
    static const_iterator end();
    static int size();
    uint64_t getValue(ExecutionState &state, ref<Expr> argument);
    std::vector<std::pair<ExecutionState*, ref<Expr> > > processNumber
    (ExecutionState *current_state, std::vector<ref<Expr> > numberbuf, Expr::Width numwidth,int ary, bool neg);
    static ref<Expr> IntCondGen(ref<Expr> bufferchar);
    static ref<Expr> OctCondGen(ref<Expr> bufferchar);
    static ref<Expr> HexCondGen(ref<Expr> bufferchar);
    void processScan(ExecutionState *current_state,Expr::Width w,ref<Expr> bufferchar,
    			ref<Expr> targetBuf,const int fileid, const ObjectPair& op, std::vector<ExecutionState*> *stateProcessed,
    			KInstruction *target, int ary,  ref<Expr> (*condFunc) (ref<Expr>));
    void processScanInt(ExecutionState *current_state,Expr::Width w,
    		ref<Expr> bufferchar, ref<Expr> targetBuf, const int fileid,
    		const ObjectPair& op, std::vector<ExecutionState*> *stateProcessed,
    		KInstruction *target);
    void processScanChar(ExecutionState *current_state,ref<Expr> bufferchar,
			ref<Expr> targetBuf);
    void processScanOct(ExecutionState *current_state,Expr::Width w,
    		ref<Expr> bufferchar, ref<Expr> targetBuf, const int fileid,
    		const ObjectPair& op, std::vector<ExecutionState*> *stateProcessed,
    		KInstruction *target);
    void processScanHex(ExecutionState *current_state,Expr::Width w,
    		ref<Expr> bufferchar, ref<Expr> targetBuf, const int fileid,
    		const ObjectPair& op, std::vector<ExecutionState*> *stateProcessed,
    		KInstruction *target);

  public:
    SpecialFunctionHandler(Executor &_executor);

    /// Perform any modifications on the LLVM module before it is
    /// prepared for execution. At the moment this involves deleting
    /// unused function bodies and marking intrinsics with appropriate
    /// flags for use in optimizations.
    void prepare();

    /// Initialize the internal handler map after the module has been
    /// prepared for execution.
    void bind();

    bool handle(ExecutionState &state, 
                llvm::Function *f,
                KInstruction *target,
                std::vector< ref<Expr> > &arguments);

    /* Convenience routines */

    std::string readStringAtAddress(ExecutionState &state, ref<Expr> address);
    
    /* Handlers */

#define HANDLER(name) void name(ExecutionState &state, \
                                KInstruction *target, \
                                std::vector< ref<Expr> > &arguments)
    HANDLER(handleAbort);
    HANDLER(handleAssert);
    HANDLER(handleAssertFail);
    HANDLER(handleAssume);
    HANDLER(handleCalloc);
    HANDLER(handleCheckMemoryAccess);
    HANDLER(handleDefineFixedObject);
    HANDLER(handleDelete);    
    HANDLER(handleDeleteArray);
    HANDLER(handleExit);
    HANDLER(handleAliasFunction);
    HANDLER(handleFree);
    HANDLER(handleGetErrno);
    HANDLER(handleGetObjSize);
    HANDLER(handleGetValue);
    HANDLER(handleIsSymbolic);
    HANDLER(handleMakeSymbolic);
    HANDLER(handleMalloc);
    HANDLER(handleMarkGlobal);
    HANDLER(handleMerge);
    HANDLER(handleNew);
    HANDLER(handleNewArray);
    HANDLER(handlePreferCex);
    HANDLER(handlePosixPreferCex);
    HANDLER(handlePrintExpr);
    HANDLER(handlePrintRange);
    HANDLER(handleRange);
    HANDLER(handleRealloc);
    HANDLER(handleReportError);
    HANDLER(handleRevirtObjects);
    HANDLER(handleSetForking);
    HANDLER(handleSilentExit);
    HANDLER(handleStackTrace);
    HANDLER(handleUnderConstrained);
    HANDLER(handleWarning);
    HANDLER(handleWarningOnce);
    HANDLER(handleAddOverflow);
    HANDLER(handleMulOverflow);
    HANDLER(handleSubOverflow);
    HANDLER(handleDivRemOverflow);
    HANDLER(handleOpen);
    HANDLER(handleMakeIOBuffer);
    HANDLER(handleFscanf);
    HANDLER(handleSscanf);
    HANDLER(handleFprintf);
    HANDLER(handleprintf);
    HANDLER(handleFputc);
    HANDLER(handleClose);
    HANDLER(handleFread);
    HANDLER(handleFwrite);
#undef HANDLER
  };
} // End klee namespace

#endif
