//===-- TreeStream.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "TreeStreamWriter"
#include "klee/Internal/ADT/TreeStream.h"

#include "klee/Internal/Support/Debug.h"

#include <cassert>
#include <iomanip>
#include <fstream>
#include <iterator>
#include <map>

#include "llvm/Support/raw_ostream.h"
#include <string.h>

using namespace klee;

///

TreeStreamWriter::TreeStreamWriter(const std::string &_path) 
  : lastID(0),
    bufferCount(0),
    path(_path),
    output(new std::ofstream(path.c_str(), 
                             std::ios::out | std::ios::binary)),
    ids(1) {
  if (!output->good()) {
    delete output;
    output = 0;
  }
}

TreeStreamWriter::~TreeStreamWriter() {
  flush();
  if (output)
    delete output;
}

bool TreeStreamWriter::good() {
  return !!output;
}

TreeOStream TreeStreamWriter::open() {
  return open(TreeOStream(*this, 0));
}

TreeOStream TreeStreamWriter::open(const TreeOStream &os) {
  assert(output && os.writer==this);
  flushBuffer();
  unsigned id = ids++;
  output->write(reinterpret_cast<const char*>(&os.id), 4);
  unsigned tag = id | (1<<31);
  output->write(reinterpret_cast<const char*>(&tag), 4);
  return TreeOStream(*this, id);
}

void TreeStreamWriter::write(TreeOStream &os, const char *s, unsigned size) {
#if 1
  if (bufferCount && 
      (os.id!=lastID || size+bufferCount>bufferSize))
    flushBuffer();
  if (bufferCount) { // (os.id==lastID && size+bufferCount<=bufferSize)
    memcpy(&buffer[bufferCount], s, size);
    bufferCount += size;
  } else if (size<bufferSize) {
    lastID = os.id;
    memcpy(buffer, s, size);
    bufferCount = size;
  } else {
    output->write(reinterpret_cast<const char*>(&os.id), 4);
    output->write(reinterpret_cast<const char*>(&size), 4);
    output->write(buffer, size);
  }
#else
  output->write(reinterpret_cast<const char*>(&os.id), 4);
  output->write(reinterpret_cast<const char*>(&size), 4);
  output->write(s, size);
#endif
}

void TreeStreamWriter::flushBuffer() {
  if (bufferCount) {    
    output->write(reinterpret_cast<const char*>(&lastID), 4);
    output->write(reinterpret_cast<const char*>(&bufferCount), 4);
    output->write(buffer, bufferCount);
    bufferCount = 0;
  }
}

void TreeStreamWriter::flush() {
  flushBuffer();
  output->flush();
}

void TreeStreamWriter::readStream(TreeStreamID streamID,
                                  std::vector<unsigned char> &out) {
  assert(streamID>0 && streamID<ids);
  flush();
  
  std::ifstream is(path.c_str(),
                   std::ios::in | std::ios::binary);
  assert(is.good());
  KLEE_DEBUG(llvm::errs() << "finding chain for: " << streamID << "\n");

  std::map<unsigned,unsigned> parents;
  std::vector<unsigned> roots;
  for (;;) {
    assert(is.good());
    unsigned id;
    unsigned tag;
    is.read(reinterpret_cast<char*>(&id), 4);
    is.read(reinterpret_cast<char*>(&tag), 4);
    if (tag&(1<<31)) { // fork
      unsigned child = tag ^ (1<<31);

      if (child==streamID) {
        roots.push_back(child);
        while (id) {
          roots.push_back(id);
          std::map<unsigned, unsigned>::iterator it = parents.find(id);
          assert(it!=parents.end());
          id = it->second;
        } 
        break;
      } else {
        parents.insert(std::make_pair(child,id));
      }
    } else {
      unsigned size = tag;
      while (size--) is.get();
    }
  }
  KLEE_DEBUG({
      llvm::errs() << "roots: ";
      for (size_t i = 0, e = roots.size(); i < e; ++i) {
        llvm::errs() << roots[i] << " ";
      }
      llvm::errs() << "\n";
    });
  is.seekg(0, std::ios::beg);
  for (;;) {
    unsigned id;
    unsigned tag;
    is.read(reinterpret_cast<char*>(&id), 4);
    is.read(reinterpret_cast<char*>(&tag), 4);
    if (!is.good()) break;
    if (tag&(1<<31)) { // fork
      unsigned child = tag ^ (1<<31);
      if (id==roots.back() && roots.size()>1 && child==roots[roots.size()-2])
        roots.pop_back();
    } else {
      unsigned size = tag;
      if (id==roots.back()) {
        while (size--) out.push_back(is.get());
      } else {
        while (size--) is.get();
      }
    }
  }  
}

///

TreeOStream::TreeOStream()
  : writer(0),
    id(0) {
}

TreeOStream::TreeOStream(TreeStreamWriter &_writer, unsigned _id)
  : writer(&_writer),
    id(_id) {
}

TreeOStream::~TreeOStream() {
}

unsigned TreeOStream::getID() const {
  assert(writer);
  return id;
}

void TreeOStream::write(const char *buffer, unsigned size) {
  assert(writer);
  writer->write(*this, buffer, size);
}

TreeOStream &TreeOStream::operator<<(const std::string &s) {
  assert(writer);
  write(s.c_str(), s.size());
  return *this;
}

void TreeOStream::flush() {
  assert(writer);
  writer->flush();
}

