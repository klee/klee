setup_artifact_variables_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local version="-${LLVM_VERSION_MAJOR}"

  # Only set LLVM_CONFIG if not set yet
  if [[ -z "${LLVM_CONFIG:-}" ]]; then
    LLVM_CONFIG=$(which "llvm-config${version}")
  fi

  local bin_path=""
  bin_path=$(which "clang${version}")
  [[ -z "${bin_path}" ]] && return 1

  bin_path="$(dirname "$(readlink -f "${bin_path}")")"
  [[ -z "${bin_path}" ]] && return 1

  BITCODE_CC="${bin_path}/clang"
  BITCODE_CXX="${bin_path}/clang++"
}

# Check if the binary artifact is installed
is_installed_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local version="${LLVM_VERSION_MAJOR}"

  # Check if clang with the right version exists
  which "clang-${version}"
}