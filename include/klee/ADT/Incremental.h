//===---- Incremental.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_INCREMENTAL_H
#define KLEE_INCREMENTAL_H

#include <cassert>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "klee/Expr/ExprUtil.h"

namespace klee {

template <typename _Tp, typename _Alloc>
void extend(std::vector<_Tp, _Alloc> &ths,
            const std::vector<_Tp, _Alloc> &other) {
  ths.reserve(ths.size() + other.size());
  ths.insert(ths.end(), other.begin(), other.end());
}

template <typename _Tp, typename _Alloc = std::allocator<_Tp>>
class inc_vector {
public:
  using vec = std::vector<_Tp, _Alloc>;
  using frame_size_it = std::vector<size_t>::const_iterator;
  using frame_it = typename vec::const_iterator;

  /// It is public, so that all vector operations are supported
  /// Everything pushed to v is pushed to the last frame
  vec v;

  std::vector<size_t> frame_sizes;

private:
  // v.size() == sum(frame_sizes) + size of the fresh frame

  size_t freshFrameSize() const {
    return v.size() -
           std::accumulate(frame_sizes.begin(), frame_sizes.end(), 0);
  }

  void take(size_t n, size_t &frames_count, size_t &frame_index) const {
    size_t i = 0;
    size_t c = n;
    for (; i < frame_sizes.size(); i++) {
      if (frame_sizes[i] > c)
        break;
      c -= frame_sizes[i];
    }
    frames_count = c;
    frame_index = i;
  }

public:
  inc_vector() {}
  inc_vector(const std::vector<_Tp> &constraints) : v(constraints) {}

  void clear() {
    v.clear();
    frame_sizes.clear();
  }

  frame_size_it begin() const { return frame_sizes.cbegin(); }
  frame_size_it end() const { return frame_sizes.cend(); }
  size_t framesSize() const { return frame_sizes.size() + 1; }

  frame_it begin(int frame_index) const {
    assert(-(long long)framesSize() <= (long long)frame_index &&
           (long long)frame_index <= (long long)framesSize());
    if (frame_index < 0)
      frame_index += framesSize();
    if ((long long)frame_index == (long long)framesSize())
      return v.end();
    auto fend = frame_sizes.begin() + frame_index;
    auto shift = std::accumulate(frame_sizes.begin(), fend, 0);
    return v.begin() + shift;
  }
  frame_it end(int frame_index) const { return begin(frame_index + 1); }
  size_t size(size_t frame_index) const {
    assert(frame_index < framesSize());
    if (frame_index == framesSize() - 1) // last frame
      return freshFrameSize();
    return frame_sizes[frame_index];
  }

  void pop(size_t popFrames) {
    assert(freshFrameSize() == 0);
    if (popFrames == 0)
      return;
    size_t toPop =
        std::accumulate(frame_sizes.end() - popFrames, frame_sizes.end(), 0);
    v.resize(v.size() - toPop);
    frame_sizes.resize(frame_sizes.size() - popFrames);
  }

  void push() {
    auto freshSize = freshFrameSize();
    frame_sizes.push_back(freshSize);
    assert(freshFrameSize() == 0);
  }

  /// ensures that last frame is empty
  void extend(const std::vector<_Tp, _Alloc> &other) {
    assert(freshFrameSize() == 0);
    // push();
    klee::extend(v, other);
    push();
  }

  /// ensures that last frame is empty
  void extend(const inc_vector<_Tp, _Alloc> &other) {
    assert(freshFrameSize() == 0);
    for (size_t i = 0, e = other.framesSize(); i < e; i++) {
      v.reserve(v.size() + other.size(i));
      v.insert(v.end(), other.begin(i), other.end(i));
      push();
    }
  }

  void takeAfter(size_t n, inc_vector<_Tp, _Alloc> &result) const {
    size_t frames_count, frame_index;
    take(n, frames_count, frame_index);
    result = *this;
    std::vector<_Tp, _Alloc>(result.v.begin() + n, result.v.end())
        .swap(result.v);
    std::vector<size_t>(result.frame_sizes.begin() + frame_index,
                        result.frame_sizes.end())
        .swap(result.frame_sizes);
    if (frames_count)
      result.frame_sizes[0] -= frames_count;
  }

  void butLast(inc_vector<_Tp, _Alloc> &result) const {
    assert(!v.empty() && "butLast of empty vector");
    assert(freshFrameSize() && "butLast of empty fresh frame");
    result = *this;
    result.v.pop_back();
  }

  void takeBefore(size_t n, size_t &toPop, size_t &takeFromOther) const {
    take(n, takeFromOther, toPop);
    toPop = frame_sizes.size() - toPop;
  }
};

using FrameId = size_t;
using FrameIds = std::unordered_set<FrameId>;

template <typename _Value, typename _Hash = std::hash<_Value>,
          typename _Pred = std::equal_to<_Value>,
          typename _Alloc = std::allocator<_Value>>
class inc_uset {
private:
  class MinFrameIds {
    FrameIds ids;
    FrameId min = std::numeric_limits<FrameId>::max();

  public:
    bool empty() const { return ids.empty(); }

    bool hasMin(FrameId other) const { return min == other && !ids.empty(); }

