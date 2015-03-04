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

#include "klee/Config/Version.h"
#include "klee/Constraints.h"
#include "klee/ExprBuilder.h"
#include "klee/Solver.h"
#include "klee/util/ExprPPrinter.h"

#include "llvm/ADT/APInt.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
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
    ParseResult() : IsValid(false), Value() {}
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
  typedef ParseResult<uint64_t> IntegerResult;

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
    ExprBuilder *Builder;

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
      assert(Tok.kind != Token::LParen && Tok.kind != Token::RParen &&
             Tok.kind != Token::LSquare && Tok.kind != Token::RSquare);
      GetNextNonCommentToken();
    }

    /// ConsumeExpectedToken - Check that the current token is of the
    /// expected kind and consume it.
    void ConsumeExpectedToken(Token::Kind k) {
      assert(Tok.kind != Token::LParen && Tok.kind != Token::RParen &&
             Tok.kind != Token::LSquare && Tok.kind != Token::RSquare);
      _ConsumeExpectedToken(k);
    }
    void _ConsumeExpectedToken(Token::Kind k) {
      assert(Tok.kind == k && "Unexpected token!");
      GetNextNonCommentToken();
    }

    void ConsumeLParen() {
      ++ParenLevel;
      _ConsumeExpectedToken(Token::LParen);
    }

    void ConsumeRParen() {
      if (ParenLevel) // Cannot go below zero.
        --ParenLevel;
      _ConsumeExpectedToken(Token::RParen);
    }

    void ConsumeLSquare() {
      ++SquareLevel;
      _ConsumeExpectedToken(Token::LSquare);
    }

    void ConsumeRSquare() {
      if (SquareLevel) // Cannot go below zero.
        --SquareLevel;
      _ConsumeExpectedToken(Token::RSquare);
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
    IntegerResult ParseIntegerConstant(Expr::Width Type);

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
               const MemoryBuffer *MB,
               ExprBuilder *_Builder) : Filename(_Filename),
                                        TheMemoryBuffer(MB),
                                        Builder(_Builder),
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
    switch (Tok.kind) {
    case Token::KWArray: {
      DeclResult Res = ParseArrayDecl();
      if (Res.isValid())
        return Res.get();
      break;
    }

    case Token::LParen: {
      DeclResult Res = ParseCommandDecl();
      if (Res.isValid())
        return Res.get();
      break;
    }

    default:
      Error("expected 'array' or '(' token.");
      ConsumeAnyToken();
    }
  }

  return 0;
}

