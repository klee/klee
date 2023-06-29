//===-- cow_ptr.h -----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KDALLOC_UTIL_COW_H
#define KDALLOC_UTIL_COW_H

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>

namespace klee::kdalloc::suballocators {
template <typename T> class CoWPtr {
  struct Wrapper {
    std::size_t referenceCount;
    T data;

    template <typename... V>
    explicit Wrapper(std::size_t const referenceCount, V &&...args)
        : referenceCount(referenceCount), data(std::forward<V>(args)...) {}
  };

  Wrapper *ptr = nullptr;

public:
  constexpr CoWPtr() = default;

  CoWPtr(CoWPtr const &other) noexcept : ptr(other.ptr) {
    if (ptr != nullptr) {
      ++ptr->referenceCount;
      assert(ptr->referenceCount > 1);
    }
  }

  CoWPtr &operator=(CoWPtr const &other) {
    if (this != &other) {
      release();

      ptr = other.ptr;
      if (ptr != nullptr) {
        ++ptr->referenceCount;
        assert(ptr->referenceCount > 1);
      }
    }
    return *this;
  }

  CoWPtr(CoWPtr &&other) noexcept : ptr(std::exchange(other.ptr, nullptr)) {}

  CoWPtr &operator=(CoWPtr &&other) noexcept {
    if (this != &other) {
      release();
      ptr = std::exchange(other.ptr, nullptr);
    }
    return *this;
  }

  /// A stand-in for C++17's `std::in_place_t`
  struct in_place_t {};

  template <typename... V>
  explicit CoWPtr(in_place_t, V &&...args)
      : ptr(new Wrapper(1, std::forward<V>(args)...)) {}

  ~CoWPtr() { release(); }

  /// Returns `true` iff `*this` is not in an empty state.
  [[nodiscard]] explicit operator bool() const noexcept { return !isEmpty(); }

  /// Returns `true` iff `*this` is in an empty state.
  [[nodiscard]] bool isEmpty() const noexcept { return ptr == nullptr; }

  /// Returns `true` iff `*this` is not in an empty state and owns the CoW
  /// object.
  [[nodiscard]] bool isOwned() const noexcept {
    return ptr != nullptr && ptr->referenceCount == 1;
  }

  /// Accesses an existing object.
  /// Must not be called when `*this` is in an empty state.
  T const &operator*() const noexcept {
    assert(!isEmpty() && "the `CoWPtr` must not be empty");
    return *get();
  }

  /// Accesses an existing object.
  /// Must not be called when `*this` is in an empty state.
  T const *operator->() const noexcept {
    assert(!isEmpty() && "the `CoWPtr` must not be empty");
    return get();
  }

  /// Gets a pointer to the managed object.
  /// Returns `nullptr` if no object is currently managed.
  T const *get() const noexcept { return ptr ? &ptr->data : nullptr; }

  /// Gets a pointer to an existing, owned object.
  /// Must not be called when `*this` does not hold CoW ownership.
  T *getOwned() noexcept {
    assert(isOwned() && "the `CoWPtr` must be owned");
    return &ptr->data;
  }

  /// Acquires CoW ownership of an existing object and returns a reference to
  /// the owned object. Must not be called when `*this` is in an empty state.
  T &acquire() {
    assert(ptr != nullptr &&
           "May only call `acquire` for an active CoW object");
    assert(ptr->referenceCount > 0);
    if (ptr->referenceCount > 1) {
      --ptr->referenceCount;
      ptr = new Wrapper{*ptr};
      ptr->referenceCount = 1;
    }
    assert(ptr->referenceCount == 1);
    return ptr->data;
  }

  /// Releases CoW ownership of an existing object or does nothing if the object
  /// does not exist. Leaves `*this` in an empty state.
  void release() noexcept(noexcept(delete ptr)) {
    if (ptr != nullptr) {
      --ptr->referenceCount;
      if (ptr->referenceCount == 0) {
        delete ptr;
      }
      ptr = nullptr;
    }
  }

  /// Leaves `*this` holding and owning an object `T(std::forward<V>(args)...)`.
  /// This function is always safe to call and will perform the operation as
  /// efficient as possible. Notably, `foo.emplace(...)` may be more efficient
  /// than the otherwise equivalent `foo = CoWPtr(CoWPtr::in_place_t{}, ...)`.
  template <typename... V> T &emplace(V &&...args) {
    if (ptr) {
      if (ptr->referenceCount == 1) {
        ptr->data = T(std::forward<V>(args)...);
      } else {
        auto *new_ptr = new Wrapper(
            1, std::forward<V>(args)...); // possibly throwing operation

        assert(ptr->referenceCount > 1);
        --ptr->referenceCount;
        ptr = new_ptr;
      }
    } else {
      ptr = new Wrapper(1, std::forward<V>(args)...);
    }
    assert(ptr != nullptr);
    assert(ptr->referenceCount == 1);
    return ptr->data;
  }
};
} // namespace klee::kdalloc::suballocators

#endif
