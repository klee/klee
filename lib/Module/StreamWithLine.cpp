#include "StreamWithLine.h"

StreamWithLine::StreamWithLine(std::unique_ptr<llvm::raw_fd_ostream> assemblyFS)
    : assemblyFS(std::move(assemblyFS)) {}

void StreamWithLine::write_impl(const char *Ptr, size_t Size) {
  const char *start = Ptr;
  const char *valid_end = Ptr;
  const char *end = Ptr + Size;
  for (const char *ch = Ptr; ch < end; ++ch) {
    switch (state) {
    case State::NewLine:
      if (*ch == '%') {
        valid_end = ch;
        state = State::FirstP;
        continue;
      }
      break;
    case State::FirstP:
      if (*ch == '%') {
        state = State::SecondP;
        continue;
      }
      break;
    case State::SecondP:
      if (*ch == '%') {
        state = State::ThirdP;
        continue;
      }
      break;
    case State::ThirdP:
      if (isdigit(*ch)) {
        assemblyFS->write(start, valid_end - start);
        start = valid_end;
        value = *ch;
        state = State::Num;
        continue;
      }
      break;
    case State::Num:
      if (isdigit(*ch)) {
        value += *ch;
        state = State::Num;
        continue;
      }
      start = ch;
      valid_end = ch;
      mapping.emplace(stoull(value), line);
      break;
    case State::Another:
      break;
    }

    switch (*ch) {
    case '\n':
      state = State::NewLine;
      line++;
      break;
    default:
      state = State::Another;
    }
  }
  assemblyFS->write(start, end - start);
  curPos += Size;
}

uint64_t StreamWithLine::current_pos() const { return curPos; }

std::unordered_map<uintptr_t, uint64_t> StreamWithLine::getMapping() const {
  return mapping;
}
