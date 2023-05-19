//===-- mapping.h -----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KDALLOC_MAPPING_H
#define KDALLOC_MAPPING_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>

#include <sys/mman.h>
#include <unistd.h>

#if defined(__linux__)
#include <linux/version.h>
#endif

#if defined(__APPLE__)
#include <mach/vm_map.h>
#include <sys/types.h>
#endif

#include "klee/Support/ErrorHandling.h"

namespace klee::kdalloc {
class Mapping {
  void *baseAddress = MAP_FAILED;
  std::size_t size = 0;

  bool try_map(std::uintptr_t baseAddress) noexcept {
    assert(this->baseAddress == MAP_FAILED);

    int flags = MAP_ANON | MAP_PRIVATE;
#if defined(__linux__)
    flags |= MAP_NORESERVE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
    if (baseAddress != 0) {
      flags |= MAP_FIXED_NOREPLACE;
    }
#endif
#elif defined(__FreeBSD__)
    if (baseAddress != 0) {
      flags |= MAP_FIXED | MAP_EXCL;
    }
#endif

    auto mappedAddress = ::mmap(reinterpret_cast<void *>(baseAddress), size,
                                PROT_READ | PROT_WRITE, flags, -1, 0);
    if (mappedAddress == MAP_FAILED) {
      this->baseAddress = MAP_FAILED;
      return false;
    }
    if (baseAddress != 0 &&
        baseAddress != reinterpret_cast<std::uintptr_t>(mappedAddress)) {
      [[maybe_unused]] int rc = ::munmap(mappedAddress, size);
      assert(rc == 0 && "munmap failed");
      this->baseAddress = MAP_FAILED;
      return false;
    }
    this->baseAddress = mappedAddress;

#if defined(__linux__)
    {
      [[maybe_unused]] int rc =
          ::madvise(this->baseAddress, size,
                    MADV_NOHUGEPAGE | MADV_DONTFORK | MADV_RANDOM);
      assert(rc == 0 && "madvise failed");
    }
#elif defined(__FreeBSD__)
    {
      [[maybe_unused]] int rc =
          ::minherit(this->baseAddress, size, INHERIT_NONE);
      assert(rc == 0 && "minherit failed");
    }
#elif defined(__APPLE__)
    {
      [[maybe_unused]] int rc =
          ::minherit(this->baseAddress, size, VM_INHERIT_NONE);
      assert(rc == 0 && "minherit failed");
    }
#endif

    return true;
  }

public:
  Mapping() = default;

  explicit Mapping(std::size_t size) noexcept : Mapping(0, size) {}

  Mapping(std::uintptr_t baseAddress, std::size_t size) noexcept : size(size) {
    try_map(baseAddress);
  }

  Mapping(Mapping const &) = delete;
  Mapping &operator=(Mapping const &) = delete;

  Mapping(Mapping &&other) noexcept
      : baseAddress(other.baseAddress), size(other.size) {
    other.baseAddress = MAP_FAILED;
    other.size = 0;
  }
  Mapping &operator=(Mapping &&other) noexcept {
    if (&other != this) {
      using std::swap;
      swap(other.baseAddress, baseAddress);
      swap(other.size, size);
    }
    return *this;
  }

  [[nodiscard]] void *getBaseAddress() const noexcept {
    assert(*this && "Invalid mapping");
    return baseAddress;
  }

  [[nodiscard]] std::size_t getSize() const noexcept { return size; }

  void clear() {
    assert(*this && "Invalid mapping");

#if defined(__linux__)
    [[maybe_unused]] int rc = ::madvise(baseAddress, size, MADV_DONTNEED);
    assert(rc == 0 && "madvise failed");
#else
    auto address = reinterpret_cast<std::uintptr_t>(baseAddress);
    [[maybe_unused]] int rc = ::munmap(baseAddress, size);
    assert(rc == 0 && "munmap failed");
    baseAddress = MAP_FAILED;
    [[maybe_unused]] auto success = try_map(address);
    assert(success && "could not recreate the mapping");
#endif
  }

  explicit operator bool() const noexcept { return baseAddress != MAP_FAILED; }

  ~Mapping() {
    if (*this) {
      [[maybe_unused]] int rc = ::munmap(baseAddress, size);
      assert(rc == 0 && "munmap failed");
    }
  }
};
} // namespace klee::kdalloc

#endif
