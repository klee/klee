//===-- Parser.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "expr/Parser.h"

#include "expr/Lexer.h"

#include "klee/Constraints.h"
#include "klee/Solver.h"
#include "klee/util/ExprPPrinter.h"

#include "llvm/ADT/APInt.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Streams.h"

#include <cassert>
#include <iostream>
#include <map>
#include <cstring>

using namespace llvm;
using namespace klee;
using namespace klee::expr;

namespace {
  /// ParseResult - Represent a possibly invalid parse result.
  template<typename T>
  struct ParseResult {
    bool IsValid;
    T Value;

  public:
    ParseResult() : IsValid(false) {}
    ParseResult(T _Value) : IsValid(true), Value(_Value) {}
    ParseResult(bool _IsValid, T _Value) : IsValid(_IsValid), Value(_Value) {}

    bool isValid() { 
      return IsValid; 
    }
    T get() { 
      assert(IsValid && "get() on invalid ParseResult!");
      return Value; 
    }
  };
  
  class ExprResult {
    bool IsValid;
    ExprHandle Value;

  public:
    ExprResult() : IsValid(false) {}
    ExprResult(ExprHandle _Value) : IsValid(true), Value(_Value) {}
    ExprResult(ref<ConstantExpr> _Value) : IsValid(true), Value(_Value.get()) {}
    ExprResult(bool _IsValid, ExprHandle _Value) : IsValid(_IsValid), Value(_Value) {}

    bool isValid() { 
      return IsValid; 
    }
    ExprHandle get() { 
      assert(IsValid && "get() on invalid ParseResult!");
      return Value; 
    }
  };

  typedef ParseResult<Decl*> DeclResult;
  typedef ParseResult<Expr::Width> TypeResult;
  typedef ParseResult<VersionHandle> VersionResult;

  /// NumberOrExprResult - Represent a number or expression. This is used to
  /// wrap an expression production which may be a number, but for
  /// which the type width is unknown.
  class NumberOrExprResult {
    Token AsNumber;
    ExprResult AsExpr;
    bool IsNumber;

  public:
    NumberOrExprResult() : IsNumber(false) {}
    explicit NumberOrExprResult(Token _AsNumber) : AsNumber(_AsNumber),
                                                   IsNumber(true) {}
    explicit NumberOrExprResult(ExprResult _AsExpr) : AsExpr(_AsExpr),
                                                      IsNumber(false) {}
    
    bool isNumber() const { return IsNumber; }
    const Token &getNumber() const { 
      assert(IsNumber && "Invalid accessor call.");
      return AsNumber; 
    }
    const ExprResult &getExpr() const {
      assert(!IsNumber && "Invalid accessor call.");
      return AsExpr;
    }
  };

  /// ParserImpl - Parser implementation.
  class ParserImpl : public Parser {
    typedef std::map<const std::string, const Identifier*> IdentifierTabTy;
    typedef std::map<const Identifier*, ExprHandle> ExprSymTabTy;
    typedef std::map<const Identifier*, VersionHandle> VersionSymTabTy;

    const std::string Filename;
    const MemoryBuffer *TheMemoryBuffer;
    Lexer TheLexer;
    unsigned MaxErrors;
    unsigned NumErrors;

    // FIXME: Use LLVM symbol tables?
    IdentifierTabTy IdentifierTab;

    std::map<const Identifier*, const ArrayDecl*> ArraySymTab;
    ExprSymTabTy ExprSymTab;
    VersionSymTabTy VersionSymTab;

    /// Tok - The currently lexed token.
    Token Tok;

    /// ParenLevel - The current depth of matched '(' tokens. 
    unsigned ParenLevel;
    /// SquareLevel - The current depth of matched '[' tokens.
    unsigned SquareLevel;

    /* Core parsing functionality */
    
    const Identifier *GetOrCreateIdentifier(const Token &Tok);

    void GetNextNonCommentToken() {
      do {
        TheLexer.Lex(Tok);
      } while (Tok.kind == Token::Comment);
    }

    /// ConsumeToken - Consume the current 'peek token' and lex the next one.
    void ConsumeToken() {
      assert(Tok.kind != Token::LParen && Tok.kind != Token::RParen);
      GetNextNonCommentToken();
    }

    /// ConsumeExpectedToken - Check that the current token is of the
    /// expected kind and consume it.
    void ConsumeExpectedToken(Token::Kind k) {
      assert(Tok.kind == k && "Unexpected token!");
      GetNextNonCommentToken();
    }

    void ConsumeLParen() {
      ++ParenLevel;
      ConsumeExpectedToken(Token::LParen);
    }

    void ConsumeRParen() {
      if (ParenLevel) // Cannot go below zero.
        --ParenLevel;
      ConsumeExpectedToken(Token::RParen);
    }

    void ConsumeLSquare() {
      ++SquareLevel;
      ConsumeExpectedToken(Token::LSquare);
    }

    void ConsumeRSquare() {
      if (SquareLevel) // Cannot go below zero.
        --SquareLevel;
      ConsumeExpectedToken(Token::RSquare);
    }

    void ConsumeAnyToken() {
      switch (Tok.kind) {
      case Token::LParen: return ConsumeLParen();
      case Token::RParen: return ConsumeRParen();
      case Token::LSquare: return ConsumeLSquare();
      case Token::RSquare: return ConsumeRSquare();
      default: 
        return ConsumeToken();
      }
    }

    /* Utility functions */

    /// SkipUntilRParen - Scan forward to the next token following an
    /// rparen at the given level, or EOF, whichever is first.
    void SkipUntilRParen(unsigned Level) {
      // FIXME: I keep wavering on whether it is an error to call this
      // with the current token an rparen. In most cases this should
      // have been handled differently (error reported,
      // whatever). Audit & resolve.
      assert(Level <= ParenLevel && 
             "Refusing to skip until rparen at higher level.");
      while (Tok.kind != Token::EndOfFile) {
        if (Tok.kind == Token::RParen && ParenLevel == Level) {
          ConsumeRParen();
          break;
        }
        ConsumeAnyToken();
      }
    }

