//===-- EventRecorder.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEEF Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef KLEE_EVENT_RECORDER_H
#define KLEE_EVENT_RECORDER_H

#include "klee/Expr/Path.h"

namespace klee {

template <typename T> class ref;
template <typename T> class ImmutableList;
struct CodeFlowJson;
class CodeEvent;

/// @brief Class capable of storing code events
/// and serializing them into SARIF format.
class EventRecorder {
private:
  /// @brief Inner storage of code events.
  ImmutableList<ref<CodeEvent>> events;

public:
  EventRecorder() = default;

  EventRecorder(const EventRecorder &);
  EventRecorder &operator=(const EventRecorder &);
  ~EventRecorder() = default;

  /// @brief Remembers event.
  /// @param event Event to record.
  void record(const ref<CodeEvent> &event);

  /// @brief Appends all events from the given `EventRecorder`
  /// to this recorder.
  /// @param rhs `EventRecorder` to get events from.
  void append(const EventRecorder &rhs);

  /// @brief Returns events from the history in specified range.
  /// @param begin range begin.
  /// @param end range end.
  /// @return `EventRecorder` containing recorder events from inner storage.
  EventRecorder inRange(const Path::PathIndex &begin,
                        const Path::PathIndex &end) const;

  /// @brief Returns events from the history from the givent event.
  /// @param begin range begin.
  /// @return `EventRecorder` containing recorder events from inner storage.
  EventRecorder tail(const Path::PathIndex &begin) const;

  /// @brief Returns last recorder event.
  /// @return Last recorded `CodeEvent`.
  ref<CodeEvent> last() const;

  /// @brief Tests whether this event recorder is empty.
  /// @return `true` if there are no events have been recorded.
  bool empty() const;

  /// @brief Serializes events in this recorder to SARIF format.
  /// @return Structure ready for wrapping into json
  /// (i.e. `json(serialize())`) and serialization.
  CodeFlowJson serialize() const;
};

} // namespace klee

#endif // KLEE_EVENT_RECORDER_H
