validate_build_config_llvm() {
  # Check variables that are needed for building
  [[ -n "${LLVM_SRC_BASE}" ]] || {
    echo "LLVM_SRC_BASE not set"
    exit 1
  }
  [[ -n "${LLVM_VERSION_SHORT}" ]] || {
    echo "LLVM_VERSION_SHORT not set"
    exit 1
  }
}

setup_build_variables_llvm() {
  source "${DIR}/common-functions"
  LLVM_SUFFIX=""

  local BuildMode=""
  local enable_optimized=$(to_bool "${ENABLE_OPTIMIZED}")
  local enable_debug=$(to_bool "${ENABLE_DEBUG}")
  local disable_assertions=$(to_bool "${DISABLE_ASSERTIONS}")
  local requires_rtti=$(to_bool "${REQUIRES_RTTI}")

  if [[ "${enable_optimized}" == "1" ]]; then
    LLVM_SUFFIX+="_O"
    BuildMode="Release"
  else
    LLVM_SUFFIX+="_NO"
  fi

  if [[ ("${enable_debug}" == "1") || ("${enable_optimized}" != "1") ]]; then
    LLVM_SUFFIX+="_D"
    [ -z "$BuildMode" ] && BuildMode="Debug" || BuildMode="$BuildMode+Debug"
  else
    LLVM_SUFFIX+="_ND"
  fi

  if [[ "${disable_assertions}" == "1" || -z "${disable_assertions}" ]]; then
    LLVM_SUFFIX+="_NA"
  else
    LLVM_SUFFIX+="_A"
    [ -z "$BuildMode" ] && BuildMode="Asserts" || BuildMode="$BuildMode+Asserts"
  fi

  if [[ "${requires_rtti}" == "1" ]]; then
    LLVM_SUFFIX+="_RTTI"
  fi

  if [[ "${SANITIZER_SUFFIX}x" != "x" ]]; then
    LLVM_SUFFIX+="${SANITIZER_SUFFIX}"
  fi

  LLVM_SRC_BASE="${BASE}/llvm-${LLVM_VERSION_SHORT}"
  LLVM_BUILD="${LLVM_SRC_BASE}-build${LLVM_SUFFIX}"
  LLVM_INSTALL="${LLVM_SRC_BASE}-install${LLVM_SUFFIX}"
  LLVM_BIN="${LLVM_INSTALL}/bin"
  LLVM_BUILD_BIN="${LLVM_BUILD}/bin"

  if [[ "${SANITIZER_BUILD:-}" == "memory" ]]; then
    LLVM_BUILD_LIBCXX="${LLVM_INSTALL}-libcxx"
  fi


  LLVM_CONFIG="${LLVM_BIN}/llvm-config"
  BITCODE_CC="${LLVM_BIN}/clang"
  BITCODE_CXX="${LLVM_BIN}/clang++"
}

download_llvm() {
  # Skip step if we already checked out code
  [[ -f "${LLVM_SRC_BASE}/.src_checked_out" ]] && return 0

  source "${DIR}/common-functions"
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"

  branch_name="release/${LLVM_VERSION_MAJOR}.x"
  git_clone_or_update "https://github.com/llvm/llvm-project.git" "${LLVM_SRC_BASE}" "${branch_name}" || exit 1
  # Apply existing patches if needed
  if [ -f "${DIR}/patches/llvm${LLVM_VERSION_SHORT}.patch" ]; then
    cd "${LLVM_SRC_BASE}" || (
      echo "Directory does not exist"
      exit 1
    )
    patch -p1 --follow-symlinks -i "${DIR}/patches/llvm${LLVM_VERSION_SHORT}.patch" || return 1
  fi

  touch "${LLVM_SRC_BASE}/.src_checked_out"
}


