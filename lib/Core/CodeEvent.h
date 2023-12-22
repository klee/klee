//===-- CodeEvent.h ---------------------------------------------*- C++ -*-===//
//
//                     The KLEEF Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef KLEE_CODE_EVENT_H
#define KLEE_CODE_EVENT_H

#include "CodeLocation.h"
#include "klee/ADT/Ref.h"

#include "klee/Module/KModule.h"
#include "klee/Module/KValue.h"
#include "klee/Module/SarifReport.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
DISABLE_WARNING_POP

#include <optional>
#include <string>

namespace klee {

/// @brief Base unit for storing information about event from source code.
/// Chain of such units may form a history for particular execution state.
class CodeEvent {
public:
  /// @brief Server for LLVM RTTI purposes.
  const enum EventKind { ALLOC, BR, CALL, ERR, RET } kind;

public:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  /// @brief Location information for the event
  const ref<CodeLocation> location;

  explicit CodeEvent(EventKind kind, const ref<CodeLocation> &location)
      : kind(kind), location(location) {}

  /// @brief Additional info to current event.
  /// @return String describing event.
  virtual std::string description() const = 0;

  /// @brief Serialize event to the JSON format.
  /// @return JSON object describing event.
  LocationJson serialize() const {
    LocationJson result;

    result.message = {{description()}};
    result.physicalLocation = {{location->serialize()}};

    return result;
  }

  /// @brief Kind of event used for LLVM RTTI.
  /// @return Kind of event.
  EventKind getKind() const { return kind; }

  virtual ~CodeEvent() = default;
};

/// @brief Event describing any allocating event.
struct AllocEvent : public CodeEvent {
  AllocEvent(ref<CodeLocation> location)
      : CodeEvent(EventKind::ALLOC, location) {}

  std::string description() const override {
    const KValue *source = location->source;
    if (llvm::isa<KGlobalVariable>(source)) {
      return std::string("Global memory allocation");
    }

    if (llvm::isa<llvm::AllocaInst>(source->unwrap())) {
      return std::string("Local memory allocation");
    }

    return std::string("Heap memory allocation");
  }

  static bool classof(const CodeEvent *rhs) {
    return rhs->getKind() == EventKind::ALLOC;
  }
};

/// @brief Event describing conditional `br` event.
class BrEvent : public CodeEvent {
private:
  /// @brief Described chosen branch event: `true` if
  /// `then`-branch was chosen and `false` otherwise.
  bool chosenBranch = true;

public:
  explicit BrEvent(const ref<CodeLocation> &location)
      : CodeEvent(EventKind::BR, location) {}

  /// @brief Modifies chosen branch for `this` event.
  /// @param branch true if condition in chosen branch is true and false
  /// otherwise.
  /// @return Reference to this object.
  /// @note This function does not return modified copy of this object; it
  /// returns *this* object.
  BrEvent &withBranch(bool branch) {
    chosenBranch = branch;
    return *this;
  }

  std::string description() const override {
    return std::string("Choosing ") +
           (chosenBranch ? std::string("true") : std::string("false")) +
           std::string(" branch");
  }

  static bool classof(const CodeEvent *rhs) {
    return rhs->getKind() == EventKind::BR;
  }
};

/// @brief Event describing conditional `call` to function event.
class CallEvent : public CodeEvent {
private:
  /// @brief Called function. Provides additional info
  /// for event description.
  const KFunction *const called;

public:
  explicit CallEvent(const ref<CodeLocation> &location,
                     const KFunction *const called)
      : CodeEvent(EventKind::CALL, location), called(called) {}

  std::string description() const override {
    return std::string("Calling '") + called->getName().str() +
           std::string("()'");
  }

  static bool classof(const CodeEvent *rhs) {
    return rhs->getKind() == EventKind::CALL;
  }
};

/// @brief Event describing any error event.
/// Kind of error is described in ruleID.
struct ErrorEvent : public CodeEvent {
public:
  /// @brief ID for this error.
  const StateTerminationType ruleID;

  /// @brief Message describing this error.
  const std::string message;

  /// @brief Event associated with this error
  /// which may be treated as a "source" of error
  /// (e.g. memory allocation for Out-Of-Bounds error).
  const std::optional<ref<CodeEvent>> source;

  ErrorEvent(const ref<CodeEvent> &source, const ref<CodeLocation> &sink,
             StateTerminationType ruleID, const std::string &message)
      : CodeEvent(EventKind::ERR, sink), ruleID(ruleID), message(message),
        source(source) {}

  ErrorEvent(const ref<CodeLocation> &sink, StateTerminationType ruleID,
             const std::string &message)
      : CodeEvent(EventKind::ERR, sink), ruleID(ruleID), message(message),
        source(std::nullopt) {}

  std::string description() const override { return message; }

  static bool classof(const CodeEvent *rhs) {
    return rhs->getKind() == EventKind::ERR;
  }
};

/// @brief Event describing conditional `return` from function event.
class ReturnEvent : public CodeEvent {
private:
  /// @brief Function to which control flow returns.
  const KFunction *const caller;

public:
  explicit ReturnEvent(const ref<CodeLocation> &location,
                       const KFunction *const caller)
      : CodeEvent(EventKind::RET, location), caller(caller) {}

  std::string description() const override {
    return std::string("Returning to '") + caller->getName().str() +
           std::string("()'");
  }

  static bool classof(const CodeEvent *rhs) {
    return rhs->getKind() == EventKind::RET;
  }
};

} // namespace klee

#endif // KLEE_CODE_EVENT_H
