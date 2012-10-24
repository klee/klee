//===-- Interpreter.h - Abstract Execution Engine Interface -----*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//===----------------------------------------------------------------------===//

#ifndef KLEE_INTERPRETER_H
#define KLEE_INTERPRETER_H

#include <vector>
#include <string>
#include <map>
#include <set>

struct KTest;

namespace llvm {
class Function;
class Module;
}

namespace klee {
class ExecutionState;
class Interpreter;
class TreeStreamWriter;

class InterpreterHandler {
public:
  InterpreterHandler() {}
  virtual ~InterpreterHandler() {}

  virtual std::ostream &getInfoStream() const = 0;

  virtual std::string getOutputFilename(const std::string &filename) = 0;
  virtual std::ostream *openOutputFile(const std::string &filename) = 0;

  virtual void incPathsExplored() = 0;

  virtual void processTestCase(const ExecutionState &state,
                               const char *err, 
                               const char *suffix) = 0;
};

class Interpreter {
public:
  /// ModuleOptions - Module level options which can be set when
  /// registering a module with the interpreter.
  struct ModuleOptions {
    std::string LibraryDir;
    bool Optimize;
    bool CheckDivZero;

    ModuleOptions(const std::string& _LibraryDir, 
                  bool _Optimize, bool _CheckDivZero)
      : LibraryDir(_LibraryDir), Optimize(_Optimize), 
        CheckDivZero(_CheckDivZero) {}
  };

  enum LogType
  {
	  STP, //.CVC (STP's native language)
	  KQUERY, //.PC files (kQuery native language)
	  SMTLIB2 //.SMT2 files (SMTLIB version 2 files)
  };

  /// InterpreterOptions - Options varying the runtime behavior during
  /// interpretation.
  struct InterpreterOptions {
    /// A frequency at which to make concrete reads return constrained
    /// symbolic values. This is used to test the correctness of the
    /// symbolic execution on concrete programs.
    unsigned MakeConcreteSymbolic;

    InterpreterOptions()
      : MakeConcreteSymbolic(false)
    {}
  };

protected:
  const InterpreterOptions interpreterOpts;

  Interpreter(const InterpreterOptions &_interpreterOpts)
    : interpreterOpts(_interpreterOpts)
  {}

public:
  virtual ~Interpreter() {}

  static Interpreter *create(const InterpreterOptions &_interpreterOpts,
                             InterpreterHandler *ih);

  /// Register the module to be executed.  
  ///
  /// \return The final module after it has been optimized, checks
  /// inserted, and modified for interpretation.
  virtual const llvm::Module * 
  setModule(llvm::Module *module, 
            const ModuleOptions &opts) = 0;

  // supply a tree stream writer which the interpreter will use
  // to record the concrete path (as a stream of '0' and '1' bytes).
  virtual void setPathWriter(TreeStreamWriter *tsw) = 0;

  // supply a tree stream writer which the interpreter will use
  // to record the symbolic path (as a stream of '0' and '1' bytes).
  virtual void setSymbolicPathWriter(TreeStreamWriter *tsw) = 0;

  // supply a test case to replay from. this can be used to drive the
  // interpretation down a user specified path. use null to reset.
  virtual void setReplayOut(const struct KTest *out) = 0;

  // supply a list of branch decisions specifying which direction to
  // take on forks. this can be used to drive the interpretation down
  // a user specified path. use null to reset.
  virtual void setReplayPath(const std::vector<bool> *path) = 0;

  // supply a set of symbolic bindings that will be used as "seeds"
  // for the search. use null to reset.
  virtual void useSeeds(const std::vector<struct KTest *> *seeds) = 0;

  virtual void runFunctionAsMain(llvm::Function *f,
                                 int argc,
                                 char **argv,
                                 char **envp) = 0;

  /*** Runtime options ***/

  virtual void setHaltExecution(bool value) = 0;

  virtual void setInhibitForking(bool value) = 0;

  /*** State accessor methods ***/

  virtual unsigned getPathStreamID(const ExecutionState &state) = 0;

  virtual unsigned getSymbolicPathStreamID(const ExecutionState &state) = 0;
  
  virtual void getConstraintLog(const ExecutionState &state,
                                std::string &res,
                                LogType logFormat = STP) = 0;

  virtual bool getSymbolicSolution(const ExecutionState &state, 
                                   std::vector< 
                                   std::pair<std::string,
                                   std::vector<unsigned char> > >
                                   &res) = 0;

  virtual void getCoveredLines(const ExecutionState &state,
                               std::map<const std::string*, std::set<unsigned> > &res) = 0;
};

} // End klee namespace

#endif