configure_llvm() {
  local enable_optimized=$(to_bool "${ENABLE_OPTIMIZED}")
  local enable_debug=$(to_bool "${ENABLE_DEBUG}")
  local disable_assertions=$(to_bool "${DISABLE_ASSERTIONS}")
  local requires_rtti=$(to_bool "${REQUIRES_RTTI}")

  # Configure; build; and install
  mkdir -p "${LLVM_BUILD}"

  # Configure LLVM
  CONFIG=(
    "-DCMAKE_INSTALL_PREFIX=${LLVM_INSTALL}"
  )


  if [[ "${SANITIZER_BUILD:-}" == "memory" ]]; then
    # Two stage process, build libcxx and libcxxabi

    # Skip building if already finished
    if [[ ! -f "${LLVM_BUILD_LIBCXX}/.build_finished" ]]; then
      mkdir -p "${LLVM_BUILD_LIBCXX}"
      cd "${LLVM_BUILD_LIBCXX}"
      cmake -GNinja "${LLVM_SRC_BASE}/llvm" \
        -DCMAKE_BUILD_TYPE=Release \
        -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi" \
        -DCMAKE_C_COMPILER="${SANITIZER_C_COMPILER}" \
        -DCMAKE_CXX_COMPILER="${SANITIZER_CXX_COMPILER}" \
        -DLLVM_USE_SANITIZER=MemoryWithOrigins
      cmake --build . -- cxx cxxabi || return 1
      touch "${LLVM_BUILD_LIBCXX}/.build_finished"
    fi

    CONFIG+=(
      -DCMAKE_CXX_FLAGS="-fsanitize=memory -stdlib=libc++ -isystem ${LLVM_BUILD_LIBCXX}/include -isystem ${LLVM_BUILD_LIBCXX}/include/c++/v1"
    )

    LLVM_LINKER_FLAGS="-lc++abi -Wl,--rpath=${LLVM_BUILD_LIBCXX}/lib -L${LLVM_BUILD_LIBCXX}/lib -fsanitize=memory -nostdinc++"
  fi

  cd "${LLVM_BUILD}"

  # Select components to build
  local ENABLED_LLVM_PROJECTS
  ENABLED_LLVM_PROJECTS="clang"

  # cmake build
  if [[ "${enable_optimized}" == "1" && "${enable_debug}" != "1" ]]; then
    CONFIG+=("-DCMAKE_BUILD_TYPE=Release")
  fi
  if [[ "${enable_optimized}" == "1" && "${enable_debug}" == "1" ]]; then
    CONFIG+=("-DCMAKE_BUILD_TYPE=RelWithDebInfo")
  fi
  if [[ "${enable_optimized}" != "1" && "${enable_debug}" == "1" ]]; then
    CONFIG+=("-DCMAKE_BUILD_TYPE=Debug")
  fi

  if [[ "${disable_assertions}" == "1" ]]; then
    CONFIG+=("-DLLVM_ENABLE_ASSERTIONS=Off")
  else
    CONFIG+=("-DLLVM_ENABLE_ASSERTIONS=On")
  fi

  if [[ -n "${LLVM_CFLAGS:-}" ]]; then
    CONFIG+=("-DCMAKE_C_FLAGS=\"$LLVM_CFLAGS\"")
  fi

  if [[ -n "${LLVM_CXXFLAGS:-}" ]]; then
    CONFIG+=("-DCMAKE_CXX_FLAGS=\"$LLVM_CXXFLAGS\"")
  fi

  if [[ "${requires_rtti}" -eq 1 ]]; then
    CONFIG+=("-DLLVM_ENABLE_RTTI=TRUE")
  fi

  # Remove unneeded targets
  CONFIG+=(
    "-DLLVM_INCLUDE_BENCHMARKS:BOOL=OFF"
    "-DLLVM_INCLUDE_EXAMPLES:BOOL=OFF"
    "-DLLVM_INCLUDE_TESTS:BOOL=OFF"
    "-DLLVM_OPTIMIZED_TABLEGEN:BOOL=TRUE"
    "-DLLVM_TARGETS_TO_BUILD:STRING=X86"
    "-DLLVM_INSTALL_UTILS:BOOL=TRUE" # Install FileCheck and Not
    "-DLLVM_PARALLEL_LINK_JOBS=1"
    "-DCLANG_INCLUDE_TESTS:BOOL=OFF"
  )

  if [[ "${SANITIZER_BUILD:-}" == "memory" ]]; then
    # We have to build without libunwind if RTTI is disables
    CONFIG+=("-DLLVM_ENABLE_PROJECTS=${ENABLED_LLVM_PROJECTS}")
  else
      CONFIG+=(
          "-DLLVM_BUILD_LLVM_DYLIB:BOOL=ON"
          "-DLLVM_LINK_LLVM_DYLIB:BOOL=ON"
          "-DLLVM_BUILD_STATIC:BOOL=OFF"
          "-DLIBCLANG_BUILD_STATIC:BOOL=OFF"
      )
      CONFIG+=("-DLLVM_ENABLE_PROJECTS=${ENABLED_LLVM_PROJECTS};libcxx;libcxxabi;libunwind")
  fi

  if [[ -n ${SANITIZER_BUILD} ]]; then
    if [[ -n "${SANITIZER_C_COMPILER:-}" ]]; then
      CC="${SANITIZER_C_COMPILER}"
    fi
    if [[ -n "${SANITIZER_CXX_COMPILER:-}" ]]; then
      CXX="${SANITIZER_CXX_COMPILER}"
    fi

    if [[ "${SANITIZER_BUILD}" == "address" ]] ; then
      CONFIG+=("-DLLVM_USE_SANITIZER=Address")
    fi
    if [[ "${SANITIZER_BUILD}" == "undefined" ]] ; then
      CONFIG+=("-DLLVM_USE_SANITIZER=Undefined")
    fi
    if [[ "${SANITIZER_BUILD}" == "memory" ]] ; then
      CONFIG+=("-DLLVM_USE_SANITIZER=MemoryWithOrigins")
    fi
  fi

  (
    if [[ -n "${CC:-}" ]]; then
      export "CC=${CC}"
    fi

    if [[ -n "${CXX:-}" ]]; then
      export "CXX=${CXX}"
    fi

    if [[ -n "${LLVM_LINKER_FLAGS:-}" ]]; then
      export "LDFLAGS=${LLVM_LINKER_FLAGS}"
    fi

    cmake -G Ninja "${CONFIG[@]}" "${LLVM_SRC_BASE}/llvm"
  )
}
build_llvm() {
  # Skip building if already finished
  [[ -f "${LLVM_BUILD}/.build_finished" ]] && return 0

  configure_llvm || return 1
  (
    if [[ -n "${CC:-}" ]]; then
      export "CC=${CC}"
    fi

    if [[ -n "${CXX:-}" ]]; then
      export "CXX=${CXX}"
    fi

    if [[ "${SANITIZER_BUILD:-}" == "memory" ]]; then
      ninja clang || return 1
    else
      ninja || return 1
    fi
  ) || return 1
  touch "${LLVM_BUILD}/.build_finished"

}

