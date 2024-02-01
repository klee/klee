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

// NOTE: Needs to be kept in sync with compiler-rt/lib/ubsan/ubsan_handlers.cpp
// from LLVM project.

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
}

__attribute__((noreturn)) static void report_error(const char *msg,
                                                   const char *suffix) {
  klee_report_error(__FILE__, __LINE__, msg, suffix);
}

static const char *get_suffix(ErrorType ET) {
  switch (ET) {
  case ErrorType::GenericUB:
    // This ErrorType is only used in actual LLVM runtime
    // when `report_error_type` environment option is set to false.
    // It should never happen in KLEE runtime.
    return "exec.err";
  case ErrorType::NullPointerUse:
  case ErrorType::NullPointerUseWithNullability:
  case ErrorType::NullptrWithOffset:
  case ErrorType::NullptrWithNonZeroOffset:
  case ErrorType::NullptrAfterNonZeroOffset:
  case ErrorType::PointerOverflow:
  case ErrorType::MisalignedPointerUse:
  case ErrorType::AlignmentAssumption:
    return "ptr.err";
  case ErrorType::InsufficientObjectSize:
    // Convenient test has not been found in LLVM sources and therefore not been
    // added.
    return "ptr.err";
  case ErrorType::SignedIntegerOverflow:
  case ErrorType::UnsignedIntegerOverflow:
    return "overflow.err";
  case ErrorType::IntegerDivideByZero:
  case ErrorType::FloatDivideByZero:
    return "div.err";
  case ErrorType::InvalidBuiltin:
    return "invalid_builtin_use.err";
  case ErrorType::InvalidObjCCast:
    // Option `fsanitize=objc-cast` is not supported due to the requirement for
    // Darwin system.
    return "exec.err";
  case ErrorType::ImplicitUnsignedIntegerTruncation:
  case ErrorType::ImplicitSignedIntegerTruncation:
    return "implicit_truncation.err";
  case ErrorType::ImplicitIntegerSignChange:
  case ErrorType::ImplicitSignedIntegerTruncationOrSignChange:
    return "implicit_conversion.err";
  case ErrorType::InvalidShiftBase:
  case ErrorType::InvalidShiftExponent:
    return "overflow.err";
  case ErrorType::OutOfBoundsIndex:
    return "ptr.err";
  case ErrorType::UnreachableCall:
    return "unreachable_call.err";
  case ErrorType::MissingReturn:
    return "missing_return.err";
  case ErrorType::NonPositiveVLAIndex:
    return "ptr.err";
  case ErrorType::FloatCastOverflow:
    return "overflow.err";
  case ErrorType::InvalidBoolLoad:
  case ErrorType::InvalidEnumLoad:
    return "invalid_load.err";
  case ErrorType::FunctionTypeMismatch:
    // This check is unsupported
    return "exec.err";
  case ErrorType::InvalidNullReturn:
  case ErrorType::InvalidNullReturnWithNullability:
  case ErrorType::InvalidNullArgument:
  case ErrorType::InvalidNullArgumentWithNullability:
    return "nullable_attribute.err";
  case ErrorType::DynamicTypeMismatch:
  case ErrorType::CFIBadType:
    // These checks are unsupported
    return "exec.err";
  default:
    // In case something is not modelled
    return "exec.err";
  }
}
__attribute__((noreturn)) static void report_error_type(ErrorType ET) {
  report_error(ConvertTypeToString(ET), get_suffix(ET));
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
  uptr Alignment = (uptr)1 << Data->LogAlignment;
  ErrorType ET;
  if (!Pointer)
    ET = (Data->TypeCheckKind == TCK_NonnullAssign)
             ? ErrorType::NullPointerUseWithNullability
             : ErrorType::NullPointerUse;
  else if (Pointer & (Alignment - 1))
    ET = ErrorType::MisalignedPointerUse;
  else
    ET = ErrorType::InsufficientObjectSize;

  report_error_type(ET);
}

extern "C" void __ubsan_handle_type_mismatch_v1(TypeMismatchData *Data,
                                                ValueHandle Pointer) {

  handleTypeMismatchImpl(Data, Pointer);
}

extern "C" void __ubsan_handle_type_mismatch_v1_abort(TypeMismatchData *Data,
                                                      ValueHandle Pointer) {

  handleTypeMismatchImpl(Data, Pointer);
}

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
  if (Data->Type.isIntegerTy())
    report_error("integer division overflow", "overflow.err");
  else {
    ErrorType ET = ErrorType::FloatDivideByZero;
    report_error_type(ET);
  }
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
  report_error("shift out of bounds", "overflow.err");
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
  report_error("load invalid value", "invalid_load.err");
}

extern "C" void __ubsan_handle_load_invalid_value(InvalidValueData *Data,
                                                  ValueHandle Val) {
  handleLoadInvalidValue(Data, Val);
}
extern "C" void __ubsan_handle_load_invalid_value_abort(InvalidValueData *Data,
                                                        ValueHandle Val) {
  handleLoadInvalidValue(Data, Val);
}

static void handleImplicitConversion(ImplicitConversionData *Data,
                                     ValueHandle /*Src*/, ValueHandle /*Dst*/) {
  ErrorType ET = ErrorType::GenericUB;

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

static void handleNonNullReturn(NonNullReturnData * /*Data*/,
                                SourceLocation * /*LocPtr*/, bool IsAttr) {
  ErrorType ET = IsAttr ? ErrorType::InvalidNullReturn
                        : ErrorType::InvalidNullReturnWithNullability;
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

extern "C" void __ubsan_handle_nullability_return_v1(NonNullReturnData *Data,
                                                     SourceLocation *LocPtr) {
  handleNonNullReturn(Data, LocPtr, false);
}

extern "C" void
__ubsan_handle_nullability_return_v1_abort(NonNullReturnData *Data,
                                           SourceLocation *LocPtr) {
  handleNonNullReturn(Data, LocPtr, false);
}

static void handleNonNullArg(NonNullArgData * /*Data*/, bool IsAttr) {
  ErrorType ET = IsAttr ? ErrorType::InvalidNullArgument
                        : ErrorType::InvalidNullArgumentWithNullability;
  report_error_type(ET);
}

extern "C" void __ubsan_handle_nonnull_arg(NonNullArgData *Data) {
  handleNonNullArg(Data, true);
}

extern "C" void __ubsan_handle_nonnull_arg_abort(NonNullArgData *Data) {
  handleNonNullArg(Data, true);
}

extern "C" void __ubsan_handle_nullability_arg(NonNullArgData *Data) {
  handleNonNullArg(Data, false);
}

extern "C" void __ubsan_handle_nullability_arg_abort(NonNullArgData *Data) {

  handleNonNullArg(Data, false);
}

static void handlePointerOverflowImpl(PointerOverflowData * /*Data*/,
                                      ValueHandle Base, ValueHandle Result) {
  ErrorType ET;
  if (Base == 0 && Result == 0)
    ET = ErrorType::NullptrWithOffset;
  else if (Base == 0 && Result != 0)
    ET = ErrorType::NullptrWithNonZeroOffset;
  else if (Base != 0 && Result == 0)
    ET = ErrorType::NullptrAfterNonZeroOffset;
  else
    ET = ErrorType::PointerOverflow;
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

} // namespace __ubsan