/// ParseArrayDecl - Parse an array declaration. The lexer should be positioned
/// at the opening 'array'.
///
/// array-declaration = "array" name "[" [ size ] "]" ":" domain "->" range 
///                       "=" array-initializer
/// array-initializer = "symbolic" | "{" { numeric-literal } "}"
DeclResult ParserImpl::ParseArrayDecl() {
  // FIXME: Recovery here is horrible, we need to scan to next decl start or
  // something.
  ConsumeExpectedToken(Token::KWArray);
  
  if (Tok.kind != Token::Identifier) {
    Error("expected identifier token.");
    return DeclResult();
  }

  Token Name = Tok;
  IntegerResult Size;
  TypeResult DomainType;
  TypeResult RangeType;
  std::vector< ref<ConstantExpr> > Values;

  ConsumeToken();
  
  if (Tok.kind != Token::LSquare) {
    Error("expected '['.");
    goto exit;
  }
  ConsumeLSquare();

  if (Tok.kind != Token::RSquare) {
    Size = ParseIntegerConstant(64);
  }
  if (Tok.kind != Token::RSquare) {
    Error("expected ']'.");
    goto exit;
  }
  ConsumeRSquare();
  
  if (Tok.kind != Token::Colon) {
    Error("expected ':'.");
    goto exit;
  }
  ConsumeExpectedToken(Token::Colon);

  DomainType = ParseTypeSpecifier();
  if (Tok.kind != Token::Arrow) {
    Error("expected '->'.");
    goto exit;
  }
  ConsumeExpectedToken(Token::Arrow);

  RangeType = ParseTypeSpecifier();
  if (Tok.kind != Token::Equals) {
    Error("expected '='.");
    goto exit;
  }
  ConsumeExpectedToken(Token::Equals);

  if (Tok.kind == Token::KWSymbolic) {
    ConsumeExpectedToken(Token::KWSymbolic);    
  } else if (Tok.kind == Token::LSquare) {
    ConsumeLSquare();
    while (Tok.kind != Token::RSquare) {
      if (Tok.kind == Token::EndOfFile) {
        Error("unexpected end of file.");
        goto exit;
      }

      ExprResult Res = ParseNumber(RangeType.get());
      if (Res.isValid())
        Values.push_back(cast<ConstantExpr>(Res.get()));
    }
    ConsumeRSquare();
  } else {
    Error("expected 'symbolic' or '['.");
    goto exit;
  }

  // Type check size.
  if (!Size.isValid()) {
    if (Values.empty()) {
      Error("unsized arrays are not yet supported.");
      Size = 1;
    } else {
      Size = Values.size();
    }
  }

  if (!Values.empty()) {
    if (Size.get() != Values.size()) {
      // FIXME: Lame message.
      Error("constant arrays must be completely specified.");
      Values.clear();
    }

    for (unsigned i = 0; i != Size.get(); ++i) {
      // FIXME: Must be constant expression.
    }
  }

  // FIXME: Validate that size makes sense for domain type.

  if (DomainType.get() != Expr::Int32) {
    Error("array domain must currently be w32.");
    DomainType = Expr::Int32;
    Values.clear();
  }

  if (RangeType.get() != Expr::Int8) {
    Error("array domain must currently be w8.");
    RangeType = Expr::Int8;
    Values.clear();
  }

  // FIXME: Validate that this array is undeclared.

 exit:
  if (!Size.isValid())
    Size = 1;
  if (!DomainType.isValid())
    DomainType = 32;
  if (!RangeType.isValid())
    RangeType = 8;

  // FIXME: Array should take domain and range.
  const Identifier *Label = GetOrCreateIdentifier(Name);
  const Array *Root;
  if (!Values.empty())
    Root = Array::CreateArray(Label->Name, Size.get(),
			      &Values[0], &Values[0] + Values.size());
  else
    Root = Array::CreateArray(Label->Name, Size.get());
  ArrayDecl *AD = new ArrayDecl(Label, Size.get(), 
                                DomainType.get(), RangeType.get(), Root);

  ArraySymTab.insert(std::make_pair(Label, AD));

  // Create the initial version reference.
  VersionSymTab.insert(std::make_pair(Label,
                                      UpdateList(Root, NULL)));

  return AD;
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
/// 'query' expressions-list expression [expressions-list [array-list]]
DeclResult ParserImpl::ParseQueryCommand() {
  std::vector<ExprHandle> Constraints;
  std::vector<ExprHandle> Values;
  std::vector<const Array*> Objects;
  ExprResult Res;

  // FIXME: We need a command for this. Or something.
  ExprSymTab.clear();
  VersionSymTab.clear();

  // Reinsert initial array versions.
  // FIXME: Remove this!
  for (std::map<const Identifier*, const ArrayDecl*>::iterator
         it = ArraySymTab.begin(), ie = ArraySymTab.end(); it != ie; ++it) {
    VersionSymTab.insert(std::make_pair(it->second->Name,
                                        UpdateList(it->second->Root, NULL)));
  }


  ConsumeExpectedToken(Token::KWQuery);
  if (Tok.kind != Token::LSquare) {
    Error("malformed query, expected constraint list.");
    SkipUntilRParen();
    return DeclResult();
  }

  ConsumeLSquare();
  // FIXME: Should avoid reading past unbalanced parens here.
  while (Tok.kind != Token::RSquare) {
    if (Tok.kind == Token::EndOfFile) {
      Error("unexpected end of file.");
      Res = ExprResult(Builder->Constant(0, Expr::Bool));
      goto exit;
    }

    ExprResult Constraint = ParseExpr(TypeResult(Expr::Bool));
    if (Constraint.isValid())
      Constraints.push_back(Constraint.get());
  }
  ConsumeRSquare();

  Res = ParseExpr(TypeResult(Expr::Bool));
  if (!Res.isValid()) // Error emitted by ParseExpr.
    Res = ExprResult(Builder->Constant(0, Expr::Bool));

  // Return if there are no optional lists of things to evaluate.
  if (Tok.kind == Token::RParen)
    goto exit;

  if (Tok.kind != Token::LSquare) {
    Error("malformed query, expected expression list.");
    SkipUntilRParen();
    return DeclResult();
  }
  
  ConsumeLSquare();
  // FIXME: Should avoid reading past unbalanced parens here.
  while (Tok.kind != Token::RSquare) {
    if (Tok.kind == Token::EndOfFile) {
      Error("unexpected end of file.");
      goto exit;
    }

    ExprResult Res = ParseExpr(TypeResult());
    if (Res.isValid())
      Values.push_back(Res.get());
  }
  ConsumeRSquare();

  // Return if there are no optional lists of things to evaluate.
  if (Tok.kind == Token::RParen)
    goto exit;

  if (Tok.kind != Token::LSquare) {
    Error("malformed query, expected array list.");
    SkipUntilRParen();
    return DeclResult();
  }

  ConsumeLSquare();
  // FIXME: Should avoid reading past unbalanced parens here.
  while (Tok.kind != Token::RSquare) {
    if (Tok.kind == Token::EndOfFile) {
      Error("unexpected end of file.");
      goto exit;
    }

    // FIXME: Factor out.
    if (Tok.kind != Token::Identifier) {
      Error("unexpected token.");
      ConsumeToken();
      continue;
    }

    Token LTok = Tok;
    const Identifier *Label = GetOrCreateIdentifier(Tok);
    ConsumeToken();

    // Lookup array.
    std::map<const Identifier*, const ArrayDecl*>::iterator
      it = ArraySymTab.find(Label);

    if (it == ArraySymTab.end()) {
      Error("unknown array", LTok);
    } else {
      Objects.push_back(it->second->Root);
    }
  }
  ConsumeRSquare();

 exit:
  if (Tok.kind != Token::EndOfFile)
    ExpectRParen("unexpected argument to 'query'.");  
  return new QueryCommand(Constraints, Res.get(), Values, Objects);
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
    return ExprResult(Builder->Constant(Value, Expr::Bool));
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
      ref<Expr> Value = Builder->Constant(0, ExpectedType.get());
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
  eMacroKind_ReadLSB = Expr::LastKind + 1, // Multibyte read
  eMacroKind_ReadMSB,                      // Multibyte write
  eMacroKind_Neg,                          // 0 - x // CrC: will disappear soon
  eMacroKind_Concat,                       // Magic concatenation syntax
  eMacroKind_LastMacroKind = eMacroKind_Concat
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

    if (memcmp(Tok.start, "Not", 3) == 0)
      return SetOK(Expr::Not, true, 1);
    if (memcmp(Tok.start, "And", 3) == 0)
      return SetOK(Expr::And, true, 2);
    if (memcmp(Tok.start, "Shl", 3) == 0)
      return SetOK(Expr::Shl, true, 2);
    if (memcmp(Tok.start, "Xor", 3) == 0)
      return SetOK(Expr::Xor, true, 2);

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
      return ExprResult(Builder->Constant(0, ResTy));
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
    return Builder->Constant(0, ResTy);
  }

  ExprResult Arg = ParseExpr(IsFixed ? ResTy : TypeResult());
  if (!Arg.isValid())
    Arg = Builder->Constant(0, ResTy);

  ExpectRParen("unexpected argument in unary expression.");  
  ExprHandle E = Arg.get();
  switch (Kind) {
  case eMacroKind_Neg:
    return Builder->Sub(Builder->Constant(0, E->getWidth()), E);
  case Expr::Not:
    // FIXME: Type check arguments.
    return Builder->Not(E);
  case Expr::SExt:
    // FIXME: Type check arguments.
    return Builder->SExt(E, ResTy);
  case Expr::ZExt:
    // FIXME: Type check arguments.
    return Builder->ZExt(E, ResTy);
  default:
    Error("internal error, unhandled kind.", Name);
    return Builder->Constant(0, ResTy);
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
    return Builder->Constant(0, ResTy);

  ref<Expr> LHS_E = LHS.get(), RHS_E = RHS.get();
  if (LHS_E->getWidth() != RHS_E->getWidth()) {
    Error("type widths do not match in binary expression", Name);
    return Builder->Constant(0, ResTy);
  }

  switch (Kind) {    
  case Expr::Add: return Builder->Add(LHS_E, RHS_E);
  case Expr::Sub: return Builder->Sub(LHS_E, RHS_E);
  case Expr::Mul: return Builder->Mul(LHS_E, RHS_E);
  case Expr::UDiv: return Builder->UDiv(LHS_E, RHS_E);
  case Expr::SDiv: return Builder->SDiv(LHS_E, RHS_E);
  case Expr::URem: return Builder->URem(LHS_E, RHS_E);
  case Expr::SRem: return Builder->SRem(LHS_E, RHS_E);

  case Expr::AShr: return Builder->AShr(LHS_E, RHS_E);
  case Expr::LShr: return Builder->LShr(LHS_E, RHS_E);
  case Expr::Shl: return Builder->Shl(LHS_E, RHS_E);

  case Expr::And: return Builder->And(LHS_E, RHS_E);
  case Expr::Or:  return Builder->Or(LHS_E, RHS_E);
  case Expr::Xor: return Builder->Xor(LHS_E, RHS_E);

  case Expr::Eq:  return Builder->Eq(LHS_E, RHS_E);
  case Expr::Ne:  return Builder->Ne(LHS_E, RHS_E);
  case Expr::Ult: return Builder->Ult(LHS_E, RHS_E);
  case Expr::Ule: return Builder->Ule(LHS_E, RHS_E);
  case Expr::Ugt: return Builder->Ugt(LHS_E, RHS_E);
  case Expr::Uge: return Builder->Uge(LHS_E, RHS_E);
  case Expr::Slt: return Builder->Slt(LHS_E, RHS_E);
  case Expr::Sle: return Builder->Sle(LHS_E, RHS_E);
  case Expr::Sgt: return Builder->Sgt(LHS_E, RHS_E);
  case Expr::Sge: return Builder->Sge(LHS_E, RHS_E);
  default:
    Error("FIXME: unhandled kind.", Name);
    return Builder->Constant(0, ResTy);
  }  
}

ExprResult ParserImpl::ParseSelectParenExpr(const Token &Name, 
                                            Expr::Width ResTy) {
  // FIXME: Why does this need to be here?
  if (Tok.kind == Token::RParen) {
    Error("unexpected end of arguments.", Name);
    ConsumeRParen();
    return Builder->Constant(0, ResTy);
  }

  ExprResult Cond = ParseExpr(Expr::Bool);
  ExprResult LHS, RHS;
  ParseMatchedBinaryArgs(Name, ResTy, LHS, RHS);
  if (!Cond.isValid() || !LHS.isValid() || !RHS.isValid())
    return Builder->Constant(0, ResTy);
  return Builder->Select(Cond.get(), LHS.get(), RHS.get());
}


// FIXME: Rewrite to only accept binary form. Make type optional.
ExprResult ParserImpl::ParseConcatParenExpr(const Token &Name,
                                            Expr::Width ResTy) {
  std::vector<ExprHandle> Kids;
  
  unsigned Width = 0;
  while (Tok.kind != Token::RParen) {
    ExprResult E = ParseExpr(TypeResult());

    // Skip to end of expr on error.
    if (!E.isValid()) {
      SkipUntilRParen();
      return Builder->Constant(0, ResTy);
    }
    
    Kids.push_back(E.get());
    Width += E.get()->getWidth();
  }
  
  ConsumeRParen();

  if (Width != ResTy) {
    Error("concat does not match expected result size.");
    return Builder->Constant(0, ResTy);
  }

  // FIXME: Use builder!
  return ConcatExpr::createN(Kids.size(), &Kids[0]);
}

IntegerResult ParserImpl::ParseIntegerConstant(Expr::Width Type) {
  ExprResult Res = ParseNumber(Type);

  if (!Res.isValid())
    return IntegerResult();

  return cast<ConstantExpr>(Res.get())->getZExtValue(Type);
}

ExprResult ParserImpl::ParseExtractParenExpr(const Token &Name,
                                             Expr::Width ResTy) {
  IntegerResult OffsetExpr = ParseIntegerConstant(Expr::Int32);
  ExprResult Child = ParseExpr(TypeResult());

  ExpectRParen("unexpected argument to expression.");

  if (!OffsetExpr.isValid() || !Child.isValid())
    return Builder->Constant(0, ResTy);

  unsigned Offset = (unsigned) OffsetExpr.get();
  if (Offset + ResTy > Child.get()->getWidth()) {
    Error("extract out-of-range of child expression.", Name);
    return Builder->Constant(0, ResTy);
  }

  return Builder->Extract(Child.get(), Offset, ResTy);
}

ExprResult ParserImpl::ParseAnyReadParenExpr(const Token &Name,
                                             unsigned Kind,
                                             Expr::Width ResTy) {
  NumberOrExprResult Index = ParseNumberOrExpr();
  VersionResult Array = ParseVersionSpecifier();
  ExpectRParen("unexpected argument in read expression.");
  
  if (!Array.isValid())
    return Builder->Constant(0, ResTy);

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
    return Builder->Constant(0, ResTy);
  else if (IndexExpr.get()->getWidth() != ArrayDomainType) {
    Error("index width does not match array domain.");
    return Builder->Constant(0, ResTy);
  }

  // FIXME: Check range width.

  switch (Kind) {
  default:
    assert(0 && "Invalid kind.");
    return Builder->Constant(0, ResTy);
  case eMacroKind_ReadLSB:
  case eMacroKind_ReadMSB: {
    unsigned NumReads = ResTy / ArrayRangeType;
    if (ResTy != NumReads*ArrayRangeType) {
      Error("invalid ordered read (not multiple of range type).", Name);
      return Builder->Constant(0, ResTy);
    }
    std::vector<ExprHandle> Kids(NumReads);
    ExprHandle Index = IndexExpr.get();
    for (unsigned i=0; i != NumReads; ++i) {
      // FIXME: We rely on folding here to not complicate things to where the
      // Read macro pattern fails to match.
      ExprHandle OffsetIndex = Index;
      if (i)
        OffsetIndex = AddExpr::create(OffsetIndex,
                                      Builder->Constant(i, ArrayDomainType));
      Kids[i] = Builder->Read(Array.get(), OffsetIndex);
    }
    if (Kind == eMacroKind_ReadLSB)
      std::reverse(Kids.begin(), Kids.end());
    // FIXME: Use builder!
    return ConcatExpr::createN(NumReads, &Kids[0]);
  }
  case Expr::Read:
    return Builder->Read(Array.get(), IndexExpr.get());
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

    if (Tok.kind != Token::Colon) {
      VersionSymTabTy::iterator it = VersionSymTab.find(Label);
      
      if (it == VersionSymTab.end()) {
        Error("invalid version reference.", LTok);
        return VersionResult(false, UpdateList(0, NULL));
      }

      return it->second;
    }

    ConsumeToken();
    if (VersionSymTab.count(Label)) {
      Error("duplicate update list label definition.", LTok);
      Label = 0;
    }
  }

  VersionResult Res = ParseVersion();
  // Define update list to avoid use-of-undef errors.
  if (!Res.isValid()) {
    Res = VersionResult(true, UpdateList(Array::CreateArray("", 0), NULL));
  }
  
  if (Label)
    VersionSymTab.insert(std::make_pair(Label, Res.get()));
  return Res;
}

namespace {
  /// WriteInfo - Helper class used for storing information about a parsed
  /// write, prior to type checking.
  struct WriteInfo {
    NumberOrExprResult LHS;
    NumberOrExprResult RHS;
    Token LHSTok;
    Token RHSTok;
    
    WriteInfo(NumberOrExprResult _LHS, NumberOrExprResult _RHS,
              Token _LHSTok, Token _RHSTok) : LHS(_LHS), RHS(_RHS),
                                              LHSTok(_LHSTok), RHSTok(_RHSTok) {
    }
  };
}

/// version - '[' update-list? ']' '@' version-specifier
/// update-list - empty
/// update-list - lhs '=' rhs [',' update-list]
VersionResult ParserImpl::ParseVersion() {
  if (Tok.kind != Token::LSquare)
    return VersionResult(false, UpdateList(0, NULL));
  
  std::vector<WriteInfo> Writes;
  ConsumeLSquare();
  for (;;) {
    Token LHSTok = Tok;
    NumberOrExprResult LHS = ParseNumberOrExpr();
    
    if (Tok.kind != Token::Equals) {
      Error("expected '='.", Tok);
      break;
    }
    
    ConsumeToken();
    Token RHSTok = Tok;
    NumberOrExprResult RHS = ParseNumberOrExpr();

    Writes.push_back(WriteInfo(LHS, RHS, LHSTok, RHSTok));
    
    if (Tok.kind == Token::Comma)
      ConsumeToken();
    else
      break;
  }
  ExpectRSquare("expected close of update list");

  VersionHandle Base(0, NULL);

  if (Tok.kind != Token::At) {
    Error("expected '@'.", Tok);
    return VersionResult(false, UpdateList(0, NULL));
  } 

  ConsumeExpectedToken(Token::At);

  VersionResult BaseRes = ParseVersionSpecifier();
  if (!BaseRes.isValid())
    return BaseRes;

  Base = BaseRes.get();

  Expr::Width ArrayDomainType = Expr::Int32;
  Expr::Width ArrayRangeType = Expr::Int8;

  for (std::vector<WriteInfo>::reverse_iterator it = Writes.rbegin(), 
         ie = Writes.rend(); it != ie; ++it) {
    const WriteInfo &WI = *it;
    ExprResult LHS, RHS;
    // FIXME: This can be factored into common helper for coercing a
    // NumberOrExpr into an Expr.
    if (WI.LHS.isNumber()) {
      LHS = ParseNumberToken(ArrayDomainType, WI.LHS.getNumber());
    } else {
      LHS = WI.LHS.getExpr();
      if (LHS.isValid() && LHS.get()->getWidth() != ArrayDomainType) {
        Error("invalid write index (doesn't match array domain).", WI.LHSTok);
        LHS = ExprResult();
      }
    }

    if (WI.RHS.isNumber()) {
      RHS = ParseNumberToken(ArrayRangeType, WI.RHS.getNumber());
    } else {
      RHS = WI.RHS.getExpr();
      if (RHS.isValid() && RHS.get()->getWidth() != ArrayRangeType) {
        Error("invalid write value (doesn't match array range).", WI.RHSTok);
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
      return Builder->Constant(0, Type);
    }
  }

  // This is a simple but slow way to handle overflow.
  APInt Val(RadixBits * N, 0);
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
      return Builder->Constant(0, Type);
    }

    if (Digit >= Radix) {
      Error("invalid character in numeric token (out of range).", Tok);
      return Builder->Constant(0, Type);
    }

    DigitVal = Digit;
    Val = Val * RadixVal + DigitVal;
  }

  // FIXME: Actually do the check for overflow.
  if (HasMinus)
    Val = -Val;

  if (Type < Val.getBitWidth())
    Val=Val.trunc(Type);
  else if (Type > Val.getBitWidth())
    Val=Val.zext(Type);

  return ExprResult(Builder->Constant(Val));
}