install_llvm() {
  cd "${LLVM_BUILD}"
  ninja install
  # Remove debug information from binaries
  strip "${LLVM_INSTALL}/bin/"* || /bin/true
  strip "${LLVM_INSTALL}/lib/libclang"* || /bin/true
  strip "${LLVM_INSTALL}/lib/libLTO"* || /bin/true

  touch "${LLVM_INSTALL}/.install_finished"
}

# Check if the binary artifact is installed
is_installed_llvm() {
  (
    setup_build_variables_llvm
    # Check if the specific llvm-config exists
    [[ -f "${LLVM_INSTALL}/bin/llvm-config" ]]
    [[ -f "${LLVM_INSTALL}/.install_finished" ]]
  ) || return 1
}

# Returns a unique id for this configuration
get_docker_config_id_llvm() {
  (
    setup_build_variables_llvm
    echo "${LLVM_VERSION_SHORT}${LLVM_SUFFIX}"
  )
}

setup_artifact_variables_llvm() {
  setup_build_variables_llvm
}

get_build_artifacts_llvm() {
  (
    setup_build_variables_llvm
    if [[ "${SANITIZER_BUILD:-}" == "memory" ]]; then
      echo "${LLVM_BUILD_LIBCXX}"
    fi
    echo "${LLVM_INSTALL}"
  )
}
