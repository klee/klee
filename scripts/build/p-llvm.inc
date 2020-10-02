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

  source "${DIR}/common-functions"
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"

  branch_name="release/${LLVM_VERSION_MAJOR}.x"
  if [[ "${LLVM_VERSION_MAJOR}" -le 3 ]]; then
    branch_name="release/${LLVM_VERSION}.x"
  fi
  git_clone_or_update "https://github.com/llvm/llvm-project.git" "${LLVM_SRC_BASE}" "${branch_name}" || exit 1
  if [[ "${LLVM_VERSION_MAJOR}" -lt 4 ]]; then
    # Use symlinks for older versions whose build systems do not support the monorepo directory layout
    ln -s "${LLVM_SRC_BASE}/clang" "${LLVM_SRC_BASE}/llvm/tools/"
    ln -s "${LLVM_SRC_BASE}/compiler-rt" "${LLVM_SRC_BASE}/llvm/projects/"
    ln -s "${LLVM_SRC_BASE}/libcxx" "${LLVM_SRC_BASE}/llvm/projects/"
    ln -s "${LLVM_SRC_BASE}/libcxxabi" "${LLVM_SRC_BASE}/llvm/projects/"
  fi

  # Apply existing patches if needed
  if [ -f "${DIR}/patches/llvm${LLVM_VERSION_SHORT}.patch" ]; then
     cd "${LLVM_SRC_BASE}" || (echo "Directory does not exist"; exit 1)
     patch -p1 --follow-symlinks -i "${DIR}/patches/llvm${LLVM_VERSION_SHORT}.patch" || return 1
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
       cmake -GNinja -DCMAKE_BUILD_TYPE=Release \
      "-DLLVM_OPTIMIZED_TABLEGEN=ON" \
        "-DLLVM_ENABLE_PROJECTS=clang;compiler-rt;libcxx;libcxxabi" \
      "-DLLVM_TARGETS_TO_BUILD=X86" \
        "${LLVM_SRC_BASE}/llvm"
    ninja compiler-rt cxx cxxabi clang || return 1

       # Build instrumented libc/libc++
       mkdir -p "${SANITIZER_LLVM_LIBCXX}"
       cd "${SANITIZER_LLVM_LIBCXX}"
       cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        "-DLLVM_ENABLE_PROJECTS=clang;compiler-rt;libcxx;libcxxabi" \
        -DLLVM_USE_SANITIZER=MemoryWithOrigins \
        "${SANITIZER_CMAKE_C_COMPILER[@]}" \
        "${SANITIZER_CMAKE_CXX_COMPILER[@]}" \
      "-DLLVM_OPTIMIZED_TABLEGEN=ON" \
      "-DLLVM_TARGETS_TO_BUILD=X86" \
        "${LLVM_SRC_BASE}/llvm"
    ninja compiler-rt cxx cxxabi|| return 1

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
          "-DLLVM_ENABLE_PROJECTS=clang;compiler-rt;libcxx;libcxxabi" \
          -DLLVM_ENABLE_ASSERTIONS=On \
          -DLLVM_USE_SANITIZER=MemoryWithOrigins \
          -DLLVM_ENABLE_LIBCXX=ON \
          -DCMAKE_EXE_LINKER_FLAGS="${LD_F}" \
          -DCMAKE_INSTALL_PREFIX="${LLVM_INSTALL}" \
      -DBUILD_SHARED_LIBS=ON \
      -DLLVM_OPTIMIZED_TABLEGEN=ON \
      -DLLVM_TARGETS_TO_BUILD=X86 \
          "${LLVM_SRC_BASE}/llvm"
      # Build clang as a dependency and install all needed packages
    ninja clang || return 1
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

  # Select components to build
  if [[ ${LLVM_VERSION_SHORT} -ge 60 ]]; then
    CONFIG+=("-DLLVM_ENABLE_PROJECTS=clang;compiler-rt;libcxx;libcxxabi")
  else
    CONFIG+=("-DLLVM_ENABLE_PROJECTS=clang;compiler-rt")
  fi

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
    "-DCLANG_INCLUDE_TESTS=OFF"
     "-DLLVM_INCLUDE_BENCHMARKS=OFF"
     "-DBUILD_SHARED_LIBS=ON"
    "-DLLVM_OPTIMIZED_TABLEGEN=ON"
    "-DLLVM_TARGETS_TO_BUILD=X86"
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
    "${variables[*]}" cmake "${CONFIG[@]}" "${LLVM_SRC_BASE}/llvm"
  else
    cmake "${CONFIG[@]}" "${LLVM_SRC_BASE}/llvm"
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
