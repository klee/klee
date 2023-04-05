#include "StreamWithLine.h"

StreamWithLine::StreamWithLine(llvm::StringRef Filename, std::error_code &EC)
    : fs(Filename, EC), state(State::NewLine) {}

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
        fs.write(start, valid_end - start);
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
  fs.write(start, end - start);
  curpos += Size;
}

uint64_t StreamWithLine::current_pos() const { return curpos; }

std::unordered_map<uintptr_t, uint64_t> StreamWithLine::getMapping() const {
  return mapping;
}
