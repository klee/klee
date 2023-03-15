//===-- sized_regions.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KDALLOC_UTIL_SIZED_REGIONS_H
#define KDALLOC_UTIL_SIZED_REGIONS_H

#include "../define.h"
#include "../location_info.h"
#include "cow_ptr.h"

#include <cassert>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>

namespace klee::kdalloc::suballocators {
class SizedRegions;

class Node final {
  friend class SizedRegions;

  CoWPtr<Node> lhs;
  CoWPtr<Node> rhs;

  // std::size_t _precomputed_hash;
  char *baseAddress;
  std::size_t size;

  inline std::uintptr_t hash() const noexcept {
    // This hash function implements a very simple hash based only on the base
    // address. The hash is (only) used to break ties w.r.t. the heap property.
    // Note that multiplication with an odd number (as done here) modulo a power
    // of two (the domain of std::size_t) is a permutation, meaning that the
    // hash is guaranteed to break any ties (as tree-keys are unique in the
    // treap). The constant is chosen in such a way that the high bits of the
    // result depend each byte of input while still enabling a fast computation
    // due to the low number of 1-bits involved. It is also chosen odd to ensure
    // co-primality with powers of two.

    static_assert(
        std::numeric_limits<std::uintptr_t>::digits <= 64,
        "Please choose a more appropriate constant in the line below.");
    return reinterpret_cast<std::uintptr_t>(baseAddress) *
           static_cast<std::uintptr_t>(0x8080'8080'8080'8081u);
  }

public:
  char *const &getBaseAddress() const noexcept { return baseAddress; }
  void setBaseAddress(char *const newBaseAddress) noexcept {
    baseAddress = newBaseAddress;
  }

  std::size_t const &getSize() const noexcept { return size; }
  void setSize(std::size_t const newSize) noexcept { size = newSize; }

  Node(char *const baseAddress, std::size_t const size) noexcept
      : baseAddress(baseAddress), size(size) {}
};

class SizedRegions {
  CoWPtr<Node> root;

  static void insertRec(CoWPtr<Node> &treap, CoWPtr<Node> &&region) {
    assert(treap && "target subtreap must exist (insertion at the call "
                    "site is trivial otherwise)");
    assert(region && "region to be inserted must exist");
    assert(treap->getBaseAddress() != region->getBaseAddress() &&
           "multiple insertions of the same tree-key are not supported");

    auto &node = treap.acquire();
    if (region->getBaseAddress() < node.getBaseAddress()) {
      if (node.lhs) {
        insertRec(node.lhs, std::move(region));
      } else {
        node.lhs = std::move(region);
      }
      if (node.lhs->getSize() > node.getSize() ||
          (node.lhs->getSize() == node.getSize() &&
           node.lhs->hash() > node.hash())) {
        auto temp = std::move(node.lhs);
        auto &nodeTemp = temp.acquire();
        node.lhs = std::move(nodeTemp.rhs);
        nodeTemp.rhs = std::move(treap);
        treap = std::move(temp);
      }
    } else {
      if (node.rhs) {
        insertRec(node.rhs, std::move(region));
      } else {
        node.rhs = std::move(region);
      }
      if (node.rhs->getSize() > node.getSize() ||
          (node.rhs->getSize() == node.getSize() &&
           node.rhs->hash() > node.hash())) {
        auto temp = std::move(node.rhs);
        auto &nodeTemp = temp.acquire();
        node.rhs = std::move(nodeTemp.lhs);
        nodeTemp.lhs = std::move(treap);
        treap = std::move(temp);
      }
    }
  }

