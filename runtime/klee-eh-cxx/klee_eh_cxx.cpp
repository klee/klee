//===-- klee_eh_cxx.cpp----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <typeinfo>

#include <unwind.h>

#include <klee/klee.h>

// from libcxxabi
#ifdef __has_include
#  if __has_include(<cxa_exception.h>)
#    include <cxa_exception.h> // renamed with LLVM 10
#  elif __has_include(<cxa_exception.hpp>)
#    include <cxa_exception.hpp>
#  else
#    error "missing 'cxa_exception' header from libcxxabi"
#  endif
#else
#  include <cxa_exception.hpp> // assume old name
#endif
#include <private_typeinfo.h>

// implemented in the SpecialFunctionHandler
extern "C" std::uint32_t klee_eh_typeid_for(const void *);

// we use some internals of libcxxabi, so we make our life easier here
using __shim_type_info = __cxxabiv1::__shim_type_info;
using __cxa_exception = __cxxabiv1::__cxa_exception;

namespace {
// Used to determine whether a catch-clause can catch an exception
bool _klee_eh_can_catch(const void *catcherType,
                        const __shim_type_info *exceptionType,
                        void *&adjustedPtr) {
  return static_cast<const __shim_type_info *>(catcherType)
      ->can_catch(exceptionType, adjustedPtr);
}

// Some utility functions to convert between the different exception
// representations. These are mostly copied from libcxxabi, except
// for small adjustments so they work in our code as well.

// Copied from libcxxabi/cxa_exception.cpp (including comments)
inline
__cxa_exception*
cxa_exception_from_thrown_object(void* thrown_object)
{
    return static_cast<__cxa_exception*>(thrown_object) - 1;
}

// Copied from libcxxabi/cxa_exception.cpp (including comments)
//
//  Get the exception object from the unwind pointer.
//  Relies on the structure layout, where the unwind pointer is right in
//  front of the user's exception object
inline
__cxa_exception*
cxa_exception_from_exception_unwind_exception(_Unwind_Exception* unwind_exception)
{
  return cxa_exception_from_thrown_object(unwind_exception + 1);
}

// Copied from libcxxabi/cxa_personality.cpp (including comments)
void*
get_thrown_object_ptr(_Unwind_Exception* unwind_exception)
{
  // added for usage inside KLEE (i.e., outside of libcxxabi)
  using namespace __cxxabiv1;

  // Even for foreign exceptions, the exception object is *probably* at unwind_exception + 1
  //    Regardless, this library is prohibited from touching a foreign exception
  void* adjustedPtr = unwind_exception + 1;
  if (__getExceptionClass(unwind_exception) == kOurDependentExceptionClass)
      adjustedPtr = ((__cxa_dependent_exception*)adjustedPtr - 1)->primaryException;
  return adjustedPtr;
}
} // namespace

// Our implementation of a personality function for handling
// libcxxabi-exceptions. Follows how libcxxabi's __gxx_personality_v0 handles
// exceptions.
extern "C" std::int32_t
_klee_eh_cxx_personality(_Unwind_Exception *exceptionPointer,
                         const std::size_t num_bytes,
                         const unsigned char *lp_clauses) {
  assert(__cxxabiv1::__isOurExceptionClass(exceptionPointer) &&
         "unexpected exception class found");

  __cxa_exception *exception =
      cxa_exception_from_exception_unwind_exception(exceptionPointer);
  const __shim_type_info *exceptionType =
      static_cast<const __shim_type_info *>(exception->exceptionType);
  void *const exceptionObjectPtr = get_thrown_object_ptr(exceptionPointer);

  const unsigned char *cur_pos = lp_clauses;
  const unsigned char *lp_clauses_end = lp_clauses + num_bytes;

  while (cur_pos < lp_clauses_end) {
    const unsigned char type = *cur_pos;
    cur_pos += 1;
    if (type == 0) { // catch-clause
      const void *catcherType = *reinterpret_cast<const void *const *>(cur_pos);
      cur_pos += sizeof(const void *);
      if (catcherType == NULL) {
        // catch-all clause
        exception->adjustedPtr = exceptionObjectPtr;
        return klee_eh_typeid_for(catcherType);
      }
      void *adjustedPtr = exceptionObjectPtr;
      if (_klee_eh_can_catch(catcherType, exceptionType, adjustedPtr)) {
        exception->adjustedPtr = adjustedPtr;
        return klee_eh_typeid_for(catcherType);
      }
    } else { // filter-clause
      unsigned char num_filters = type - 1;
      while (num_filters > 0) {
        const void *catcher = *reinterpret_cast<const void *const *>(cur_pos);
        cur_pos += sizeof(const void *);
        if (catcher == NULL) {
          // catch-all filter clause, should always be entered
          exception->adjustedPtr = exceptionObjectPtr;
          return -1;
        }
        void *adjustedPtr = exceptionObjectPtr;
        if (_klee_eh_can_catch(catcher, exceptionType, adjustedPtr)) {
          exception->adjustedPtr = adjustedPtr;
          break;
        }
        num_filters -= 1;
      }
      if (num_filters == 0) {
        // exception was not one of the given ones -> filter it out
        return -1;
      }
    }
  }

  // no handling clause found
  return 0;
}

