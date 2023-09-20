//===-- Expr.cpp ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Expr.h"

#include "klee/Config/Version.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"
#include "klee/Support/RoundingModeUtil.h"
#include "klee/util/APFloatEval.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Hashing.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(13, 0)
#include "llvm/ADT/StringExtras.h"
#endif
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <cfenv>
#include <sstream>
#include <vector>

using namespace klee;
using namespace llvm;

namespace klee {
llvm::cl::OptionCategory
    ExprCat("Expression building and printing options",
            "These options impact the way expressions are build and printed.");
}

namespace {
cl::opt<bool> ConstArrayOpt(
    "const-array-opt", cl::init(false),
    cl::desc(
        "Enable an optimization involving all-constant arrays (default=false)"),
    cl::cat(klee::ExprCat));

cl::opt<bool>
    SingleReprForNaN("single-repr-for-nan", cl::init(true),
                     cl::desc("When constant folding produce a consistent bit "
                              "pattern for NaN (default=true)."));
} // namespace

/***/

unsigned Expr::count = 0;

ref<Expr> Expr::createTempRead(const Array *array, Expr::Width w,
                               unsigned off) {
  UpdateList ul(array, 0);

  switch (w) {
  default:
    assert(0 && "invalid width");
  case Expr::Bool:
    return ZExtExpr::create(
        ReadExpr::create(ul, ConstantExpr::alloc(off, Expr::Int32)),
        Expr::Bool);
  case Expr::Int8:
    return ReadExpr::create(ul, ConstantExpr::alloc(0, Expr::Int32));
  case Expr::Int16:
    return ConcatExpr::create(
        ReadExpr::create(ul, ConstantExpr::alloc(off + 1, Expr::Int32)),
        ReadExpr::create(ul, ConstantExpr::alloc(off, Expr::Int32)));
  case Expr::Int32:
    return ConcatExpr::create4(
        ReadExpr::create(ul, ConstantExpr::alloc(off + 3, Expr::Int32)),
        ReadExpr::create(ul, ConstantExpr::alloc(off + 2, Expr::Int32)),
        ReadExpr::create(ul, ConstantExpr::alloc(off + 1, Expr::Int32)),
        ReadExpr::create(ul, ConstantExpr::alloc(off, Expr::Int32)));
  case Expr::Int64:
    return ConcatExpr::create8(
        ReadExpr::create(ul, ConstantExpr::alloc(off + 7, Expr::Int32)),
        ReadExpr::create(ul, ConstantExpr::alloc(off + 6, Expr::Int32)),
        ReadExpr::create(ul, ConstantExpr::alloc(off + 5, Expr::Int32)),
        ReadExpr::create(ul, ConstantExpr::alloc(off + 4, Expr::Int32)),
        ReadExpr::create(ul, ConstantExpr::alloc(off + 3, Expr::Int32)),
        ReadExpr::create(ul, ConstantExpr::alloc(off + 2, Expr::Int32)),
        ReadExpr::create(ul, ConstantExpr::alloc(off + 1, Expr::Int32)),
        ReadExpr::create(ul, ConstantExpr::alloc(off, Expr::Int32)));

  case Expr::Fl80: {
    ref<Expr> bytes[10];
    for (int i = 0; i < 10; ++i) {
      bytes[i] =
          ReadExpr::create(ul, ConstantExpr::alloc(off + 9 - i, Expr::Int32));
    }
    return ConcatExpr::createN(10, bytes);
  }
  case Expr::Int128: {
    ref<Expr> bytes[16];
    for (int i = 0; i < 16; ++i) {
      bytes[i] =
          ReadExpr::create(ul, ConstantExpr::alloc(off + 15 - i, Expr::Int32));
    }
    return ConcatExpr::createN(16, bytes);
  }
  }
}

void Expr::splitAnds(ref<Expr> e, std::vector<ref<Expr>> &exprs) {
  if (auto andExpr = dyn_cast<AndExpr>(e)) {
    splitAnds(andExpr->getKid(0), exprs);
    splitAnds(andExpr->getKid(1), exprs);
  } else {
    exprs.push_back(e);
  }
}

int Expr::compare(const Expr &b) const {
  static ExprEquivSet equivs;
  int r = compare(b, equivs);
  equivs.clear();
  return r;
}

bool Expr::equals(const Expr &b) const {
  return (toBeCleared || b.toBeCleared) || (isCached && b.isCached)
             ? this == &b
             : compare(b) == 0;
}

// returns 0 if b is structurally equal to *this
int Expr::compare(const Expr &b, ExprEquivSet &equivs) const {
  if (this == &b)
    return 0;

  const Expr *ap, *bp;
  if (this < &b) {
    ap = this;
    bp = &b;
  } else {
    ap = &b;
    bp = this;
  }

  if (equivs.count(std::make_pair(ap, bp)))
    return 0;

  Kind ak = getKind(), bk = b.getKind();
  if (ak != bk)
    return (ak < bk) ? -1 : 1;

  if (hashValue != b.hashValue)
    return (hashValue < b.hashValue) ? -1 : 1;

  if (int res = compareContents(b))
    return res;

  unsigned aN = getNumKids();
  for (unsigned i = 0; i < aN; i++)
    if (int res = getKid(i)->compare(*b.getKid(i), equivs))
      return res;

  equivs.insert(std::make_pair(ap, bp));
  return 0;
}

void Expr::printKind(llvm::raw_ostream &os, Kind k) {
  switch (k) {
#define X(C)                                                                   \
  case C:                                                                      \
    os << #C;                                                                  \
    break
    X(Constant);
    X(NotOptimized);
    X(Read);
    X(Select);
    X(Concat);
    X(Extract);
    X(ZExt);
    X(SExt);
    X(FPExt);
    X(FPTrunc);
    X(FPToUI);
    X(FPToSI);
    X(UIToFP);
    X(SIToFP);
    X(Add);
    X(Sub);
    X(Mul);
    X(UDiv);
    X(SDiv);
    X(URem);
    X(SRem);
    X(Not);
    X(IsNaN);
    X(IsInfinite);
    X(IsNormal);
    X(IsSubnormal);
    X(And);
    X(Or);
    X(Xor);
    X(Shl);
    X(LShr);
    X(AShr);
    X(FAdd);
    X(FSub);
    X(FMul);
    X(FDiv);
    X(FRem);
    X(FMax);
    X(FMin);
    X(Eq);
    X(Ne);
    X(Ult);
    X(Ule);
    X(Ugt);
    X(Uge);
    X(Slt);
    X(Sle);
    X(Sgt);
    X(Sge);
    X(FOEq);
    X(FOLt);
    X(FOLe);
    X(FOGt);
    X(FOGe);
    X(FSqrt);
    X(FAbs);
    X(FNeg);
    X(FRint);
#undef X
  default:
    assert(0 && "invalid kind");
  }
}

////////
//
// Simple hash functions for various kinds of Exprs
//
///////

unsigned Expr::computeHash() {
  unsigned res = getKind() * Expr::MAGIC_HASH_CONSTANT;

  int n = getNumKids();
  for (int i = 0; i < n; i++) {
    res <<= 1;
    res ^= getKid(i)->hash() * Expr::MAGIC_HASH_CONSTANT;
  }

  hashValue = res;
  return hashValue;
}

unsigned ConstantExpr::computeHash() {
  Expr::Width w = getWidth();
  if (w <= 64)
    hashValue = value.getLimitedValue() ^ (w * MAGIC_HASH_CONSTANT);
  else
    hashValue = hash_value(value) ^ (w * MAGIC_HASH_CONSTANT);

  return hashValue;
}

unsigned CastExpr::computeHash() {
  unsigned res = getWidth() * Expr::MAGIC_HASH_CONSTANT;
  hashValue = res ^ src->hash() * Expr::MAGIC_HASH_CONSTANT;
  return hashValue;
}

unsigned ExtractExpr::computeHash() {
  unsigned res = offset * Expr::MAGIC_HASH_CONSTANT;
  res ^= getWidth() * Expr::MAGIC_HASH_CONSTANT;
  hashValue = res ^ expr->hash() * Expr::MAGIC_HASH_CONSTANT;
  return hashValue;
}

unsigned ReadExpr::computeHash() {
  unsigned res = index->hash() * Expr::MAGIC_HASH_CONSTANT;
  res ^= updates.hash();
  hashValue = res;
  return hashValue;
}

unsigned NotExpr::computeHash() {
  hashValue = expr->hash() * Expr::MAGIC_HASH_CONSTANT * Expr::Not;
  return hashValue;
}

unsigned IsNaNExpr::computeHash() {
  hashValue = expr->hash() * Expr::MAGIC_HASH_CONSTANT * Expr::IsNaN;
  return hashValue;
}

unsigned IsInfiniteExpr::computeHash() {
  hashValue = expr->hash() * Expr::MAGIC_HASH_CONSTANT * Expr::IsInfinite;
  return hashValue;
}

unsigned IsNormalExpr::computeHash() {
  hashValue = expr->hash() * Expr::MAGIC_HASH_CONSTANT * Expr::IsNormal;
  return hashValue;
}

unsigned IsSubnormalExpr::computeHash() {
  hashValue = expr->hash() * Expr::MAGIC_HASH_CONSTANT * Expr::IsSubnormal;
  return hashValue;
}

unsigned FSqrtExpr::computeHash() {
  hashValue = expr->hash() * Expr::MAGIC_HASH_CONSTANT * Expr::FSqrt;
  return hashValue;
}

unsigned FAbsExpr::computeHash() {
  hashValue = expr->hash() * Expr::MAGIC_HASH_CONSTANT * Expr::FAbs;
  return hashValue;
}

unsigned FNegExpr::computeHash() {
  hashValue = expr->hash() * Expr::MAGIC_HASH_CONSTANT * Expr::FNeg;
  return hashValue;
}

unsigned FRintExpr::computeHash() {
  hashValue = expr->hash() * Expr::MAGIC_HASH_CONSTANT * Expr::FRint;
  return hashValue;
}

