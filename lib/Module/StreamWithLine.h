#ifndef KLEE_STREAMWITHLINE_H
#define KLEE_STREAMWITHLINE_H

#include <llvm/Support/raw_ostream.h>
#include <unordered_map>

class StreamWithLine : public llvm::raw_ostream {
  uint64_t line = 0;
  uint64_t curPos = 0;
  std::string value = "";
  std::unordered_map<uintptr_t, uint64_t> mapping = {};
  std::unique_ptr<llvm::raw_fd_ostream> assemblyFS;

  enum State { NewLine, FirstP, SecondP, ThirdP, Num, Another } state;

  void write_impl(const char *Ptr, size_t Size) override;
  virtual uint64_t current_pos() const override;

public:
  StreamWithLine(std::unique_ptr<llvm::raw_fd_ostream> assemblyFS);

  std::unordered_map<uintptr_t, uint64_t> getMapping() const;
};

#endif // KLEE_STREAMWITHLINE_H
