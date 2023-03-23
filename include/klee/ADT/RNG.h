//===-- RNG.h ---------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_RNG_H
#define KLEE_RNG_H

#include <random>

namespace klee {
  struct RNG : std::mt19937 {
    RNG();
    explicit RNG(RNG::result_type seed);

    /* generates a random number on [0,0xffffffff]-interval */
    unsigned int getInt32();
    /* generates a random number on [0,0x7fffffff]-interval */
    int getInt31();
    /* generates a random number on [0,1]-real-interval */
    double getDoubleLR();
    float getFloatLR();
    /* generates a random number on [0,1)-real-interval */
    double getDoubleL();
    float getFloatL();
    /* generates a random number on (0,1)-real-interval */
    double getDouble();
    float getFloat();
    /* generators a random flop */
    bool getBool();
  };
}

#endif /* KLEE_RNG_H */