    /// SkipUntilRParen - Scan forward until reaching an rparen token
    /// at the current level (or EOF).
    void SkipUntilRParen() {
      SkipUntilRParen(ParenLevel);
    }
    
    /// ExpectRParen - Utility method to close an sexp. This expects to
    /// eat an rparen, and emits a diagnostic and skips to the next one
    /// (or EOF) if it cannot.
    void ExpectRParen(const char *Msg) {
      if (Tok.kind == Token::EndOfFile) {
        // FIXME: Combine with Msg
        Error("expected ')' but found end-of-file.", Tok);
      } else if (Tok.kind != Token::RParen) {
        Error(Msg, Tok);
        SkipUntilRParen();
      } else {
        ConsumeRParen();
      }
    }

    /// SkipUntilRSquare - Scan forward to the next token following an
    /// rsquare at the given level, or EOF, whichever is first.
    void SkipUntilRSquare(unsigned Level) {
      // FIXME: I keep wavering on whether it is an error to call this
      // with the current token an rparen. In most cases this should
      // have been handled differently (error reported,
      // whatever). Audit & resolve.
      assert(Level <= ParenLevel && 
             "Refusing to skip until rparen at higher level.");
      while (Tok.kind != Token::EndOfFile) {
        if (Tok.kind == Token::RSquare && ParenLevel == Level) {
          ConsumeRSquare();
          break;
        }
        ConsumeAnyToken();
      }
    }

    /// SkipUntilRSquare - Scan forward until reaching an rsquare token
    /// at the current level (or EOF).
    void SkipUntilRSquare() {
      SkipUntilRSquare(ParenLevel);
    }
    
    /// ExpectRSquare - Utility method to close an array. This expects
    /// to eat an rparen, and emits a diagnostic and skips to the next
    /// one (or EOF) if it cannot.
    void ExpectRSquare(const char *Msg) {
      if (Tok.kind == Token::EndOfFile) {
        // FIXME: Combine with Msg
        Error("expected ']' but found end-of-file.", Tok);
      } else if (Tok.kind != Token::RSquare) {
        Error(Msg, Tok);
        SkipUntilRSquare();
      } else {
        ConsumeRSquare();
      }
    }

    /*** Grammar productions ****/

    /* Top level decls */

    DeclResult ParseArrayDecl();
    DeclResult ParseExprVarDecl();
    DeclResult ParseVersionVarDecl();
    DeclResult ParseCommandDecl();

    /* Commands */

    DeclResult ParseQueryCommand();

    /* Etc. */

    NumberOrExprResult ParseNumberOrExpr();

    ExprResult ParseExpr(TypeResult ExpectedType);
    ExprResult ParseParenExpr(TypeResult ExpectedType);
    ExprResult ParseUnaryParenExpr(const Token &Name,
                                   unsigned Kind, bool IsFixed,
                                   Expr::Width ResTy);
    ExprResult ParseBinaryParenExpr(const Token &Name,
                                    unsigned Kind, bool IsFixed,
                                    Expr::Width ResTy);
    ExprResult ParseSelectParenExpr(const Token &Name, Expr::Width ResTy);
    ExprResult ParseConcatParenExpr(const Token &Name, Expr::Width ResTy);
    ExprResult ParseExtractParenExpr(const Token &Name, Expr::Width ResTy);
    ExprResult ParseAnyReadParenExpr(const Token &Name,
                                     unsigned Kind,
                                     Expr::Width ResTy);
    void ParseMatchedBinaryArgs(const Token &Name, 
                                TypeResult ExpectType,
                                ExprResult &LHS, ExprResult &RHS);
    ExprResult ParseNumber(Expr::Width Width);
    ExprResult ParseNumberToken(Expr::Width Width, const Token &Tok);

    VersionResult ParseVersionSpecifier();
    VersionResult ParseVersion();

    TypeResult ParseTypeSpecifier();

    /*** Diagnostics ***/
    
    void Error(const char *Message, const Token &At);
    void Error(const char *Message) { Error(Message, Tok); }

  public:
    ParserImpl(const std::string _Filename,
               const MemoryBuffer *MB) : Filename(_Filename),
                                         TheMemoryBuffer(MB),
                                         TheLexer(MB),
                                         MaxErrors(~0u),
                                         NumErrors(0) {}

    /// Initialize - Initialize the parsing state. This must be called
    /// prior to the start of parsing.
    void Initialize() {
      ParenLevel = SquareLevel = 0;

      ConsumeAnyToken();
    }

    /* Parser interface implementation */

    virtual Decl *ParseTopLevelDecl();

    virtual void SetMaxErrors(unsigned N) {
      MaxErrors = N;
    }

    virtual unsigned GetNumErrors() const {
      return NumErrors; 
    }
  };
}

const Identifier *ParserImpl::GetOrCreateIdentifier(const Token &Tok) {
  // FIXME: Make not horribly inefficient please.
  assert(Tok.kind == Token::Identifier && "Expected only identifier tokens.");
  std::string Name(Tok.start, Tok.length);
  IdentifierTabTy::iterator it = IdentifierTab.find(Name);
  if (it != IdentifierTab.end())
    return it->second;

  Identifier *I = new Identifier(Name);
  IdentifierTab.insert(std::make_pair(Name, I));

  return I;
}

Decl *ParserImpl::ParseTopLevelDecl() {
  // Repeat until success or EOF.
  while (Tok.kind != Token::EndOfFile) {
    // Only handle commands for now.
    if (Tok.kind == Token::LParen) {
      DeclResult Res = ParseCommandDecl();
      if (Res.isValid())
        return Res.get();
    } else {
      Error("expected '(' token.");
      ConsumeAnyToken();
    }
  }

  return 0;
}

