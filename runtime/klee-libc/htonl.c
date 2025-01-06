/*===-- htonl.c -----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <stdint.h>

#undef htons
#undef htonl
#undef ntohs
#undef ntohl

/* Make sure we can recognize the endianness. */
#if !defined(__BYTE_ORDER__) || !defined(__ORDER_LITTLE_ENDIAN__)
#error "Unknown platform endianness!"
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

uint16_t htons(uint16_t v) {
  return (v >> 8) | (v << 8);
}
uint32_t htonl(uint32_t v) {
  return htons(v >> 16) | (htons((uint16_t) v) << 16);
}

#else

uint16_t htons(uint16_t v) {
  return v;
}
uint32_t htonl(uint32_t v) {
  return v;
}

#endif

uint16_t ntohs(uint16_t v) {
  return htons(v);
}
uint32_t ntohl(uint32_t v) {
  return htonl(v);
}
