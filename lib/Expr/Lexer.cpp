//===-- Lexer.cpp ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "expr/Lexer.h"

#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include <iomanip>
#include <iostream>
#include <string.h>

using namespace llvm;
using namespace klee;
using namespace klee::expr;

///

const char *Token::getKindName() const {
  switch (kind) {
  default:
  case Unknown:    return "Unknown";
  case Arrow:      return "Arrow";
  case At:         return "At";
  case Colon:      return "Colon";
  case Comma:      return "Comma";
  case Comment:    return "Comment";
  case EndOfFile:  return "EndOfFile";
  case Equals:     return "Equals";
  case Identifier: return "Identifier";
  case KWArray:    return "KWArray";
  case KWFalse:    return "KWFalse";
  case KWQuery:    return "KWQuery";
  case KWReserved: return "KWReserved";
  case KWSymbolic: return "KWSymbolic";
  case KWTrue:     return "KWTrue";
  case KWWidth:    return "KWWidth";
  case LBrace:     return "LBrace";
  case LParen:     return "LParen";
  case LSquare:    return "LSquare";
  case Number:     return "Number";
  case RBrace:     return "RBrace";
  case RParen:     return "RParen";
  case RSquare:    return "RSquare";
  case Semicolon:  return "Semicolon";
  }
}

void Token::dump() {
  llvm::errs() << "(Token \"" << getKindName() << "\" "
               << (void*) start << " " << length << " "
               << line << " " << column << ")";
}

///

static inline bool isInternalIdentifierChar(int Char) {
  return isalnum(Char) || Char == '_' || Char == '.' || Char == '-';
}

Lexer::Lexer(const llvm::MemoryBuffer *MB) 
  : BufferPos(MB->getBufferStart()), BufferEnd(MB->getBufferEnd()), 
    LineNumber(1), ColumnNumber(0) {
}

Lexer::~Lexer() {
}

int Lexer::PeekNextChar() {
  if (BufferPos == BufferEnd)
    return -1;
  return *BufferPos;
}

int Lexer::GetNextChar() {
  if (BufferPos == BufferEnd)
    return -1;

  // Handle DOS/Mac newlines here, by stripping duplicates and by
  // returning '\n' for both.
  char Result = *BufferPos++;
  if (Result == '\n' || Result == '\r') {
    if (BufferPos != BufferEnd && *BufferPos == ('\n' + '\r' - Result))
      ++BufferPos;
    Result = '\n';
  }

  if (Result == '\n') {
    ++LineNumber;
    ColumnNumber = 0;
  } else {
    ++ColumnNumber;
  }

  return Result;
}

Token &Lexer::SetTokenKind(Token &Result, Token::Kind k) {
  Result.kind = k;
  Result.length = BufferPos - Result.start;
  return Result;
}