/// ParseCommandDecl - Parse a command declaration. The lexer should
/// be positioned at the opening '('.
///
/// command = '(' name ... ')'
DeclResult ParserImpl::ParseCommandDecl() {
  ConsumeLParen();

  if (!Tok.isKeyword()) {
    Error("malformed command.");
    SkipUntilRParen();
    return DeclResult();
  }

  switch (Tok.kind) {
  case Token::KWQuery:
    return ParseQueryCommand();

  default:
    Error("malformed command (unexpected keyword).");
    SkipUntilRParen();
    return DeclResult();
  }
}

/// ParseQueryCommand - Parse query command. The lexer should be
/// positioned at the 'query' keyword.
/// 
/// 'query' expressions-list expression [expression-list [array-list]]
DeclResult ParserImpl::ParseQueryCommand() {
  // FIXME: We need a command for this. Or something.
  ExprSymTab.clear();
  VersionSymTab.clear();

  std::vector<ExprHandle> Constraints;
  ConsumeExpectedToken(Token::KWQuery);
  if (Tok.kind != Token::LSquare) {
    Error("malformed query, expected constraint list.");
    SkipUntilRParen();
    return DeclResult();
  }

  ConsumeExpectedToken(Token::LSquare);
  // FIXME: Should avoid reading past unbalanced parens here.
  while (Tok.kind != Token::RSquare) {
    if (Tok.kind == Token::EndOfFile) {
      Error("unexpected end of file.");
      return new QueryCommand(Constraints.begin(), Constraints.end(),
                              ConstantExpr::alloc(false, Expr::Bool));
    }

    ExprResult Res = ParseExpr(TypeResult(Expr::Bool));
    if (Res.isValid())
      Constraints.push_back(Res.get());
  }

  ConsumeRSquare();

  ExprResult Res = ParseExpr(TypeResult());
  if (!Res.isValid()) // Error emitted by ParseExpr.
    Res = ExprResult(ConstantExpr::alloc(0, Expr::Bool));

  ExpectRParen("unexpected argument to 'query'.");  
  return new QueryCommand(Constraints.begin(), Constraints.end(),
                          Res.get());
}

/// ParseNumberOrExpr - Parse an expression whose type cannot be
/// predicted.
NumberOrExprResult ParserImpl::ParseNumberOrExpr() {
  if (Tok.kind == Token::Number){ 
    Token Num = Tok;
    ConsumeToken();
    return NumberOrExprResult(Num);
  } else {
    return NumberOrExprResult(ParseExpr(TypeResult()));
  }
}

/// ParseExpr - Parse an expression with the given \arg
/// ExpectedType. \arg ExpectedType can be invalid if the type cannot
/// be inferred from the context.
///
/// expr = false | true
/// expr = <constant>
/// expr = <identifier>
/// expr = [<identifier>:] paren-expr
ExprResult ParserImpl::ParseExpr(TypeResult ExpectedType) {
  // FIXME: Is it right to need to do this here?
  if (Tok.kind == Token::EndOfFile) {
    Error("unexpected end of file.");
    return ExprResult();
  }

  if (Tok.kind == Token::KWFalse || Tok.kind == Token::KWTrue) {
    bool Value = Tok.kind == Token::KWTrue;
    ConsumeToken();
    return ExprResult(ConstantExpr::alloc(Value, Expr::Bool));
  }
  
  if (Tok.kind == Token::Number) {
    if (!ExpectedType.isValid()) {
      Error("cannot infer type of number.");
      ConsumeToken();
      return ExprResult();
    }
    
    return ParseNumber(ExpectedType.get());
  }

  const Identifier *Label = 0;
  if (Tok.kind == Token::Identifier) {
    Token LTok = Tok;
    Label = GetOrCreateIdentifier(Tok);
    ConsumeToken();

    if (Tok.kind != Token::Colon) {
      ExprSymTabTy::iterator it = ExprSymTab.find(Label);

      if (it == ExprSymTab.end()) {
        Error("invalid expression label reference.", LTok);
        return ExprResult();
      }

      return it->second;
    }

    ConsumeToken();
    if (ExprSymTab.count(Label)) {
      Error("duplicate expression label definition.", LTok);
      Label = 0;
    }
  }

  Token Start = Tok;
  ExprResult Res = ParseParenExpr(ExpectedType);
  if (!Res.isValid()) {
    // If we know the type, define the identifier just so we don't get
    // use-of-undef errors. 
    // FIXME: Maybe we should let the symbol table map to invalid
    // entries?
    if (Label && ExpectedType.isValid()) {
      ref<Expr> Value = ConstantExpr::alloc(0, ExpectedType.get());
      ExprSymTab.insert(std::make_pair(Label, Value));
    }
    return Res;
  } else if (ExpectedType.isValid()) {
    // Type check result.    
    if (Res.get()->getWidth() != ExpectedType.get()) {
      // FIXME: Need more info, and range
      Error("expression has incorrect type.", Start);
      return ExprResult();
    }
  }

  if (Label)
    ExprSymTab.insert(std::make_pair(Label, Res.get()));
  return Res;
}

// Additional kinds for macro forms.
enum MacroKind {
  eMacroKind_Not = Expr::LastKind + 1,     // false == x
  eMacroKind_Neg,                          // 0 - x
  eMacroKind_ReadLSB,                      // Multibyte read
  eMacroKind_ReadMSB,                      // Multibyte write
  eMacroKind_Concat,                       // Magic concatenation syntax
  eMacroKind_LastMacroKind = eMacroKind_ReadMSB
};

