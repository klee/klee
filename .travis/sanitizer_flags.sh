# This file is meant to be included by shell scripts
# that need to do a sanitized build.

# Users of this script can use this variable
# to detect if a Sanitized build was enabled.
IS_SANITIZED_BUILD=0

# AddressSanitizer
if [ "X${ASAN_BUILD}" == "X1" ]; then
  echo "Using ASan"
  ASAN_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
  ASAN_C_FLAGS="${ASAN_CXX_FLAGS}"
  ASAN_LD_FLAGS="${ASAN_CXX_FLAGS}"
  IS_SANITIZED_BUILD=1
else
  echo "Not using ASan"
  ASAN_CXX_FLAGS=""
  ASAN_C_FLAGS=""
  ASAN_LD_FLAGS=""
fi

# Undefined Behaviour Sanitizer
if [ "X${UBSAN_BUILD}" == "X1" ]; then
  echo "Using UBSan"
  UBSAN_CXX_FLAGS="-fsanitize=undefined"
  UBSAN_C_FLAGS="${UBSAN_CXX_FLAGS}"
  UBSAN_LD_FLAGS="${UBSAN_CXX_FLAGS}"
  IS_SANITIZED_BUILD=1
else
  echo "Not using UBSan"
  UBSAN_CXX_FLAGS=""
  UBSAN_C_FLAGS=""
  UBSAN_LD_FLAGS=""
fi

# Set variables to be used by clients.
SANITIZER_CXX_FLAGS="${ASAN_CXX_FLAGS} ${UBSAN_CXX_FLAGS}"
SANITIZER_C_FLAGS="${ASAN_C_FLAGS} ${UBSAN_C_FLAGS}"
SANITIZER_LD_FLAGS="${ASAN_LD_FLAGS} ${UBSAN_LD_FLAGS}"
