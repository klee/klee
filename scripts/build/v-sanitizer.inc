#!/bin/bash
# Variables for sanitizer handling

required_variables_sanitizer=(
  "SANITIZER_BUILD"
)

artifact_dependency_sanitizer=("")

required_variables_check_sanitizer() {
  # Allowe empty sanitizer builds
  [[ -z "${SANITIZER_BUILD}" ]] && return 0
  
  local sanitizers
  IFS=":" read -r -a sanitizers <<< "${SANITIZER_BUILD}"
  for sanitizer in "${sanitizers[@]}"; do
    [[ -z "${sanitizer}" ]] && continue
    [[ "${sanitizer}" == "address" ]] && continue
    [[ "${sanitizer}" == "memory" ]] && continue
    [[ "${sanitizer}" == "undefined" ]] && continue
    
    echo "Unknown sanitizer: $sanitizer"
    exit 1
  done
}

setup_variables_sanitizer() {
  if [[ -z "${SANITIZER_BUILD}" ]]; then
    SANITIZERS=("")
    SANITIZER_SUFFIX=""
    return 0
  fi

  [[ -n ${SANITIZER_INIT+X} ]] && return 0
  SANITIZER_INIT=1

  # Remove ":" separator
  local sanitizers
  local e
  local sorted_sanitizers

  IFS=":" read -r -a sanitizers <<< "${SANITIZER_BUILD}"
  sorted_sanitizers=( $( for e in "${sanitizers[@]}"; do echo "$e"; done | sort) )
  SANITIZERS=("${sorted_sanitizers[@]}")


  # Set variables used by clients.
  ## Build variables
  [[ -z ${SANITIZER_CXX_FLAGS+x} ]] && SANITIZER_CXX_FLAGS=("")
  [[ -z ${SANITIZER_C_FLAGS+x} ]] && SANITIZER_C_FLAGS=("")
  [[ -z ${SANITIZER_LD_FLAGS+x} ]] && SANITIZER_LD_FLAGS=("")

  # Docker_suffix
  SANITIZER_SUFFIX=""

  local flags
  for sanitizer in "${sorted_sanitizers[@]}"; do
    [[ -z "${sanitizer}" ]] && continue
    # AddressSanitizer
    if [ "${sanitizer}" == "address" ]; then
      echo "Using ASan"
      flags=( "-fsanitize=address" "-fno-omit-frame-pointer" "-g")
      SANITIZER_CXX_FLAGS+=("${flags[@]}")
      SANITIZER_C_FLAGS+=("${flags[@]}")
      SANITIZER_LD_FLAGS+=("${flags[@]}")
      SANITIZER_SUFFIX+="_asan"
      continue
    fi

    # Undefined Behaviour Sanitizer
    if [ "${sanitizer}" == "undefined" ]; then
      echo "Using UBSan"
      flags=("-fsanitize=undefined" "-fno-omit-frame-pointer" "-g" "-fsanitize=integer")
      SANITIZER_CXX_FLAGS+=("${flags[@]}")
      SANITIZER_C_FLAGS+=("${flags[@]}")
      SANITIZER_LD_FLAGS+=("${flags[@]}" "-lubsan" "-fuse-ld=gold")
      SANITIZER_SUFFIX+="_ubsan"
      continue
    fi

    # Memory Sanitizer
    if [ "${sanitizer}" == "memory" ]; then
      echo "Using Memory Sanitizer"
      SANITIZER_SUFFIX+="_memsan"
      continue
    fi

    echo "Unknown sanitizer: $sanitizer"
    exit 1
  done
  }