ref<Expr> Expr::createFromKind(Kind k, std::vector<CreateArg> args) {
  unsigned numArgs = args.size();
  (void)numArgs;

  switch (k) {
  case Constant:
  case Extract:
  case Read:
  default:
    assert(0 && "invalid kind");

  case NotOptimized:
    assert(numArgs == 1 && args[0].isExpr() &&
           "invalid args array for given opcode");
    return NotOptimizedExpr::create(args[0].expr);

  case Select:
    assert(numArgs == 3 && args[0].isExpr() && args[1].isExpr() &&
           args[2].isExpr() && "invalid args array for Select opcode");
    return SelectExpr::create(args[0].expr, args[1].expr, args[2].expr);

  case Concat: {
    assert(numArgs == 2 && args[0].isExpr() && args[1].isExpr() &&
           "invalid args array for Concat opcode");

    return ConcatExpr::create(args[0].expr, args[1].expr);
  }

#define CAST_EXPR_CASE(T)                                                      \
  case T:                                                                      \
    assert(numArgs == 2 && args[0].isExpr() && args[1].isWidth() &&            \
           "invalid args array for given opcode");                             \
    return T##Expr::create(args[0].expr, args[1].width);

#define BINARY_EXPR_CASE(T)                                                    \
  case T:                                                                      \
    assert(numArgs == 2 && args[0].isExpr() && args[1].isExpr() &&             \
           "invalid args array for given opcode");                             \
    return T##Expr::create(args[0].expr, args[1].expr);

    CAST_EXPR_CASE(ZExt);
    CAST_EXPR_CASE(SExt);

    BINARY_EXPR_CASE(Add);
    BINARY_EXPR_CASE(Sub);
    BINARY_EXPR_CASE(Mul);
    BINARY_EXPR_CASE(UDiv);
    BINARY_EXPR_CASE(SDiv);
    BINARY_EXPR_CASE(URem);
    BINARY_EXPR_CASE(SRem);
    BINARY_EXPR_CASE(And);
    BINARY_EXPR_CASE(Or);
    BINARY_EXPR_CASE(Xor);
    BINARY_EXPR_CASE(Shl);
    BINARY_EXPR_CASE(LShr);
    BINARY_EXPR_CASE(AShr);

    BINARY_EXPR_CASE(Eq);
    BINARY_EXPR_CASE(Ne);
    BINARY_EXPR_CASE(Ult);
    BINARY_EXPR_CASE(Ule);
    BINARY_EXPR_CASE(Ugt);
    BINARY_EXPR_CASE(Uge);
    BINARY_EXPR_CASE(Slt);
    BINARY_EXPR_CASE(Sle);
    BINARY_EXPR_CASE(Sgt);
    BINARY_EXPR_CASE(Sge);

#define FP_CAST_EXPR_CASE(T)                                                   \
  case T:                                                                      \
    assert(numArgs == 3 && args[0].isExpr() && args[1].isWidth() &&            \
           args[2].isRoundingMode() && "invalid args array for given opcode"); \
    return T##Expr::create(args[0].expr, args[1].width, args[2].rm);
    FP_CAST_EXPR_CASE(FPTrunc);
    FP_CAST_EXPR_CASE(FPToUI);
    FP_CAST_EXPR_CASE(FPToSI);
    FP_CAST_EXPR_CASE(UIToFP);
    FP_CAST_EXPR_CASE(SIToFP);
#undef FP_CAST_EXPR_CASE

    BINARY_EXPR_CASE(FOEq);
    BINARY_EXPR_CASE(FOLt);
    BINARY_EXPR_CASE(FOLe);
    BINARY_EXPR_CASE(FOGt);
    BINARY_EXPR_CASE(FOGe);
#define BINARY_FP_RM_EXPR_CASE(T)                                              \
  case T:                                                                      \
    assert(numArgs == 3 && args[0].isExpr() && args[1].isExpr() &&             \
           args[2].isRoundingMode() &&                                         \
           "invalid args array for given opccode");                            \
    return T##Expr::create(args[0].expr, args[1].expr, args[2].rm);
    BINARY_FP_RM_EXPR_CASE(FAdd)
    BINARY_FP_RM_EXPR_CASE(FSub)
    BINARY_FP_RM_EXPR_CASE(FMul)
    BINARY_FP_RM_EXPR_CASE(FDiv)
    BINARY_FP_RM_EXPR_CASE(FRem)
    BINARY_FP_RM_EXPR_CASE(FMax)
    BINARY_FP_RM_EXPR_CASE(FMin)
#undef BINARY_FP_RM_EXPR_CASE
  case FSqrt:
    assert(numArgs == 2 && args[0].isExpr() && args[1].isRoundingMode() &&
           "invalid args array for given opccode");
    return FSqrtExpr::create(args[0].expr, args[1].rm);
  case FRint:
    assert(numArgs == 2 && args[0].isExpr() && args[1].isRoundingMode() &&
           "invalid args array for given opccode");
    return FRintExpr::create(args[0].expr, args[1].rm);
#define UNARY_EXPR_CASE(T)                                                     \
  case T:                                                                      \
    assert(numArgs == 1 && args[0].isExpr() &&                                 \
           "invalid args array for given opccode");                            \
    return T##Expr::create(args[0].expr);
    UNARY_EXPR_CASE(Not);
    UNARY_EXPR_CASE(FAbs);
    UNARY_EXPR_CASE(FNeg);
    UNARY_EXPR_CASE(IsNaN);
    UNARY_EXPR_CASE(IsInfinite);
    UNARY_EXPR_CASE(IsNormal);
    UNARY_EXPR_CASE(IsSubnormal);
#undef UNARY_EXPR_CASE
  }
}

void Expr::printWidth(llvm::raw_ostream &os, Width width) {
  switch (width) {
  case Expr::Bool:
    os << "Expr::Bool";
    break;
  case Expr::Int8:
    os << "Expr::Int8";
    break;
  case Expr::Int16:
    os << "Expr::Int16";
    break;
  case Expr::Int32:
    os << "Expr::Int32";
    break;
  case Expr::Int64:
    os << "Expr::Int64";
    break;
  case Expr::Int128:
    os << "Expr::Int128";
    break;
  case Expr::Int256:
    os << "Expr::Int256";
    break;
  case Expr::Int512:
    os << "Expr::Int512";
    break;
  case Expr::Fl80:
    os << "Expr::Fl80";
    break;
  default:
    os << "<invalid type: " << (unsigned)width << ">";
  }
}

ref<Expr> Expr::createImplies(ref<Expr> hyp, ref<Expr> conc) {
  return OrExpr::create(Expr::createIsZero(hyp), conc);
}

ref<Expr> Expr::createIsZero(ref<Expr> e) {
  if (e->getWidth() == Expr::Bool) {
    return NotExpr::create(e);
  } else {
    return EqExpr::create(ConstantExpr::create(0, e->getWidth()), e);
  }
}

ref<Expr> Expr::createTrue() { return ConstantExpr::create(1, Expr::Bool); }

ref<Expr> Expr::createFalse() { return ConstantExpr::create(0, Expr::Bool); }

void Expr::print(llvm::raw_ostream &os) const {
  ExprPPrinter::printSingleExpr(os, const_cast<Expr *>(this));
}

void Expr::dump() const {
  this->print(errs());
  errs() << "\n";
}

std::string Expr::toString() const {
  std::string str;
  llvm::raw_string_ostream output(str);
  this->print(output);
  return str;
}

/***/

Expr::ExprCacheSet Expr::cachedExpressions;

Expr::~Expr() {
  Expr::count--;
  if (isCached) {
    toBeCleared = true;
    cachedExpressions.cache.erase(this);
  }
}

ref<Expr> Expr::createCachedExpr(ref<Expr> e) {

  std::pair<CacheType::const_iterator, bool> success =
      cachedExpressions.cache.insert(e.get());

  if (success.second) {
    // Cache miss
    e->isCached = true;
    return e;
  }
  // Cache hit
  return (ref<Expr>)*(success.first);
}
/***/

ref<Expr> ConstantExpr::fromMemory(void *address, Width width) {
  switch (width) {
  case Expr::Bool:
    return ConstantExpr::create(*((uint8_t *)address), width);
  case Expr::Int8:
    return ConstantExpr::create(*((uint8_t *)address), width);
  case Expr::Int16:
    return ConstantExpr::create(*((uint16_t *)address), width);
  case Expr::Int32:
    return ConstantExpr::create(*((uint32_t *)address), width);
  case Expr::Int64:
    return ConstantExpr::create(*((uint64_t *)address), width);
  // FIXME: what about machines without x87 support?
  case Expr::Fl80: {
    size_t numWords = (width + llvm::APFloatBase::integerPartWidth - 1) /
                      llvm::APFloatBase::integerPartWidth;
    return ConstantExpr::alloc(llvm::APInt(
        width, llvm::ArrayRef<uint64_t>((const uint64_t *)address, numWords)));
  }
  case Expr::Int128:
  case Expr::Int256:
  case Expr::Int512: {
    size_t numWords = width / APInt::APINT_BITS_PER_WORD;
    return ConstantExpr::alloc(llvm::APInt(
        width, llvm::ArrayRef<uint64_t>((const uint64_t *)address, numWords)));
  }
  default:
    // Fl80 branch should also be evaluated as APFloat and not as long double
    return ConstantExpr::alloc(
        llvm::APInt(width,
                    (width + llvm::APFloatBase::integerPartWidth - 1) /
                        llvm::APFloatBase::integerPartWidth,
                    (const uint64_t *)address));
  }
}

void ConstantExpr::toMemory(void *address) {
  auto width = getWidth();
  switch (width) {
  default:
    assert(0 && "invalid type");
  case Expr::Bool:
    *((uint8_t *)address) = getZExtValue(1);
    break;
  case Expr::Int8:
    *((uint8_t *)address) = getZExtValue(8);
    break;
  case Expr::Int16:
    *((uint16_t *)address) = getZExtValue(16);
    break;
  case Expr::Int32:
    *((uint32_t *)address) = getZExtValue(32);
    break;
  case Expr::Int64:
    *((uint64_t *)address) = getZExtValue(64);
    break;
  case Expr::Int128:
  case Expr::Int256:
  case Expr::Int512:
    memcpy(address, value.getRawData(), width / 8);
  // FIXME: what about machines without x87 support?
  case Expr::Fl80:
    *((long double *)address) = *(const long double *)value.getRawData();
    break;
  }
}

void ConstantExpr::toString(std::string &Res, unsigned radix) const {
  if (mIsFloat) {
    llvm::APFloat asF = getAPFloatValue();
    switch (radix) {
    case 10: {
      llvm::SmallVector<char, 16> result;
      // This is supposed to print with enough precision that it can
      // survive a round trip (without precision loss) when using round
      // to nearest even.
      asF.toString(result, /*FormatPrecision=*/0, /*FormatMaxPadding=*/0);
      Res = std::string(result.begin(), result.end());
      return;
    }
    case 16: {
      // Emit C99 Hex float
      unsigned count = 0;
      // Example format is -0x1.000p+4
      // The total number of characters needed is approximately
      //
      // 5 + ceil(significand_bits/4) + 2 + ceil(exponent_bits*log_10(2)) + 1
      //
      // + 1 is for null terminator
      //
      // For IEEEquad (largest we support)
      // significand_bits = 112
      // exponent_bits = 15
      //
      // so we need a buffer size at most 41 bits
      char buffer[41];
      // The rounding mode does not matter here when we set hexDigits to 0 which
      // will give the precise representation. So any rounding mode will do.
      count = asF.convertToHexString(buffer,
                                     /*hexDigits=*/0,
                                     /*upperCase=*/false,
                                     llvm::APFloat::rmNearestTiesToEven);
      assert(count < sizeof(buffer) / sizeof(char));
      Res = buffer;
      return;
    }
    default:
      assert(0 && "Unsupported radix for floating point constant");
    }
  }
#if LLVM_VERSION_CODE >= LLVM_VERSION(13, 0)
  Res = llvm::toString(value, radix, false);
#else
  Res = value.toString(radix, false);
#endif
}

ConstantExpr::ConstantExpr(const llvm::APFloat &v)
    : value(v.bitcastToAPInt()), mIsFloat(true) {
  assert(&(v.getSemantics()) == &(getFloatSemantics()) &&
         "float semantics mismatch");
}

const llvm::fltSemantics &ConstantExpr::widthToFloatSemantics(Width width) {
  switch (width) {
  case Expr::Int16:
    return llvm::APFloat::IEEEhalf();
  case Expr::Int32:
    return llvm::APFloat::IEEEsingle();
  case Expr::Int64:
    return llvm::APFloat::IEEEdouble();
  case Expr::Fl80:
    return llvm::APFloat::x87DoubleExtended();
  case 128:
    return llvm::APFloat::IEEEquad();
  default:
    return llvm::APFloat::Bogus();
  }
}

const llvm::fltSemantics &ConstantExpr::getFloatSemantics() const {
  return widthToFloatSemantics(getWidth());
}

llvm::APFloat ConstantExpr::getAPFloatValue() const {
  const llvm::fltSemantics &fs = getFloatSemantics();
  assert(&fs != &(llvm::APFloat::Bogus()) && "Invalid float semantics");
  return llvm::APFloat(fs, getAPValue());
}