/// ParseTypeSpecifier - Parse a type specifier.
///
/// type = w[0-9]+
TypeResult ParserImpl::ParseTypeSpecifier() {
  assert(Tok.kind == Token::KWWidth && "Unexpected token.");

  // FIXME: Need APInt technically.
  int width = atoi(std::string(Tok.start+1,Tok.length-1).c_str());
  ConsumeToken();

  // FIXME: We should impose some sort of maximum just for sanity?
  return TypeResult(width);
}

void ParserImpl::Error(const char *Message, const Token &At) {
  ++NumErrors;
  if (MaxErrors && NumErrors >= MaxErrors)
    return;

  llvm::errs() << Filename
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
  llvm::errs() << std::string(LineBegin, LineEnd) << "\n";

  // Show the caret or squiggly, making sure to print back spaces the
  // same.
  for (const char *S=LineBegin; S != At.start; ++S)
    llvm::errs() << (isspace(*S) ? *S : ' ');
  if (At.length > 1) {
    for (unsigned i=0; i<At.length; ++i)
      llvm::errs() << '~';
  } else
    llvm::errs() << '^';
  llvm::errs() << '\n';
}

// AST API
// FIXME: Move out of parser.

Decl::Decl(DeclKind _Kind) : Kind(_Kind) {}