// Implemented in the SpecialFunctionHandler
extern "C" void _klee_eh_Unwind_RaiseException_impl(void *);

// Provide an implementation of the Itanium unwinding base-API
// (https://itanium-cxx-abi.github.io/cxx-abi/abi-eh.html#base-abi)
// These functions will be called by language-specific exception handling
// implementations, such as libcxxabi.
extern "C" {
// The main entry point for stack unwinding. This function performs the 2-phase
// unwinding process, calling the personality function as required. We implement
// this mostly inside KLEE as it requires direct manipulation of the state's
// stack.
_Unwind_Reason_Code
_Unwind_RaiseException(struct _Unwind_Exception *exception_object) {
  // this will either unwind the stack if a handler is found, or return if not
  _klee_eh_Unwind_RaiseException_impl(exception_object);

  // if we couldn't find a handler, return an error
  return _URC_END_OF_STACK;
}

// According to the Itanium ABI:
// "Deletes the given exception object. If a given runtime resumes normal
// execution after catching a foreign exception, it will not know how to delete
// that exception. Such an exception will be deleted by calling
// _Unwind_DeleteException. This is a convenience function that calls the
// function pointed to by the exception_cleanup field of the exception header."
void _Unwind_DeleteException(struct _Unwind_Exception *exception_object) {
  // Implemented in the same way as LLVM's libunwind version
  if (exception_object->exception_cleanup != NULL)
    exception_object->exception_cleanup(_URC_FOREIGN_EXCEPTION_CAUGHT,
                                        exception_object);
}

// At the LLVM IR level we will encounter the `resume`-Instruction instead of
// calls to this function. LLVM lowers `resume` to a call to `_Unwind_Resume`
// during later compilation stages.
void _Unwind_Resume(struct _Unwind_Exception *exception_object) {
  klee_report_error(__FILE__, __LINE__, "_Unwind_Resume should not appear",
                    "unexpected.err");
}

// The rest of these assume a binary execution environment and thus aren't
// supported. While ForcedUnwind could be called by users, it is never actually
// called inside of libcxxabi. The others are usually called by the personality
// function, but we implement that ourselves.
_Unwind_Reason_Code
_Unwind_ForcedUnwind(struct _Unwind_Exception *exception_object,
                     _Unwind_Stop_Fn stop, void *stop_parameter) {
  klee_report_error(__FILE__, __LINE__, "_Unwind_ForcedUnwind not supported",
                    "unsupported.err");
}

_Unwind_Word _Unwind_GetGR(struct _Unwind_Context *context, int index) {
  klee_report_error(__FILE__, __LINE__, "_Unwind_GetGR not supported",
                    "unsupported.err");
}

void _Unwind_SetGR(struct _Unwind_Context *context, int index,
                   _Unwind_Word new_value) {
  klee_report_error(__FILE__, __LINE__, "_Unwind_SetGR not supported",
                    "unsupported.err");
}

_Unwind_Word _Unwind_GetIP(struct _Unwind_Context *context) {
  klee_report_error(__FILE__, __LINE__, "_Unwind_GetIP not supported",
                    "unsupported.err");
}

void _Unwind_SetIP(struct _Unwind_Context *context, _Unwind_Word new_value) {
  klee_report_error(__FILE__, __LINE__, "_Unwind_SetIP not unsupported",
                    "unsupported.err");
}

void *_Unwind_GetLanguageSpecificData(struct _Unwind_Context *context) {
  klee_report_error(__FILE__, __LINE__,
                    "_Unwind_GetLanguageSpecificData not supported",
                    "unsupported.err");
}

_Unwind_Ptr _Unwind_GetRegionStart(struct _Unwind_Context *context) {
  klee_report_error(__FILE__, __LINE__, "_Unwind_GetRegionStart not supported",
                    "unsupported.err");
}
}