// We should probably move all this x87 fp80 stuff into some
// utility file.
namespace {
bool shouldTryNativex87Eval(const ConstantExpr *lhs, const ConstantExpr *rhs) {
  return lhs && rhs && lhs->getWidth() == 80 && rhs->getWidth() == 80;
};

// WORKAROUND: A bug in llvm::APFloat means long doubles aren't evaluated
// properly in some cases ( https://llvm.org/bugs/show_bug.cgi?id=31292 ).
// Workaround this by evaulating natively if possible.
ref<ConstantExpr> TryNativeX87FP80EvalCmp(const ConstantExpr *lhs,
                                          const ConstantExpr *rhs,
                                          Expr::Kind op) {
  if (!shouldTryNativex87Eval(lhs, rhs))
    return nullptr;

#ifdef __x86_64__
  // Use APInt directly because making an APFloat might change the bit pattern.
  long double lhsAsNative = GetNativeX87FP80FromLLVMAPInt(lhs->getAPValue());
  long double rhsAsNative = GetNativeX87FP80FromLLVMAPInt(rhs->getAPValue());
  bool nativeResult = false;
  switch (op) {
  case Expr::FOEq:
    nativeResult = (lhsAsNative == rhsAsNative);
    break;
  case Expr::FOLt:
    nativeResult = (lhsAsNative < rhsAsNative);
    break;
  case Expr::FOLe:
    nativeResult = (lhsAsNative <= rhsAsNative);
    break;
  case Expr::FOGt:
    nativeResult = (lhsAsNative > rhsAsNative);
    break;
  case Expr::FOGe:
    nativeResult = (lhsAsNative >= rhsAsNative);
    break;
  default:
    llvm_unreachable("Unhandled Expr kind");
  }

  return ConstantExpr::alloc(nativeResult, Expr::Bool);
#else
  klee_warning_once(0, "Trying to evaluate x87 fp80 constant non natively."
                       "Results may be wrong");
  return NULL;
#endif
}

ref<ConstantExpr> TryNativeX87FP80EvalArith(const ConstantExpr *lhs,
                                            const ConstantExpr *rhs,
                                            Expr::Kind op,
                                            llvm::APFloat::roundingMode rm) {
  if (!shouldTryNativex87Eval(lhs, rhs))
    return NULL;
#ifdef __x86_64__
  int roundingMode = LLVMRoundingModeToCRoundingMode(rm);
  if (roundingMode == -1) {
    klee_warning_once(
        0, "Cannot eval x87 fp80 constant natively due to rounding mode"
           "Results may be wrong");
    return NULL;
  }

  // Use APInt directly because making an APFloat might change the bit pattern.
  long double lhsAsNative = GetNativeX87FP80FromLLVMAPInt(lhs->getAPValue());
  long double rhsAsNative = GetNativeX87FP80FromLLVMAPInt(rhs->getAPValue());
  long double nativeResult = false;
  // Save the floating point environment.
  fenv_t fpEnv;
  if (fegetenv(&fpEnv)) {
    llvm::errs() << "Failed to save floating point environment\n";
    abort();
  }
  if (fesetround(roundingMode)) {
    llvm::errs() << "Failed to set new rounding mode\n";
    abort();
  }
  switch (op) {
  case Expr::FAdd:
    nativeResult = lhsAsNative + rhsAsNative;
    break;
  case Expr::FSub:
    nativeResult = lhsAsNative - rhsAsNative;
    break;
  case Expr::FMul:
    nativeResult = lhsAsNative * rhsAsNative;
    break;
  case Expr::FDiv:
    nativeResult = lhsAsNative / rhsAsNative;
    break;
  case Expr::FRem:
    nativeResult = fmodl(lhsAsNative, rhsAsNative);
    break;
  case Expr::FMax:
    nativeResult = fmaxl(lhsAsNative, rhsAsNative);
    break;
  case Expr::FMin:
    nativeResult = fminl(lhsAsNative, rhsAsNative);
    break;
  default:
    llvm_unreachable("Unhandled Expr kind");
  }
  // Restore the floating point environment
  if (fesetenv(&fpEnv)) {
    llvm::errs() << "Failed to restore floating point environment\n";
    abort();
  }

  llvm::APInt apint = GetAPIntFromLongDouble(nativeResult);
  assert(apint.getBitWidth() == 80);
  return ConstantExpr::alloc(apint);
#else
  klee_warning_once(0, "Trying to evaluate x87 fp80 constant non natively."
                       "Results may be wrong");
  return NULL;
#endif
}

ref<ConstantExpr> TryNativeX87FP80EvalCast(const ConstantExpr *ce,
                                           Expr::Width outWidth, Expr::Kind op,
                                           llvm::APFloat::roundingMode rm) {
  if (!shouldTryNativex87Eval(ce, ce))
    return NULL;
#ifdef __x86_64__
  int roundingMode = LLVMRoundingModeToCRoundingMode(rm);
  if (roundingMode == -1) {
    klee_warning_once(
        0, "Cannot eval x87 fp80 constant natively due to rounding mode"
           "Results may be wrong");
    return NULL;
  }
  // Use APInt directly because making an APFloat might change the bit pattern.
  long double argAsNative = GetNativeX87FP80FromLLVMAPInt(ce->getAPValue());
  // Save the floating point environment.
  fenv_t fpEnv;
  if (fegetenv(&fpEnv)) {
    llvm::errs() << "Failed to save floating point environment\n";
    abort();
  }
  if (fesetround(roundingMode)) {
    llvm::errs() << "Failed to set new rounding mode\n";
    abort();
  }
  llvm::APInt apint;
  switch (op) {
  case Expr::FPTrunc: {
    switch (outWidth) {
    case 64: {
      assert(sizeof(double) * 8 == 64);
      double resultAsDouble = (double)argAsNative;
      apint = APInt::doubleToBits(resultAsDouble);
      assert(apint.getBitWidth() == 64);
      break;
    }
    case 32: {
      assert(sizeof(float) * 8 == 32);
      float resultAsFloat = (float)argAsNative;
      apint = APInt::floatToBits(resultAsFloat);
      assert(apint.getBitWidth() == 32);
      break;
    }
    default:
      llvm_unreachable("Unhandled Expr width");
    }
  } break;
  case Expr::FPToSI: {
    uint64_t data = 0;
    unsigned numBits = 0;
    switch (outWidth) {
#define CASE(WIDTH)                                                            \
  case WIDTH: {                                                                \
    int##WIDTH##_t toSI = (int##WIDTH##_t)argAsNative;                         \
    memcpy(&data, &toSI, sizeof(toSI));                                        \
    numBits = WIDTH;                                                           \
    break;                                                                     \
  }
      CASE(8)
      CASE(16)
      CASE(32)
      CASE(64)
#undef CASE
    default:
      llvm_unreachable("Unhandled Expr cast width");
    }
    assert(numBits > 0);
    apint = llvm::APInt(numBits, data, /*signed=*/true);
    assert(apint.getBitWidth() == numBits);
  } break;
  case Expr::FPToUI: {
    uint64_t data = 0;
    unsigned numBits = 0;
    switch (outWidth) {
#define CASE(WIDTH)                                                            \
  case WIDTH: {                                                                \
    uint##WIDTH##_t toUI = (uint##WIDTH##_t)argAsNative;                       \
    memcpy(&data, &toUI, sizeof(toUI));                                        \
    numBits = WIDTH;                                                           \
    break;                                                                     \
  }
      CASE(8)
      CASE(16)
      CASE(32)
      CASE(64)
#undef CASE
    default:
      llvm_unreachable("Unhandled Expr cast width");
    }
    assert(numBits > 0);
    apint = llvm::APInt(numBits, data, /*signed=*/false);
    assert(apint.getBitWidth() == numBits);
  } break;
  default:
    llvm_unreachable("Unhandled Expr kind");
  }
  // Restore the floating point environment
  if (fesetenv(&fpEnv)) {
    llvm::errs() << "Failed to restore floating point environment\n";
    abort();
  }
  return ConstantExpr::alloc(apint);
#else
  klee_warning_once(0, "Trying to evaluate x87 fp80 constant non natively."
                       "Results may be wrong");
  return NULL;
#endif
}

// This is a hack to by-pass evaluation of NaN arguments by APFloat.  We need
// to do this in-order to have the semantics of KLEE's Expr language co-incide
// with Z3's. This is a delicate balencing act (native vs KLEE Expr vs Z3 Expr)
// which I'm trying to get right but probably will fail in some places.
ref<ConstantExpr> tryUnaryOpNaNArgs(const ConstantExpr *arg) {
  if (!SingleReprForNaN)
    return NULL;

  Expr::Width width = arg->getWidth();
  switch (width) {
  case Expr::Int16:
  case Expr::Int32:
  case Expr::Int64:
  case Expr::Int128: {
    // For these cases llvm::APFloat behaves well so
    // we can use it to test if the expression is a NaN
    llvm::APFloat asF = arg->getAPFloatValue();
    if (asF.isNaN())
      return ConstantExpr::GetNaN(width);
    break;
  }
  case Expr::Fl80: {
    // For x87 fp80 we can't use llvm::APFloat directly
    // if the values are "unsupported" values (see 8.2.2 Unsupported Double
    // Extended-Precision Float-Point Encodings and Pseudo-Denormals" from the
    // Intel(R) 64 and IA-32 Architectures Software Developer's Manual) because
    // it incorreclty identifies these arguments as NaNs.
    llvm::APInt api = arg->getAPValue();
    assert(api.getBitWidth() == 80);
    if (api[63]) {
      // This can **only** be a IEEE754 NaN if the explicit significand integer
      // bit is 1. It should be safe to use APFloat now to check if it's a NaN.
      llvm::APFloat asF = arg->getAPFloatValue();
      if (asF.isNaN())
        return ConstantExpr::GetNaN(width);
    }
    break;
  }
  default:
    llvm_unreachable("Unhandled width");
  }
  return NULL;
}

ref<ConstantExpr> tryBinaryOpNaNArgs(const ConstantExpr *lhs,
                                     const ConstantExpr *rhs) {
  ref<ConstantExpr> lhsIsNaN = tryUnaryOpNaNArgs(lhs);
  if (lhsIsNaN.get())
    return lhsIsNaN;
  ref<ConstantExpr> rhsIsNaN = tryUnaryOpNaNArgs(rhs);
  if (rhsIsNaN.get())
    return rhsIsNaN;
  return NULL;
}

ref<ConstantExpr> tryBinaryOpBothNaNArgs(const ConstantExpr *lhs,
                                         const ConstantExpr *rhs) {
  ref<ConstantExpr> lhsIsNaN = tryUnaryOpNaNArgs(lhs);
  ref<ConstantExpr> rhsIsNaN = tryUnaryOpNaNArgs(rhs);
  if (lhsIsNaN.get() && rhsIsNaN.get()) {
    return lhsIsNaN;
  }
  return NULL;
}
} // namespace

ref<ConstantExpr> ConstantExpr::Concat(const ref<ConstantExpr> &RHS) {
  Expr::Width W = getWidth() + RHS->getWidth();
  APInt Tmp(value);
  Tmp = Tmp.zext(W);
  Tmp <<= RHS->getWidth();
  Tmp |= APInt(RHS->value).zext(W);

  return ConstantExpr::alloc(Tmp);
}

ref<ConstantExpr> ConstantExpr::Extract(unsigned Offset, Width W) {
  return ConstantExpr::alloc(APInt(value.ashr(Offset)).zextOrTrunc(W));
}

ref<ConstantExpr> ConstantExpr::ZExt(Width W) {
  return ConstantExpr::alloc(APInt(value).zextOrTrunc(W));
}