void ArrayDecl::dump() {
  llvm::outs() << "array " << Root->name
            << "[" << Root->size << "]"
            << " : " << 'w' << Domain << " -> " << 'w' << Range << " = ";

  if (Root->isSymbolicArray()) {
    llvm::outs() << "symbolic\n";
  } else {
    llvm::outs() << '[';
    for (unsigned i = 0, e = Root->size; i != e; ++i) {
      if (i)
        llvm::outs() << " ";
      llvm::outs() << Root->constantValues[i];
    }
    llvm::outs() << "]\n";
  }
}

void QueryCommand::dump() {
  const ExprHandle *ValuesBegin = 0, *ValuesEnd = 0;
  const Array * const* ObjectsBegin = 0, * const* ObjectsEnd = 0;
  if (!Values.empty()) {
    ValuesBegin = &Values[0];
    ValuesEnd = ValuesBegin + Values.size();
  }
  if (!Objects.empty()) {
    ObjectsBegin = &Objects[0];
    ObjectsEnd = ObjectsBegin + Objects.size();
  }
  ExprPPrinter::printQuery(llvm::outs(), ConstraintManager(Constraints),
                           Query, ValuesBegin, ValuesEnd,
                           ObjectsBegin, ObjectsEnd,
                           false);
}

// Public parser API

Parser::Parser() {
}

Parser::~Parser() {
}

Parser *Parser::Create(const std::string Filename,
                       const MemoryBuffer *MB,
                       ExprBuilder *Builder) {
  ParserImpl *P = new ParserImpl(Filename, MB, Builder);
  P->Initialize();
  return P;
}
