//===-- ExecutionState.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXECUTIONSTATE_H
#define KLEE_EXECUTIONSTATE_H

#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/Internal/ADT/TreeStream.h"

// FIXME: We do not want to be exposing these? :(
#include "../../lib/Core/AddressSpace.h"
#include "klee/Internal/Module/KInstIterator.h"

#include <map>
#include <set>
#include <vector>

namespace klee {
class Array;
class CallPathNode;
struct Cell;
struct KFunction;
struct KInstruction;
class MemoryObject;
class PTreeNode;
struct InstructionInfo;

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const MemoryMap &mm);

struct StackFrame {
  KInstIterator caller;
  KFunction *kf;
  CallPathNode *callPathNode;

  std::vector<const MemoryObject *> allocas;
  Cell *locals;

  /// Minimum distance to an uncovered instruction once the function
  /// returns. This is not a good place for this but is used to
  /// quickly compute the context sensitive minimum distance to an
  /// uncovered instruction. This value is updated by the StatsTracker
  /// periodically.
  unsigned minDistToUncoveredOnReturn;

  // For vararg functions: arguments not passed via parameter are
  // stored (packed tightly) in a local (alloca) memory object. This
  // is setup to match the way the front-end generates vaarg code (it
  // does not pass vaarg through as expected). VACopy is lowered inside
  // of intrinsic lowering.
  MemoryObject *varargs;

  StackFrame(KInstIterator caller, KFunction *kf);
  StackFrame(const StackFrame &s);
  ~StackFrame();
};

/// @brief ExecutionState representing a path under exploration
class ExecutionState {
public:
  typedef std::vector<StackFrame> stack_ty;

private:
  // unsupported, use copy constructor
  ExecutionState &operator=(const ExecutionState &);

  std::map<std::string, std::string> fnAliases;

  int FscanfBytesRead = 0;

public:
  // Execution - Control Flow specific

  /// @brief Pointer to instruction to be executed after the current
  /// instruction
  KInstIterator pc;

  /// @brief Pointer to instruction which is currently executed
  KInstIterator prevPC;

  /// @brief Stack representing the current instruction stream
  stack_ty stack;

  /// @brief Remember from which Basic Block control flow arrived
  /// (i.e. to select the right phi values)
  unsigned incomingBBIndex;

  // Overall state of the state - Data specific

  /// @brief Address space used by this state (e.g. Global and Heap)
  AddressSpace addressSpace;

  /// @brief Constraints collected so far
  ConstraintManager constraints;

  /// Statistics and information

  /// @brief Costs for all queries issued for this state, in seconds
  mutable double queryCost;

  /// @brief Weight assigned for importance of this state.  Can be
  /// used for searchers to decide what paths to explore
  double weight;

  /// @brief Exploration depth, i.e., number of times KLEE branched for this state
  unsigned depth;

  /// @brief History of complete path: represents branches taken to
  /// reach/create this state (both concrete and symbolic)
  TreeOStream pathOS;

  /// @brief History of symbolic path: represents symbolic branches
  /// taken to reach/create this state
  TreeOStream symPathOS;

  /// @brief Counts how many instructions were executed since the last new
  /// instruction was covered.
  unsigned instsSinceCovNew;

  /// @brief Whether a new instruction was covered in this state
  bool coveredNew;

  /// @brief Disables forking for this state. Set by user code
  bool forkDisabled;

  /// @brief Set containing which lines in which files are covered by this state
  std::map<const std::string *, std::set<unsigned> > coveredLines;

  /// @brief Pointer to the process tree of the current state
  PTreeNode *ptreeNode;

  /// @brief Ordered list of symbolics: used to generate test cases.
  //
  // FIXME: Move to a shared list structure (not critical).
  std::vector<std::pair<const MemoryObject *, const Array *> > symbolics;

  /// @brief Set of used array names for this state.  Used to avoid collisions.
  std::set<std::string> arrayNames;

  std::string getFnAlias(std::string fn);
  void addFnAlias(std::string old_fn, std::string new_fn);
  void removeFnAlias(std::string fn);

  bool targetFunc;

