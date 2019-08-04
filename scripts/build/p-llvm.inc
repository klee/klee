validate_build_config_llvm() {
    # Check variables that are needed for building 
    [[ -n "${LLVM_SRC_BASE}" ]] || { echo "LLVM_SRC_BASE not set"; exit 1; }
    [[ -n "${LLVM_VERSION_SHORT}" ]] || { echo "LLVM_VERSION_SHORT not set"; exit 1; }
}

setup_build_variables_llvm() {
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

    LLVM_CONFIG="${LLVM_BIN}/llvm-config"
    BITCODE_CC="${LLVM_BIN}/clang"
    BITCODE_CXX="${LLVM_BIN}/clang++"


    for sanitizer in "${SANITIZERS[@]}"; do
      [[ -z "${sanitizer}" ]] && continue
      # Undefined Behaviour Sanitizer
      if [ "${sanitizer}" == "memory" ]; then
        SANITIZER_LLVM_UNINSTRUMENTED="${LLVM_BUILD}_uninstrumented"
        SANITIZER_LLVM_LIBCXX="${LLVM_BUILD}_libcxx"

        SANITIZER_C_COMPILER="${SANITIZER_LLVM_UNINSTRUMENTED}/bin/clang"
        SANITIZER_CXX_COMPILER="${SANITIZER_LLVM_UNINSTRUMENTED}/bin/clang++"
        SANITIZER_CMAKE_C_COMPILER=("-DCMAKE_C_COMPILER=${SANITIZER_C_COMPILER}")
        SANITIZER_CMAKE_CXX_COMPILER=("-DCMAKE_CXX_COMPILER=${SANITIZER_CXX_COMPILER}")

        MSAN_LINK_FLAGS=("-lc++abi" "-Wl,--rpath=${SANITIZER_LLVM_LIBCXX}/lib" "-L${SANITIZER_LLVM_LIBCXX}/lib")
        MSAN_FLAGS=("${MSAN_LINK_FLAGS[@]}" -nostdinc++
           -isystem "${SANITIZER_LLVM_LIBCXX}/include"
           -isystem "${SANITIZER_LLVM_LIBCXX}/include/c++/v1"
           "-fsanitize=memory"
           "-fsanitize-memory-track-origins"
           -w -fno-omit-frame-pointer -g)
        SANITIZER_CXX_FLAGS+=("${MSAN_FLAGS[@]}" "-stdlib=libc++")
        SANITIZER_C_FLAGS+=("${MSAN_FLAGS[@]}")
        SANITIZER_LD_FLAGS+=("${MSAN_LINK_FLAGS[@]}")

        # Use the uninstrumented compiler
        BITCODE_CC="${SANITIZER_C_COMPILER}"
        BITCODE_CXX="${SANITIZER_CXX_COMPILER}"
        # But point to the instrumented llvm-config
        LLVM_CONFIG="${LLVM_BIN}/llvm-config"
        continue
      fi
      # Setup default sanitizer arguments
      SANITIZER_C_COMPILER="${LLVM_BIN}/clang"
      SANITIZER_CXX_COMPILER="${LLVM_BIN}/clang++"
    done
}

download_llvm() {
  # Skip step if we already checked out code
  [[ -f "${LLVM_SRC_BASE}/.src_checked_out" ]] && return 0

  # Checkout LLVM code
  svn co "http://llvm.org/svn/llvm-project/llvm/branches/release_${LLVM_VERSION_SHORT}" "${LLVM_SRC_BASE}"
  cd "${LLVM_SRC_BASE}/tools" || (echo "Directory does not exist"; exit 1)
  svn co "http://llvm.org/svn/llvm-project/cfe/branches/release_${LLVM_VERSION_SHORT}" clang
  cd "${LLVM_SRC_BASE}/projects" || (echo "Directory does not exist"; exit 1)
  svn co "http://llvm.org/svn/llvm-project/compiler-rt/branches/release_${LLVM_VERSION_SHORT}" compiler-rt
  cd "${LLVM_SRC_BASE}/projects" || (echo "Directory does not exist"; exit 1)
  svn co "http://llvm.org/svn/llvm-project/libcxx/branches/release_${LLVM_VERSION_SHORT}" libcxx
  cd "${LLVM_SRC_BASE}/projects" || (echo "Directory does not exist"; exit 1)
  svn co "http://llvm.org/svn/llvm-project/libcxxabi/branches/release_${LLVM_VERSION_SHORT}" libcxxabi

  # Apply existing patches if needed
  if [ -f "${DIR}/patches/llvm${LLVM_VERSION_SHORT}.patch" ]; then
     cd "${LLVM_SRC_BASE}" || (echo "Directory does not exist"; exit 1)
     patch -p0 -i "${DIR}/patches/llvm${LLVM_VERSION_SHORT}.patch"
  fi

  touch "${LLVM_SRC_BASE}/.src_checked_out" 
}

