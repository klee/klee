//===-- EventRecorder.cpp ---------------------------------------*- C++ -*-===//
//
//                     The KLEEF Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "EventRecorder.h"
#include "CodeEvent.h"

#include "klee/ADT/ImmutableList.h"
#include "klee/ADT/Ref.h"
#include "klee/Expr/Path.h"
#include "klee/Module/SarifReport.h"

#include <cassert>
#include <optional>
#include <utility>

using namespace klee;

EventRecorder::EventRecorder(const EventRecorder &rhs) : events(rhs.events) {}

EventRecorder &EventRecorder::operator=(const EventRecorder &rhs) {
  events = rhs.events;
  return *this;
}

void EventRecorder::record(const ref<CodeEvent> &event) {
  assert(event->location);
  assert((empty() ||
          Path::PathIndexCompare{}(last()->location->pathIndex,
                                   event->location->pathIndex) ||
          !Path::PathIndexCompare{}(event->location->pathIndex,
                                    last()->location->pathIndex)) &&
         "Event must have later pathIndex than last recorded");
  events.push_back(event);
}

void EventRecorder::append(const EventRecorder &rhs) {
  for (const auto &event : rhs.events) {
    record(event);
  }
}

CodeFlowJson EventRecorder::serialize() const {
  CodeFlowJson codeFlow{};
  ThreadFlowJson threadFlow{};

  for (const auto &event : events) {
    LocationJson location = event->serialize();
    ThreadFlowLocationJson threadFlowLocation{{std::move(location)},
                                              std::nullopt};
    threadFlow.locations.push_back(std::move(threadFlowLocation));
  }

  codeFlow.threadFlows.push_back(std::move(threadFlow));

  return codeFlow;
}

EventRecorder EventRecorder::inRange(const Path::PathIndex &begin,
                                     const Path::PathIndex &end) const {
  EventRecorder result;

  if (Path::PathIndexCompare{}(end, begin)) {
    return result;
  }

  for (const auto &event : events) {
    const Path::PathIndex &eventPathIndex = event->location->pathIndex;
    if (!Path::PathIndexCompare{}(eventPathIndex, begin)) {
      if (Path::PathIndexCompare{}(end, eventPathIndex)) {
        break;
      }
      result.record(event);
    }
  }

  return result;
}

bool EventRecorder::empty() const { return events.empty(); }

ref<CodeEvent> EventRecorder::last() const { return events.back(); }

EventRecorder EventRecorder::tail(const Path::PathIndex &begin) const {
  if (events.empty()) {
    return EventRecorder();
  }
  return inRange(begin, events.back()->location->pathIndex);
}