/// LookupExprInfo - Return information on the named token, if it is
/// recognized.
///
/// \param Kind [out] - The Expr::Kind or MacroKind of the identifier.
/// \param IsFixed [out] - True if the given kinds result and
/// (expression) arguments are all of the same width.
/// \param NumArgs [out] - The number of expression arguments for this
/// kind. -1 indicates the kind is variadic or has non-expression
/// arguments.
/// \return True if the token is a valid kind or macro name.
static bool LookupExprInfo(const Token &Tok, unsigned &Kind, 
                           bool &IsFixed, int &NumArgs) {
#define SetOK(kind, isfixed, numargs) (Kind=kind, IsFixed=isfixed,\
                                       NumArgs=numargs, true)
  assert(Tok.kind == Token::Identifier && "Unexpected token.");
  
  switch (Tok.length) {
  case 2:
    if (memcmp(Tok.start, "Eq", 2) == 0)
      return SetOK(Expr::Eq, false, 2);
    if (memcmp(Tok.start, "Ne", 2) == 0)
      return SetOK(Expr::Ne, false, 2);

    if (memcmp(Tok.start, "Or", 2) == 0)
      return SetOK(Expr::Or, true, 2);
    break;

  case 3:
    if (memcmp(Tok.start, "Add", 3) == 0)
      return SetOK(Expr::Add, true, 2);
    if (memcmp(Tok.start, "Sub", 3) == 0)
      return SetOK(Expr::Sub, true, 2);
    if (memcmp(Tok.start, "Mul", 3) == 0)
      return SetOK(Expr::Mul, true, 2);

    if (memcmp(Tok.start, "And", 3) == 0)
      return SetOK(Expr::And, true, 2);
    if (memcmp(Tok.start, "Shl", 3) == 0)
      return SetOK(Expr::Shl, true, 2);
    if (memcmp(Tok.start, "Xor", 3) == 0)
      return SetOK(Expr::Xor, true, 2);

    if (memcmp(Tok.start, "Not", 3) == 0)
      return SetOK(eMacroKind_Not, true, 1);
    if (memcmp(Tok.start, "Neg", 3) == 0)
      return SetOK(eMacroKind_Neg, true, 1);
    if (memcmp(Tok.start, "Ult", 3) == 0)
      return SetOK(Expr::Ult, false, 2);
    if (memcmp(Tok.start, "Ule", 3) == 0)
      return SetOK(Expr::Ule, false, 2);
    if (memcmp(Tok.start, "Ugt", 3) == 0)
      return SetOK(Expr::Ugt, false, 2);
    if (memcmp(Tok.start, "Uge", 3) == 0)
      return SetOK(Expr::Uge, false, 2);
    if (memcmp(Tok.start, "Slt", 3) == 0)
      return SetOK(Expr::Slt, false, 2);
    if (memcmp(Tok.start, "Sle", 3) == 0)
      return SetOK(Expr::Sle, false, 2);
    if (memcmp(Tok.start, "Sgt", 3) == 0)
      return SetOK(Expr::Sgt, false, 2);
    if (memcmp(Tok.start, "Sge", 3) == 0)
      return SetOK(Expr::Sge, false, 2);
    break;

  case 4:
    if (memcmp(Tok.start, "Read", 4) == 0)
      return SetOK(Expr::Read, true, -1);
    if (memcmp(Tok.start, "AShr", 4) == 0)
      return SetOK(Expr::AShr, true, 2);
    if (memcmp(Tok.start, "LShr", 4) == 0)
      return SetOK(Expr::LShr, true, 2);

    if (memcmp(Tok.start, "UDiv", 4) == 0)
      return SetOK(Expr::UDiv, true, 2);
    if (memcmp(Tok.start, "SDiv", 4) == 0)
      return SetOK(Expr::SDiv, true, 2);
    if (memcmp(Tok.start, "URem", 4) == 0)
      return SetOK(Expr::URem, true, 2);
    if (memcmp(Tok.start, "SRem", 4) == 0)
      return SetOK(Expr::SRem, true, 2);
    
    if (memcmp(Tok.start, "SExt", 4) == 0)
      return SetOK(Expr::SExt, false, 1);
    if (memcmp(Tok.start, "ZExt", 4) == 0)
      return SetOK(Expr::ZExt, false, 1);
    break;
    
  case 6:
    if (memcmp(Tok.start, "Concat", 6) == 0)
      return SetOK(eMacroKind_Concat, false, -1); 
    if (memcmp(Tok.start, "Select", 6) == 0)
      return SetOK(Expr::Select, false, 3);
    break;
    
  case 7:
    if (memcmp(Tok.start, "Extract", 7) == 0)
      return SetOK(Expr::Extract, false, -1);
    if (memcmp(Tok.start, "ReadLSB", 7) == 0)
      return SetOK(eMacroKind_ReadLSB, true, -1);
    if (memcmp(Tok.start, "ReadMSB", 7) == 0)
      return SetOK(eMacroKind_ReadMSB, true, -1);
    break;
  }

  return false;
#undef SetOK
}