ref<ConstantExpr> ConstantExpr::SExt(Width W) {
  return ConstantExpr::alloc(APInt(value).sextOrTrunc(W));
}

ref<ConstantExpr> ConstantExpr::Add(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value + RHS->value);
}

ref<ConstantExpr> ConstantExpr::Neg() { return ConstantExpr::alloc(-value); }

ref<ConstantExpr> ConstantExpr::Sub(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value - RHS->value);
}

ref<ConstantExpr> ConstantExpr::Mul(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value * RHS->value);
}

ref<ConstantExpr> ConstantExpr::UDiv(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.udiv(RHS->value));
}

ref<ConstantExpr> ConstantExpr::SDiv(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.sdiv(RHS->value));
}

ref<ConstantExpr> ConstantExpr::URem(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.urem(RHS->value));
}

ref<ConstantExpr> ConstantExpr::SRem(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.srem(RHS->value));
}

ref<ConstantExpr> ConstantExpr::And(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value & RHS->value);
}

ref<ConstantExpr> ConstantExpr::Or(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value | RHS->value);
}

ref<ConstantExpr> ConstantExpr::Xor(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value ^ RHS->value);
}

ref<ConstantExpr> ConstantExpr::Shl(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.shl(RHS->value));
}

ref<ConstantExpr> ConstantExpr::LShr(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.lshr(RHS->value));
}

ref<ConstantExpr> ConstantExpr::AShr(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.ashr(RHS->value));
}

ref<ConstantExpr> ConstantExpr::Not() { return ConstantExpr::alloc(~value); }

