// -*- C++ -*-
#ifndef KLEE_SEARCHERUTIL_H
#define KLEE_SEARCHERUTIL_H

#include "ExecutionState.h"

namespace klee {

struct SearcherAction {
  friend class ref<SearcherAction>;

protected:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

public:
  enum class Kind { Forward };

  SearcherAction() = default;
  virtual ~SearcherAction() = default;

  virtual Kind getKind() const = 0;

  static bool classof(const SearcherAction *) { return true; }
};

struct ForwardAction : public SearcherAction {
  friend class ref<ForwardAction>;

  ExecutionState *state;

  ForwardAction(ExecutionState *_state) : state(_state) {}

  Kind getKind() const { return Kind::Forward; }
  static bool classof(const SearcherAction *A) {
    return A->getKind() == Kind::Forward;
  }
  static bool classof(const ForwardAction *) { return true; }
};

} // namespace klee

#endif
