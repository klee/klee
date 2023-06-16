/*===-- klee.h --------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#ifndef KLEE_H
#define KLEE_H

#include "stddef.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Add an accesible memory object at a user specified location. It
 * is the users responsibility to make sure that these memory
 * objects do not overlap. These memory objects will also
 * (obviously) not correctly interact with external function
 * calls.
 */
void klee_define_fixed_object(void *addr, size_t nbytes);

/* klee_make_symbolic - Make the contents of the object pointer to by \arg
 * addr symbolic.
 *
 * \arg addr - The start of the object.
 * \arg nbytes - The number of bytes to make symbolic; currently this *must*
 * be the entire contents of the object.
 * \arg name - A name used for identifying the object in messages, output
 * files, etc. If NULL, object is called "unnamed".
 */
void klee_make_symbolic(void *addr, size_t nbytes, const char *name);

/* klee_range - Construct a symbolic value in the signed interval
 * [begin,end).
 *
 * \arg name - A name used for identifying the object in messages, output
 * files, etc. If NULL, object is called "unnamed".
 */
int klee_range(int begin, int end, const char *name);

/*  klee_int - Construct an unconstrained symbolic integer.
 *
 * \arg name - An optional name, used for identifying the object in messages,
 * output files, etc.
 */
int klee_int(const char *name);

/* klee_silent_exit - Terminate the current KLEE process without generating a
 * test file.
 */
__attribute__((noreturn)) void klee_silent_exit(int status);

/* klee_abort - Abort the current KLEE process. */
__attribute__((noreturn)) void klee_abort(void);

/* klee_report_error - Report a user defined error and terminate the current
 * KLEE process.
 *
 * \arg file - The filename to report in the error message.
 * \arg line - The line number to report in the error message.
 * \arg message - A string to include in the error message.
 * \arg suffix - The suffix to use for error files.
 */
__attribute__((noreturn)) void klee_report_error(const char *file, int line,
                                                 const char *message,
                                                 const char *suffix);

/* called by checking code to get size of memory. */
size_t klee_get_obj_size(void *ptr);

/* print the tree associated w/ a given expression. */
void klee_print_expr(const char *msg, ...);

/* NB: this *does not* fork n times and return [0,n) in children.
 * It makes n be symbolic and returns: caller must compare N times.
 */
uintptr_t klee_choose(uintptr_t n);

/* special klee assert macro. this assert should be used when path consistency
 * across platforms is desired (e.g., in tests).
 * NB: __assert_fail is a klee "special" function
 */
#define klee_assert(expr)                                                      \
  ((expr) ? (void)(0)                                                          \
          : __assert_fail(#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__))

/* Return true if the given value is symbolic (represented by an
 * expression) in the current state. This is primarily for debugging
 * and writing tests but can also be used to enable prints in replay
 * mode.
 */
unsigned klee_is_symbolic(uintptr_t n);

/* Return true if replaying a concrete test case using the libkleeRuntime
 * library Return false if executing symbolically in KLEE.
 */
unsigned klee_is_replay(void);

/* The following intrinsics are primarily intended for internal use
   and may have peculiar semantics. */

void klee_assume(uintptr_t condition);
void klee_warning(const char *message);
void klee_warning_once(const char *message);
void klee_prefer_cex(void *object, uintptr_t condition);
void klee_posix_prefer_cex(void *object, uintptr_t condition);
void klee_mark_global(void *object);

/* Return a possible constant value for the input expression. This
   allows programs to forcibly concretize values on their own. */
#define KLEE_GET_VALUE_PROTO(suffix, type)                                     \
  type klee_get_value##suffix(type expr)

KLEE_GET_VALUE_PROTO(f, float);
KLEE_GET_VALUE_PROTO(d, double);
KLEE_GET_VALUE_PROTO(l, long);
KLEE_GET_VALUE_PROTO(ll, long long);
KLEE_GET_VALUE_PROTO(_i32, int32_t);
KLEE_GET_VALUE_PROTO(_i64, int64_t);

#undef KLEE_GET_VALUE_PROTO

/* Ensure that memory in the range [address, address+size) is
   accessible to the program. If some byte in the range is not
   accessible an error will be generated and the state
   terminated.

   The current implementation requires both address and size to be
   constants and that the range lie within a single object. */
void klee_check_memory_access(const void *address, size_t size);

/* Enable/disable forking. */
void klee_set_forking(unsigned enable);

/* Print stack trace. */
void klee_stack_trace(void);

/* Print range for given argument and tagged with name */
void klee_print_range(const char *name, int arg);

/* Get errno value of the current state */
int klee_get_errno(void);

/* KLEE float intrinsics */
#ifdef __cplusplus
#define KLEE_BOOL_TYPE bool
#else
#define KLEE_BOOL_TYPE _Bool
#endif
KLEE_BOOL_TYPE klee_is_nan_float(float f);
KLEE_BOOL_TYPE klee_is_nan_double(double d);
KLEE_BOOL_TYPE klee_is_infinite_float(float f);
KLEE_BOOL_TYPE klee_is_infinite_double(double d);
KLEE_BOOL_TYPE klee_is_normal_float(float f);
KLEE_BOOL_TYPE klee_is_normal_double(double d);
KLEE_BOOL_TYPE klee_is_subnormal_float(float f);
KLEE_BOOL_TYPE klee_is_subnormal_double(double d);
#if defined(__x86_64__) || defined(__i386__)
KLEE_BOOL_TYPE klee_is_nan_long_double(long double f);
KLEE_BOOL_TYPE klee_is_infinite_long_double(long double d);
KLEE_BOOL_TYPE klee_is_normal_long_double(long double d);
KLEE_BOOL_TYPE klee_is_subnormal_long_double(long double f);
#endif

#undef KLEE_BOOL_TYPE

enum KleeRoundingMode {
  KLEE_FP_RNE = 0, // Round to nearest ties to even.
  KLEE_FP_RNA,
  KLEE_FP_RU,
  KLEE_FP_RD,
  KLEE_FP_RZ,
  KLEE_FP_UNKNOWN
};

void klee_set_rounding_mode(enum KleeRoundingMode rm);
enum KleeRoundingMode klee_get_rounding_mode();

float klee_rintf(float f);
double klee_rint(double d);
float klee_sqrt_float(float f);
double klee_sqrt_double(double d);
float klee_abs_float(float);
double klee_abs_double(double);
#if defined(__x86_64__) || defined(__i386__)
long double klee_sqrt_long_double(long double d);
long double klee_abs_long_double(long double);
long double klee_rintl(long double d);
#endif
#ifdef __cplusplus
}
#endif

// UTBot uses these functions to wrap user functions which work with
// stdin/stdout
void klee_init_env(int *argcPtr, char ***argvPtr);
void check_stdin_read();
void *__klee_wrapped_malloc(size_t size);

#endif /* KLEE_H */