  class fileDesc{
  private:
	  int fileNumber;
	  ObjectPair targetBuffer;
	  bool read;
	  bool write;
	  unsigned int offset;
	  int size;
  public:
	  fileDesc(int id,std::pair<ObjectPair, int> buffer):fileNumber(id),targetBuffer(buffer.first),read(true),write(false),offset(0),size(buffer.second){

	  };
	  fileDesc(int id,std::pair<ObjectPair, int> buffer,std::string wr):fileNumber(id),targetBuffer(buffer.first),read(true),write(false),offset(0),size(buffer.second){
		  if(wr.compare("w")==0){
			  write = true;
			  read = false;
		  }
		  else if(wr.compare("r")==0){

		  }
		  else if(wr.compare("wr")==0 || wr.compare("rw")==0){
			  write = true;
		  }
		  else{
			  assert("Unrecognized ReadWrite indicator");
		  }
	  };

	  ObjectPair getBuffer(){
		  return targetBuffer;
	  }

	  int getoffset (){
		  return offset;
	  }

	  int getsize(){
		  return size;
	  }

	  bool ifRead(){
		  return read;
	  }

	  bool ifWrite(){
		  return write;
	  }

	  void incOffset(){
		  offset++;
	  }
	  void decOffset(){
		  offset--;
	  }
  };

  class IObuffer{
  private:
	 std::vector<ref<Expr>  >numberbuf;
	 bool neg = false;
  public:
	  IObuffer(){

	  };
	  virtual ~IObuffer(){};
	  void addDigit(ref<Expr> digit){
		  numberbuf.push_back(digit);
	  }

	  const std::vector<ref<Expr> >& getBuffer() const{
		  return numberbuf;
	  };

	  const bool getNeg() const{
		  return neg;
	  }

	  ref<Expr> processNumber(Expr::Width numwidth,int ary){
		  	if(numberbuf.size()==0){
		  		return NULL;
		  	}
			std::vector<ref<Expr> >::reverse_iterator numit;
			ref<Expr> sum = ConstantExpr::create(0,numwidth);//Width here is for %d and %i
			ref<Expr> digit;
			int mul = 1;
			for(numit = numberbuf.rbegin(); numit!=numberbuf.rend();numit++){
				digit = SubExpr::create(*numit,ConstantExpr::create(48,ConstantExpr::Int8));
				digit = ZExtExpr::create(digit,numwidth);
				sum = AddExpr::create(sum,MulExpr::create(digit,ConstantExpr::create(mul,numwidth)));
				mul= mul * ary;
			}
			if(neg){
				sum = SubExpr::create(ConstantExpr::create(0,numwidth),sum);
			}
			return sum;
	  }

	  void clear(){
		  numberbuf.clear();
		  neg = false;
	  }

	  void setneg(){
		  neg = true;
	  }
  };

  IObuffer ioBuffer;

  int getBytesRead(){
	  return FscanfBytesRead;
  }

  void incBytesRead(){
	  FscanfBytesRead++;
  }

  void clearBytesRead(){
	  FscanfBytesRead = 0;
  }
private:
  ExecutionState() : ptreeNode(0) {}

  /*
   * Buffer for IO Functions, file descriptor like structure
   */
  std::vector<std::pair<std::pair<ObjectPair, int>, std::string > > bufferList;

  std::vector<fileDesc> fileDescriptor;

public:
  ExecutionState(KFunction *kf);

  // XXX total hack, just used to make a state so solver can
  // use on structure
  ExecutionState(const std::vector<ref<Expr> > &assumptions);

  ExecutionState(const ExecutionState &state);

  ~ExecutionState();

  ExecutionState *branch();

  void pushFrame(KInstIterator caller, KFunction *kf);
  void popFrame();

  void addSymbolic(const MemoryObject *mo, const Array *array);
  void addConstraint(ref<Expr> e) { constraints.addConstraint(e); }

  bool merge(const ExecutionState &b);
  void dumpStack(llvm::raw_ostream &out) const;

  /*
   * Gladtbx: add File Descriptor
   */
  void addBuffer(  std::pair<std::pair<ObjectPair, int>, std::string > entry){
	  bufferList.push_back(entry);
  }

  fileDesc* getBuffer(int id){
	  return &fileDescriptor[id-1];
  }

  std::pair<ObjectPair, int> lookupFile(std::string fileName){
	  std::vector<std::pair<std::pair<ObjectPair, int>, std::string > >::iterator it;
	  for(it = bufferList.begin(); it != bufferList.end(); it++){
		  if(it->second == fileName){
			  return it->first;
		  }
	  }
	  ObjectPair temp(NULL,NULL);
	  return  std::pair<ObjectPair, int>(temp,0);
  }

  int createFileDesc(std::pair<ObjectPair, int> buffer, std::string wr){
	  int id = fileDescriptor.size() + 1;
	  fileDesc entry(id, buffer, wr);
	  fileDescriptor.push_back(entry);
	  return id;
  }
};
}

#endif