ref<ConstantExpr> ConstantExpr::Eq(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value == RHS->value, Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::Ne(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value != RHS->value, Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::Ult(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.ult(RHS->value), Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::Ule(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.ule(RHS->value), Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::Ugt(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.ugt(RHS->value), Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::Uge(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.uge(RHS->value), Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::Slt(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.slt(RHS->value), Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::Sle(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.sle(RHS->value), Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::Sgt(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.sgt(RHS->value), Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::Sge(const ref<ConstantExpr> &RHS) {
  return ConstantExpr::alloc(value.sge(RHS->value), Expr::Bool);
}

// Floating point

ref<ConstantExpr> ConstantExpr::GetNaN(Expr::Width w) {
#define LLVMFltSemantics(str) llvm::APFloat::str()
  // These values have been chosen to be consistent with Z3 when
  // rewriter.hi_fp_unspecified=false
  llvm::APInt apint;
  const llvm::fltSemantics *sem;
  switch (w) {
  case Int16: {
    apint = llvm::APInt(/*numBits=*/16, (uint64_t)0x7c01, /*isSigned=*/false);
    sem = &(LLVMFltSemantics(IEEEhalf));
    break;
  }
  case Int32: {
    apint =
        llvm::APInt(/*numBits=*/32, (uint64_t)0x7f800001, /*isSigned=*/false);
    sem = &(LLVMFltSemantics(IEEEsingle));
    break;
  }
  case Int64: {
    apint = llvm::APInt(/*numBits=*/64, (uint64_t)0x7ff0000000000001,
                        /*isSigned=*/false);
    sem = &(LLVMFltSemantics(IEEEdouble));
    break;
  }
  case Fl80: {
    // 0x7FFF8000000000000001
    uint64_t temp[] = {0x8000000000000001, (uint64_t)0x7FFF};
    apint = llvm::APInt(/*numBits=*/80, temp);
    sem = &(LLVMFltSemantics(x87DoubleExtended));
    break;
  }
  case Int128: {
    // 0x7FFF0000000000000000000000000001
    uint64_t temp[] = {0x0000000000000001, 0x7FFF000000000000};
    apint = llvm::APInt(/*numBits=*/128, temp);
    sem = &(LLVMFltSemantics(IEEEquad));
    break;
  }
  }
#ifndef NDEBUG
  // Make an APFloat from the bits and check its a NaN
  llvm::APFloat asF(*sem, apint);
  assert(asF.isNaN() && "Failed to create a NaN");
#endif
#undef LLVMFltSemantics
  return ConstantExpr::alloc(apint);
}

ref<ConstantExpr> ConstantExpr::FOEq(const ref<ConstantExpr> &RHS) {
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalCmp(this, RHS.get(), Expr::FOEq);
  if (nativeEval.get())
    return nativeEval;

  APFloat lhsF = this->getAPFloatValue();
  APFloat rhsF = RHS->getAPFloatValue();
  APFloat::cmpResult cmpRes = lhsF.compare(rhsF);
  bool result = (cmpRes == APFloat::cmpEqual);
  return ConstantExpr::alloc(result, Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::FOLt(const ref<ConstantExpr> &RHS) {
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalCmp(this, RHS.get(), Expr::FOLt);
  if (nativeEval.get())
    return nativeEval;

  APFloat lhsF = this->getAPFloatValue();
  APFloat rhsF = RHS->getAPFloatValue();
  APFloat::cmpResult cmpRes = lhsF.compare(rhsF);
  bool result = (cmpRes == APFloat::cmpLessThan);
  return ConstantExpr::alloc(result, Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::FOLe(const ref<ConstantExpr> &RHS) {
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalCmp(this, RHS.get(), Expr::FOLe);
  if (nativeEval.get())
    return nativeEval;

  APFloat lhsF = this->getAPFloatValue();
  APFloat rhsF = RHS->getAPFloatValue();
  APFloat::cmpResult cmpRes = lhsF.compare(rhsF);
  bool result =
      (cmpRes == APFloat::cmpLessThan) || (cmpRes == APFloat::cmpEqual);
  return ConstantExpr::alloc(result, Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::FOGt(const ref<ConstantExpr> &RHS) {
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalCmp(this, RHS.get(), Expr::FOGt);
  if (nativeEval.get())
    return nativeEval;

  APFloat lhsF = this->getAPFloatValue();
  APFloat rhsF = RHS->getAPFloatValue();
  APFloat::cmpResult cmpRes = lhsF.compare(rhsF);
  bool result = (cmpRes == APFloat::cmpGreaterThan);
  return ConstantExpr::alloc(result, Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::FOGe(const ref<ConstantExpr> &RHS) {
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalCmp(this, RHS.get(), Expr::FOGe);
  if (nativeEval.get())
    return nativeEval;

  APFloat lhsF = this->getAPFloatValue();
  APFloat rhsF = RHS->getAPFloatValue();
  APFloat::cmpResult cmpRes = lhsF.compare(rhsF);
  bool result =
      (cmpRes == APFloat::cmpGreaterThan) || (cmpRes == APFloat::cmpEqual);
  return ConstantExpr::alloc(result, Expr::Bool);
}

ref<ConstantExpr> ConstantExpr::FAdd(const ref<ConstantExpr> &RHS,
                                     llvm::APFloat::roundingMode rm) const {
  ref<ConstantExpr> nanEval = tryBinaryOpNaNArgs(this, RHS.get());
  if (nanEval.get())
    return nanEval;
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalArith(this, RHS.get(), Expr::FAdd, rm);
  if (nativeEval.get())
    return nativeEval;

  APFloat result(this->getAPFloatValue());
  // Should we use the status?
  result.add(RHS->getAPFloatValue(), rm);
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FSub(const ref<ConstantExpr> &RHS,
                                     llvm::APFloat::roundingMode rm) const {
  ref<ConstantExpr> nanEval = tryBinaryOpNaNArgs(this, RHS.get());
  if (nanEval.get())
    return nanEval;
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalArith(this, RHS.get(), Expr::FSub, rm);
  if (nativeEval.get())
    return nativeEval;

  APFloat result(this->getAPFloatValue());
  // Should we use the status?
  result.subtract(RHS->getAPFloatValue(), rm);
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FMul(const ref<ConstantExpr> &RHS,
                                     llvm::APFloat::roundingMode rm) const {
  ref<ConstantExpr> nanEval = tryBinaryOpNaNArgs(this, RHS.get());
  if (nanEval.get())
    return nanEval;
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalArith(this, RHS.get(), Expr::FMul, rm);
  if (nativeEval.get())
    return nativeEval;

  APFloat result(this->getAPFloatValue());
  // Should we use the status?
  result.multiply(RHS->getAPFloatValue(), rm);
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FDiv(const ref<ConstantExpr> &RHS,
                                     llvm::APFloat::roundingMode rm) const {
  ref<ConstantExpr> nanEval = tryBinaryOpNaNArgs(this, RHS.get());
  if (nanEval.get())
    return nanEval;
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalArith(this, RHS.get(), Expr::FDiv, rm);
  if (nativeEval.get())
    return nativeEval;

  APFloat result(this->getAPFloatValue());
  // Should we use the status?
  result.divide(RHS->getAPFloatValue(), rm);
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FRem(const ref<ConstantExpr> &RHS,
                                     llvm::APFloat::roundingMode rm) const {
  ref<ConstantExpr> nanEval = tryBinaryOpNaNArgs(this, RHS.get());
  if (nanEval.get())
    return nanEval;
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalArith(this, RHS.get(), Expr::FRem, rm);
  if (nativeEval.get())
    return nativeEval;

  APFloat result(this->getAPFloatValue());
  // Should we use the status?
  result.mod(RHS->getAPFloatValue());
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FMax(const ref<ConstantExpr> &RHS,
                                     llvm::APFloat::roundingMode rm) const {
  ref<ConstantExpr> nanEval = tryBinaryOpBothNaNArgs(this, RHS.get());
  if (nanEval.get())
    return nanEval;
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalArith(this, RHS.get(), Expr::FMax, rm);
  if (nativeEval.get())
    return nativeEval;

  APFloat result = maxnum(this->getAPFloatValue(), RHS->getAPFloatValue());
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FMin(const ref<ConstantExpr> &RHS,
                                     llvm::APFloat::roundingMode rm) const {
  ref<ConstantExpr> nanEval = tryBinaryOpBothNaNArgs(this, RHS.get());
  if (nanEval.get())
    return nanEval;
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalArith(this, RHS.get(), Expr::FMin, rm);
  if (nativeEval.get())
    return nativeEval;

  APFloat result = minnum(this->getAPFloatValue(), RHS->getAPFloatValue());
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FPExt(Width W) const {
  // FIXME: Make the semantics here consistent with Z3.
  assert(W > this->getWidth() && "Invalid FPExt");
  APFloat result(this->getAPFloatValue());
  const llvm::fltSemantics &newType = widthToFloatSemantics(W);
  bool losesInfo = false;
  // The rounding mode has no meaning when extending so we can use any
  // rounding mode here.

  // Should we use the status?
  result.convert(newType, llvm::APFloat::rmNearestTiesToEven, &losesInfo);
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FPTrunc(Width W,
                                        llvm::APFloat::roundingMode rm) const {
  assert(W < this->getWidth() && "Invalid FPTrunc");
  // FIXME: Make the semantics here consistent with Z3.
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalCast(this, W, Expr::FPTrunc, rm);
  if (nativeEval.get())
    return nativeEval;

  APFloat result(this->getAPFloatValue());
  const llvm::fltSemantics &newType = widthToFloatSemantics(W);
  bool losesInfo = false;
  // Should we use the status?
  result.convert(newType, rm, &losesInfo);
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FPToUI(Width W,
                                       llvm::APFloat::roundingMode rm) const {
  // FIXME: Make the semantics here consistent with Z3.
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalCast(this, W, Expr::FPToUI, rm);
  if (nativeEval.get())
    return nativeEval;

  APFloat asF(this->getAPFloatValue());
  // Should we use the status?
  APSInt result(/*BitWidth=*/W, /*isUnsigned=*/true);
  bool isExact = false;
  // What are the semantics when ``asF`` is negative?
  asF.convertToInteger(result, rm, &isExact);
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FPToSI(Width W,
                                       llvm::APFloat::roundingMode rm) const {
  // FIXME: Make the semantics here consistent with Z3.
  ref<ConstantExpr> nativeEval =
      TryNativeX87FP80EvalCast(this, W, Expr::FPToSI, rm);
  if (nativeEval.get())
    return nativeEval;

  APFloat asF(this->getAPFloatValue());
  // Should we use the status?

  APSInt result(/*BitWidth=*/(uint32_t)W, /*isUnsigned=*/false);
  bool isExact = false;
  asF.convertToInteger(result, rm, &isExact);
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::UIToFP(Width W,
                                       llvm::APFloat::roundingMode rm) const {
  const llvm::fltSemantics &newType = widthToFloatSemantics(W);
  llvm::APFloat asF(newType);
  // Should we use the status?
  asF.convertFromAPInt(value, /*isSigned=*/false, rm);
  return ConstantExpr::alloc(asF);
}

ref<ConstantExpr> ConstantExpr::SIToFP(Width W,
                                       llvm::APFloat::roundingMode rm) const {
  const llvm::fltSemantics &newType = widthToFloatSemantics(W);
  llvm::APFloat asF(newType);
  // Should we use the status?
  asF.convertFromAPInt(value, /*isSigned=*/true, rm);
  return ConstantExpr::alloc(asF);
}

ref<ConstantExpr> ConstantExpr::FSqrt(llvm::APFloat::roundingMode rm) const {
  ref<ConstantExpr> nanEval = tryUnaryOpNaNArgs(this);
  if (nanEval.get())
    return nanEval;
  APFloat arg(this->getAPFloatValue());
  llvm::APFloat result = klee::evalSqrt(arg, rm);
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FRint(llvm::APFloat::roundingMode rm) const {
  ref<ConstantExpr> nanEval = tryUnaryOpNaNArgs(this);
  if (nanEval.get())
    return nanEval;
  APFloat result(this->getAPFloatValue());
  result.roundToIntegral(rm);
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FAbs() const {
  // FIXME: Make the semantics here consistent with Z3.
  APFloat result(this->getAPFloatValue());
  if (result.isNegative())
    result.changeSign();
  return ConstantExpr::alloc(result);
}

ref<ConstantExpr> ConstantExpr::FNeg() const {
  // FIXME: Make the semantics here consistent with Z3.
  APFloat result(this->getAPFloatValue());
  result.changeSign();
  return ConstantExpr::alloc(result);
}
/***/

ref<Expr> NotOptimizedExpr::create(ref<Expr> src) {
  return NotOptimizedExpr::alloc(src);
}

/***/

Array::Array(ref<Expr> _size, ref<SymbolicSource> _source, Expr::Width _domain,
             Expr::Width _range, unsigned _id)
    : size(_size), source(_source), domain(_domain), range(_range), id(_id) {
  ref<ConstantExpr> constantSize = dyn_cast<ConstantExpr>(size);
  assert(
      (!isa<ConstantSource>(_source) ||
       cast<ConstantSource>(_source)->size() == constantSize->getZExtValue()) &&
      "Invalid size for constant array!");
  computeHash();
#ifndef NDEBUG
  if (isa<ConstantSource>(_source)) {
    for (const ref<ConstantExpr> *
             it = cast<ConstantSource>(_source)->constantValues.data(),
            *end = cast<ConstantSource>(_source)->constantValues.data() +
                   cast<ConstantSource>(_source)->constantValues.size();
         it != end; ++it)
      assert((*it)->getWidth() == getRange() &&
             "Invalid initial constant value!");
  }
#endif // NDEBUG

  std::set<const Array *> allArrays = _source->getRelatedArrays();
  std::vector<const Array *> sizeArrays;
  findObjects(_size, sizeArrays);
  for (auto i : sizeArrays) {
    allArrays.insert(i);
  }
  for (auto i : allArrays) {
    dependency.insert(i);
    for (auto j : i->dependency) {
      dependency.insert(j);
    }
  }
}

Array::~Array() {}

unsigned Array::computeHash() {
  unsigned res = 0;
  res = (res * Expr::MAGIC_HASH_CONSTANT) + size->hash();
  res = (res * Expr::MAGIC_HASH_CONSTANT) + source->hash();
  hashValue = res;
  return hashValue;
}
/***/

ref<Expr> ReadExpr::create(const UpdateList &ul, ref<Expr> index) {
  // rollback update nodes if possible

  // Iterate through the update list from the most recent to the
  // least recent to find a potential written value for a concrete index;
  // stop if an update with symbolic has been found as we don't know which
  // array element has been updated
  auto un = ul.head.get();
  bool updateListHasSymbolicWrites = false;
  for (; un; un = un->next.get()) {
    ref<Expr> cond = EqExpr::create(index, un->index);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(cond)) {
      if (CE->isTrue())
        // Return the found value
        return un->value;
    } else {
      // Found write with symbolic index
      updateListHasSymbolicWrites = true;
      break;
    }
  }

  if (isa<ConstantSource>(ul.root->source) && !updateListHasSymbolicWrites) {
    // No updates with symbolic index to a constant array have been found
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(index)) {
      assert(CE->getWidth() <= 64 && "Index too large");
      ref<ConstantSource> constantSource =
          cast<ConstantSource>(ul.root->source);
      uint64_t concreteIndex = CE->getZExtValue();
      uint64_t size = constantSource->constantValues.size();
      if (concreteIndex < size) {
        return constantSource->constantValues[concreteIndex];
      }
    }
  }

  // Now, no update with this concrete index exists
  // Try to remove any most recent but unimportant updates
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(index)) {
    if (ConstantExpr *arrayConstantSize =
            dyn_cast<ConstantExpr>(ul.root->size)) {
      assert(CE->getWidth() <= 64 && "Index too large");
      uint64_t concreteIndex = CE->getZExtValue();
      uint64_t size = arrayConstantSize->getZExtValue();
      if (concreteIndex < size) {
        // Create shortened update list
        UpdateList newUpdateList(ul.root, un);
        return ReadExpr::alloc(newUpdateList, index);
      }
    }
  }

  return ReadExpr::alloc(ul, index);
}

int ReadExpr::compareContents(const Expr &b) const {
  return updates.compare(static_cast<const ReadExpr &>(b).updates);
}

ref<Expr> SelectExpr::create(ref<Expr> c, ref<Expr> t, ref<Expr> f) {
  Expr::Width kt = t->getWidth();

  assert(c->getWidth() == Bool && "type mismatch");
  assert(kt == f->getWidth() && "type mismatch");

  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(c)) {
    return CE->isTrue() ? t : f;
  } else if (t == f) {
    return t;
  } else if (kt == Expr::Bool) { // c ? t : f  <=> (c and t) or (not c and f)
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(t)) {
      if (CE->isTrue()) {
        return OrExpr::create(c, f);
      } else {
        return AndExpr::create(Expr::createIsZero(c), f);
      }
    } else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(f)) {
      if (CE->isTrue()) {
        return OrExpr::create(Expr::createIsZero(c), t);
      } else {
        return AndExpr::create(c, t);
      }
    }
  } else if (isa<SelectExpr>(t) || isa<SelectExpr>(f)) {
    if (SelectExpr *se = dyn_cast<SelectExpr>(t)) { // c1 ? (c2 ? t2 : f2) : f1
      if (se->trueExpr ==
          f) { // c1 ? (c2 ? f1 : f2) : f1 <=> c1 /\ not c2 ? f2 : f1
        return SelectExpr::create(
            AndExpr::create(c, Expr::createIsZero(se->cond)), se->falseExpr, f);
      }
      if (se->falseExpr ==
          f) { // c1 ? (c2 ? t2 : f1) : f1 <=> c1 /\ c2 ? t2 : f1
        return SelectExpr::create(AndExpr::create(c, se->cond), se->trueExpr,
                                  f);
      }
    }
    if (SelectExpr *se = dyn_cast<SelectExpr>(f)) { // c1 ? t1 : (c2 ? t2 : f2)
      if (se->trueExpr ==
          t) { // c1 ? t1 : (c2 ? t1 : f2) <=> not c1 /\ not c2 ? f2 : t1
        return SelectExpr::create(AndExpr::create(Expr::createIsZero(c),
                                                  Expr::createIsZero(se->cond)),
                                  se->falseExpr, t);
      }
      if (se->falseExpr ==
          t) { // c1 ? t1 : (c2 ? t2 : t1) <=> not c1 /\ c2 ? t2 : t1
        return SelectExpr::create(
            AndExpr::create(Expr::createIsZero(c), se->cond), se->trueExpr, t);
      }
    }
  } else if (!isa<ConstantExpr>(t) && isa<ConstantExpr>(f)) {
    return SelectExpr::alloc(Expr::createIsZero(c), f, t);
  }

  return SelectExpr::alloc(c, t, f);
}

/***/

ref<Expr> ConcatExpr::create(const ref<Expr> &l, const ref<Expr> &r) {
  Expr::Width w = l->getWidth() + r->getWidth();

  // Fold concatenation of constants.
  //
  // FIXME: concat 0 x -> zext x ?
  if (ConstantExpr *lCE = dyn_cast<ConstantExpr>(l))
    if (ConstantExpr *rCE = dyn_cast<ConstantExpr>(r))
      return lCE->Concat(rCE);

  // Merge contiguous Extracts
  if (ExtractExpr *ee_left = dyn_cast<ExtractExpr>(l)) {
    if (ExtractExpr *ee_right = dyn_cast<ExtractExpr>(r)) {
      if (ee_left->expr == ee_right->expr &&
          ee_right->offset + ee_right->width == ee_left->offset) {
        return ExtractExpr::create(ee_left->expr, ee_right->offset, w);
      }
    }
  }

  return ConcatExpr::alloc(l, r);
}

/// Shortcut to concat N kids.  The chain returned is unbalanced to the right
ref<Expr> ConcatExpr::createN(unsigned n_kids, const ref<Expr> kids[]) {
  assert(n_kids > 0);
  if (n_kids == 1)
    return kids[0];

  ref<Expr> r = ConcatExpr::create(kids[n_kids - 2], kids[n_kids - 1]);
  for (int i = n_kids - 3; i >= 0; i--)
    r = ConcatExpr::create(kids[i], r);
  return r;
}

/// Shortcut to concat 4 kids.  The chain returned is unbalanced to the right
ref<Expr> ConcatExpr::create4(const ref<Expr> &kid1, const ref<Expr> &kid2,
                              const ref<Expr> &kid3, const ref<Expr> &kid4) {
  return ConcatExpr::create(
      kid1, ConcatExpr::create(kid2, ConcatExpr::create(kid3, kid4)));
}

/// Shortcut to concat 8 kids.  The chain returned is unbalanced to the right
ref<Expr> ConcatExpr::create8(const ref<Expr> &kid1, const ref<Expr> &kid2,
                              const ref<Expr> &kid3, const ref<Expr> &kid4,
                              const ref<Expr> &kid5, const ref<Expr> &kid6,
                              const ref<Expr> &kid7, const ref<Expr> &kid8) {
  return ConcatExpr::create(
      kid1,
      ConcatExpr::create(
          kid2,
          ConcatExpr::create(
              kid3, ConcatExpr::create(
                        kid4, ConcatExpr::create4(kid5, kid6, kid7, kid8)))));
}

/***/

ref<Expr> ExtractExpr::create(ref<Expr> expr, unsigned off, Width w) {
  unsigned kw = expr->getWidth();
  assert(w > 0 && off + w <= kw && "invalid extract");

  if (w == kw) {
    return expr;
  } else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    return CE->Extract(off, w);
  } else if (SelectExpr *se = dyn_cast<SelectExpr>(expr)) {
  } else {
    // Extract(Concat)
    if (ConcatExpr *ce = dyn_cast<ConcatExpr>(expr)) {
      // if the extract skips the right side of the concat
      if (off >= ce->getRight()->getWidth())
        return ExtractExpr::create(ce->getLeft(),
                                   off - ce->getRight()->getWidth(), w);

      // if the extract skips the left side of the concat
      if (off + w <= ce->getRight()->getWidth())
        return ExtractExpr::create(ce->getRight(), off, w);

      // E(C(x,y)) = C(E(x), E(y))
      return ConcatExpr::create(
          ExtractExpr::create(ce->getKid(0), 0,
                              w - ce->getKid(1)->getWidth() + off),
          ExtractExpr::create(ce->getKid(1), off,
                              ce->getKid(1)->getWidth() - off));
    }
  }

  return ExtractExpr::alloc(expr, off, w);
}

/***/

ref<Expr> NotExpr::create(const ref<Expr> &e) {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e))
    return CE->Not();

  if (NotExpr *NE = dyn_cast<NotExpr>(e)) {
    return NE->expr;
  }

  if (AndExpr *AE = dyn_cast<AndExpr>(e)) {
    return OrExpr::create(NotExpr::create(AE->left),
                          NotExpr::create(AE->right));
  }

  if (OrExpr *OE = dyn_cast<OrExpr>(e)) {
    return AndExpr::create(NotExpr::create(OE->left),
                           NotExpr::create(OE->right));
  }

  if (SelectExpr *SE = dyn_cast<SelectExpr>(e)) {
    return SelectExpr::create(SE->cond, NotExpr::create(SE->trueExpr),
                              NotExpr::create(SE->falseExpr));
  }

  return NotExpr::alloc(e);
}

/***/

ref<Expr> ZExtExpr::create(const ref<Expr> &e, Width w) {
  unsigned kBits = e->getWidth();
  if (w == kBits) {
    return e;
  } else if (w < kBits) { // trunc
    return ExtractExpr::create(e, 0, w);
  } else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e)) {
    return CE->ZExt(w);
  } else if (SelectExpr *se = dyn_cast<SelectExpr>(e)) {
    if (isa<ConstantExpr>(se->trueExpr)) {
      return SelectExpr::create(se->cond, ZExtExpr::create(se->trueExpr, w),
                                ZExtExpr::create(se->falseExpr, w));
    }
  }

  return ZExtExpr::alloc(e, w);
}

ref<Expr> SExtExpr::create(const ref<Expr> &e, Width w) {
  unsigned kBits = e->getWidth();
  if (w == kBits) {
    return e;
  } else if (w < kBits) { // trunc
    return ExtractExpr::create(e, 0, w);
  } else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e)) {
    return CE->SExt(w);
  } else if (SelectExpr *se = dyn_cast<SelectExpr>(e)) {
    if (isa<ConstantExpr>(se->trueExpr)) {
      return SelectExpr::create(se->cond, SExtExpr::create(se->trueExpr, w),
                                SExtExpr::create(se->falseExpr, w));
    }
  }

  return SExtExpr::alloc(e, w);
}

/***/

ref<Expr> FPExtExpr::create(const ref<Expr> &e, Width w) {
  unsigned kBits = e->getWidth();
  if (w == kBits) {
    return e;
  } else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e)) {
    return CE->FPExt(w);
  } else {
    return FPExtExpr::alloc(e, w);
  }
}

ref<Expr> FPTruncExpr::create(const ref<Expr> &e, Width w,
                              llvm::APFloat::roundingMode rm) {
  unsigned kBits = e->getWidth();
  if (w == kBits) {
    return e;
  } else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e)) {
    return CE->FPTrunc(w, rm);
  } else {
    return FPTruncExpr::alloc(e, w, rm);
  }
}

ref<Expr> FPToUIExpr::create(const ref<Expr> &e, Width w,
                             llvm::APFloat::roundingMode rm) {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e)) {
    return CE->FPToUI(w, rm);
  } else {
    return FPToUIExpr::alloc(e, w, rm);
  }
}

ref<Expr> FPToSIExpr::create(const ref<Expr> &e, Width w,
                             llvm::APFloat::roundingMode rm) {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e)) {
    return CE->FPToSI(w, rm);
  } else {
    return FPToSIExpr::alloc(e, w, rm);
  }
}

ref<Expr> UIToFPExpr::create(const ref<Expr> &e, Width w,
                             llvm::APFloat::roundingMode rm) {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e)) {
    return CE->UIToFP(w, rm);
  } else {
    return UIToFPExpr::alloc(e, w, rm);
  }
}

ref<Expr> SIToFPExpr::create(const ref<Expr> &e, Width w,
                             llvm::APFloat::roundingMode rm) {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e)) {
    return CE->SIToFP(w, rm);
  } else {
    return SIToFPExpr::alloc(e, w, rm);
  }
}

/***/

static ref<Expr> AndExpr_create(Expr *l, Expr *r);
static ref<Expr> XorExpr_create(Expr *l, Expr *r);

static ref<Expr> EqExpr_createPartial(Expr *l, const ref<ConstantExpr> &cr);
static ref<Expr> AndExpr_createPartialR(const ref<ConstantExpr> &cl, Expr *r);
static ref<Expr> SubExpr_createPartialR(const ref<ConstantExpr> &cl, Expr *r);
static ref<Expr> XorExpr_createPartialR(const ref<ConstantExpr> &cl, Expr *r);

static ref<Expr> AddExpr_createPartialR(const ref<ConstantExpr> &cl, Expr *r) {
  Expr::Width type = cl->getWidth();

  if (type == Expr::Bool) {
    return XorExpr_createPartialR(cl, r);
  } else if (cl->isZero()) {
    return r;
  } else {
    Expr::Kind rk = r->getKind();
    if (rk == Expr::Add &&
        isa<ConstantExpr>(r->getKid(0))) { // A + (B+c) == (A+B) + c
      return AddExpr::create(AddExpr::create(cl, r->getKid(0)), r->getKid(1));
    } else if (rk == Expr::Sub &&
               isa<ConstantExpr>(r->getKid(0))) { // A + (B-c) == (A+B) - c
      return SubExpr::create(AddExpr::create(cl, r->getKid(0)), r->getKid(1));
    } else {
      return AddExpr::alloc(cl, r);
    }
  }
}
static ref<Expr> AddExpr_createPartial(Expr *l, const ref<ConstantExpr> &cr) {
  return AddExpr_createPartialR(cr, l);
}
static ref<Expr> AddExpr_create(Expr *l, Expr *r) {
  Expr::Width type = l->getWidth();

  if (type == Expr::Bool) {
    return XorExpr_create(l, r);
  } else {
    Expr::Kind lk = l->getKind(), rk = r->getKind();
    if (lk == Expr::Add &&
        isa<ConstantExpr>(l->getKid(0))) { // (k+a)+b = k+(a+b)
      return AddExpr::create(l->getKid(0), AddExpr::create(l->getKid(1), r));
    } else if (lk == Expr::Sub &&
               isa<ConstantExpr>(l->getKid(0))) { // (k-a)+b = k+(b-a)
      return AddExpr::create(l->getKid(0), SubExpr::create(r, l->getKid(1)));
    } else if (rk == Expr::Add &&
               isa<ConstantExpr>(r->getKid(0))) { // a + (k+b) = k+(a+b)
      return AddExpr::create(r->getKid(0), AddExpr::create(l, r->getKid(1)));
    } else if (rk == Expr::Sub &&
               isa<ConstantExpr>(r->getKid(0))) { // a + (k-b) = k+(a-b)
      return AddExpr::create(r->getKid(0), SubExpr::create(l, r->getKid(1)));
    } else {
      return AddExpr::alloc(l, r);
    }
  }
}

static ref<Expr> SubExpr_createPartialR(const ref<ConstantExpr> &cl, Expr *r) {
  Expr::Width type = cl->getWidth();

  if (type == Expr::Bool) {
    return XorExpr_createPartialR(cl, r);
  } else {
    Expr::Kind rk = r->getKind();
    if (rk == Expr::Add &&
        isa<ConstantExpr>(r->getKid(0))) { // A - (B+c) == (A-B) - c
      return SubExpr::create(SubExpr::create(cl, r->getKid(0)), r->getKid(1));
    } else if (rk == Expr::Sub &&
               isa<ConstantExpr>(r->getKid(0))) { // A - (B-c) == (A-B) + c
      return AddExpr::create(SubExpr::create(cl, r->getKid(0)), r->getKid(1));
    } else {
      return SubExpr::alloc(cl, r);
    }
  }
}
static ref<Expr> SubExpr_createPartial(Expr *l, const ref<ConstantExpr> &cr) {
  // l - c => l + (-c)
  return AddExpr_createPartial(l,
                               ConstantExpr::alloc(0, cr->getWidth())->Sub(cr));
}
static ref<Expr> SubExpr_create(Expr *l, Expr *r) {
  Expr::Width type = l->getWidth();

  if (type == Expr::Bool) {
    return XorExpr_create(l, r);
  } else if (*l == *r) {
    return ConstantExpr::alloc(0, type);
  } else {
    Expr::Kind lk = l->getKind(), rk = r->getKind();
    if (lk == Expr::Add &&
        isa<ConstantExpr>(l->getKid(0))) { // (k+a)-b = k+(a-b)
      return AddExpr::create(l->getKid(0), SubExpr::create(l->getKid(1), r));
    } else if (lk == Expr::Sub &&
               isa<ConstantExpr>(l->getKid(0))) { // (k-a)-b = k-(a+b)
      return SubExpr::create(l->getKid(0), AddExpr::create(l->getKid(1), r));
    } else if (rk == Expr::Add &&
               isa<ConstantExpr>(r->getKid(0))) { // a - (k+b) = (a-c) - k
      return SubExpr::create(SubExpr::create(l, r->getKid(1)), r->getKid(0));
    } else if (rk == Expr::Sub &&
               isa<ConstantExpr>(r->getKid(0))) { // a - (k-b) = (a+b) - k
      return SubExpr::create(AddExpr::create(l, r->getKid(1)), r->getKid(0));
    } else {
      return SubExpr::alloc(l, r);
    }
  }
}

static ref<Expr> MulExpr_createPartialR(const ref<ConstantExpr> &cl, Expr *r) {
  Expr::Width type = cl->getWidth();

  if (type == Expr::Bool) {
    return AndExpr_createPartialR(cl, r);
  } else if (cl->isOne()) {
    return r;
  } else if (cl->isZero()) {
    return cl;
  } else {
    return MulExpr::alloc(cl, r);
  }
}
static ref<Expr> MulExpr_createPartial(Expr *l, const ref<ConstantExpr> &cr) {
  return MulExpr_createPartialR(cr, l);
}
static ref<Expr> MulExpr_create(Expr *l, Expr *r) {
  Expr::Width type = l->getWidth();

  if (type == Expr::Bool) {
    return AndExpr::alloc(l, r);
  } else {
    return MulExpr::alloc(l, r);
  }
}

static ref<Expr> AndExpr_createPartial(Expr *l, const ref<ConstantExpr> &cr) {
  if (cr->isAllOnes()) {
    return l;
  } else if (cr->isZero()) {
    return cr;
  } else {
    return AndExpr::alloc(l, cr);
  }
}
static ref<Expr> AndExpr_createPartialR(const ref<ConstantExpr> &cl, Expr *r) {
  return AndExpr_createPartial(r, cl);
}
static ref<Expr> AndExpr_create(Expr *l, Expr *r) {
  return AndExpr::alloc(l, r);
}

static ref<Expr> OrExpr_createPartial(Expr *l, const ref<ConstantExpr> &cr) {
  if (cr->isAllOnes()) {
    return cr;
  } else if (cr->isZero()) {
    return l;
  } else {
    return OrExpr::alloc(l, cr);
  }
}
static ref<Expr> OrExpr_createPartialR(const ref<ConstantExpr> &cl, Expr *r) {
  return OrExpr_createPartial(r, cl);
}
static ref<Expr> OrExpr_create(Expr *l, Expr *r) { return OrExpr::alloc(l, r); }

static ref<Expr> XorExpr_createPartialR(const ref<ConstantExpr> &cl, Expr *r) {
  if (cl->isZero()) {
    return r;
  } else if (cl->getWidth() == Expr::Bool) {
    return EqExpr_createPartial(r, ConstantExpr::create(0, Expr::Bool));
  } else {
    return XorExpr::alloc(cl, r);
  }
}

static ref<Expr> XorExpr_createPartial(Expr *l, const ref<ConstantExpr> &cr) {
  return XorExpr_createPartialR(cr, l);
}
static ref<Expr> XorExpr_create(Expr *l, Expr *r) {
  return XorExpr::alloc(l, r);
}

static ref<Expr> UDivExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  if (l->getWidth() == Expr::Bool) { // r must be 1
    return l;
  } else {
    return UDivExpr::alloc(l, r);
  }
}

static ref<Expr> SDivExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  if (l->getWidth() == Expr::Bool) { // r must be 1
    return l;
  } else {
    return SDivExpr::alloc(l, r);
  }
}

static ref<Expr> URemExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  if (l->getWidth() == Expr::Bool) { // r must be 1
    return ConstantExpr::create(0, Expr::Bool);
  } else {
    return URemExpr::alloc(l, r);
  }
}

static ref<Expr> SRemExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  if (l->getWidth() == Expr::Bool) { // r must be 1
    return ConstantExpr::create(0, Expr::Bool);
  } else {
    return SRemExpr::alloc(l, r);
  }
}

static ref<Expr> ShlExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  if (l->getWidth() == Expr::Bool) { // l & !r
    return AndExpr::create(l, Expr::createIsZero(r));
  } else {
    return ShlExpr::alloc(l, r);
  }
}