static bool isReservedKW(const char *Str, unsigned N) {
    unsigned i;

  // Check for i[0-9]+
  if (N>1 && Str[0] == 'i') {
    for (i=1; i<N; ++i)
      if (!isdigit(Str[i]))
        break;
    if (i==N)
      return true;
  }

  // Check for fp[0-9]+([.].*)?$
  if (N>3 && Str[0]=='f' && Str[1]=='p' && isdigit(Str[2])) {
    for (i=3; i<N; ++i)
      if (!isdigit(Str[i]))
        break;
    if (i==N || Str[i]=='.')
      return true;
  }
  
  return false;
}
static bool isWidthKW(const char *Str, unsigned N) {
  if (N<2 || Str[0] != 'w')
    return false;
  for (unsigned i=1; i<N; ++i)
    if (!isdigit(Str[i]))
      return false;
  return true;
}
Token &Lexer::SetIdentifierTokenKind(Token &Result) {
  unsigned Length = BufferPos - Result.start;
  switch (Length) {
  case 3:
    if (memcmp("def", Result.start, 3) == 0)
      return SetTokenKind(Result, Token::KWReserved);
    if (memcmp("var", Result.start, 3) == 0)
      return SetTokenKind(Result, Token::KWReserved);
    break;

  case 4:
    if (memcmp("true", Result.start, 4) == 0)
      return SetTokenKind(Result, Token::KWTrue);
    break;

  case 5:
    if (memcmp("array", Result.start, 5) == 0)
      return SetTokenKind(Result, Token::KWArray);
    if (memcmp("false", Result.start, 5) == 0)
      return SetTokenKind(Result, Token::KWFalse);
    if (memcmp("query", Result.start, 5) == 0)
      return SetTokenKind(Result, Token::KWQuery);
    break;      
    
  case 6:
    if (memcmp("define", Result.start, 6) == 0)
      return SetTokenKind(Result, Token::KWReserved);
    break;

  case 7:
    if (memcmp("declare", Result.start, 7) == 0)
      return SetTokenKind(Result, Token::KWReserved);
    break;

  case 8:
    if (memcmp("symbolic", Result.start, 8) == 0)
      return SetTokenKind(Result, Token::KWSymbolic);
    break;
  }

  if (isReservedKW(Result.start, Length))
    return SetTokenKind(Result, Token::KWReserved);
  if (isWidthKW(Result.start, Length))
    return SetTokenKind(Result, Token::KWWidth);

  return SetTokenKind(Result, Token::Identifier);
}

void Lexer::SkipToEndOfLine() {
  for (;;) {
    int Char = GetNextChar();
    if (Char == -1 || Char =='\n')
      break;
  }
}

Token &Lexer::LexNumber(Token &Result) {
  while (isalnum(PeekNextChar()) || PeekNextChar()=='_')
    GetNextChar();
  return SetTokenKind(Result, Token::Number);
}

Token &Lexer::LexIdentifier(Token &Result) {
  while (isInternalIdentifierChar(PeekNextChar()))
    GetNextChar();

  // Recognize keywords specially.
  return SetIdentifierTokenKind(Result);
}

Token &Lexer::Lex(Token &Result) {
  Result.kind = Token::Unknown;
  Result.length = 0;
  Result.start = BufferPos;
  
  // Skip whitespace.
  while (isspace(PeekNextChar()))
    GetNextChar();

  Result.start = BufferPos;
  Result.line = LineNumber;
  Result.column = ColumnNumber;
  int Char = GetNextChar();
  switch (Char) {
  case -1:  return SetTokenKind(Result, Token::EndOfFile);
    
  case '(': return SetTokenKind(Result, Token::LParen);
  case ')': return SetTokenKind(Result, Token::RParen);
  case ',': return SetTokenKind(Result, Token::Comma);
  case ':': return SetTokenKind(Result, Token::Colon);
  case ';': return SetTokenKind(Result, Token::Semicolon);
  case '=': return SetTokenKind(Result, Token::Equals);
  case '@': return SetTokenKind(Result, Token::At);
  case '[': return SetTokenKind(Result, Token::LSquare);
  case ']': return SetTokenKind(Result, Token::RSquare);
  case '{': return SetTokenKind(Result, Token::LBrace);
  case '}': return SetTokenKind(Result, Token::RBrace);

  case '#':
    SkipToEndOfLine();
    return SetTokenKind(Result, Token::Comment);

  case '+': {
    if (isdigit(PeekNextChar()))
      return LexNumber(Result);
    else
      return SetTokenKind(Result, Token::Unknown);
  }

  case '-': {
    int Next = PeekNextChar();
    if (Next == '>')
      return GetNextChar(), SetTokenKind(Result, Token::Arrow);
    else if (isdigit(Next))
      return LexNumber(Result);
    else
      return SetTokenKind(Result, Token::Unknown);
    break;
  }

  default:
    if (isdigit(Char))
      return LexNumber(Result);
    else if (isalpha(Char) || Char == '_')
      return LexIdentifier(Result);
    return SetTokenKind(Result, Token::Unknown);
  }
}