  static void mergeTreaps(CoWPtr<Node> *target, CoWPtr<Node> lhs,
                          CoWPtr<Node> rhs) {
    assert(!*target && "empty the target first");
    assert(
        (lhs || rhs) &&
        "merging two empty subtreaps should be handled by hand, as it does not "
        "require "
        "acquisition of the object holding `*target` (usually a parent node)");
    for (;;) {
      assert(!*target);
      if (!lhs) {
        *target = std::move(rhs);
        break;
      }
      if (!rhs) {
        *target = std::move(lhs);
        break;
      }
      assert(lhs->getBaseAddress() < rhs->getBaseAddress() &&
             "tree property violated");
      if (lhs->getSize() > rhs->getSize() ||
          (lhs->getSize() == rhs->getSize() && lhs->hash() > rhs->hash())) {
        // Ties w.r.t. the heap property (size) are explicitly broken using a
        // hash derived from the tree key. At the time of writing, the "hash"
        // function is a permutation (if `std::size_t` and `std::uintptr_t` are
        // of the same size), which means that hash collisions are not just
        // unlikely, but rather impossible.
        Node &lhsNode = lhs.acquire();
        *target = std::move(lhs);
        lhs = std::move(lhsNode.rhs);
        target = &lhsNode.rhs;
      } else {
        Node &rhsNode = rhs.acquire();
        *target = std::move(rhs);
        rhs = std::move(rhsNode.lhs);
        target = &rhsNode.lhs;
      }
    }
    assert(!lhs && "lhs must have been consumed");
    assert(!rhs && "rhs must have been consumed");
  }

public:
  constexpr SizedRegions() = default;
  SizedRegions(SizedRegions const &other) = default;
  SizedRegions &operator=(SizedRegions const &other) = default;
  SizedRegions(SizedRegions &&other) = default;
  SizedRegions &operator=(SizedRegions &&other) = default;

  [[nodiscard]] bool isEmpty() const noexcept { return !root; }

  /// Computes the LocationInfo. This functionality really belongs to the
  /// `LargeObjectAllocator`, as it assumes that this treap contains free
  /// regions in between allocations. It also knows that there is a redzone at
  /// the beginning and end and in between any two allocations.
  inline LocationInfo getLocationInfo(char const *const address,
                                      std::size_t const size) const noexcept {
    assert(root && "Cannot compute location_info for an empty treap");

    Node const *currentNode = &*root;
    Node const *closestPredecessor = nullptr;
    for (;;) {
      if (currentNode->getBaseAddress() <= address) {
        if (address < currentNode->getBaseAddress() + currentNode->getSize()) {
          if (address + size <=
              currentNode->getBaseAddress() + currentNode->getSize()) {
            return LocationInfo::LI_Unallocated;
          } else {
            return LocationInfo::LI_Unaligned;
          }
        } else {
          closestPredecessor = currentNode;
          if (!currentNode->rhs) {
            return {LocationInfo::LI_AllocatedOrQuarantined,
                    closestPredecessor->getBaseAddress() +
                        closestPredecessor->getSize()};
          }
          currentNode = &*currentNode->rhs;
        }
      } else {
        assert(closestPredecessor &&
               "If there is no closest predecessor, there must not be a "
               "predecessor at all. Since regions are only ever split, "
               "this would mean that the lookup is outside the valid "
               "range, which has to be handled by the caller.");
        if (currentNode->getBaseAddress() < address + size) {
          return LocationInfo::LI_Unaligned;
        }
        if (!currentNode->lhs) {
          return {LocationInfo::LI_AllocatedOrQuarantined,
                  closestPredecessor->getBaseAddress() +
                      closestPredecessor->getSize()};
        }
        currentNode = &*currentNode->lhs;
      }
    }
  }

  void insert(char *const baseAddress, std::size_t const size) {
    assert(size > 0 && "region to be inserted must contain at least one byte");
    insert(CoWPtr<Node>(CoWPtr<Node>::in_place_t{}, baseAddress, size));
  }

  void insert(CoWPtr<Node> region) {
    assert(region && "region to be inserted must exist");
    assert(region->getSize() > 0 &&
           "region to be inserted must contain at least one byte");
    if (region->lhs || region->rhs) {
      if (region.isOwned()) {
        auto &node = region.acquire();
        node.lhs.release();
        node.rhs.release();
      } else {
        // If region is not owned, then acquiring it would copy the `lhs` and
        // `rhs` members, thereby incrementing and decrementing the refcounts in
        // unrelated objects. To avoid this, we simply recreate the region.
        region = CoWPtr<Node>(CoWPtr<Node>::in_place_t{},
                              region->getBaseAddress(), region->getSize());
      }
    }
    assert(!region->lhs);
    assert(!region->rhs);

    auto *target = &root;
    while (*target && ((*target)->getSize() > region->getSize() ||
                       ((*target)->getSize() == region->getSize() &&
                        (*target)->hash() > region->hash()))) {
      auto &node = target->acquire();
      assert(node.getBaseAddress() != region->getBaseAddress() &&
             "multiple insertions of the same tree-key are not supported");
      target = region->getBaseAddress() < node.getBaseAddress() ? &node.lhs
                                                                : &node.rhs;
    }
    if (!*target) {
      *target = std::move(region);
    } else {
      insertRec(*target, std::move(region));
    }
  }