static ref<Expr> LShrExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  if (l->getWidth() == Expr::Bool) { // l & !r
    return AndExpr::create(l, Expr::createIsZero(r));
  } else {
    return LShrExpr::alloc(l, r);
  }
}

static ref<Expr> AShrExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  if (l->getWidth() == Expr::Bool) { // l
    return l;
  } else {
    return AShrExpr::alloc(l, r);
  }
}

#define BCREATE_R(_e_op, _op, partialL, partialR)                              \
  ref<Expr> _e_op ::create(const ref<Expr> &l, const ref<Expr> &r) {           \
    assert(l->getWidth() == r->getWidth() && "type mismatch");                 \
    if (SelectExpr *sel = dyn_cast<SelectExpr>(l)) {                           \
      if (isa<ConstantExpr>(sel->trueExpr)) {                                  \
        return SelectExpr::create(sel->cond, _e_op::create(sel->trueExpr, r),  \
                                  _e_op::create(sel->falseExpr, r));           \
      }                                                                        \
    }                                                                          \
    if (SelectExpr *ser = dyn_cast<SelectExpr>(r)) {                           \
      if (isa<ConstantExpr>(ser->trueExpr)) {                                  \
        return SelectExpr::create(ser->cond, _e_op::create(l, ser->trueExpr),  \
                                  _e_op::create(l, ser->falseExpr));           \
      }                                                                        \
    }                                                                          \
    if (ConstantExpr *cl = dyn_cast<ConstantExpr>(l)) {                        \
      if (ConstantExpr *cr = dyn_cast<ConstantExpr>(r))                        \
        return cl->_op(cr);                                                    \
      return _e_op##_createPartialR(cl, r.get());                              \
    } else if (ConstantExpr *cr = dyn_cast<ConstantExpr>(r)) {                 \
      return _e_op##_createPartial(l.get(), cr);                               \
    }                                                                          \
    return _e_op##_create(l.get(), r.get());                                   \
  }