build_llvm() {
    local enable_optimized=$(to_bool "${ENABLE_OPTIMIZED}")
    local enable_debug=$(to_bool "${ENABLE_DEBUG}")
    local disable_assertions=$(to_bool "${DISABLE_ASSERTIONS}")
    local requires_rtti=$(to_bool "${REQUIRES_RTTI}")

    # For memory sanitizer, we have a multi-stage build process
    if [[ "${SANITIZER_BUILD}" == "memory" ]]; then
       # Build uninstrumented compiler
       mkdir -p "${SANITIZER_LLVM_UNINSTRUMENTED}"
       cd "${SANITIZER_LLVM_UNINSTRUMENTED}"
       cmake -GNinja -DCMAKE_BUILD_TYPE=Release "${LLVM_SRC_BASE}"
       ninja

       # Build instrumented libc/libc++
       mkdir -p "${SANITIZER_LLVM_LIBCXX}"
       cd "${SANITIZER_LLVM_LIBCXX}"
       cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DLLVM_USE_SANITIZER=MemoryWithOrigins \
        "${SANITIZER_CMAKE_C_COMPILER[@]}" \
        "${SANITIZER_CMAKE_CXX_COMPILER[@]}" \
        "${LLVM_SRC_BASE}"
       ninja cxx cxxabi

       # Build instrumented clang
       mkdir -p "${LLVM_BUILD}"
       cd "${LLVM_BUILD}"
       C_F="${SANITIZER_C_FLAGS[*]}"
       CXX_F="${SANITIZER_CXX_FLAGS[*]}"
       LD_F="${SANITIZER_LD_FLAGS[*]}"
       cmake -GNinja \
          "${SANITIZER_CMAKE_C_COMPILER[@]}" \
          "${SANITIZER_CMAKE_CXX_COMPILER[@]}" \
          -DCMAKE_C_FLAGS="$C_F" \
          -DCMAKE_CXX_FLAGS="${CXX_F}" \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          -DLLVM_ENABLE_ASSERTIONS=On \
          -DLLVM_USE_SANITIZER=MemoryWithOrigins \
          -DLLVM_ENABLE_LIBCXX=ON \
          -DCMAKE_EXE_LINKER_FLAGS="${LD_F}" \
          -DCMAKE_INSTALL_PREFIX="${LLVM_INSTALL}" \
          "${LLVM_SRC_BASE}"
      # Build clang as a dependency and install all needed packages
      ninja clang
      return 0
    fi

  # Configure; build; and install
  mkdir -p "${LLVM_BUILD}"
  cd "${LLVM_BUILD}"

  # Skip building if already finished
  [[ -e "${LLVM_BUILD}/.build_finished" ]] && return 0


  # Configure LLVM
  CONFIG=(
     "-DCMAKE_INSTALL_PREFIX=${LLVM_INSTALL}"
     "-DLLVM_BUILD_LLVM_DYLIB=TRUE"
  )
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
     "-DLLVM_INCLUDE_EXAMPLES=OFF"
#     "-DLLVM_INCLUDE_TESTS=OFF"
#     "-DCLANG_INCLUDE_TESTS=OFF"
     "-DLLVM_INCLUDE_BENCHMARKS=OFF"
     "-DBUILD_SHARED_LIBS=ON"
  )

  local variables=("")

  if [[ -n "${CC:-}" ]]; then
    variables+=("CC=${CC}")
  fi

  if [[ -n "${CXX:-}" ]]; then
    variables+=("CXX=${CXX}")
  fi

  if [[ -n "${LDFLAGS:-}" ]]; then
    variables+=("LDFLAGS=${LLVM_LDFLAGS}")
  fi

  if [[ -n "${variables[*]}" ]]; then
    "${variables[*]}" cmake "${CONFIG[@]}" "${LLVM_SRC_BASE}"
  else
    cmake "${CONFIG[@]}" "${LLVM_SRC_BASE}"
  fi

  # Linking LLVM can require a lot of memory.
  # First try multicore - if that doesn't work, try single core
  (make "-j$(nproc)") || (make) || return 1
  touch "${LLVM_BUILD}/.build_finished"
}

install_llvm() {
  if [[ "${SANITIZER_BUILD}" != "memory" ]]; then
    cd "${LLVM_BUILD}"
    make "-j$(nproc)" install
    cp "${LLVM_BUILD_BIN}/FileCheck" "${LLVM_INSTALL}/bin/"
    cp "${LLVM_BUILD_BIN}/not" "${LLVM_INSTALL}/bin/"
  
    # Remove debug information from binaries
    strip "${LLVM_INSTALL}/bin/"* || /bin/true
    strip "${LLVM_INSTALL}/lib/libclang"* || /bin/true
    strip "${LLVM_INSTALL}/lib/libLTO"* || /bin/true
  else
    # Handle memory sanitizer install
    LLVM_PACKAGES=( \
      install-clang install-llvm-config install-llvm-objdump
      install-llvm-link install-llvm-ar install-llvm-nm install-llvm-dis
      install-clang-headers install-llvm-as
      install-llvm-symbolizer install-LLVMSupport install-lli not FileCheck )
  
    if [[ ${LLVM_VERSION_SHORT} -eq 38 ]]; then
      LLVM_PACKAGES=("${LLVM_PACKAGES[@]}" installhdrs)
    else
      LLVM_PACKAGES=("${LLVM_PACKAGES[@]}" install-llvm-headers)
    fi
  
    ninja "${LLVM_PACKAGES[@]}"

    for i in $(ninja -t targets |grep install-LLVM | cut -d : -f 1); do ninja "$i"; done
    
    cp "${LLVM_BUILD}/bin/FileCheck" "${LLVM_INSTALL}/bin/"
    cp "${LLVM_BUILD}/bin/not" "${LLVM_INSTALL}/bin/"
  fi

  touch "${LLVM_INSTALL}/.install_finished"
}

# Check if the binary artifact is installed
is_installed_llvm() {
  (
    setup_build_variables_llvm
    # Check if the specific llvm-config exists
    [[ -f "${LLVM_BUILD_BIN}/llvm-config" ]]
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
    echo "${LLVM_INSTALL}"
    echo "${LLVM_SRC_BASE}"
    [[ "${sanitizer}" == "memory" ]] && echo "${SANITIZER_LLVM_UNINSTRUMENTED}"
    [[ "${sanitizer}" == "memory" ]] && echo "${SANITIZER_LLVM_LIBCXX}"
  )
}