  [[nodiscard]] CoWPtr<Node> const &getLargestRegion() const noexcept {
    assert(root && "querying the largest region only makes sense if the "
                   "treap is not empty");
    return root;
  }

  CoWPtr<Node> extractLargestRegion() {
    assert(root && "cannot extract the largest region of an empty treap");

    auto result = std::move(root);
    if (result->lhs || result->rhs) {
      auto &node = result.acquire();
      mergeTreaps(&root, std::move(node.lhs), std::move(node.rhs));
    }
    return result;
  }

private:
  static void pushDownRightHeap(CoWPtr<Node> *target, CoWPtr<Node> update,
                                CoWPtr<Node> rhs, std::size_t const newSize,
                                std::uintptr_t const newHash) {
    while (rhs && (newSize < rhs->getSize() ||
                   (newSize == rhs->getSize() && newHash < rhs->hash()))) {
      // optimization opportunity: once lhs does not exist anymore, it will
      // never exist in the future
      *target = std::move(rhs);
      auto &targetNode = target->acquire();
      rhs = std::move(targetNode.lhs);
      target = &targetNode.lhs;
    }

    update.getOwned().rhs = std::move(rhs);
    *target = std::move(update);
  }

  static void pushDownLeftHeap(CoWPtr<Node> *target, CoWPtr<Node> update,
                               CoWPtr<Node> lhs, std::size_t const newSize,
                               std::uintptr_t const newHash) {
    while (lhs && (newSize < lhs->getSize() ||
                   (newSize == lhs->getSize() && newHash < lhs->hash()))) {
      // optimization opportunity: once lhs does not exist anymore, it will
      // never exist in the future
      *target = std::move(lhs);
      auto &targetNode = target->acquire();
      lhs = std::move(targetNode.rhs);
      target = &targetNode.rhs;
    }

    update.getOwned().lhs = std::move(lhs);
    *target = std::move(update);
  }

public:
  void resizeLargestRegion(std::size_t const newSize) {
    assert(root && "updating the largest region only makes sense if the "
                   "treap is not empty");

    auto update = std::move(root);
    auto &node = update.acquire();
    node.setSize(newSize);
    auto const newHash = node.hash();
    auto lhs = std::move(node.lhs);
    auto rhs = std::move(node.rhs);

    auto *target = &root;

    if (!lhs || !(newSize < lhs->getSize() ||
                  (newSize == lhs->getSize() && newHash < lhs->hash()))) {
      node.lhs = std::move(lhs);
      pushDownRightHeap(target, std::move(update), std::move(rhs), newSize,
                        newHash);
    } else if (!rhs ||
               !(newSize < rhs->getSize() ||
                 (newSize == rhs->getSize() && newHash < rhs->hash()))) {
      node.rhs = std::move(rhs);
      pushDownLeftHeap(target, std::move(update), std::move(lhs), newSize,
                       newHash);
    } else {
      for (;;) {
        assert(lhs && (newSize < lhs->getSize() ||
                       (newSize == lhs->getSize() && newHash < lhs->hash())));
        assert(rhs && (newSize < rhs->getSize() ||
                       (newSize == rhs->getSize() && newHash < rhs->hash())));

        if (lhs->getSize() < rhs->getSize() ||
            (lhs->getSize() == rhs->getSize() && lhs->hash() < rhs->hash())) {
          *target = std::move(rhs);
          auto &targetNode = target->acquire();
          rhs = std::move(targetNode.lhs);
          target = &targetNode.lhs;

          if (!rhs || !(newSize < rhs->getSize() ||
                        (newSize == rhs->getSize() && newHash < rhs->hash()))) {
            node.rhs = std::move(rhs);
            pushDownLeftHeap(target, std::move(update), std::move(lhs), newSize,
                             newHash);
            return;
          }
        } else {
          *target = std::move(lhs);
          auto &targetNode = target->acquire();
          lhs = std::move(targetNode.rhs);
          target = &targetNode.rhs;

          if (!lhs || !(newSize < lhs->getSize() ||
                        (newSize == lhs->getSize() && newHash < lhs->hash()))) {
            node.lhs = std::move(lhs);
            pushDownRightHeap(target, std::move(update), std::move(rhs),
                              newSize, newHash);
            return;
          }
        }
      }
    }
  }

