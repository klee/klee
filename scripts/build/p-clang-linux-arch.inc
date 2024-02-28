install_binary_artifact_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local version="-${LLVM_VERSION_MAJOR}"

  source "${DIR}/common-functions"

  dependencies=(
    "llvm${version}"
    "clang${version}"
  )

  #Install essential dependencies
  with_sudo pacman --no-confirm -S "${dependencies[@]}" || return 1
}

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