/// ParseParenExpr - Parse a parenthesized expression with the given
/// \arg ExpectedType. \arg ExpectedType can be invalid if the type
/// cannot be inferred from the context.
///
/// paren-expr = '(' type number ')'
/// paren-expr = '(' identifier [type] expr+ ')
/// paren-expr = '(' ('Read' | 'ReadMSB' | 'ReadLSB') type expr update-list ')'
ExprResult ParserImpl::ParseParenExpr(TypeResult FIXME_UNUSED) {
  if (Tok.kind != Token::LParen) {
    Error("unexpected token.");
    ConsumeAnyToken();
    return ExprResult();
  }

  ConsumeLParen();
  
  // Check for coercion case (w32 11).
  if (Tok.kind == Token::KWWidth) {
    TypeResult ExpectedType = ParseTypeSpecifier();

    if (Tok.kind != Token::Number) {
      Error("coercion can only apply to a number.");
      SkipUntilRParen();
      return ExprResult();
    }
    
    // Make sure this was a type specifier we support.
    ExprResult Res;
    if (ExpectedType.isValid()) 
      Res = ParseNumber(ExpectedType.get());
    else
      ConsumeToken();

    ExpectRParen("unexpected argument in coercion.");  
    return Res;
  }
  
  if (Tok.kind != Token::Identifier) {
    Error("unexpected token, expected expression.");
    SkipUntilRParen();
    return ExprResult();
  }

  Token Name = Tok;
  ConsumeToken();

  // FIXME: Use invalid type (i.e. width==0)?
  Token TypeTok = Tok;
  bool HasType = TypeTok.kind == Token::KWWidth;
  TypeResult Type = HasType ? ParseTypeSpecifier() : Expr::Bool;

  // FIXME: For now just skip to rparen on error. It might be nice
  // to try and actually parse the child nodes though for error
  // messages & better recovery?
  if (!Type.isValid()) {
    SkipUntilRParen();
    return ExprResult();
  }
  Expr::Width ResTy = Type.get();

  unsigned ExprKind;
  bool IsFixed;
  int NumArgs;
  if (!LookupExprInfo(Name, ExprKind, IsFixed, NumArgs)) {
    // FIXME: For now just skip to rparen on error. It might be nice
    // to try and actually parse the child nodes though for error
    // messages & better recovery?
    Error("unknown expression kind.", Name);
    SkipUntilRParen();
    return ExprResult();
  }

  // See if we have to parse this form specially.
  if (NumArgs == -1) {
    switch (ExprKind) {
    case eMacroKind_Concat:
      return ParseConcatParenExpr(Name, ResTy);

    case Expr::Extract:
      return ParseExtractParenExpr(Name, ResTy);

    case eMacroKind_ReadLSB:
    case eMacroKind_ReadMSB:
    case Expr::Read:
      return ParseAnyReadParenExpr(Name, ExprKind, ResTy);

    default:
      Error("internal error, unimplemented special form.", Name);
      SkipUntilRParen();
      return ExprResult(ConstantExpr::alloc(0, ResTy));
    }
  }

  switch (NumArgs) {
  case 1:
    return ParseUnaryParenExpr(Name, ExprKind, IsFixed, ResTy);
  case 2:
    return ParseBinaryParenExpr(Name, ExprKind, IsFixed, ResTy);
  case 3:
    if (ExprKind == Expr::Select)
      return ParseSelectParenExpr(Name, ResTy);
  default:
    assert(0 && "Invalid argument kind (number of args).");
    return ExprResult();
  }
}

ExprResult ParserImpl::ParseUnaryParenExpr(const Token &Name,
                                           unsigned Kind, bool IsFixed,
                                           Expr::Width ResTy) {
  if (Tok.kind == Token::RParen) {
    Error("unexpected end of arguments.", Name);
    ConsumeRParen();
    return ConstantExpr::alloc(0, ResTy);
  }

  ExprResult Arg = ParseExpr(IsFixed ? ResTy : TypeResult());
  if (!Arg.isValid())
    Arg = ConstantExpr::alloc(0, ResTy);

  ExpectRParen("unexpected argument in unary expression.");  
  ExprHandle E = Arg.get();
  switch (Kind) {
  case eMacroKind_Not:
    return EqExpr::alloc(ConstantExpr::alloc(0, E->getWidth()), E);
  case eMacroKind_Neg:
    return SubExpr::alloc(ConstantExpr::alloc(0, E->getWidth()), E);
  case Expr::SExt:
    // FIXME: Type check arguments.
    return SExtExpr::alloc(E, ResTy);
  case Expr::ZExt:
    // FIXME: Type check arguments.
    return ZExtExpr::alloc(E, ResTy);
  default:
    Error("internal error, unhandled kind.", Name);
    return ConstantExpr::alloc(0, ResTy);
  }
}

/// ParseMatchedBinaryArgs - Parse a pair of arguments who are
/// expected to be of the same type. Upon return, if both LHS and RHS
/// are valid then they are guaranteed to have the same type.
///
/// Name - The name token of the expression, for diagnostics.
/// ExpectType - The expected type of the arguments, if known.
void ParserImpl::ParseMatchedBinaryArgs(const Token &Name, 
                                        TypeResult ExpectType,
                                        ExprResult &LHS, ExprResult &RHS) {
  if (Tok.kind == Token::RParen) {
    Error("unexpected end of arguments.", Name);
    ConsumeRParen();
    return;
  }

  // Avoid NumberOrExprResult overhead and give more precise
  // diagnostics when we know the type.
  if (ExpectType.isValid()) {
    LHS = ParseExpr(ExpectType);
    if (Tok.kind == Token::RParen) {
      Error("unexpected end of arguments.", Name);
      ConsumeRParen();
      return;
    }
    RHS = ParseExpr(ExpectType);
  } else {
    NumberOrExprResult LHS_NOE = ParseNumberOrExpr();

    if (Tok.kind == Token::RParen) {
      Error("unexpected end of arguments.", Name);
      ConsumeRParen();
      return;
    }

    if (LHS_NOE.isNumber()) {
      NumberOrExprResult RHS_NOE = ParseNumberOrExpr();
      
      if (RHS_NOE.isNumber()) {
        Error("ambiguous arguments to expression.", Name);
      } else {
        RHS = RHS_NOE.getExpr();
        if (RHS.isValid())
          LHS = ParseNumberToken(RHS.get()->getWidth(), LHS_NOE.getNumber());
      }
    } else {
      LHS = LHS_NOE.getExpr();
      if (!LHS.isValid()) {
        // FIXME: Should suppress ambiguity warnings here.
        RHS = ParseExpr(TypeResult());
      } else {
        RHS = ParseExpr(LHS.get()->getWidth());
      }
    }
  }

  ExpectRParen("unexpected argument to expression.");
}

