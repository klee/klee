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

namespace klee {
  class RNG {
  private:
    /* Period parameters */  
    static const int N = 624;
    static const int M = 397;
    static const unsigned int MATRIX_A = 0x9908b0dfUL;   /* constant vector a */
    static const unsigned int UPPER_MASK = 0x80000000UL; /* most significant w-r bits */
    static const unsigned int LOWER_MASK = 0x7fffffffUL; /* least significant r bits */
      
  private:
    unsigned int mt[N]; /* the array for the state vector  */
    int mti;
    
  public:
    RNG(unsigned int seed=5489UL);
  
    void seed(unsigned int seed);
    
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
