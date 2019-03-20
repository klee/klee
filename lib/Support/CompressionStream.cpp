//===-- CompressionStream.cpp --------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"
#include "klee/Config/Version.h"
#ifdef HAVE_ZLIB_H
#include "klee/Internal/Support/CompressionStream.h"

#include "llvm/Support/FileSystem.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace klee {

compressed_fd_ostream::compressed_fd_ostream(const std::string &Filename,
                                             std::string &ErrorInfo)
    : llvm::raw_ostream(), pos(0) {
  ErrorInfo = "";
  // Open file in binary mode
#if LLVM_VERSION_CODE >= LLVM_VERSION(7, 0)
  std::error_code EC =
      llvm::sys::fs::openFileForWrite(Filename, FD);
#else
  std::error_code EC =
      llvm::sys::fs::openFileForWrite(Filename, FD, llvm::sys::fs::F_None);
#endif
  if (EC) {
    ErrorInfo = EC.message();
    FD = -1;
    return;
  }
  // Initialize the compression library
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.next_in = Z_NULL;
  strm.next_out = buffer;
  strm.avail_in = 0;
  strm.avail_out = BUFSIZE;

  const auto ret = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, 31,
                                8 /* memory usage default, 0 smallest, 9 highest*/,
                                Z_DEFAULT_STRATEGY);
  if (ret != Z_OK)
    ErrorInfo = "Deflate initialisation returned with error: " + std::to_string(ret);
}

void compressed_fd_ostream::writeFullCompressedData() {
  // Check if no space available and write the buffer
  if (strm.avail_out == 0) {
    write_file(reinterpret_cast<const char *>(buffer), BUFSIZE);
    strm.next_out = buffer;
    strm.avail_out = BUFSIZE;
  }
}

void compressed_fd_ostream::flush_compressed_data() {
  // flush data from the raw buffer
  flush();

  // write the remaining data
  int deflate_res = Z_OK;
  while (deflate_res == Z_OK) {
    // Check if no space available and write the buffer
    writeFullCompressedData();
    deflate_res = deflate(&strm, Z_FINISH);
  }
  assert(deflate_res == Z_STREAM_END);
  write_file(reinterpret_cast<const char *>(buffer), BUFSIZE - strm.avail_out);
}

compressed_fd_ostream::~compressed_fd_ostream() {
  if (FD >= 0) {
    // write the remaining data
    flush_compressed_data();
    close(FD);
  }
  deflateEnd(&strm);
}

void compressed_fd_ostream::write_impl(const char *Ptr, size_t Size) {
  strm.next_in =
      const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>(Ptr));
  strm.avail_in = Size;

  // Check if there is still data to compress
  while (strm.avail_in != 0) {
    // compress data
    const auto res __attribute__ ((unused)) = deflate(&strm, Z_NO_FLUSH);
    assert(res == Z_OK);
    writeFullCompressedData();
  }
}

void compressed_fd_ostream::write_file(const char *Ptr, size_t Size) {
  pos += Size;
  assert(FD >= 0 && "File already closed");
  do {
    ssize_t ret = ::write(FD, Ptr, Size);
    if (ret < 0) {
      if (errno == EINTR || errno == EAGAIN)
        continue;
      assert(0 && "Could not write to file");
      break;
    }

    Ptr += ret;
    Size -= ret;
  } while (Size > 0);
}
}
#endif
