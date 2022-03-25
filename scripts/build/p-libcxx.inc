# Build scripts for libc++
setup_build_variables_libcxx() {
  LIBCXX_SRC="${BASE}/llvm-${LLVM_VERSION_SHORT}"
  LIBCXX_BUILD="${BASE}/libc++-build-${LLVM_VERSION_SHORT}"
  LIBCXX_INSTALL="${BASE}/libc++-install-${LLVM_VERSION_SHORT}"
}

download_libcxx() {
  # Skip step if we already checked out code
  [[ -f "${LIBCXX_SRC}/.src_checked_out" ]] && return 0

  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local version="release/${LLVM_VERSION_MAJOR}.x"

  git clone --single-branch --branch "${version}" --depth 1 "https://github.com/llvm/llvm-project.git" "${LIBCXX_SRC}"

  # Apply existing patches if needed
  if [ -f "${DIR}/patches/libcxx${LLVM_VERSION_SHORT}.patch" ]; then
     cd "${LIBCXX_SRC}" || (echo "Directory does not exist"; exit 1)
     patch -p1 --follow-symlinks -i "${DIR}/patches/libcxx${LLVM_VERSION_SHORT}.patch" || return 1
  fi


  touch "${LIBCXX_SRC}/.src_checked_out" 
}

build_libcxx() { 
  mkdir -p "${LIBCXX_BUILD}"
  cd "${LIBCXX_BUILD}"
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"

  local cmake_flags=(
   "-DLLVM_ENABLE_PROJECTS=libcxx;libcxxabi"
   "-DLLVM_ENABLE_THREADS:BOOL=OFF"
   "-DLIBCXX_ENABLE_THREADS:BOOL=OFF"
   "-DLIBCXX_ENABLE_SHARED:BOOL=ON"
   "-DLIBCXXABI_ENABLE_THREADS:BOOL=OFF"
   "-DCMAKE_BUILD_TYPE:STRING=Release"
   "-DCMAKE_INSTALL_PREFIX=${LIBCXX_INSTALL}"
  )

  # Static ABI libraries are not supported under OS X
  if [[ "${OS}" == "osx" ]]; then
    cmake_flags+=("-DLIBCXX_ENABLE_STATIC_ABI_LIBRARY:BOOL=OFF")
  else
    cmake_flags+=("-DLIBCXX_ENABLE_STATIC_ABI_LIBRARY:BOOL=ON")
  fi

  (
    export CC=wllvm
    export CXX=wllvm++
    export LLVM_COMPILER=clang
    export LLVM_COMPILER_PATH="$(dirname "${BITCODE_CC}")"

    cmake "${cmake_flags[@]}" "${LIBCXX_SRC}/llvm"
    make cxx -j$(nproc) || make cxx
  )
}

install_libcxx() {
  cd "${LIBCXX_BUILD}"
  (
    export CC=wllvm
    export CXX=wllvm++
    export LLVM_COMPILER=clang
    export LLVM_COMPILER_PATH="$(dirname "${BITCODE_CC}")"

    cd "${LIBCXX_BUILD}/projects"
    make install

    local libraries

    if [[ "${OS}" == "osx" ]]; then
      libraries=("${LIBCXX_INSTALL}"/lib/lib*.dylib)
    else
      libraries=("${LIBCXX_INSTALL}"/lib/lib*.so)
    fi

    local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
    libraries+=("${LIBCXX_INSTALL}"/lib/lib*.a)


    for p in "${libraries[@]}" ; do
      extract-bc "$p"
    done

    touch "${LIBCXX_INSTALL}"/.is_installed
  )
  return 0
}

# Check if the binary artifact is installed
is_installed_libcxx() {
  (
    setup_build_variables_libcxx
    [[ -f "${LIBCXX_INSTALL}"/.is_installed ]]
  ) || return 1
}

get_docker_config_id_libcxx() {
  (
    echo "${LLVM_VERSION_SHORT}"
  )
}

get_build_artifacts_libcxx() {
  (
    setup_build_variables_libcxx
    echo "${LIBCXX_INSTALL}"
  )
}

setup_artifact_variables_libcxx() {
  setup_build_variables_libcxx
}