ExprResult ParserImpl::ParseBinaryParenExpr(const Token &Name,
                                           unsigned Kind, bool IsFixed,
                                           Expr::Width ResTy) {
  ExprResult LHS, RHS;
  ParseMatchedBinaryArgs(Name, IsFixed ? TypeResult(ResTy) : TypeResult(), 
                         LHS, RHS);
  if (!LHS.isValid() || !RHS.isValid())
    return ConstantExpr::alloc(0, ResTy);

  ref<Expr> LHS_E = LHS.get(), RHS_E = RHS.get();
  if (LHS_E->getWidth() != RHS_E->getWidth()) {
    Error("type widths do not match in binary expression", Name);
    return ConstantExpr::alloc(0, ResTy);
  }

  switch (Kind) {    
  case Expr::Add: return AddExpr::alloc(LHS_E, RHS_E);
  case Expr::Sub: return SubExpr::alloc(LHS_E, RHS_E);
  case Expr::Mul: return MulExpr::alloc(LHS_E, RHS_E);
  case Expr::UDiv: return UDivExpr::alloc(LHS_E, RHS_E);
  case Expr::SDiv: return SDivExpr::alloc(LHS_E, RHS_E);
  case Expr::URem: return URemExpr::alloc(LHS_E, RHS_E);
  case Expr::SRem: return SRemExpr::alloc(LHS_E, RHS_E);

  case Expr::AShr: return AShrExpr::alloc(LHS_E, RHS_E);
  case Expr::LShr: return LShrExpr::alloc(LHS_E, RHS_E);
  case Expr::Shl: return AndExpr::alloc(LHS_E, RHS_E);

  case Expr::And: return AndExpr::alloc(LHS_E, RHS_E);
  case Expr::Or:  return OrExpr::alloc(LHS_E, RHS_E);
  case Expr::Xor: return XorExpr::alloc(LHS_E, RHS_E);

  case Expr::Eq:  return EqExpr::alloc(LHS_E, RHS_E);
  case Expr::Ne:  return NeExpr::alloc(LHS_E, RHS_E);
  case Expr::Ult: return UltExpr::alloc(LHS_E, RHS_E);
  case Expr::Ule: return UleExpr::alloc(LHS_E, RHS_E);
  case Expr::Ugt: return UgtExpr::alloc(LHS_E, RHS_E);
  case Expr::Uge: return UgeExpr::alloc(LHS_E, RHS_E);
  case Expr::Slt: return SltExpr::alloc(LHS_E, RHS_E);
  case Expr::Sle: return SleExpr::alloc(LHS_E, RHS_E);
  case Expr::Sgt: return SgtExpr::alloc(LHS_E, RHS_E);
  case Expr::Sge: return SgeExpr::alloc(LHS_E, RHS_E);
  default:
    Error("FIXME: unhandled kind.", Name);
    return ConstantExpr::alloc(0, ResTy);
  }  
}

ExprResult ParserImpl::ParseSelectParenExpr(const Token &Name, 
                                            Expr::Width ResTy) {
  // FIXME: Why does this need to be here?
  if (Tok.kind == Token::RParen) {
    Error("unexpected end of arguments.", Name);
    ConsumeRParen();
    return ConstantExpr::alloc(0, ResTy);
  }

  ExprResult Cond = ParseExpr(Expr::Bool);
  ExprResult LHS, RHS;
  ParseMatchedBinaryArgs(Name, ResTy, LHS, RHS);
  if (!Cond.isValid() || !LHS.isValid() || !RHS.isValid())
    return ConstantExpr::alloc(0, ResTy);
  return SelectExpr::alloc(Cond.get(), LHS.get(), RHS.get());
}


// need to decide if we want to allow n-ary Concat expressions in the
// language
ExprResult ParserImpl::ParseConcatParenExpr(const Token &Name,
                                            Expr::Width ResTy) {
  std::vector<ExprHandle> Kids;
  
  unsigned Width = 0;
  while (Tok.kind != Token::RParen) {
    ExprResult E = ParseExpr(TypeResult());

    // Skip to end of expr on error.
    if (!E.isValid()) {
      SkipUntilRParen();
      return ConstantExpr::alloc(0, ResTy);
    }
    
    Kids.push_back(E.get());
    Width += E.get()->getWidth();
  }
  
  ConsumeRParen();

  if (Width != ResTy) {
    Error("concat does not match expected result size.");
    return ConstantExpr::alloc(0, ResTy);
  }

  return ConcatExpr::createN(Kids.size(), &Kids[0]);
}

ExprResult ParserImpl::ParseExtractParenExpr(const Token &Name,
                                             Expr::Width ResTy) {
  // FIXME: Pull out parse constant integer expression.
  ExprResult OffsetExpr = ParseNumber(Expr::Int32);
  ExprResult Child = ParseExpr(TypeResult());

  ExpectRParen("unexpected argument to expression.");

  if (!OffsetExpr.isValid() || !Child.isValid())
    return ConstantExpr::alloc(0, ResTy);

  unsigned Offset = 
    (unsigned) cast<ConstantExpr>(OffsetExpr.get())->getConstantValue();

  if (Offset + ResTy > Child.get()->getWidth()) {
    Error("extract out-of-range of child expression.", Name);
    return ConstantExpr::alloc(0, ResTy);
  }

  return ExtractExpr::alloc(Child.get(), Offset, ResTy);
}

