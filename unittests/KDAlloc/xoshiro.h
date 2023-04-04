//===-- xoshiro.h ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cstdint>

/// A xoshiro512** generator
class xoshiro512 {
  static constexpr const ::std::uint64_t A = 11;
  static constexpr const ::std::uint64_t B = 21;

  static constexpr const ::std::uint64_t S = 5;
  static constexpr const ::std::uint64_t R = 7;
  static constexpr const ::std::uint64_t T = 9;

  ::std::uint64_t s[8] = {0x243F6A8885A308D3u, 0x13198A2E03707344u,
                          0xA4093822299F31D0u, 0x082EFA98EC4E6C89u,
                          0x452821E638D01377u, 0xBE5466CF34E90C6Cu,
                          0xC0AC29B7C97C50DDu, 0x3F84D5B5B5470917u};

public:
  explicit xoshiro512(std::uint64_t const seed) noexcept {
    for (auto &x : s) {
      x ^= seed;
    }
  }

  using result_type = std::uint64_t;

  std::uint64_t operator()() noexcept {
    auto const t = s[1] * S;
    auto const result = ((t << R) | (t >> (64 - R))) * T;
    auto const u = s[1] << A;
    s[2] ^= s[0];
    s[5] ^= s[1];
    s[1] ^= s[2];
    s[7] ^= s[3];
    s[3] ^= s[4];
    s[4] ^= s[5];
    s[0] ^= s[6];
    s[6] ^= s[7];
    s[6] ^= u;
    s[7] = ((s[7] << B) | (s[7] >> (64 - B)));
    return result;
  }

  static constexpr ::std::uint64_t(min)() noexcept { return 0; }
  static constexpr ::std::uint64_t(max)() noexcept {
    return static_cast<std::uint64_t>(0) - static_cast<std::uint64_t>(1);
  }
};