    void insert(FrameId i) {
      ids.insert(i);
      if (i < min)
        min = i;
    }

    MinFrameIds bound(FrameId upperBound) {
      MinFrameIds result;
      std::copy_if(ids.begin(), ids.end(),
                   std::inserter(result.ids, result.ids.begin()),
                   [upperBound](FrameId i) { return i <= upperBound; });
      auto min_it = std::min_element(result.ids.begin(), result.ids.end());
      if (min_it == result.ids.end())
        result.min = std::numeric_limits<FrameId>::max();
      else
        result.min = *min_it;
      return result;
    }
  };

  using idMap = std::unordered_map<_Value, MinFrameIds, _Hash, _Pred, _Alloc>;
  using citerator = typename idMap::const_iterator;
  idMap ids;
  FrameId current_frame = 0;

public:
  size_t framesSize() const { return current_frame + 1; }

  void clear() {
    ids.clear();
    current_frame = 0;
  }

  class frame_it
      : public std::iterator<std::forward_iterator_tag, ref<Expr>, int> {
    citerator set_it;
    const citerator set_ite;
    const FrameId frame_index = 0;

    void gotoNext() {
      while (set_it != set_ite && !set_it->second.hasMin(frame_index))
        set_it++;
    }

  public:
    using value_type = _Value;

    explicit frame_it(const idMap &ids)
        : set_it(ids.end()), set_ite(ids.end()) {}
    explicit frame_it(const idMap &ids, FrameId frame_index)
        : set_it(ids.begin()), set_ite(ids.end()), frame_index(frame_index) {
      gotoNext();
    }

    bool operator!=(const frame_it &other) const {
      return set_it != other.set_it;
    }

    const _Value &operator*() const { return set_it->first; }

    frame_it &operator++() {
      if (set_it != set_ite) {
        set_it++;
        gotoNext();
      }
      return *this;
    }
  };

  class all_it
      : public std::iterator<std::forward_iterator_tag, ref<Expr>, int> {
    citerator set_it;

  public:
    using value_type = _Value;

    explicit all_it(citerator set_it) : set_it(set_it) {}

    bool operator!=(const all_it &other) const {
      return set_it != other.set_it;
    }

    const _Value &operator*() const { return set_it->first; }

    all_it &operator++() {
      set_it++;
      return *this;
    }
  };

  all_it begin() const { return all_it(ids.begin()); }
  all_it end() const { return all_it(ids.end()); }

  frame_it begin(int frame_index) const {
    assert(-(long long)framesSize() <= (long long)frame_index &&
           (long long)frame_index <= (long long)framesSize());
    if (frame_index < 0)
      frame_index += framesSize();
    return frame_it(ids, frame_index);
  }
  frame_it end(int frame_index) const { return frame_it(ids); }

  void insert(const _Value &v) { ids[v].insert(current_frame); }

  template <class _InputIterator>
  void insert(_InputIterator __first, _InputIterator __last) {
    for (; __first != __last; __first++)
      ids[*__first].insert(current_frame);
  }

  void pop(size_t popFrames) {
    current_frame -= popFrames;
    idMap newIdMap;
    for (auto &keyAndIds : ids) {
      MinFrameIds newIds = keyAndIds.second.bound(current_frame);
      if (!newIds.empty())
        newIdMap.insert(std::make_pair(keyAndIds.first, newIds));
    }
    ids = newIdMap;
  }

  void push() { current_frame++; }
};

template <typename _Key, typename _Tp, typename _Hash = std::hash<_Key>,
          typename _Pred = std::equal_to<_Key>,
          typename _Alloc = std::allocator<std::pair<const _Key, _Tp>>>
class inc_umap {
private:
  std::unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc> map;
  using idMap = std::unordered_map<_Key, FrameIds, _Hash, _Pred, _Alloc>;
  idMap ids;
  FrameId current_frame = 0;

public:
  void clear() {
    map.clear();
    ids.clear();
    current_frame = 0;
  }

  void insert(const std::pair<_Key, _Tp> &pair) {
    map.insert(pair);
    ids[pair.first].insert(current_frame);
  }

  _Tp &operator[](const _Key &key) {
    ids[key].insert(current_frame);
    return map[key];
  }

  size_t count(const _Key &key) const { return map.count(key); }

  const _Tp &at(_Key &key) const { return map.at(key); }

  void pop(size_t popFrames) {
    current_frame -= popFrames;
    idMap newIdMap;
    for (auto &keyAndIds : ids) {
      FrameIds newIds;
      for (auto id : keyAndIds.second)
        if (id <= current_frame)
          newIds.insert(id);
      if (newIds.empty())
        map.erase(keyAndIds.first);
      else
        newIdMap.insert(std::make_pair(keyAndIds.first, newIds));
    }
    ids = newIdMap;
  }

  void push() { current_frame++; }

  void dump() const {
    for (auto kv : map) {
      kv.first.dump();
      llvm::errs() << "----->\n";
      kv.second.dump();
      llvm::errs() << "\n;;;;;;;;;\n";
    }
  }
};

} // namespace klee

#endif /* KLEE_INCREMENTAL_H */