ExprResult ParserImpl::ParseAnyReadParenExpr(const Token &Name,
                                             unsigned Kind,
                                             Expr::Width ResTy) {
  NumberOrExprResult Index = ParseNumberOrExpr();
  VersionResult Array = ParseVersionSpecifier();
  ExpectRParen("unexpected argument in read expression.");
  
  if (!Array.isValid())
    return ConstantExpr::alloc(0, ResTy);

  // FIXME: Need generic way to get array width. Needs to work with
  // anonymous arrays.
  Expr::Width ArrayDomainType = Expr::Int32;
  Expr::Width ArrayRangeType = Expr::Int8;

  // Coerce number to correct type.
  ExprResult IndexExpr;
  if (Index.isNumber())
    IndexExpr = ParseNumberToken(ArrayDomainType, Index.getNumber());
  else
    IndexExpr = Index.getExpr();
  
  if (!IndexExpr.isValid())
    return ConstantExpr::alloc(0, ResTy);
  else if (IndexExpr.get()->getWidth() != ArrayDomainType) {
    Error("index width does not match array domain.");
    return ConstantExpr::alloc(0, ResTy);
  }

  // FIXME: Check range width.

  switch (Kind) {
  default:
    assert(0 && "Invalid kind.");
    return ConstantExpr::alloc(0, ResTy);
  case eMacroKind_ReadLSB:
  case eMacroKind_ReadMSB: {
    unsigned NumReads = ResTy / ArrayRangeType;
    if (ResTy != NumReads*ArrayRangeType) {
      Error("invalid ordered read (not multiple of range type).", Name);
      return ConstantExpr::alloc(0, ResTy);
    }
    std::vector<ExprHandle> Kids;
    Kids.reserve(NumReads);
    ExprHandle Index = IndexExpr.get();
    for (unsigned i=0; i<NumReads; ++i) {
      // FIXME: using folding here
      ExprHandle OffsetIndex = AddExpr::create(IndexExpr.get(),
                                               ConstantExpr::alloc(i, ArrayDomainType));
      Kids.push_back(ReadExpr::alloc(Array.get(), OffsetIndex));
    }
    if (Kind == eMacroKind_ReadLSB)
      std::reverse(Kids.begin(), Kids.end());
    return ConcatExpr::createN(NumReads, &Kids[0]);
  }
  case Expr::Read:
    return ReadExpr::alloc(Array.get(), IndexExpr.get());
  }
}

/// version-specifier = <identifier>
/// version-specifier = [<identifier>:] [ version ]
VersionResult ParserImpl::ParseVersionSpecifier() {
  const Identifier *Label = 0;
  if (Tok.kind == Token::Identifier) {
    Token LTok = Tok;
    Label = GetOrCreateIdentifier(Tok);
    ConsumeToken();

    // FIXME: hack: add array declarations and ditch this.
    if (memcmp(Label->Name.c_str(), "arr", 3) == 0) {
      // Declare or create array.
      const ArrayDecl *&A = ArraySymTab[Label];
      if (!A) {
        //        Array = new ArrayDecl(Label, 0, 32, 8);
        unsigned id = atoi(&Label->Name.c_str()[3]);
        Array *root = new Array(0, id, 0);
        // Create update list mapping of name -> array.
        VersionSymTab.insert(std::make_pair(Label,
                                            UpdateList(root, true, NULL)));
      }
    }

    if (Tok.kind != Token::Colon) {
      VersionSymTabTy::iterator it = VersionSymTab.find(Label);
      
      if (it == VersionSymTab.end()) {
        Error("invalid update list label reference.", LTok);
        return VersionResult(false,
                             UpdateList(0, true, NULL));
      }

      return it->second;
    }

    ConsumeToken();
    if (VersionSymTab.count(Label)) {
      Error("duplicate update list label definition.", LTok);
      Label = 0;
    }
  }

  Token Start = Tok;
  VersionResult Res = ParseVersion();
  // Define update list to avoid use-of-undef errors.
  if (!Res.isValid())
    Res = VersionResult(false,
                        UpdateList(0, true, NULL));
  
  if (Label)
    VersionSymTab.insert(std::make_pair(Label, Res.get()));
  return Res;
}

/// version - '[' update-list? ']' ['@' version-specifier]
/// update-list - empty
/// update-list - lhs '=' rhs [',' update-list]
VersionResult ParserImpl::ParseVersion() {
  if (Tok.kind != Token::LSquare)
    return VersionResult(false, UpdateList(0, false, NULL));
  
  std::vector< std::pair<NumberOrExprResult, NumberOrExprResult> > Writes;
  ConsumeLSquare();
  for (;;) {
    // FIXME: Type check exprs.

    // FIXME: We need to do this (the above) anyway just to handle
    // implicit constants correctly.
    NumberOrExprResult LHS = ParseNumberOrExpr();
    
    if (Tok.kind != Token::Equals) {
      Error("expected '='.", Tok);
      break;
    }
    
    ConsumeToken();
    NumberOrExprResult RHS = ParseNumberOrExpr();

    Writes.push_back(std::make_pair(LHS, RHS));
    
    if (Tok.kind == Token::Comma)
      ConsumeToken();
    else
      break;
  }
  ExpectRSquare("expected close of update list");

  VersionHandle Base(0, false, NULL);

  // Anonymous array case.
  if (Tok.kind != Token::At) { 
    Array *root = new Array(0, 0, 0);
    Base = UpdateList(root, false, NULL);
  } else {
    ConsumeToken();

    VersionResult BaseRes = ParseVersionSpecifier();
    if (!BaseRes.isValid())
      return BaseRes;

    Base = BaseRes.get();
  }

  Expr::Width ArrayDomainType = Expr::Int32;
  Expr::Width ArrayRangeType = Expr::Int8;

  for (std::vector< std::pair<NumberOrExprResult, NumberOrExprResult> >::reverse_iterator
         it = Writes.rbegin(), ie = Writes.rend(); it != ie; ++it) {
    ExprResult LHS, RHS;
    // FIXME: This can be factored into common helper for coercing a
    // NumberOrExpr into an Expr.
    if (it->first.isNumber()) {
      LHS = ParseNumberToken(ArrayDomainType, it->first.getNumber());
    } else {
      LHS = it->first.getExpr(); 
      if (LHS.isValid() && LHS.get()->getWidth() != ArrayDomainType) {
        // FIXME: bad token location. We should maybe try and know the
        // array up-front?
        Error("invalid value in write index (doesn't match domain).", Tok);
        LHS = ExprResult();
      }
    }

    if (it->second.isNumber()) {
      RHS = ParseNumberToken(ArrayRangeType, it->second.getNumber());
    } else {
      RHS = it->second.getExpr();
      if (RHS.isValid() && RHS.get()->getWidth() != ArrayRangeType) {
        // FIXME: bad token location. We should maybe try and know the
        // array up-front?
        Error("invalid value in write assignment (doesn't match range).", Tok);
        RHS = ExprResult();
      }
    }
    
    if (LHS.isValid() && RHS.isValid())
      Base.extend(LHS.get(), RHS.get());
  }

  return Base;
}

