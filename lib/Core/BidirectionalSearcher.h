#ifndef KLEE_BIDIRECTIONALSEARCHER_H
#define KLEE_BIDIRECTIONALSEARCHER_H

#include "ObjectManager.h"
#include "Searcher.h"
#include "SearcherUtil.h"

namespace klee {

class IBidirectionalSearcher : public Subscriber {
public:
  virtual ref<SearcherAction> selectAction() = 0;
  virtual bool empty() = 0;
  virtual ~IBidirectionalSearcher() {}
};

class ForwardOnlySearcher : public IBidirectionalSearcher {
public:
  ref<SearcherAction> selectAction() override;
  void update(ref<ObjectManager::Event>) override;
  bool empty() override;
  explicit ForwardOnlySearcher(Searcher *searcher);
  ~ForwardOnlySearcher() override;

private:
  Searcher *searcher;
};

} // namespace klee

#endif