#define BCREATE(_e_op, _op)                                                    \
  ref<Expr> _e_op ::create(const ref<Expr> &l, const ref<Expr> &r) {           \
    assert(l->getWidth() == r->getWidth() && "type mismatch");                 \
    if (SelectExpr *sel = dyn_cast<SelectExpr>(l)) {                           \
      if (isa<ConstantExpr>(sel->trueExpr)) {                                  \
        return SelectExpr::create(sel->cond, _e_op::create(sel->trueExpr, r),  \
                                  _e_op::create(sel->falseExpr, r));           \
      }                                                                        \
    }                                                                          \
    if (SelectExpr *ser = dyn_cast<SelectExpr>(r)) {                           \
      if (isa<ConstantExpr>(ser->trueExpr)) {                                  \
        return SelectExpr::create(ser->cond, _e_op::create(l, ser->trueExpr),  \
                                  _e_op::create(l, ser->falseExpr));           \
      }                                                                        \
    }                                                                          \
    if (ConstantExpr *cl = dyn_cast<ConstantExpr>(l))                          \
      if (ConstantExpr *cr = dyn_cast<ConstantExpr>(r))                        \
        return cl->_op(cr);                                                    \
    return _e_op##_create(l, r);                                               \
  }

BCREATE_R(AddExpr, Add, AddExpr_createPartial, AddExpr_createPartialR)
BCREATE_R(SubExpr, Sub, SubExpr_createPartial, SubExpr_createPartialR)
BCREATE_R(MulExpr, Mul, MulExpr_createPartial, MulExpr_createPartialR)
BCREATE_R(AndExpr, And, AndExpr_createPartial, AndExpr_createPartialR)
BCREATE_R(OrExpr, Or, OrExpr_createPartial, OrExpr_createPartialR)
BCREATE_R(XorExpr, Xor, XorExpr_createPartial, XorExpr_createPartialR)
BCREATE(UDivExpr, UDiv)
BCREATE(SDivExpr, SDiv)
BCREATE(URemExpr, URem)
BCREATE(SRemExpr, SRem)
BCREATE(ShlExpr, Shl)
BCREATE(LShrExpr, LShr)
BCREATE(AShrExpr, AShr)

#define CMPCREATE(_e_op, _op)                                                  \
  ref<Expr> _e_op ::create(const ref<Expr> &l, const ref<Expr> &r) {           \
    assert(l->getWidth() == r->getWidth() && "type mismatch");                 \
    if (SelectExpr *sel = dyn_cast<SelectExpr>(l)) {                           \
      if (isa<ConstantExpr>(sel->trueExpr)) {                                  \
        return SelectExpr::create(sel->cond, _e_op::create(sel->trueExpr, r),  \
                                  _e_op::create(sel->falseExpr, r));           \
      }                                                                        \
    }                                                                          \
    if (SelectExpr *ser = dyn_cast<SelectExpr>(r)) {                           \
      if (isa<ConstantExpr>(ser->trueExpr)) {                                  \
        return SelectExpr::create(ser->cond, _e_op::create(l, ser->trueExpr),  \
                                  _e_op::create(l, ser->falseExpr));           \
      }                                                                        \
    }                                                                          \
    if (ConstantExpr *cl = dyn_cast<ConstantExpr>(l))                          \
      if (ConstantExpr *cr = dyn_cast<ConstantExpr>(r))                        \
        return cl->_op(cr);                                                    \
    return _e_op##_create(l, r);                                               \
  }

#define CMPCREATE_T(_e_op, _op, _reflexive_e_op, partialL, partialR)           \
  ref<Expr> _e_op ::create(const ref<Expr> &l, const ref<Expr> &r) {           \
    assert(l->getWidth() == r->getWidth() && "type mismatch");                 \
    if (SelectExpr *sel = dyn_cast<SelectExpr>(l)) {                           \
      if (isa<ConstantExpr>(sel->trueExpr)) {                                  \
        return SelectExpr::create(sel->cond, _e_op::create(sel->trueExpr, r),  \
                                  _e_op::create(sel->falseExpr, r));           \
      }                                                                        \
    }                                                                          \
    if (SelectExpr *ser = dyn_cast<SelectExpr>(r)) {                           \
      if (isa<ConstantExpr>(ser->trueExpr)) {                                  \
        return SelectExpr::create(ser->cond, _e_op::create(l, ser->trueExpr),  \
                                  _e_op::create(l, ser->falseExpr));           \
      }                                                                        \
    }                                                                          \
    if (ConstantExpr *cl = dyn_cast<ConstantExpr>(l)) {                        \
      if (ConstantExpr *cr = dyn_cast<ConstantExpr>(r))                        \
        return cl->_op(cr);                                                    \
      return partialR(cl, r.get());                                            \
    } else if (ConstantExpr *cr = dyn_cast<ConstantExpr>(r)) {                 \
      return partialL(l.get(), cr);                                            \
    }                                                                          \
    return _e_op##_create(l.get(), r.get());                                   \
  }

static ref<Expr> EqExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  if (l == r) {
    return ConstantExpr::alloc(1, Expr::Bool);
  } else if (isa<AddExpr>(l) &&
             (cast<AddExpr>(l)->left == r || cast<AddExpr>(l)->right == r)) {
    ref<AddExpr> al = cast<AddExpr>(l);
    if (al->left == r) {
      return Expr::createIsZero(al->right);
    } else {
      return Expr::createIsZero(al->left);
    }
  } else if (isa<AddExpr>(r) &&
             (cast<AddExpr>(r)->left == l || cast<AddExpr>(r)->right == l)) {
    ref<AddExpr> ar = cast<AddExpr>(r);
    if (ar->left == l) {
      return Expr::createIsZero(ar->right);
    } else {
      return Expr::createIsZero(ar->left);
    }
  } else if (isa<AddExpr>(l) && isa<AddExpr>(r)) {
    ref<AddExpr> al = cast<AddExpr>(l);
    ref<AddExpr> ar = cast<AddExpr>(r);
    if (al->left == ar->left) {
      return EqExpr::create(al->right, ar->right);
    }
    if (al->left == ar->right) {
      return EqExpr::create(al->right, ar->left);
    }
    if (al->right == ar->left) {
      return EqExpr::create(al->left, ar->right);
    }
    if (al->right == ar->right) {
      return EqExpr::create(al->left, ar->left);
    }
  } else if (isa<SelectExpr>(l) || isa<SelectExpr>(r)) {
    if (SelectExpr *se = dyn_cast<SelectExpr>(l)) {
      if (isa<ConstantExpr>(se->trueExpr)) {
        return SelectExpr::create(se->cond, EqExpr::create(se->trueExpr, r),
                                  EqExpr::create(se->falseExpr, r));
      }
    }
    if (SelectExpr *se = dyn_cast<SelectExpr>(r)) {
      if (isa<ConstantExpr>(se->trueExpr)) {
        return SelectExpr::create(se->cond, EqExpr::create(l, se->trueExpr),
                                  EqExpr::create(l, se->falseExpr));
      }
    }
  }

  return EqExpr::alloc(l, r);
}