  CoWPtr<Node> extractRegion(char const *const baseAddress) {
    CoWPtr<Node> *target = &root;
    for (;;) {
      assert(*target && "cannot extract region that is not part of the treap");
      if ((*target)->getBaseAddress() < baseAddress) {
        target = &target->acquire().rhs;
      } else if ((*target)->getBaseAddress() > baseAddress) {
        target = &target->acquire().lhs;
      } else {
        assert((*target)->getBaseAddress() == baseAddress);
        auto result = std::move(*target);
        if (result->lhs || result->rhs) {
          auto &node = result.acquire();
          mergeTreaps(target, std::move(node.lhs), std::move(node.rhs));
        }
        return result;
      }
    }
  }

private:
  static CoWPtr<Node> mergeRegionWithPreviousRec(CoWPtr<Node> &treap,
                                                 char *const baseAddress) {
    assert(treap && "cannot extract region that is not part of the treap");
    if (baseAddress < treap->getBaseAddress()) {
      auto &node = treap.acquire();
      if (auto greater = mergeRegionWithPreviousRec(node.lhs, baseAddress)) {
        if (node.getBaseAddress() < greater->getBaseAddress()) {
          auto newSize = static_cast<std::size_t>(greater->getBaseAddress() -
                                                  node.getBaseAddress() +
                                                  greater->getSize());
          assert(
              newSize > node.getSize() &&
              "Sizes only get greater by adding `greater - smaller` to them");
          node.setSize(newSize);
          return {};
        } else {
          return greater;
        }
      } else {
        if (node.lhs->getSize() > node.getSize() ||
            (node.lhs->getSize() == node.getSize() &&
             node.lhs->hash() > node.hash())) {
          auto temp = std::move(node.lhs);
          auto &nodeTemp = temp.getOwned();
          node.lhs = std::move(nodeTemp.rhs);
          nodeTemp.rhs = std::move(treap);
          treap = std::move(temp);
        }
        return {};
      }
    } else if (treap->getBaseAddress() < baseAddress) {
      auto &node = treap.acquire();
      if (auto greater = mergeRegionWithPreviousRec(node.rhs, baseAddress)) {
        if (node.getBaseAddress() < greater->getBaseAddress()) {
          auto newSize = static_cast<std::size_t>(greater->getBaseAddress() -
                                                  node.getBaseAddress() +
                                                  greater->getSize());
          assert(
              newSize > node.getSize() &&
              "Sizes only get greater by adding `greater - smaller` to them");
          node.setSize(newSize);
          return {};
        } else {
          return greater;
        }
      } else {
        if (node.rhs->getSize() > node.getSize() ||
            (node.rhs->getSize() == node.getSize() &&
             node.rhs->hash() > node.hash())) {
          auto temp = std::move(node.rhs);
          auto &nodeTemp = temp.getOwned();
          node.rhs = std::move(nodeTemp.lhs);
          nodeTemp.lhs = std::move(treap);
          treap = std::move(temp);
        }
        return greater;
      }
    } else {
      assert(treap->getBaseAddress() == baseAddress);
      // target is now the greater (w.r.t. the tree key) of the two regions we
      // are looking for
      if (treap->lhs) {
        CoWPtr<Node> lhs, rhs;
        if (treap.isOwned()) {
          auto &node = treap.acquire();
          lhs = std::move(node.lhs);
          rhs = std::move(node.rhs);
        } else {
          lhs = treap->lhs;
          rhs = treap->rhs;
        }
        auto const greaterBaseAddress = treap->getBaseAddress();
        auto const greaterSize = treap->getSize();

        auto *target = &lhs;
        while ((*target)->rhs) {
          target = &target->acquire().rhs;
        }
        treap = std::move(*target);
        auto &node = treap.acquire();
        *target = std::move(node.lhs);

        assert(greaterBaseAddress > node.getBaseAddress());
        auto newSize = static_cast<std::size_t>(
            greaterBaseAddress - node.getBaseAddress() + greaterSize);
        assert(newSize > node.getSize() &&
               "Sizes only get greater by adding `greater - smaller` to them");
        node.setSize(newSize);

        assert(!node.rhs && "We looked for a node that has no rhs");
        assert((!rhs || node.getBaseAddress() < rhs->getBaseAddress()) &&
               "`lesser` comes from the left subtree of `greater`, while "
               "rhs is the right subtree of `greater`");
        assert((!rhs || node.getSize() > rhs->getSize()) &&
               "The old size of `greater` was >= that of its right subtree, "
               "so the new size (of `lesser`) is strictly greater");
        node.rhs = std::move(rhs);

        assert(!node.lhs && "The lhs of this node has already been removed");
        assert((!lhs || lhs->getBaseAddress() < node.getBaseAddress()) &&
               "`lesser` is the greatest child of the `lhs` subtree");
        assert((!lhs || node.getSize() > lhs->getSize()) &&
               "The old size of `greater` was >= that of its left subtree, "
               "so the new size (of `lesser`) is strictly greater");
        node.lhs = std::move(lhs);
        return {};
      } else {
        auto greater = std::move(treap);
        if (greater->rhs) {
          if (greater.isOwned()) {
            treap = std::move(greater.acquire().rhs);
          } else {
            treap = greater->rhs;
          }
        }
        return greater; // no move to enable copy elision
      }
    }
  }

public:
  /// This function merges the region starting at the given address, with the
  /// region immediately preceding it. Both regions must exist. The resulting
  /// region has the same base address as the lesser region with its size large
  /// enough to cover the space to the end of the greater region (filling any
  /// holes in the process).
  inline void mergeRegionWithPrevious(char *const baseAddress) {
    assert(root && "An empty treap holds no regions to merge");
    mergeRegionWithPreviousRec(root, baseAddress);
  }

private:
  template <typename OutStream>
  static void dumpRec(OutStream &out, std::string &prefix,
                      CoWPtr<Node> const &treap) {
    if (treap) {
      out << "[" << static_cast<void *>(treap->getBaseAddress()) << "; "
          << static_cast<void *>(treap->getBaseAddress() + treap->getSize())
          << ") of size " << treap->getSize() << "\n";

      auto oldPrefix = prefix.size();

      out << prefix << "├";
      prefix += "│";
      dumpRec(out, prefix, treap->lhs);

      prefix.resize(oldPrefix);

      out << prefix << "╰";
      prefix += " ";
      dumpRec(out, prefix, treap->rhs);
    } else {
      out << "<nil>\n";
    }
  }

public:
  template <typename OutStream> OutStream &dump(OutStream &out) {
    std::string prefix;
    dumpRec(out, prefix, root);
    return out;
  }

private:
  static void checkInvariants(std::pair<bool, bool> &result,
                              CoWPtr<Node> const &treap) {
    if (!result.first && !result.second) {
      return;
    }

    if (treap->lhs) {
      if (treap->lhs->getBaseAddress() >= treap->getBaseAddress()) {
        result.first = false;
      }
      if (treap->lhs->getSize() > treap->getSize()) {
        result.second = false;
      }
      checkInvariants(result, treap->lhs);
    }
    if (treap->rhs) {
      if (treap->rhs->getBaseAddress() <= treap->getBaseAddress()) {
        result.first = false;
      }
      if (treap->rhs->getSize() > treap->getSize()) {
        result.second = false;
      }
      checkInvariants(result, treap->rhs);
    }
  }

public:
  std::pair<bool, bool> checkInvariants() const {
    std::pair<bool, bool> result = {true, true};
    if (root) {
      checkInvariants(result, root);
    }
    return result;
  }
};
} // namespace klee::kdalloc::suballocators

#endif
