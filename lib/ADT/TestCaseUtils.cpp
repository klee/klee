// -*- C++ -*-
#include "klee/ADT/TestCaseUtils.h"
#include <cstring>
#include <cstdlib>

Offset createOffset(unsigned offset, unsigned index) {
  Offset ret;
  ret.offset = offset;
  ret.index = index;
  return ret;
}

ConcretizedObject createConcretizedObject(char *name, unsigned char *values,
                                          unsigned size, Offset *offsets, unsigned n_offsets) {
  ConcretizedObject ret;
  ret.name = new char[strlen(name)];
  strcpy(ret.name, name);
  
  ret.values = new unsigned char[size];
  for(size_t i = 0; i<size; i++) {
    ret.values[i] = values[i];
  }
  
  ret.offsets = (Offset*)malloc(sizeof(Offset)*n_offsets);
  for(size_t i = 0; i<n_offsets; i++) {
    ret.offsets[i] = offsets[i];
  }
  return ret;
}

ConcretizedObject
createConcretizedObject(const char *name, std::vector<unsigned char> &values) {
  ConcretizedObject ret;
  ret.name = new char[strlen(name)];
  strcpy(ret.name, name);
  ret.size = values.size();
  ret.values = new unsigned char[values.size()];
  for(size_t i = 0; i<values.size(); i++) {
    ret.values[i] = values[i];
  }
  ret.n_offsets = 0;

  return ret;
}