/// Tries to optimize EqExpr cl == rd, where cl is a ConstantExpr and
/// rd a ReadExpr.  If rd is a read into an all-constant array,
/// returns a disjunction of equalities on the index.  Otherwise,
/// returns the initial equality expression.
static ref<Expr> TryConstArrayOpt(const ref<ConstantExpr> &cl, ReadExpr *rd) {
  if (rd->updates.root->isSymbolicArray() || rd->updates.getSize())
    return EqExpr_create(cl, rd);

  // Number of positions in the array that contain value ct.
  unsigned numMatches = 0;

  // for now, just assume standard "flushing" of a concrete array,
  // where the concrete array has one update for each index, in order
  ref<Expr> res = ConstantExpr::alloc(0, Expr::Bool);
  if (ref<ConstantSource> constantSource =
          dyn_cast<ConstantSource>(rd->updates.root->source)) {
    for (unsigned i = 0, e = constantSource->constantValues.size(); i != e;
         ++i) {
      if (cl == constantSource->constantValues[i]) {
        // Arbitrary maximum on the size of disjunction.
        if (++numMatches > 100)
          return EqExpr_create(cl, rd);

        ref<Expr> mayBe = EqExpr::create(
            rd->index, ConstantExpr::alloc(i, rd->index->getWidth()));
        res = OrExpr::create(res, mayBe);
      }
    }
  }

  return res;
}

static ref<Expr> EqExpr_createPartialR(const ref<ConstantExpr> &cl, Expr *r) {
  Expr::Width width = cl->getWidth();

  Expr::Kind rk = r->getKind();
  if (width == Expr::Bool) {
    if (cl->isTrue()) {
      return r;
    } else {
      // 0 == ...

      if (rk == Expr::Eq) {
        const EqExpr *ree = cast<EqExpr>(r);

        // eliminate double negation
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ree->left)) {
          // 0 == (0 == A) => A
          if (CE->getWidth() == Expr::Bool && CE->isFalse())
            return ree->right;
        }
      } else if (rk == Expr::Or) {
        const OrExpr *roe = cast<OrExpr>(r);

        // transform not(or(a,b)) to and(not a, not b)
        return AndExpr::create(Expr::createIsZero(roe->left),
                               Expr::createIsZero(roe->right));
      }
      return NotExpr::create(r);
    }
  } else if (rk == Expr::SExt) {
    // (sext(a,T)==c) == (a==c)
    const SExtExpr *see = cast<SExtExpr>(r);
    Expr::Width fromBits = see->src->getWidth();
    ref<ConstantExpr> trunc = cl->ZExt(fromBits);

    // pathological check, make sure it is possible to
    // sext to this value *from any value*
    if (cl == trunc->SExt(width)) {
      return EqExpr::create(see->src, trunc);
    } else {
      return ConstantExpr::create(0, Expr::Bool);
    }
  } else if (rk == Expr::ZExt) {
    // (zext(a,T)==c) == (a==c)
    const ZExtExpr *zee = cast<ZExtExpr>(r);
    Expr::Width fromBits = zee->src->getWidth();
    ref<ConstantExpr> trunc = cl->ZExt(fromBits);

    // pathological check, make sure it is possible to
    // zext to this value *from any value*
    if (cl == trunc->ZExt(width)) {
      return EqExpr::create(zee->src, trunc);
    } else {
      return ConstantExpr::create(0, Expr::Bool);
    }
  } else if (rk == Expr::Add) {
    const AddExpr *ae = cast<AddExpr>(r);
    if (isa<ConstantExpr>(ae->left)) {
      // c0 = c1 + b => c0 - c1 = b
      return EqExpr_createPartialR(
          cast<ConstantExpr>(SubExpr::create(cl, ae->left)), ae->right.get());
    }
  } else if (rk == Expr::Sub) {
    const SubExpr *se = cast<SubExpr>(r);
    if (isa<ConstantExpr>(se->left)) {
      // c0 = c1 - b => c1 - c0 = b
      return EqExpr_createPartialR(
          cast<ConstantExpr>(SubExpr::create(se->left, cl)), se->right.get());
    }
    if (cl->isZero()) {
      return EqExpr::create(se->left, se->right);
    }
  } else if (rk == Expr::Read && ConstArrayOpt) {
    return TryConstArrayOpt(cl, static_cast<ReadExpr *>(r));
  }

  return EqExpr_create(cl, r);
}

static ref<Expr> EqExpr_createPartial(Expr *l, const ref<ConstantExpr> &cr) {
  return EqExpr_createPartialR(cr, l);
}

ref<Expr> NeExpr::create(const ref<Expr> &l, const ref<Expr> &r) {
  return Expr::createIsZero(EqExpr::create(l, r));
}

ref<Expr> UgtExpr::create(const ref<Expr> &l, const ref<Expr> &r) {
  return UltExpr::create(r, l);
}
ref<Expr> UgeExpr::create(const ref<Expr> &l, const ref<Expr> &r) {
  return UleExpr::create(r, l);
}

ref<Expr> SgtExpr::create(const ref<Expr> &l, const ref<Expr> &r) {
  return SltExpr::create(r, l);
}
ref<Expr> SgeExpr::create(const ref<Expr> &l, const ref<Expr> &r) {
  return SleExpr::create(r, l);
}

static ref<Expr> UltExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  Expr::Width t = l->getWidth();
  if (t == Expr::Bool) { // !l && r
    return AndExpr::create(Expr::createIsZero(l), r);
  } else if (r->isOne()) {
    return EqExpr::create(l, ConstantExpr::create(0, t));
  } else if (r->isZero()) {
    return Expr::createFalse();
  } else {
    return UltExpr::alloc(l, r);
  }
}

static ref<Expr> UleExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  if (l->getWidth() == Expr::Bool) { // !(l && !r)
    return OrExpr::create(Expr::createIsZero(l), r);
  } else if (r->isZero()) {
    return EqExpr::create(l, r);
  } else {
    return UleExpr::alloc(l, r);
  }
}

static ref<Expr> SltExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  if (l->getWidth() == Expr::Bool) { // l && !r
    return AndExpr::create(l, Expr::createIsZero(r));
  } else {
    return SltExpr::alloc(l, r);
  }
}

static ref<Expr> SleExpr_create(const ref<Expr> &l, const ref<Expr> &r) {
  if (l->getWidth() == Expr::Bool) { // !(!l && r)
    return OrExpr::create(l, Expr::createIsZero(r));
  } else {
    return SleExpr::alloc(l, r);
  }
}

CMPCREATE_T(EqExpr, Eq, EqExpr, EqExpr_createPartial, EqExpr_createPartialR)
CMPCREATE(UltExpr, Ult)
CMPCREATE(UleExpr, Ule)
CMPCREATE(SltExpr, Slt)
CMPCREATE(SleExpr, Sle)

#define FOCMPCREATE(_e_op, _op)                                                \
  ref<Expr> _e_op::create(const ref<Expr> &l, const ref<Expr> &r) {            \
    assert(l->getWidth() == r->getWidth() && "type mismatch");                 \
    if (ConstantExpr *cl = dyn_cast<ConstantExpr>(l)) {                        \
      if (ConstantExpr *cr = dyn_cast<ConstantExpr>(r))                        \
        return cl->_op(cr);                                                    \
      if (cl->getAPFloatValue().isNaN())                                       \
        return ConstantExpr::alloc(0, Expr::Bool);                             \
    } else if (ConstantExpr *cr = dyn_cast<ConstantExpr>(r)) {                 \
      if (cr->getAPFloatValue().isNaN())                                       \
        return ConstantExpr::alloc(0, Expr::Bool);                             \
    }                                                                          \
    return _e_op::alloc(l, r);                                                 \
  }

FOCMPCREATE(FOEqExpr, FOEq)
FOCMPCREATE(FOLtExpr, FOLt)
FOCMPCREATE(FOLeExpr, FOLe)
FOCMPCREATE(FOGtExpr, FOGt)
FOCMPCREATE(FOGeExpr, FOGe)

#define FARITHCREATE(_e_op, _op)                                               \
  ref<Expr> _e_op::create(const ref<Expr> &l, const ref<Expr> &r,              \
                          llvm::APFloat::roundingMode rm) {                    \
    assert(l->getWidth() == r->getWidth() && "type mismatch");                 \
    if (ConstantExpr *cl = dyn_cast<ConstantExpr>(l))                          \
      if (ConstantExpr *cr = dyn_cast<ConstantExpr>(r))                        \
        return cl->_op(cr, rm);                                                \
    return _e_op::alloc(l, r, rm);                                             \
  }

FARITHCREATE(FAddExpr, FAdd)
FARITHCREATE(FSubExpr, FSub)
FARITHCREATE(FMulExpr, FMul)
FARITHCREATE(FDivExpr, FDiv)
FARITHCREATE(FRemExpr, FRem)
FARITHCREATE(FMaxExpr, FMax)
FARITHCREATE(FMinExpr, FMin)

ref<Expr> IsNaNExpr::create(const ref<Expr> &e) {
  if (ConstantExpr *ce = dyn_cast<ConstantExpr>(e)) {
    return ConstantExpr::alloc(ce->getAPFloatValue().isNaN(), Expr::Bool);
  }
  return IsNaNExpr::alloc(e);
}

ref<Expr> IsInfiniteExpr::create(const ref<Expr> &e) {
  if (ConstantExpr *ce = dyn_cast<ConstantExpr>(e)) {
    return ConstantExpr::alloc(ce->getAPFloatValue().isInfinity(), Expr::Bool);
  }
  return IsInfiniteExpr::alloc(e);
}

ref<Expr> IsNormalExpr::create(const ref<Expr> &e) {
  if (ConstantExpr *ce = dyn_cast<ConstantExpr>(e)) {
    return ConstantExpr::alloc(ce->getAPFloatValue().isNormal(), Expr::Bool);
  }
  return IsNormalExpr::alloc(e);
}

ref<Expr> IsSubnormalExpr::create(const ref<Expr> &e) {
  if (ConstantExpr *ce = dyn_cast<ConstantExpr>(e)) {
    return ConstantExpr::alloc(ce->getAPFloatValue().isDenormal(), Expr::Bool);
  }
  return IsSubnormalExpr::alloc(e);
}

ref<Expr> FSqrtExpr::create(ref<Expr> const &e,
                            llvm::APFloat::roundingMode rm) {
  if (ConstantExpr *ce = dyn_cast<ConstantExpr>(e)) {
    return ce->FSqrt(rm);
  }
  return FSqrtExpr::alloc(e, rm);
}

ref<Expr> FRintExpr::create(ref<Expr> const &e,
                            llvm::APFloat::roundingMode rm) {
  if (ConstantExpr *ce = dyn_cast<ConstantExpr>(e)) {
    return ce->FRint(rm);
  }
  return FRintExpr::alloc(e, rm);
}

ref<Expr> FAbsExpr::create(ref<Expr> const &e) {
  if (ConstantExpr *ce = dyn_cast<ConstantExpr>(e)) {
    return ce->FAbs();
  }
  return FAbsExpr::alloc(e);
}

ref<Expr> FNegExpr::create(ref<Expr> const &e) {
  if (ConstantExpr *ce = dyn_cast<ConstantExpr>(e)) {
    return ce->FNeg();
  }
  return FNegExpr::alloc(e);
}

ref<Expr> IsNaNExpr::either(const ref<Expr> &e0, const ref<Expr> &e1) {
  return OrExpr::create(IsNaNExpr::create(e0), IsNaNExpr::create(e1));
}

ref<Expr> IsInfiniteExpr::either(const ref<Expr> &e0, const ref<Expr> &e1) {
  return OrExpr::create(IsInfiniteExpr::create(e0), IsInfiniteExpr::create(e1));
}

ref<Expr> IsNormalExpr::either(const ref<Expr> &e0, const ref<Expr> &e1) {
  return OrExpr::create(IsNormalExpr::create(e0), IsNormalExpr::create(e1));
}

ref<Expr> IsSubnormalExpr::either(const ref<Expr> &e0, const ref<Expr> &e1) {
  return OrExpr::create(IsSubnormalExpr::create(e0),
                        IsSubnormalExpr::create(e1));
}
