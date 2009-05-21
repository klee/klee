//===-- TreeStream.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __UTIL_TREESTREAM_H__
#define __UTIL_TREESTREAM_H__

#include <string>
#include <iostream>
#include <vector>

namespace klee {

  typedef unsigned TreeStreamID;
  class TreeOStream;

  class TreeStreamWriter {
    static const unsigned bufferSize = 4*4096;

    friend class TreeOStream;

  private:
    char buffer[bufferSize];
    unsigned lastID, bufferCount;

    std::string path;
    std::ofstream *output;
    unsigned ids;

    void write(TreeOStream &os, const char *s, unsigned size);
    void flushBuffer();

  public:
    TreeStreamWriter(const std::string &_path);
    ~TreeStreamWriter();

    bool good();

    TreeOStream open();
    TreeOStream open(const TreeOStream &node);

    void flush();

    // hack, to be replace by proper stream capabilities
    void readStream(TreeStreamID id,
                    std::vector<unsigned char> &out);
  };

  class TreeOStream {
    friend class TreeStreamWriter;

  private:
    TreeStreamWriter *writer;
    unsigned id;
    
    TreeOStream(TreeStreamWriter &_writer, unsigned _id);

  public:
    TreeOStream();
    ~TreeOStream();

    unsigned getID() const;

    void write(const char *buffer, unsigned size);

    TreeOStream &operator<<(const std::string &s);

    void flush();
  };
}

#endif
