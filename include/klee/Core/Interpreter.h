//===-- Interpreter.h - Abstract Execution Engine Interface -----*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//===----------------------------------------------------------------------===//

#ifndef KLEE_INTERPRETER_H
#define KLEE_INTERPRETER_H

#include "TerminationTypes.h"
#include "klee/Module/Annotation.h"

#include "klee/Module/SarifReport.h"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

struct KTest;

namespace llvm {
class BasicBlock;
class Function;
class LLVMContext;
class Module;
class Type;
class raw_ostream;
class raw_fd_ostream;
} // namespace llvm

namespace klee {
class ExecutionState;
struct SarifReport;
class Interpreter;
class TreeStreamWriter;

class InterpreterHandler {
public:
  InterpreterHandler() {}
  virtual ~InterpreterHandler() {}

  virtual llvm::raw_ostream &getInfoStream() const = 0;

  virtual std::string getOutputFilename(const std::string &filename) = 0;
  virtual std::unique_ptr<llvm::raw_fd_ostream>
  openOutputFile(const std::string &filename) = 0;

  virtual void incPathsCompleted() = 0;
  virtual void incPathsExplored(std::uint32_t num = 1) = 0;

  virtual void processTestCase(const ExecutionState &state, const char *message,
                               const char *suffix, bool isError = false) = 0;
};

/// [File][Line][Column] -> Opcode
using FLCtoOpcode = std::unordered_map<
    std::string,
    std::unordered_map<
        unsigned, std::unordered_map<unsigned, std::unordered_set<unsigned>>>>;

enum class MockStrategyKind {
  Naive,        // For each function call new symbolic value is generated
  Deterministic // Each function is treated as uninterpreted function in SMT.
                // Compatible with Z3 solver only
};

enum class MockPolicy {
  None,   // No mock function generated
  Failed, // Generate symbolic value for failed external calls
  All     // Generate IR module with all external values
};

enum class MockMutableGlobalsPolicy {
  None, // No mock for globals
  All,  // Mock globals on module build stage and generate bc module for it
};

class Interpreter {
public:
  enum class GuidanceKind {
    NoGuidance,       // Default symbolic execution
    CoverageGuidance, // Use GuidedSearcher and guidedRun to maximize full code
                      // coverage
    ErrorGuidance     // Use GuidedSearcher and guidedRun to maximize specified
                      // targets coverage
  };

  /// ModuleOptions - Module level options which can be set when
  /// registering a module with the interpreter.
  struct ModuleOptions {
    std::string LibraryDir;
    std::string EntryPoint;
    std::string OptSuffix;
    std::string MainCurrentName;
    std::string MainNameAfterMock;
    std::string AnnotationsFile;
    bool Optimize;
    bool Simplify;
    bool CheckDivZero;
    bool CheckOvershift;
    bool AnnotateOnlyExternal;
    bool WithFPRuntime;
    bool WithPOSIXRuntime;

    ModuleOptions(const std::string &_LibraryDir,
                  const std::string &_EntryPoint, const std::string &_OptSuffix,
                  const std::string &_MainCurrentName,
                  const std::string &_MainNameAfterMock,
                  const std::string &_AnnotationsFile, bool _Optimize,
                  bool _Simplify, bool _CheckDivZero, bool _CheckOvershift,
                  bool _AnnotateOnlyExternal, bool _WithFPRuntime,
                  bool _WithPOSIXRuntime)
        : LibraryDir(_LibraryDir), EntryPoint(_EntryPoint),
          OptSuffix(_OptSuffix), MainCurrentName(_MainCurrentName),
          MainNameAfterMock(_MainNameAfterMock),
          AnnotationsFile(_AnnotationsFile), Optimize(_Optimize),
          Simplify(_Simplify), CheckDivZero(_CheckDivZero),
          CheckOvershift(_CheckOvershift),
          AnnotateOnlyExternal(_AnnotateOnlyExternal),
          WithFPRuntime(_WithFPRuntime), WithPOSIXRuntime(_WithPOSIXRuntime) {}
  };

  enum LogType {
    STP,    //.CVC (STP's native language)
    KQUERY, //.KQUERY files (kQuery native language)
    SMTLIB2 //.SMT2 files (SMTLIB version 2 files)
  };

