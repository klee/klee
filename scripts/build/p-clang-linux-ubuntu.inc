install_binary_artifact_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local version="-${LLVM_VERSION_MAJOR}"

  source "${DIR}/common-functions"
  with_sudo apt-get update -y

  # Check if package in standard repository otherwise use upstream
  if ! apt-cache show "llvm${version}"; then
    if [[ -z "$(which wget)" ]]; then
      # Add certificate
      with_sudo apt-get update -y
      dependencies=(
        ca-certificates
        wget
        lsb-release
        gnupg
      )
      with_sudo apt-get -y --no-install-recommends install "${dependencies[@]}"
    fi

    # Add LLVM upstream repository if available
    codename="$(lsb_release --codename --short)"
    if wget -q "https://apt.llvm.org/${codename}/dists/llvm-toolchain-${codename}${version}/"; then
      apt_entry="deb http://apt.llvm.org/${codename}/ llvm-toolchain-${codename}${version} main"
      if ! grep -rq "${apt_entry}" /etc/apt; then
        wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key| with_sudo apt-key add -
        echo "${apt_entry}" | with_sudo tee -a /etc/apt/sources.list
        with_sudo apt-get update -y
      fi
    fi
  fi

  dependencies=(
    "llvm${version}"
    "clang${version}"
  )

  #Install essential dependencies
  with_sudo apt-get -y --no-install-recommends install "${dependencies[@]}" || return 1
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