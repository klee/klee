#include "BidirectionalSearcher.h"
#include "ObjectManager.h"
#include "Searcher.h"
#include "SearcherUtil.h"

namespace klee {

ref<SearcherAction> ForwardOnlySearcher::selectAction() {
  return new ForwardAction(&searcher->selectState());
}

bool ForwardOnlySearcher::empty() { return searcher->empty(); }

void ForwardOnlySearcher::update(ref<ObjectManager::Event> e) {
  if (auto statesEvent = dyn_cast<ObjectManager::States>(e)) {
    searcher->update(statesEvent->modified, statesEvent->added,
                     statesEvent->removed);
  }
}

ForwardOnlySearcher::ForwardOnlySearcher(Searcher *_searcher) {
  searcher = _searcher;
}

ForwardOnlySearcher::~ForwardOnlySearcher() { delete searcher; }

} // namespace klee