  /// InterpreterOptions - Options varying the runtime behavior during
  /// interpretation.
  struct InterpreterOptions {
    /// A frequency at which to make concrete reads return constrained
    /// symbolic values. This is used to test the correctness of the
    /// symbolic execution on concrete programs.
    unsigned MakeConcreteSymbolic;
    GuidanceKind Guidance;
    std::optional<SarifReport> Paths;
    MockPolicy Mock;
    MockStrategyKind MockStrategy;
    MockMutableGlobalsPolicy MockMutableGlobals;

    InterpreterOptions(std::optional<SarifReport> Paths)
        : MakeConcreteSymbolic(false), Guidance(GuidanceKind::NoGuidance),
          Paths(std::move(Paths)), Mock(MockPolicy::None),
          MockStrategy(MockStrategyKind::Naive),
          MockMutableGlobals(MockMutableGlobalsPolicy::None) {}
  };

protected:
  const InterpreterOptions &interpreterOpts;

  Interpreter(const InterpreterOptions &_interpreterOpts)
      : interpreterOpts(_interpreterOpts) {}

public:
  virtual ~Interpreter() {}

  static Interpreter *create(llvm::LLVMContext &ctx,
                             const InterpreterOptions &_interpreterOpts,
                             InterpreterHandler *ih);

  /// Register the module to be executed.
  /// \param userModules A list of user modules that should form the final
  ///                module
  /// \param libsModules A list of libs modules that should form the final
  ///                module
  /// \return The final module after it has been optimized, checks
  /// inserted, and modified for interpretation.
  virtual llvm::Module *setModule(
      std::vector<std::unique_ptr<llvm::Module>> &userModules,
      std::vector<std::unique_ptr<llvm::Module>> &libsModules,
      const ModuleOptions &opts, std::set<std::string> &&mainModuleFunctions,
      std::set<std::string> &&mainModuleGlobals, FLCtoOpcode &&origInstructions,
      const std::set<std::string> &ignoredExternals,
      std::vector<std::pair<std::string, std::string>> redefinitions) = 0;

  // supply a tree stream writer which the interpreter will use
  // to record the concrete path (as a stream of '0' and '1' bytes).
  virtual void setPathWriter(TreeStreamWriter *tsw) = 0;

  // supply a tree stream writer which the interpreter will use
  // to record the symbolic path (as a stream of '0' and '1' bytes).
  virtual void setSymbolicPathWriter(TreeStreamWriter *tsw) = 0;

  // supply a test case to replay from. this can be used to drive the
  // interpretation down a user specified path. use null to reset.
  virtual void setReplayKTest(const struct KTest *out) = 0;

  // supply a list of branch decisions specifying which direction to
  // take on forks. this can be used to drive the interpretation down
  // a user specified path. use null to reset.
  virtual void setReplayPath(const std::vector<bool> *path) = 0;

  // supply a set of symbolic bindings that will be used as "seeds"
  // for the search. use null to reset.
  virtual void useSeeds(const std::vector<struct KTest *> *seeds) = 0;

  virtual void runFunctionAsMain(llvm::Function *f, int argc, char **argv,
                                 char **envp) = 0;

  /*** Runtime options ***/

  virtual void setHaltExecution(HaltExecution::Reason value) = 0;

  virtual HaltExecution::Reason getHaltExecution() = 0;

  virtual void setInhibitForking(bool value) = 0;

  virtual void prepareForEarlyExit() = 0;

  virtual bool hasTargetForest() const = 0;

  /*** State accessor methods ***/

  virtual unsigned getPathStreamID(const ExecutionState &state) = 0;

  virtual unsigned getSymbolicPathStreamID(const ExecutionState &state) = 0;

  virtual void getConstraintLog(const ExecutionState &state, std::string &res,
                                LogType logFormat = STP) = 0;

  virtual bool getSymbolicSolution(const ExecutionState &state, KTest &res) = 0;

  virtual void logState(const ExecutionState &state, int id,
                        std::unique_ptr<llvm::raw_fd_ostream> &f) = 0;

  virtual void
  getCoveredLines(const ExecutionState &state,
                  std::map<std::string, std::set<unsigned>> &res) = 0;

  virtual void getBlockPath(const ExecutionState &state,
                            std::string &blockPath) = 0;
};

} // namespace klee

#endif /* KLEE_INTERPRETER_H */
