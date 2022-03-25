setup_artifact_variables_sanitizer() {
  local SANITIZER_LLVM_VERSION_MAJOR="${SANITIZER_LLVM_VERSION/.*/}"
  local SANITIZER_LLVM_VERSION_MINOR="${SANITIZER_LLVM_VERSION/*./}"
  local version=""
  [[ "${SANITIZER_LLVM_VERSION_MAJOR}" -le 6 ]] && version="-${SANITIZER_LLVM_VERSION}"
  [[ "${SANITIZER_LLVM_VERSION_MAJOR}" -ge 7 ]] && version="-${SANITIZER_LLVM_VERSION_MAJOR}"

  # Only set LLVM_CONFIG if not set yet
  if [[ -z "${SANITIZER_LLVM_CONFIG-}" ]]; then
    SANITIZER_LLVM_CONFIG=$(which "llvm-config${version}")
  fi


  local bin_path=""
  bin_path="$(dirname "$(readlink -f "$(which "clang${version}")")")"

  SANITIZER_BITCODE_CC="${bin_path}/clang"
  SANITIZER_BITCODE_CXX="${bin_path}/clang++"

  if [[ -z ${SANITIZER_C_COMPILER:=} ]]; then
    SANITIZER_C_COMPILER="${SANITIZER_BITCODE_CC}"
  fi

  if [[ -z ${SANITIZER_CXX_COMPILER:=} ]]; then
    SANITIZER_CXX_COMPILER="${SANITIZER_BITCODE_CXX}"
  fi
}

# Check if the binary artifact is installed
is_installed_sanitizer() {
  if [[ -z ${SANITIZER_BUILD:-} ]]; then
   return 1
  fi
  local SANITIZER_LLVM_VERSION_MAJOR="${SANITIZER_LLVM_VERSION/.*/}"
  local SANITIZER_LLVM_VERSION_MINOR="${SANITIZER_LLVM_VERSION/*./}"
  local version=""
  [[ "${SANITIZER_LLVM_VERSION_MAJOR}" -le 6 ]] && version="${SANITIZER_LLVM_VERSION}"
  [[ "${SANITIZER_LLVM_VERSION_MAJOR}" -ge 7 ]] && version="${SANITIZER_LLVM_VERSION_MAJOR}"

  # Check if clang with the right version exists
  which "clang-${version}"
}