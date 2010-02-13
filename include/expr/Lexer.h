//===-- Lexer.h -------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXPR_LEXER_H
#define KLEE_EXPR_LEXER_H

#include <string>

namespace llvm {
  class MemoryBuffer;
}

namespace klee {
namespace expr {
  struct Token {
    enum Kind {
      At,                       ///< '@'
      Arrow,                    ///< '->'
      Colon,                    ///< ':'
      Comma,                    ///< ','
      Comment,                  ///< #[^\n]+
      EndOfFile,                ///< <end of file>
      Equals,                   ///< ' = '
      Identifier,               ///< [a-zA-Z_][a-zA-Z0-9._]*
      KWArray,                  ///< 'array'
      KWFalse,                  ///< 'false'
      KWQuery,                  ///< 'query'
      KWReserved,               ///< fp[0-9]+([.].*)?, i[0-9]+
      KWSymbolic,               ///< 'symbolic'
      KWTrue,                   ///< 'true'
      KWWidth,                  ///< w[0-9]+
      LBrace,                   ///< '{'
      LParen,                   ///< '('
      LSquare,                  ///< '['
      Number,                   ///< [+-]?[0-9][a-zA-Z0-9_]+
      RBrace,                   ///< '}'
      RParen,                   ///< ')'
      RSquare,                  ///< ']'
      Semicolon,                ///< ';'
      Unknown,                  ///< <other>
      
      KWKindFirst=KWArray,
      KWKindLast=KWWidth
    };

    Kind        kind;           /// The token kind.
    const char *start;          /// The beginning of the token string. 
    unsigned    length;         /// The length of the token.
    unsigned    line;           /// The line number of the start of this token.
    unsigned    column;         /// The column number at the start of
                                /// this token.

    /// getKindName - The name of this token's kind.
    const char *getKindName() const;

    /// getString - The string spanned by this token. This is not
    /// particularly efficient, use start and length when reasonable.
    std::string getString() const { return std::string(start, length); }

    /// isKeyword - True if this token is a keyword.
    bool isKeyword() const { 
      return kind >= KWKindFirst && kind <= KWKindLast; 
    }

    // dump - Dump the token to stderr.
    void dump();
  };

  /// Lexer - Interface for lexing tokens from a .pc language file.
  class Lexer {
    const char *BufferPos;      /// The current lexer position.
    const char *BufferEnd;      /// The buffer end position.
    unsigned    LineNumber;     /// The current line.
    unsigned    ColumnNumber;   /// The current column.

    /// GetNextChar - Eat a character or -1 from the stream.
    int GetNextChar();

    /// PeekNextChar - Return the next character without consuming it
    /// from the stream. This does not perform newline
    /// canonicalization.
    int PeekNextChar();

    /// SetTokenKind - Set the token kind and length (using the
    /// token's start pointer, which must have been initialized).
    Token &SetTokenKind(Token &Result, Token::Kind k);

    /// SetTokenKind - Set an identifiers token kind. This has the
    /// same requirements as SetTokenKind and additionally takes care
    /// of keyword recognition.
    Token &SetIdentifierTokenKind(Token &Result);
  
    void SkipToEndOfLine();

    /// LexNumber - Lex a number which does not have a base specifier.
    Token &LexNumber(Token &Result);

    /// LexIdentifier - Lex an identifier.
    Token &LexIdentifier(Token &Result);

  public:
    explicit Lexer(const llvm::MemoryBuffer *_buf);
    ~Lexer();

    /// Lex - Return the next token from the file or EOF continually
    /// when the end of the file is reached. The input argument is
    /// used as the result, for convenience.
    Token &Lex(Token &Result);
  };
}
}

#endif
