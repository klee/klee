// -*- C++ -*-
extern "C" {
  #include "TestCase.h"
}

#include <vector>

Offset createOffset(unsigned offset, unsigned index);

ConcretizedObject createConcretizedObject(char *name, unsigned char *values,
                                          unsigned size, Offset *offsets,
                                          unsigned n_offsets);

ConcretizedObject
createConcretizedObject(const char *name, std::vector<unsigned char> &values);
