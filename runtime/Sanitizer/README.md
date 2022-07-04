# KLEE UBSan runtime

## Introduction

KLEE UBSan runtime is a tailored runtime
of [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) to meet KLEE needs. For
certain reasons, not all [checks](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html#available-checks) and
diagnostics are supported in there.

## Usage

Use `clang++` to compile and link your program with sanitizer flags. Make sure to use `clang++` (not `ld`) as a
linker, so that your executable is linked with proper UBSan runtime libraries. You can use `clang` instead of `clang++`
if you’re compiling/linking C code.

For compiling/linking with **all** available checks to catch **both** undefined behaviour and unintentional issues, just
add one of the following options:

* Short exclusive form
    * `-fsanitize=undefined,float-divide-by-zero,unsigned-integer-overflow,implicit-conversion,nullability -fno-sanitize=local-bounds,function,vptr`
* Verbose inclusive form
    * LLVM 11 and lower
        * `-fsanitize=alignment,bool,builtin,array-bounds,enum,float-cast-overflow,float-divide-by-zero,implicit-unsigned-integer-truncation,implicit-signed-integer-truncation,implicit-integer-sign-change,integer-divide-by-zero,nonnull-attribute,null,nullability-arg,nullability-assign,nullability-return,object-size,pointer-overflow,return,returns-nonnull-attribute,shift,signed-integer-overflow,unreachable,unsigned-integer-overflow,vla-bound`
    * LLVM 12 and higher
        * `-fsanitize=alignment,bool,builtin,array-bounds,enum,float-cast-overflow,float-divide-by-zero,implicit-unsigned-integer-truncation,implicit-signed-integer-truncation,implicit-integer-sign-change,integer-divide-by-zero,nonnull-attribute,null,nullability-arg,nullability-assign,nullability-return,object-size,pointer-overflow,return,returns-nonnull-attribute,shift,unsigned-shift-base,signed-integer-overflow,unreachable,unsigned-integer-overflow,vla-bound`

For compiling/linking with **all** available checks to catch **only** undefined behaviour, just add
the following option:

* `-fsanitize=undefined -fno-sanitize=local-bounds,function,vptr`

## Available checks

Available checks for KLEE as are:

* `-fsanitize=alignment`
* `-fsanitize=bool`
* `-fsanitize=builtin`
* `-fsanitize=array-bounds`
* `-fsanitize=enum`
* `-fsanitize=float-cast-overflow`
* `-fsanitize=float-divide-by-zero`
* `-fsanitize=implicit-unsigned-integer-truncation`
* `-fsanitize=implicit-signed-integer-truncation`
* `-fsanitize=implicit-integer-sign-change`
* `-fsanitize=integer-divide-by-zero`
* `-fsanitize=nonnull-attribute`
* `-fsanitize=null`
* `-fsanitize=nullability-arg`
* `-fsanitize=nullability-assign`
* `-fsanitize=nullability-return`
* `-fsanitize=object-size`
* `-fsanitize=pointer-overflow`
* `-fsanitize=return`
* `-fsanitize=returns-nonnull-attribute`
* `-fsanitize=shift`
* `-fsanitize=unsigned-shift-base`
* `-fsanitize=signed-integer-overflow`
* `-fsanitize=unreachable`
* `-fsanitize=unsigned-integer-overflow`
* `-fsanitize=vla-bound`

## Unavailable checks

Also, note some unavailable checks as are:

* `-fsanitize=local-bounds`
* `-fsanitize=function`
* `-fsanitize=objc-cast`
* `-fsanitize=vptr`

## Additional Configuration

Additional configuration via UBSan options and environment variables may not work as expected, so use with care.