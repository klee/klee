# Build dependencies
install_build_dependencies_llvm() { 
  apt update -y

  dependencies=(
    build-essential
    autoconf
    automake
    groff
    gcc
    g++
    python-dev
    make
    subversion # To check out code
    zlib1g-dev
    cmake
  )
  
  if [[ "${SANITIZERS[*]}" == "memory" ]]; then
    dependencies+=(ninja-build)
  fi

  #Install essential dependencies
  apt -y --no-install-recommends install "${dependencies[@]}"
}

install_binary_artifact_llvm() {
  local enable_optimized=$(to_bool "${ENABLE_OPTIMIZED}")
  local enable_debug=$(to_bool "${ENABLE_DEBUG}")
  local disable_assertions=$(to_bool "${DISABLE_ASSERTIONS}")
  local requires_rtti=$(to_bool "${REQUIRES_RTTI}")
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local LLVM_VERSION_MINOR="${LLVM_VERSION/*./}"


  # No support for LLVM packages with debug information, incompatible if requested otherwise
  [[ "${enable_debug}" -eq 1 ]] && return 1

  # Packages are build with assertions disabled, incompatible if requested otherwise
  [[ "${disable_assertions}" -eq 0 ]] && return 1

  # Packages are build with RTTI enabled, incompatible if requested otherwise
  [[ "${requires_rtti}" -eq 0 ]] && return 1

  # Enable/Disable optimized does not matter
  
  # Add certificate
  apt update -y
  dependencies=(
    ca-certificates
    wget
  )
  apt -y --no-install-recommends install "${dependencies[@]}"
  wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key| apt-key add -
  
  local version=""
  [[ "${LLVM_VERSION_MAJOR}" -le 6 ]] && version="-${LLVM_VERSION}"
  [[ "${LLVM_VERSION_MAJOR}" -ge 7 ]] && version="-${LLVM_VERSION_MAJOR}"
  
  # Add repository
  echo "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial${version} main" >> /etc/apt/sources.list
  apt update -y

  dependencies=(
    "llvm${version}"
    "llvm${version}-dev"
    "llvm${version}-runtime"
    "clang${version}"
  )

  #Install essential dependencies
  apt -y --no-install-recommends install "${dependencies[@]}"
}

# Check if the binary artifact is installed
is_installed_llvm() {
    local lc=""
    # Check if llvm-config with the right version exists    
    lc=$(which "llvm-config-${LLVM_VERSION}")
    
    [[ -z "${lc}" ]] && return 1

    local rtti
    rtti="$(${lc} --has-rtti)"
    local assertion
    assertion="$(${lc} --assertion-mode)"
    local build_mode
    build_mode="$(${lc} --build-mode)"

    # Check requested mode with mode of the found item  
    [[ $(to_bool "${REQUIRES_RTTI}") -eq $(to_bool "${rtti}") ]] || return 1
    [[ $(to_bool "${DISABLE_ASSERTIONS}") -ne $(to_bool "${assertion}") ]] || return 1
}