/// ParseNumber - Parse a number of the given type.
ExprResult ParserImpl::ParseNumber(Expr::Width Type) {
  ExprResult Res = ParseNumberToken(Type, Tok);
  ConsumeExpectedToken(Token::Number);
  return Res;
}

/// ParseNumberToken - Parse a number of the given type from the given
/// token.
ExprResult ParserImpl::ParseNumberToken(Expr::Width Type, const Token &Tok) {
  const char *S = Tok.start;
  unsigned N = Tok.length;
  unsigned Radix = 10, RadixBits = 4;
  bool HasMinus = false;

  // Detect +/- (a number token cannot have both).
  if (S[0] == '+') {
    ++S;
    --N;
  } else if (S[0] == '-') {
    HasMinus = true;
    ++S;
    --N;
  }

  // Detect 0[box].
  if ((Tok.length >= 2 && S[0] == '0') &&
      (S[1] == 'b' || S[1] == 'o' || S[1] == 'x')) {
    if (S[1] == 'b') {
      Radix = 2; 
      RadixBits = 1;
    } else if (S[1] == 'o') {
      Radix = 8;
      RadixBits = 3;
    } else {
      Radix = 16;
      RadixBits = 4;
    }
    S += 2;
    N -= 2;

    // Diagnose 0[box] with no trailing digits.
    if (!N) {
      Error("invalid numeric token (no digits).", Tok);
      return ConstantExpr::alloc(0, Type);
    }
  }

  // This is a simple but slow way to handle overflow.
  APInt Val(std::max(64U, RadixBits * N), 0);
  APInt RadixVal(Val.getBitWidth(), Radix);
  APInt DigitVal(Val.getBitWidth(), 0);
  for (unsigned i=0; i<N; ++i) {
    unsigned Digit, Char = S[i];
    
    if (Char == '_')
      continue;
    
    if ('0' <= Char && Char <= '9')
      Digit = Char - '0';
    else if ('a' <= Char && Char <= 'z')
      Digit = Char - 'a' + 10;
    else if ('A' <= Char && Char <= 'Z')
      Digit = Char - 'A' + 10;
    else {
      Error("invalid character in numeric token.", Tok);
      return ConstantExpr::alloc(0, Type);
    }

    if (Digit >= Radix) {
      Error("invalid character in numeric token (out of range).", Tok);
      return ConstantExpr::alloc(0, Type);
    }

    DigitVal = Digit;
    Val = Val * RadixVal + DigitVal;
  }

  // FIXME: Actually do the check for overflow.
  if (HasMinus)
    Val = -Val;

  if (Type < Val.getBitWidth())
    Val = Val.trunc(Type);

  return ExprResult(ConstantExpr::alloc(Val.getZExtValue(), Type));
}

/// ParseTypeSpecifier - Parse a type specifier.
///
/// type = w[0-9]+
TypeResult ParserImpl::ParseTypeSpecifier() {
  assert(Tok.kind == Token::KWWidth && "Unexpected token.");

  // FIXME: Need APInt technically.
  Token TypeTok = Tok;
  int width = atoi(std::string(Tok.start+1,Tok.length-1).c_str());
  ConsumeToken();

  // FIXME: We should impose some sort of maximum just for sanity?
  return TypeResult(width);
}

void ParserImpl::Error(const char *Message, const Token &At) {
  ++NumErrors;
  if (MaxErrors && NumErrors >= MaxErrors)
    return;

  llvm::cerr << Filename 
             << ":" << At.line << ":" << At.column 
             << ": error: " << Message << "\n";

  // Skip carat diagnostics on EOF token.
  if (At.kind == Token::EndOfFile)
    return;
  
  // Simple caret style diagnostics.
  const char *LineBegin = At.start, *LineEnd = At.start,
    *BufferBegin = TheMemoryBuffer->getBufferStart(),
    *BufferEnd = TheMemoryBuffer->getBufferEnd();

  // Run line pointers forward and back.
  while (LineBegin > BufferBegin && 
         LineBegin[-1] != '\r' && LineBegin[-1] != '\n')
    --LineBegin;
  while (LineEnd < BufferEnd && 
         LineEnd[0] != '\r' && LineEnd[0] != '\n')
    ++LineEnd;

  // Show the line.
  llvm::cerr << std::string(LineBegin, LineEnd) << "\n";

  // Show the caret or squiggly, making sure to print back spaces the
  // same.
  for (const char *S=LineBegin; S != At.start; ++S)
    llvm::cerr << (isspace(*S) ? *S : ' ');
  if (At.length > 1) {
    for (unsigned i=0; i<At.length; ++i)
      llvm::cerr << '~';
  } else
    llvm::cerr << '^';
  llvm::cerr << '\n';
}

// AST API
// FIXME: Move out of parser.

Decl::Decl(DeclKind _Kind) : Kind(_Kind) {}

void QueryCommand::dump() {
  // FIXME: This is masking the difference between an actual query and
  // a query decl.
  ExprPPrinter::printQuery(std::cout, 
                           ConstraintManager(Constraints), 
                           Query);
}

// Public parser API

Parser::Parser() {
}

Parser::~Parser() {
}

Parser *Parser::Create(const std::string Filename,
                       const MemoryBuffer *MB) {
  ParserImpl *P = new ParserImpl(Filename, MB);
  P->Initialize();
  return P;
}
