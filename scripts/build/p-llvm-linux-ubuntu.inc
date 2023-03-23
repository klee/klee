# Build dependencies
install_build_dependencies_llvm() {
  source "${DIR}/common-functions"

  with_sudo apt update -y

  dependencies=(
    ca-certificates
    build-essential
    autoconf
    automake
    groff
    gcc
    g++
    python3-distutils
    make
    git # To check out code
    zlib1g-dev
    git
    ninja-build
  )
  
  if [[ "${SANITIZERS[*]}" == "memory" ]]; then
    dependencies+=(ninja-build)
  fi

  #Install essential dependencies
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}

install_binary_artifact_llvm() {
  # No need to check for optimised, we can build against LLVM with optimised and non-optimised versions
  #  local enable_optimized=$(to_bool "${ENABLE_OPTIMIZED}")
  local enable_debug
  enable_debug=$(to_bool "${ENABLE_DEBUG}")
  local disable_assertions
  disable_assertions=$(to_bool "${DISABLE_ASSERTIONS}")
  local requires_rtti
  requires_rtti=$(to_bool "${REQUIRES_RTTI}")
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local version="${LLVM_VERSION_MAJOR}"



  # No support for LLVM packages with debug information, incompatible if requested otherwise
  [[ "${enable_debug}" -eq 1 ]] && return 1

  # Packages are build with assertions disabled, incompatible if requested otherwise
  [[ "${disable_assertions}" -eq 0 ]] && return 1

  # Packages are build with RTTI enabled, incompatible if requested otherwise
  [[ "${requires_rtti}" -eq 0 ]] && return 1

  # Enable/Disable optimized does not matter
  source "${DIR}/common-functions"

  # Check if package in standard repository otherwise use upstream
  with_sudo apt-get update -y
  if ! apt-cache show "llvm-${version}"; then
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
    "llvm-${version}"
    "llvm-${version}-dev"
    "llvm-${version}-runtime"
    "clang-${version}"
  )

  #Install essential dependencies
  with_sudo apt-get -y --no-install-recommends install "${dependencies[@]}" || return 1
}

check_llvm_config_version() {
    local check_mode=1
    strict_mode="$1" # if llvm-config should be checked strictly
    local lc=""
    lc="$2" # path to llvm-config

    # If not set return error
    [[ -n "${lc}" ]] || return 1

    # First check, if the provided llvm-config is a full path
    if [[ ! -f "${lc}" ]]; then
      # Nothing found, assume it's just the name of the binary in path, find the path
      lc=$(which "${lc}")

      # If path not found return error
      [[ -z "${lc}" ]] && return 1
    fi

    local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
    local version="${LLVM_VERSION_MAJOR}"

    # Check for llvm-config without suffix but correct version number
    [[ $($lc --version) == "${LLVM_VERSION}"* ]] || return 1

    # In case correct version numbers are required, return already
    [[ "${check_mode}" == "0" ]] && return 0;

    local rtti
    rtti="$(${lc} --has-rtti)"
    local assertion
    assertion="$(${lc} --assertion-mode)"
    local build_mode
    build_mode="$(${lc} --build-mode)"

    # Check requested mode with mode of the found item
    [[ $(to_bool "${REQUIRES_RTTI}") -eq $(to_bool "${rtti}") ]] || return 1
    [[ $(to_bool "${DISABLE_ASSERTIONS}") -ne $(to_bool "${assertion}") ]] || return 1

    local shared_mode
    shared_mode="$(${lc} --shared-mode)" || return 1
}

# Check if the binary artifact is installed
is_installed_llvm() {
  # Check for variables set and not empty
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local version="${LLVM_VERSION_MAJOR}"

  # Check for llvm-config without suffix but correct version number
  local lc

  # First check with the version-specific number
  lc=$(which "llvm-config-${LLVM_VERSION_MAJOR}")
  check_llvm_config_version 1 "${lc}" && return 0


  return 1
}

setup_artifact_variables_llvm() {
    # Check for variables set and not empty
    local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
    local version="${LLVM_VERSION_MAJOR}"

    local lc=""
    # Check for llvm-config without suffix but correct version number
    lc=$(which "llvm-config")
    local is_ins
    is_ins=$(check_llvm_config_version 1 "${lc}")
    if [[ ! "${is_ins}" ]]; then
      # Check if llvm-config with the right version exists
      lc=$(which "llvm-config-${version}") || return 1
      is_ins=$(check_llvm_config_version 1 "${lc}") || return 1
    fi

    LLVM_CONFIG="${lc}"
    LLVM_INSTALL="$(${lc} --prefix)"
    LLVM_BIN="$(${lc} --bindir)"
    BITCODE_CC="${LLVM_BIN}/clang"
    BITCODE_CXX="${LLVM_BIN}/clang++"
}

get_build_artifacts_llvm() {
  is_installed_llvm || return 1
  return 0
}