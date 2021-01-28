//===-- ubsan_handlers.cpp ------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Error logging entry points for the UBSan runtime.
//
//===----------------------------------------------------------------------===//

#include "ubsan_handlers.h"
#include "ubsan_diag.h"

extern "C" __attribute__((noreturn)) void klee_report_error(const char *file,
                                                            int line,
                                                            const char *message,
                                                            const char *suffix);

namespace __ubsan {
static const char *ConvertTypeToString(ErrorType Type) {
  switch (Type) {
#define UBSAN_CHECK(Name, SummaryKind, FSanitizeFlagName)                      \
  case ErrorType::Name:                                                        \
    return SummaryKind;
#include "ubsan_checks.inc"
#undef UBSAN_CHECK
  }
  //  UNREACHABLE("unknown ErrorType!");
}

__attribute__((noreturn)) static void
report_error(const char *msg, const char *suffix = "undefined_behavior.err") {
  klee_report_error(__FILE__, __LINE__, msg, suffix);
}

__attribute__((noreturn)) static void report_error_type(ErrorType ET) {
  report_error(ConvertTypeToString(ET));
}

/// Situations in which we might emit a check for the suitability of a
/// pointer or glvalue. Needs to be kept in sync with CodeGenFunction.h in
/// clang.
enum TypeCheckKind {
  /// Checking the operand of a load. Must be suitably sized and aligned.
  TCK_Load,
  /// Checking the destination of a store. Must be suitably sized and aligned.
  TCK_Store,
  /// Checking the bound value in a reference binding. Must be suitably sized
  /// and aligned, but is not required to refer to an object (until the
  /// reference is used), per core issue 453.
  TCK_ReferenceBinding,
  /// Checking the object expression in a non-static data member access. Must
  /// be an object within its lifetime.
  TCK_MemberAccess,
  /// Checking the 'this' pointer for a call to a non-static member function.
  /// Must be an object within its lifetime.
  TCK_MemberCall,
  /// Checking the 'this' pointer for a constructor call.
  TCK_ConstructorCall,
  /// Checking the operand of a static_cast to a derived pointer type. Must be
  /// null or an object within its lifetime.
  TCK_DowncastPointer,
  /// Checking the operand of a static_cast to a derived reference type. Must
  /// be an object within its lifetime.
  TCK_DowncastReference,
  /// Checking the operand of a cast to a base object. Must be suitably sized
  /// and aligned.
  TCK_Upcast,
  /// Checking the operand of a cast to a virtual base object. Must be an
  /// object within its lifetime.
  TCK_UpcastToVirtualBase,
  /// Checking the value assigned to a _Nonnull pointer. Must not be null.
  TCK_NonnullAssign,
  /// Checking the operand of a dynamic_cast or a typeid expression.  Must be
  /// null or an object within its lifetime.
  TCK_DynamicOperation
};

static void handleTypeMismatchImpl(TypeMismatchData *Data,
                                   ValueHandle Pointer) {
#if LLVM_VERSION_MAJOR >= 4
  uptr Alignment = (uptr)1 << Data->LogAlignment;
#endif
  ErrorType ET;
  if (!Pointer)
#if LLVM_VERSION_MAJOR >= 11
    ET = (Data->TypeCheckKind == TCK_NonnullAssign)
             ? ErrorType::NullPointerUseWithNullability
             : ErrorType::NullPointerUse;
#else
    ET = ErrorType::NullPointerUse;
#endif
#if LLVM_VERSION_MAJOR >= 4
  else if (Pointer & (Alignment - 1))
#else
  else if (Data->Alignment && (Pointer & (Data->Alignment - 1)))
#endif
    ET = ErrorType::MisalignedPointerUse;
  else
    ET = ErrorType::InsufficientObjectSize;

  report_error_type(ET);
}

#if LLVM_VERSION_MAJOR >= 4
extern "C" void __ubsan_handle_type_mismatch_v1(TypeMismatchData *Data,
                                                ValueHandle Pointer) {

  handleTypeMismatchImpl(Data, Pointer);
}

extern "C" void __ubsan_handle_type_mismatch_v1_abort(TypeMismatchData *Data,
                                                      ValueHandle Pointer) {

  handleTypeMismatchImpl(Data, Pointer);
}
#else
extern "C" void __ubsan_handle_type_mismatch(TypeMismatchData *Data,
                                             ValueHandle Pointer) {
  handleTypeMismatchImpl(Data, Pointer);
}

extern "C" void __ubsan_handle_type_mismatch_abort(TypeMismatchData *Data,
                                                   ValueHandle Pointer) {
  handleTypeMismatchImpl(Data, Pointer);
}
#endif

#if LLVM_VERSION_MAJOR >= 8
static void handleAlignmentAssumptionImpl(AlignmentAssumptionData * /*Data*/,
                                          ValueHandle /*Pointer*/,
                                          ValueHandle /*Alignment*/,
                                          ValueHandle /*Offset*/) {
  ErrorType ET = ErrorType::AlignmentAssumption;
  report_error_type(ET);
}

extern "C" void
__ubsan_handle_alignment_assumption(AlignmentAssumptionData *Data,
                                    ValueHandle Pointer, ValueHandle Alignment,
                                    ValueHandle Offset) {
  handleAlignmentAssumptionImpl(Data, Pointer, Alignment, Offset);
}

extern "C" void __ubsan_handle_alignment_assumption_abort(
    AlignmentAssumptionData *Data, ValueHandle Pointer, ValueHandle Alignment,
    ValueHandle Offset) {
  handleAlignmentAssumptionImpl(Data, Pointer, Alignment, Offset);
}
#endif

/// \brief Common diagnostic emission for various forms of integer overflow.
static void handleIntegerOverflowImpl(OverflowData *Data, ValueHandle /*LHS*/,
                                      const char * /*Operator*/) {
  bool IsSigned = Data->Type.isSignedIntegerTy();
  ErrorType ET = IsSigned ? ErrorType::SignedIntegerOverflow
                          : ErrorType::UnsignedIntegerOverflow;
  report_error_type(ET);
}

#define UBSAN_OVERFLOW_HANDLER(handler_name, op, unrecoverable)                \
  extern "C" void handler_name(OverflowData *Data, ValueHandle LHS,            \
                               ValueHandle RHS) {                              \
    handleIntegerOverflowImpl(Data, LHS, op);                                  \
  }

UBSAN_OVERFLOW_HANDLER(__ubsan_handle_add_overflow, "+", false)
UBSAN_OVERFLOW_HANDLER(__ubsan_handle_add_overflow_abort, "+", true)
UBSAN_OVERFLOW_HANDLER(__ubsan_handle_sub_overflow, "-", false)
UBSAN_OVERFLOW_HANDLER(__ubsan_handle_sub_overflow_abort, "-", true)
UBSAN_OVERFLOW_HANDLER(__ubsan_handle_mul_overflow, "*", false)
UBSAN_OVERFLOW_HANDLER(__ubsan_handle_mul_overflow_abort, "*", true)

static void handleNegateOverflowImpl(OverflowData *Data,
                                     ValueHandle /*OldVal*/) {
  bool IsSigned = Data->Type.isSignedIntegerTy();
  ErrorType ET = IsSigned ? ErrorType::SignedIntegerOverflow
                          : ErrorType::UnsignedIntegerOverflow;
  report_error_type(ET);
}

extern "C" void __ubsan_handle_negate_overflow(OverflowData *Data,
                                               ValueHandle OldVal) {
  handleNegateOverflowImpl(Data, OldVal);
}

extern "C" void __ubsan_handle_negate_overflow_abort(OverflowData *Data,
                                                     ValueHandle OldVal) {
  handleNegateOverflowImpl(Data, OldVal);
}

static void handleDivremOverflowImpl(OverflowData *Data, ValueHandle /*LHS*/,
                                     ValueHandle /*RHS*/) {
  ErrorType ET;
  if (Data->Type.isIntegerTy())
    report_error("integer division overflow");
  else
    ET = ErrorType::FloatDivideByZero;
  report_error_type(ET);
}

extern "C" void __ubsan_handle_divrem_overflow(OverflowData *Data,
                                               ValueHandle LHS,
                                               ValueHandle RHS) {
  handleDivremOverflowImpl(Data, LHS, RHS);
}

extern "C" void __ubsan_handle_divrem_overflow_abort(OverflowData *Data,
                                                     ValueHandle LHS,
                                                     ValueHandle RHS) {
  handleDivremOverflowImpl(Data, LHS, RHS);
}

static void handleShiftOutOfBoundsImpl(ShiftOutOfBoundsData * /*Data*/,
                                       ValueHandle /*LHS*/,
                                       ValueHandle /*RHS*/) {
  report_error("shift out of bounds");
}

extern "C" void __ubsan_handle_shift_out_of_bounds(ShiftOutOfBoundsData *Data,
                                                   ValueHandle LHS,
                                                   ValueHandle RHS) {
  handleShiftOutOfBoundsImpl(Data, LHS, RHS);
}

extern "C" void
__ubsan_handle_shift_out_of_bounds_abort(ShiftOutOfBoundsData *Data,
                                         ValueHandle LHS, ValueHandle RHS) {
  handleShiftOutOfBoundsImpl(Data, LHS, RHS);
}

static void handleOutOfBoundsImpl(OutOfBoundsData * /*Data*/,
                                  ValueHandle /*Index*/) {
  ErrorType ET = ErrorType::OutOfBoundsIndex;
  report_error_type(ET);
}

extern "C" void __ubsan_handle_out_of_bounds(OutOfBoundsData *Data,
                                             ValueHandle Index) {
  handleOutOfBoundsImpl(Data, Index);
}

extern "C" void __ubsan_handle_out_of_bounds_abort(OutOfBoundsData *Data,
                                                   ValueHandle Index) {
  handleOutOfBoundsImpl(Data, Index);
}

static void handleBuiltinUnreachableImpl(UnreachableData * /*Data*/) {
  ErrorType ET = ErrorType::UnreachableCall;
  report_error_type(ET);
}

extern "C" void __ubsan_handle_builtin_unreachable(UnreachableData *Data) {
  handleBuiltinUnreachableImpl(Data);
}

static void handleMissingReturnImpl(UnreachableData * /*Data*/) {
  ErrorType ET = ErrorType::MissingReturn;
  report_error_type(ET);
}

extern "C" void __ubsan_handle_missing_return(UnreachableData *Data) {
  handleMissingReturnImpl(Data);
}

static void handleVLABoundNotPositive(VLABoundData * /*Data*/,
                                      ValueHandle /*Bound*/) {
  ErrorType ET = ErrorType::NonPositiveVLAIndex;
  report_error_type(ET);
}

extern "C" void __ubsan_handle_vla_bound_not_positive(VLABoundData *Data,
                                                      ValueHandle Bound) {
  handleVLABoundNotPositive(Data, Bound);
}

extern "C" void __ubsan_handle_vla_bound_not_positive_abort(VLABoundData *Data,
                                                            ValueHandle Bound) {
  handleVLABoundNotPositive(Data, Bound);
}

static void handleFloatCastOverflow(void * /*DataPtr*/, ValueHandle /*From*/) {
  ErrorType ET = ErrorType::FloatCastOverflow;
  report_error_type(ET);
}

extern "C" void __ubsan_handle_float_cast_overflow(void *Data,
                                                   ValueHandle From) {
  handleFloatCastOverflow(Data, From);
}

extern "C" void __ubsan_handle_float_cast_overflow_abort(void *Data,
                                                         ValueHandle From) {
  handleFloatCastOverflow(Data, From);
}

static void handleLoadInvalidValue(InvalidValueData * /*Data*/,
                                   ValueHandle /*Val*/) {
  report_error("load invalid value");
}

extern "C" void __ubsan_handle_load_invalid_value(InvalidValueData *Data,
                                                  ValueHandle Val) {
  handleLoadInvalidValue(Data, Val);
}
extern "C" void __ubsan_handle_load_invalid_value_abort(InvalidValueData *Data,
                                                        ValueHandle Val) {
  handleLoadInvalidValue(Data, Val);
}

#if LLVM_VERSION_MAJOR >= 7
static void handleImplicitConversion(ImplicitConversionData *Data,
                                     ValueHandle /*Src*/, ValueHandle /*Dst*/) {
  ErrorType ET = ErrorType::GenericUB;

#if LLVM_VERSION_MAJOR >= 8
  const TypeDescriptor &SrcTy = Data->FromType;
  const TypeDescriptor &DstTy = Data->ToType;

  bool SrcSigned = SrcTy.isSignedIntegerTy();
  bool DstSigned = DstTy.isSignedIntegerTy();

  switch (Data->Kind) {
  case ICCK_IntegerTruncation: { // Legacy, no longer used.
    // Let's figure out what it should be as per the new types, and upgrade.
    // If both types are unsigned, then it's an unsigned truncation.
    // Else, it is a signed truncation.
    if (!SrcSigned && !DstSigned) {
      ET = ErrorType::ImplicitUnsignedIntegerTruncation;
    } else {
      ET = ErrorType::ImplicitSignedIntegerTruncation;
    }
    break;
  }
  case ICCK_UnsignedIntegerTruncation:
    ET = ErrorType::ImplicitUnsignedIntegerTruncation;
    break;
  case ICCK_SignedIntegerTruncation:
    ET = ErrorType::ImplicitSignedIntegerTruncation;
    break;
  case ICCK_IntegerSignChange:
    ET = ErrorType::ImplicitIntegerSignChange;
    break;
  case ICCK_SignedIntegerTruncationOrSignChange:
    ET = ErrorType::ImplicitSignedIntegerTruncationOrSignChange;
    break;
  }
#else
  switch (Data->Kind) {
  case ICCK_IntegerTruncation:
    ET = ErrorType::ImplicitIntegerTruncation;
    break;
  }
#endif
  report_error_type(ET);
}

extern "C" void __ubsan_handle_implicit_conversion(ImplicitConversionData *Data,
                                                   ValueHandle Src,
                                                   ValueHandle Dst) {
  handleImplicitConversion(Data, Src, Dst);
}

extern "C" void
__ubsan_handle_implicit_conversion_abort(ImplicitConversionData *Data,
                                         ValueHandle Src, ValueHandle Dst) {
  handleImplicitConversion(Data, Src, Dst);
}
#endif

#if LLVM_VERSION_MAJOR >= 6
static void handleInvalidBuiltin(InvalidBuiltinData * /*Data*/) {
  ErrorType ET = ErrorType::InvalidBuiltin;
  report_error_type(ET);
}

extern "C" void __ubsan_handle_invalid_builtin(InvalidBuiltinData *Data) {
  handleInvalidBuiltin(Data);
}

extern "C" void __ubsan_handle_invalid_builtin_abort(InvalidBuiltinData *Data) {
  handleInvalidBuiltin(Data);
}
#endif

#if LLVM_VERSION_MAJOR >= 5
static void handleNonNullReturn(NonNullReturnData * /*Data*/,
                                SourceLocation * /*LocPtr*/, bool IsAttr) {
#if LLVM_VERSION_MAJOR >= 11
  ErrorType ET = IsAttr ? ErrorType::InvalidNullReturn
                        : ErrorType::InvalidNullReturnWithNullability;
#else
  ErrorType ET = ErrorType::InvalidNullReturn;
#endif
  report_error_type(ET);
}

extern "C" void __ubsan_handle_nonnull_return_v1(NonNullReturnData *Data,
                                                 SourceLocation *LocPtr) {
  handleNonNullReturn(Data, LocPtr, true);
}

extern "C" void __ubsan_handle_nonnull_return_v1_abort(NonNullReturnData *Data,
                                                       SourceLocation *LocPtr) {
  handleNonNullReturn(Data, LocPtr, true);
}
#else
static void handleNonNullReturn(NonNullReturnData * /*Data*/) {
  ErrorType ET = ErrorType::InvalidNullReturn;
  report_error_type(ET);
}

extern "C" void __ubsan_handle_nonnull_return(NonNullReturnData *Data) {
  handleNonNullReturn(Data);
}

extern "C" void __ubsan_handle_nonnull_return_abort(NonNullReturnData *Data) {
  handleNonNullReturn(Data);
}
#endif

#if LLVM_VERSION_MAJOR >= 5
extern "C" void __ubsan_handle_nullability_return_v1(NonNullReturnData *Data,
                                                     SourceLocation *LocPtr) {
  handleNonNullReturn(Data, LocPtr, false);
}

extern "C" void
__ubsan_handle_nullability_return_v1_abort(NonNullReturnData *Data,
                                           SourceLocation *LocPtr) {
  handleNonNullReturn(Data, LocPtr, false);
}
#endif

static void handleNonNullArg(NonNullArgData * /*Data*/, bool IsAttr) {
#if LLVM_VERSION_MAJOR >= 11
  ErrorType ET = IsAttr ? ErrorType::InvalidNullArgument
                        : ErrorType::InvalidNullArgumentWithNullability;
#else
  ErrorType ET = ErrorType::InvalidNullArgument;
#endif
  report_error_type(ET);
}

extern "C" void __ubsan_handle_nonnull_arg(NonNullArgData *Data) {
  handleNonNullArg(Data, true);
}

extern "C" void __ubsan_handle_nonnull_arg_abort(NonNullArgData *Data) {
  handleNonNullArg(Data, true);
}

#if LLVM_VERSION_MAJOR >= 5
extern "C" void __ubsan_handle_nullability_arg(NonNullArgData *Data) {
  handleNonNullArg(Data, false);
}

extern "C" void __ubsan_handle_nullability_arg_abort(NonNullArgData *Data) {

  handleNonNullArg(Data, false);
}
#endif

#if LLVM_VERSION_MAJOR >= 5
static void handlePointerOverflowImpl(PointerOverflowData * /*Data*/,
                                      ValueHandle Base, ValueHandle Result) {
#if LLVM_VERSION_MAJOR >= 10
  ErrorType ET;
  if (Base == 0 && Result == 0)
    ET = ErrorType::NullptrWithOffset;
  else if (Base == 0 && Result != 0)
    ET = ErrorType::NullptrWithNonZeroOffset;
  else if (Base != 0 && Result == 0)
    ET = ErrorType::NullptrAfterNonZeroOffset;
  else
    ET = ErrorType::PointerOverflow;
#else
  ErrorType ET = ErrorType::PointerOverflow;
#endif
  report_error_type(ET);
}

extern "C" void __ubsan_handle_pointer_overflow(PointerOverflowData *Data,
                                                ValueHandle Base,
                                                ValueHandle Result) {

  handlePointerOverflowImpl(Data, Base, Result);
}

extern "C" void __ubsan_handle_pointer_overflow_abort(PointerOverflowData *Data,
                                                      ValueHandle Base,
                                                      ValueHandle Result) {

  handlePointerOverflowImpl(Data, Base, Result);
}
#endif

} // namespace __ubsan