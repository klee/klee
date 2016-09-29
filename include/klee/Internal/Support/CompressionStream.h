//===-- CompressionStream.h --------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_KLEE_INTERNAL_SUPPORT_COMPRESSIONSTREAM_H_
#define INCLUDE_KLEE_INTERNAL_SUPPORT_COMPRESSIONSTREAM_H_

#include "llvm/Support/raw_ostream.h"
#include "zlib.h"

namespace klee {
const size_t BUFSIZE = 128 * 1024;

class compressed_fd_ostream : public llvm::raw_ostream {
  int FD;
  uint8_t buffer[BUFSIZE];
  z_stream strm;
  uint64_t pos;

  /// write_impl - See raw_ostream::write_impl.
  virtual void write_impl(const char *Ptr, size_t Size);
  void write_file(const char *Ptr, size_t Size);

  virtual uint64_t current_pos() const { return pos; }

  void flush_compressed_data();
  void writeFullCompressedData();

public:
  /// compressed_fd_ostream - Open the specified file for writing. If an error
  /// occurs, information about the error is put into ErrorInfo, and the stream
  /// should be immediately destroyed; the string will be empty if no error
  /// occurred. This allows optional flags to control how the file will be
  /// opened.
  compressed_fd_ostream(const char *Filename, std::string &ErrorInfo);

  ~compressed_fd_ostream();
};
}

#endif /* INCLUDE_KLEE_INTERNAL_SUPPORT_COMPRESSIONSTREAM_H_ */
