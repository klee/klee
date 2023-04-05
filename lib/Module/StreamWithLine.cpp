#include "StreamWithLine.h"

StreamWithLine::StreamWithLine(std::unique_ptr<llvm::raw_fd_ostream> assemblyFS)
    : assemblyFS(std::move(assemblyFS)), value(), state(State::NewLine) {}

void StreamWithLine::write_impl(const char *Ptr, size_t Size) {
  const char *start = Ptr;
  const char *end = Ptr + Size;
  for (const char *ch = Ptr; ch < end; ++ch) {
    switch (state) {
    case State::NewLine:
      if (*ch == '%') {
        assemblyFS->write(start, ch - start);
        start = ch;
        state = State::FirstP;
        continue;
      }
      break;
    case State::FirstP:
      if (*ch == '%') {
        state = State::SecondP;
        continue;
      }
      assemblyFS->write("%", 1);
      start = ch;
      break;
    case State::SecondP:
      if (*ch == '%') {
        state = State::ThirdP;
        continue;
      }
      assemblyFS->write("%%", 2);
      start = ch;
      break;
    case State::ThirdP:
      if (isdigit(*ch)) {
        value = *ch;
        state = State::Num;
        continue;
      }
      assemblyFS->write("%%%", 3);
      start = ch;
      break;
    case State::Num:
      if (isdigit(*ch)) {
        value += *ch;
        state = State::Num;
        continue;
      }
      start = ch;
      mapping.emplace(stoull(value), line);
      break;
    case State::Another:
      break;
    }

    if (*ch == '\n') {
      state = State::NewLine;
      line++;
      continue;
    }
    state = State::Another;
  }

  switch (state) {
  case State::NewLine:
    assemblyFS->write(start, end - start);
  case State::Another:
    assemblyFS->write(start, end - start);
    break;
  case State::FirstP:
  case State::SecondP:
  case State::ThirdP:
  case State::Num:
    break;
  }

  curPos += Size;
}

uint64_t StreamWithLine::current_pos() const { return curPos; }

std::unordered_map<uintptr_t, uint64_t> StreamWithLine::getMapping() const {
  return mapping;
}
